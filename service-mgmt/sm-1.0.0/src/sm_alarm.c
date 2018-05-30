//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_alarm.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <pthread.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_time.h"
#include "sm_node_utils.h"
#include "sm_alarm_thread.h"

#define SM_ALARM_ENTRY_INUSE                                         0xA5A5A5A5

typedef enum
{
    SM_ALARM_STATE_UNKNOWN,
    SM_ALARM_STATE_CLEARING,
    SM_ALARM_STATE_RAISING,
    SM_ALARM_STATE_MAX,
} SmAlarmStateT;

typedef struct
{
    uint32_t inuse;
    SmAlarmT alarm;
    SmAlarmStateT alarm_state;
    SmAlarmNodeNameT alarm_node_name;
    SmAlarmDomainNameT alarm_domain_name;
    SmAlarmEntityNameT alarm_entity_name;
    bool alarm_data_valid;
    SmAlarmDataT alarm_data;
} SmAlarmEntryT;

static int _server_fd = -1;
static int _client_fd = -1;
static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
static SmAlarmEntryT _alarms[SM_ALARMS_MAX];

// ****************************************************************************
// Alarm - Find Empty
// ==================
static SmAlarmEntryT* sm_alarm_find_empty( void )
{
    SmAlarmEntryT* entry;

    unsigned int entry_i;
    for( entry_i=0; SM_ALARMS_MAX > entry_i; ++entry_i )
    {
        entry = &(_alarms[entry_i]);

        if( SM_ALARM_ENTRY_INUSE != entry->inuse )
            return( entry );
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Find
// ============
static SmAlarmEntryT* sm_alarm_find( const SmAlarmT alarm, 
    const SmAlarmNodeNameT node_name, const SmAlarmDomainNameT domain_name,
    const SmAlarmEntityNameT entity_name )
{
    SmAlarmEntryT* entry;

    unsigned int entry_i;
    for( entry_i=0; SM_ALARMS_MAX > entry_i; ++entry_i )
    {
        entry = &(_alarms[entry_i]);

        if( SM_ALARM_ENTRY_INUSE != entry->inuse )
            continue;

        if(( alarm == entry->alarm )&&
           ( 0 == strcmp( node_name,   entry->alarm_node_name   ) )&&
           ( 0 == strcmp( domain_name, entry->alarm_domain_name ) )&&
           ( 0 == strcmp( entity_name, entry->alarm_entity_name ) ))
            return( entry );
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Throttle
// ================
static bool sm_alarm_throttle( const SmAlarmT alarm, SmAlarmStateT alarm_state,
    const SmAlarmNodeNameT node_name, const SmAlarmDomainNameT domain_name,
    const SmAlarmEntityNameT entity_name, SmAlarmDataT* data )
{
    bool throttle = false;
    SmAlarmEntryT* entry;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        abort();
    }

    entry = sm_alarm_find( alarm, node_name, domain_name, entity_name );
    if( NULL == entry )
    {
        entry = sm_alarm_find_empty();
        if( NULL == entry )
        {
            DPRINTFE( "Failed to store alarm, no more room." );
            goto RELEASE_MUTEX;
        }

        memset( entry, 0, sizeof(SmAlarmEntryT) );
    
        entry->inuse = SM_ALARM_ENTRY_INUSE;
        entry->alarm = alarm;
        snprintf( entry->alarm_node_name, sizeof(entry->alarm_node_name),
                  "%s", node_name );
        snprintf( entry->alarm_domain_name, sizeof(entry->alarm_domain_name),
                  "%s", domain_name );
        snprintf( entry->alarm_entity_name, sizeof(entry->alarm_entity_name),
                  "%s", entity_name );

    } else if( alarm_state == entry->alarm_state ) {
        if(( NULL == data )&&( !(entry->alarm_data_valid) ))
        {
            throttle = true;

        } else if(( NULL != data )&&( entry->alarm_data_valid )) {
            if( 0 == memcmp( &(entry->alarm_data), data, sizeof(SmAlarmDataT) ) )
            {
                throttle = true;
            }
        }
    }

    if( !throttle )
    {
        entry->alarm_state = alarm_state;

        if( NULL == data )
        {
            entry->alarm_data_valid = false;
            memset( &(entry->alarm_data), 0, sizeof(SmAlarmDataT) );
        } else {
            entry->alarm_data_valid = true;
            memcpy( &(entry->alarm_data), data, sizeof(SmAlarmDataT) );
        }
    }

RELEASE_MUTEX:
    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        abort();
    }

    return( throttle );
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Write Alarm Additional Text
// ===================================
bool sm_alarm_write_alarm_additional_text( const char data[],
    char alarm_additional_text[], int alarm_additional_text_size, int reserve )
{
    bool have_space = true;

    if( '\0' == alarm_additional_text[0] )
    {
        snprintf( alarm_additional_text, alarm_additional_text_size,
                  "%s", data );
    } else {
        int len = strlen(alarm_additional_text);
        int left = alarm_additional_text_size - len - 1;
        int data_len = (int) strlen(data);

        if( left > (data_len + reserve + 16) ) // reserve extra room
        {
            snprintf( &(alarm_additional_text[len]), left, ", %s", data );
        } else {
            snprintf( &(alarm_additional_text[len]), left, ", ..." );
            have_space = false;
        }
    }

    return( have_space );
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Raise
// =============
static void sm_alarm_raise( const SmAlarmT alarm, 
    const SmAlarmNodeNameT node_name, const SmAlarmDomainNameT domain_name,
    const SmAlarmEntityNameT entity_name, SmAlarmDataT* data )
{
    SmAlarmThreadMsgT msg;
    SmAlarmThreadMsgRaiseAlarmT* raise_alarm = &(msg.u.raise_alarm);
    int result;

    if( sm_alarm_throttle( alarm, SM_ALARM_STATE_RAISING, node_name,
                           domain_name, entity_name, data ) )
    {
        DPRINTFD( "Already raising alarm (%i) for node (%s) domain (%s) "
                  "entity (%s).", alarm, node_name, domain_name,
                  entity_name );
        return;
    }

    DPRINTFD( "Raise alarm (%i) for node (%s) domain (%s) entity (%s).",
              alarm, node_name, domain_name, entity_name );

    memset( &msg, 0, sizeof(msg) );

    msg.type = SM_ALARM_THREAD_MSG_RAISE_ALARM;

    clock_gettime( CLOCK_REALTIME, &(raise_alarm->ts_real) );

    raise_alarm->alarm = alarm;

    snprintf( raise_alarm->node_name, sizeof(raise_alarm->node_name),
              "%s", node_name );
    snprintf( raise_alarm->domain_name, sizeof(raise_alarm->domain_name),
              "%s", domain_name );
    snprintf( raise_alarm->entity_name, sizeof(raise_alarm->entity_name),
              "%s", entity_name );

    memcpy( &(raise_alarm->data), data, sizeof(SmAlarmDataT) );

    result = send( _client_fd, &msg, sizeof(msg), 0 );
    if( 0 > result )
    {
        DPRINTFE( "Failed to raise alarm (%i) for domain (%s) entity (%s), "
                  "error=%s.", alarm, domain_name, entity_name, 
                  strerror( errno ) );
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Raise State Alarm
// =========================
void sm_alarm_raise_state_alarm( const SmAlarmT alarm,
    const SmAlarmNodeNameT node_name, const SmAlarmDomainNameT domain_name,
    const SmAlarmEntityNameT entity_name, const SmAlarmSeverityT severity,
    const SmAlarmProbableCauseT probable_cause, const char entity_state[],
    const char entity_status[], const char entity_condition[],
    const SmAlarmSpecificProblemTextT specific_problem_text,
    const SmAlarmAdditionalTextT additional_text,
    const SmAlarmProposedRepairActionT proposed_repair_action,
    bool suppression_allowed )
{
    SmAlarmDataT data;

    memset( &data, 0, sizeof(SmAlarmDataT) );

    data.event_type = SM_ALARM_EVENT_TYPE_PROCESSING_ERROR_ALARM;
    data.probable_cause = probable_cause;

    snprintf( data.specific_problem_text, sizeof(data.specific_problem_text),
              "%s", specific_problem_text );

    data.perceived_severity = severity;
    data.trend_indication = SM_ALARM_TREND_INDICATION_NO_CHANGE;

    data.state_info.applicable = true;
    snprintf( data.state_info.state, sizeof(data.state_info.state),
              "%s", entity_state );
    snprintf( data.state_info.status, sizeof(data.state_info.status),
              "%s", entity_status );
    snprintf( data.state_info.condition, sizeof(data.state_info.condition),
              "%s", entity_condition );

    data.threshold_info.applicable = false;
    
    if( '\0' == proposed_repair_action[0] )
    {
        snprintf( data.proposed_repair_action,
                  sizeof(data.proposed_repair_action),
                  "contact next level of support" );
    } else {
        snprintf( data.proposed_repair_action,
                  sizeof(data.proposed_repair_action), "%s",
                  proposed_repair_action );
    }

    snprintf( data.additional_text, sizeof(data.additional_text), "%s",
              additional_text );

    data.service_affecting = true;
    data.suppression_allowed = suppression_allowed;

    sm_alarm_raise( alarm, node_name, domain_name, entity_name, &data );
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Raise Threshold Alarm
// =============================
void sm_alarm_raise_threshold_alarm( const SmAlarmT alarm,
    const SmAlarmNodeNameT node_name, const SmAlarmDomainNameT domain_name,
    const SmAlarmEntityNameT entity_name, const SmAlarmSeverityT severity,
    const SmAlarmProbableCauseT probable_cause,
    const unsigned int threshold_value, const unsigned int observed_value,
    const SmAlarmSpecificProblemTextT specific_problem_text,
    const SmAlarmAdditionalTextT additional_text,
    const SmAlarmProposedRepairActionT proposed_repair_action,
    bool suppression_allowed )
{
    SmAlarmDataT data;

    memset( &data, 0, sizeof(SmAlarmDataT) );

    data.event_type = SM_ALARM_EVENT_TYPE_PROCESSING_ERROR_ALARM;
    data.probable_cause = probable_cause;

    snprintf( data.specific_problem_text, sizeof(data.specific_problem_text),
              "%s", specific_problem_text );

    data.perceived_severity = severity;
    data.trend_indication = SM_ALARM_TREND_INDICATION_NO_CHANGE;
    data.state_info.applicable = false;

    data.threshold_info.applicable = true;
    data.threshold_info.threshold_value = threshold_value;
    data.threshold_info.observed_value = observed_value;

    if( '\0' == proposed_repair_action[0] )
    {
        snprintf( data.proposed_repair_action,
                  sizeof(data.proposed_repair_action),
                  "contact next level of support" );
    } else {
        snprintf( data.proposed_repair_action,
                  sizeof(data.proposed_repair_action), "%s",
                  proposed_repair_action );
    }

    snprintf( data.additional_text, sizeof(data.additional_text), "%s",
              additional_text );

    data.service_affecting = true;
    data.suppression_allowed = suppression_allowed;

    sm_alarm_raise( alarm, node_name, domain_name, entity_name, &data );
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Raise Node Alarm
// ========================
void sm_alarm_raise_node_alarm( const SmAlarmT alarm,
    const SmAlarmNodeNameT node_name, const SmAlarmSeverityT severity,
    const SmAlarmProbableCauseT probable_cause,
    const SmAlarmSpecificProblemTextT specific_problem_text,
    const SmAlarmAdditionalTextT additional_text,
    const SmAlarmProposedRepairActionT proposed_repair_action,
    bool suppression_allowed )
{
    SmAlarmDataT data;
    SmAlarmDomainNameT domain_name = "";
    SmAlarmEntityNameT entity_name = "";

    memset( &data, 0, sizeof(SmAlarmDataT) );

    data.event_type = SM_ALARM_EVENT_TYPE_PROCESSING_ERROR_ALARM;
    data.probable_cause = probable_cause;

    snprintf( data.specific_problem_text, sizeof(data.specific_problem_text),
              "%s", specific_problem_text );

    data.perceived_severity = severity;
    data.trend_indication = SM_ALARM_TREND_INDICATION_NO_CHANGE;
    data.state_info.applicable = false;
    data.threshold_info.applicable = false;

    if( '\0' == proposed_repair_action[0] )
    {
        snprintf( data.proposed_repair_action,
                  sizeof(data.proposed_repair_action),
                  "contact next level of support" );
    } else {
        snprintf( data.proposed_repair_action,
                  sizeof(data.proposed_repair_action), "%s",
                  proposed_repair_action );
    }

    snprintf( data.additional_text, sizeof(data.additional_text), "%s",
              additional_text );

    data.service_affecting = true;
    data.suppression_allowed = suppression_allowed;

    sm_alarm_raise( alarm, node_name, domain_name, entity_name, &data );
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Raise Software Alarm
// ============================
void sm_alarm_raise_software_alarm( const SmAlarmT alarm,
    const SmAlarmNodeNameT node_name, const SmAlarmSeverityT severity,
    const SmAlarmProbableCauseT probable_cause,
    const SmAlarmSpecificProblemTextT specific_problem_text,
    const SmAlarmAdditionalTextT additional_text,
    const SmAlarmProposedRepairActionT proposed_repair_action,
    bool suppression_allowed )
{
    SmAlarmDataT data;
    SmAlarmDomainNameT domain_name = "";
    SmAlarmEntityNameT entity_name = "";

    memset( &data, 0, sizeof(SmAlarmDataT) );

    data.event_type = SM_ALARM_EVENT_TYPE_PROCESSING_ERROR_ALARM;
    data.probable_cause = probable_cause;

    snprintf( data.specific_problem_text, sizeof(data.specific_problem_text),
              "%s", specific_problem_text );

    data.perceived_severity = severity;
    data.trend_indication = SM_ALARM_TREND_INDICATION_NO_CHANGE;
    data.state_info.applicable = false;
    data.threshold_info.applicable = false;

    if( '\0' == proposed_repair_action[0] )
    {
        snprintf( data.proposed_repair_action,
                  sizeof(data.proposed_repair_action),
                  "contact next level of support" );
    } else {
        snprintf( data.proposed_repair_action,
                  sizeof(data.proposed_repair_action), "%s",
                  proposed_repair_action );
    }

    snprintf( data.additional_text, sizeof(data.additional_text), "%s",
              additional_text );

    data.service_affecting = true;
    data.suppression_allowed = suppression_allowed;

    sm_alarm_raise( alarm, node_name, domain_name, entity_name, &data );
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Raise Communication Alarm
// =================================
void sm_alarm_raise_communication_alarm( const SmAlarmT alarm,
    const SmAlarmNodeNameT node_name, const SmAlarmEntityNameT entity_name,
    const SmAlarmSeverityT severity,
    const SmAlarmProbableCauseT probable_cause,
    const SmAlarmSpecificProblemTextT specific_problem_text,
    const SmAlarmAdditionalTextT additional_text,
    const SmAlarmProposedRepairActionT proposed_repair_action,
    bool suppression_allowed )
{
    SmAlarmDataT data;
    SmAlarmDomainNameT domain_name = "";

    memset( &data, 0, sizeof(SmAlarmDataT) );

    data.event_type = SM_ALARM_EVENT_TYPE_COMMUNICATIONS_ALARM;
    data.probable_cause = probable_cause;

    snprintf( data.specific_problem_text, sizeof(data.specific_problem_text),
              "%s", specific_problem_text );

    data.perceived_severity = severity;
    data.trend_indication = SM_ALARM_TREND_INDICATION_NO_CHANGE;
    data.state_info.applicable = false;
    data.threshold_info.applicable = false;

    if( '\0' == proposed_repair_action[0] )
    {
        snprintf( data.proposed_repair_action,
                  sizeof(data.proposed_repair_action),
                  "contact next level of support" );
    } else {
        snprintf( data.proposed_repair_action,
                  sizeof(data.proposed_repair_action), "%s",
                  proposed_repair_action );
    }

    snprintf( data.additional_text, sizeof(data.additional_text), "%s",
              additional_text );

    data.service_affecting = true;
    data.suppression_allowed = suppression_allowed;

    sm_alarm_raise( alarm, node_name, domain_name, entity_name, &data );
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Clear
// =============
void sm_alarm_clear( const SmAlarmT alarm, const SmAlarmNodeNameT node_name,
    const SmAlarmDomainNameT domain_name, const SmAlarmEntityNameT entity_name )
{
    SmAlarmThreadMsgT msg;
    SmAlarmThreadMsgClearAlarmT* clear_alarm = &(msg.u.clear_alarm);
    int result;

    if( sm_alarm_throttle( alarm, SM_ALARM_STATE_CLEARING, node_name,
                           domain_name, entity_name, NULL ) )
    {
        DPRINTFD( "Already clearing alarm (%i) for node (%s) domain (%s) "
                  "entity (%s).", alarm, node_name, domain_name,
                  entity_name );
        return;
    }

    DPRINTFD( "Clear alarm (%i) for node (%s) domain (%s) entity (%s).",
              alarm, node_name, domain_name, entity_name );

    memset( &msg, 0, sizeof(msg) );

    msg.type = SM_ALARM_THREAD_MSG_CLEAR_ALARM;

    clock_gettime( CLOCK_REALTIME, &(clear_alarm->ts_real) );

    clear_alarm->alarm = alarm;

    snprintf( clear_alarm->node_name, sizeof(clear_alarm->node_name),
              "%s", node_name );
    snprintf( clear_alarm->domain_name, sizeof(clear_alarm->domain_name),
              "%s", domain_name );
    snprintf( clear_alarm->entity_name, sizeof(clear_alarm->entity_name),
              "%s", entity_name );

    result = send( _client_fd, &msg, sizeof(msg), 0 );
    if( 0 > result )
    {
        DPRINTFE( "Failed to clear alarm (%i) for domain (%s) entity (%s), "
                  "error=%s.", alarm, domain_name, entity_name, 
                  strerror( errno ) );
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Clear All
// =================
void sm_alarm_clear_all( const SmAlarmDomainNameT domain_name )
{
    SmAlarmThreadMsgT msg;
    SmAlarmThreadMsgClearAllAlarmsT* clear_all_alarms = &(msg.u.clear_all_alarms);
    int result;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        abort();
    }

    memset( _alarms, 0, sizeof(_alarms) );

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        abort();
    }

    DPRINTFD( "Clear all alarms." );

    memset( &msg, 0, sizeof(msg) );

    msg.type = SM_ALARM_THREAD_MSG_CLEAR_ALL_ALARMS;

    clock_gettime( CLOCK_REALTIME, &(clear_all_alarms->ts_real) );

    snprintf( clear_all_alarms->domain_name,
              sizeof(clear_all_alarms->domain_name), "%s", domain_name );

    result = send( _client_fd, &msg, sizeof(msg), 0 );
    if( 0 > result )
    {
        DPRINTFE( "Failed to clear all alarms for domain (%s), error=%s.",
                  domain_name, strerror( errno ) );
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Manage Domain Alarms
// ============================
void sm_alarm_manage_domain_alarms( const SmAlarmDomainNameT domain_name,
    bool manage )
{
    SmAlarmThreadMsgT msg;
    SmAlarmThreadMsgManageDomainAlarmsT* manage_alarms = &(msg.u.manage_alarms);
    int result;

    DPRINTFD( "Manage domain (%s) alarms, set_to=%s.", domain_name,
              (manage) ? "yes" : "no" );

    memset( &msg, 0, sizeof(msg) );

    msg.type = SM_ALARM_THREAD_MSG_MANAGE_DOMAIN_ALARMS;

    manage_alarms->manage = manage;
    snprintf( manage_alarms->domain_name, sizeof(manage_alarms->domain_name),
              "%s", domain_name );

    result = send( _client_fd, &msg, sizeof(msg), 0 );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set manage alarms for domain (%s), error=%s.",
                  domain_name, strerror( errno ) );
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Initialize
// ==================
SmErrorT sm_alarm_initialize( void )
{
    int flags;
    int sockets[2];
    int buffer_len = 1048576;
    int result;
    SmErrorT error;

    DPRINTFD( "Alarm message size is %i bytes.",
              (int) sizeof(SmAlarmThreadMsgT) );

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        abort();
    }

    memset( _alarms, 0, sizeof(_alarms) );

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        abort();
    }
 
    result = socketpair( AF_UNIX, SOCK_DGRAM, 0, sockets );
    if( 0 > result )
    {
        DPRINTFE( "Failed to create alarm communication sockets, error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    int socket_i;
    for( socket_i=0; socket_i < 2; ++socket_i )
    {
        flags = fcntl( sockets[socket_i] , F_GETFL, 0 );
        if( 0 > flags )
        {
            DPRINTFE( "Failed to get alarm communication socket (%i) flags, "
                      "error=%s.", socket_i, strerror( errno ) );
            return( SM_FAILED );
        }

        result = fcntl( sockets[socket_i], F_SETFL, flags | O_NONBLOCK );
        if( 0 > result )
        {
            DPRINTFE( "Failed to set alarm communication socket (%i) to "
                      "non-blocking, error=%s.", socket_i, 
                      strerror( errno ) );
            return( SM_FAILED );
        }

        result = setsockopt( sockets[socket_i], SOL_SOCKET, SO_SNDBUF,
                             &buffer_len, sizeof(buffer_len) );
        if( 0 > result )
        {
            DPRINTFE( "Failed to set alarm communication socket (%i) "
                      "send buffer length (%i), error=%s.", socket_i,
                      buffer_len, strerror( errno ) );
            return( SM_FAILED );
        }

        result = setsockopt( sockets[socket_i], SOL_SOCKET, SO_RCVBUF,
                             &buffer_len, sizeof(buffer_len) );
        if( 0 > result )
        {
            DPRINTFE( "Failed to set alarm communication socket (%i) "
                      "receive buffer length (%i), error=%s.", socket_i,
                      buffer_len, strerror( errno ) );
            return( SM_FAILED );
        }
    }

    _server_fd = sockets[0];
    _client_fd = sockets[1];

    error = sm_alarm_thread_start( _server_fd );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to start alarm thread, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }
    
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Finalize
// ================
SmErrorT sm_alarm_finalize( void )
{
    SmErrorT error;
    
    error = sm_alarm_thread_stop();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to stop alarm thread, error=%s.",
                  sm_error_str( error ) );
    }

    if( -1 < _server_fd )
    {
        close( _server_fd );
        _server_fd = -1;
    }

    if( -1 < _client_fd )
    {
        close( _client_fd );
        _client_fd = -1;
    }

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        abort();
    }

    memset( _alarms, 0, sizeof(_alarms) );

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        abort();
    }
    
    return( SM_OKAY );
}
// ****************************************************************************
