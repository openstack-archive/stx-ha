//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_log_thread.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h> 
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <pthread.h>

#include "fm_api_wrapper.h"

#include "sm_types.h"
#include "sm_list.h"
#include "sm_time.h"
#include "sm_selobj.h"
#include "sm_timer.h"
#include "sm_debug.h"
#include "sm_trap.h"
#include "sm_thread_health.h"

#define SM_LOG_WRITE_BRIEF_LOGS

#define SM_LOG_THREAD_NAME                                          "sm_log"
#define SM_LOG_THREAD_LOG_REPORT_TIMER_IN_MS                            5000
#define SM_LOG_THREAD_TICK_INTERVAL_IN_MS                               5000
#define SM_LOG_CUSTOMER_LOG_FILE                  "/var/log/sm-customer.log"

#define SM_LOG_ENTRY_INUSE                                        0xFDFDFDFD

typedef struct
{
    uint32_t inuse;
    SFmAlarmDataT fm_log_data;
} SmLogEntryT;

static sig_atomic_t _stay_on;
static bool _thread_created = false;
static pthread_t _log_thread;
static SmTimerIdT _log_report_timer_id = SM_TIMER_ID_INVALID;
static int _server_fd = -1;
static bool _server_fd_registered = false;
static SmListT* _logs = NULL;
static SmLogEntryT _log_storage[SM_LOGS_MAX];
static uint64_t _customer_log_id = 0;
static FILE * _customer_log = NULL;

// ****************************************************************************
// Log Thread - Log String
// =======================
static const char * sm_log_thread_log_str( SmLogT log )
{
    switch( log )
    {
        case SM_LOG_UNKNOWN:
            return( "unknown" );
        break;

        case SM_LOG_SERVICE_GROUP_STATE:
            return( "service-group-state" );
        break;

        case SM_LOG_SERVICE_GROUP_REDUNDANCY:
            return( "service-group-redundancy" );
        break;

        case SM_LOG_SOFTWARE_MODIFICATION:
            return( "software-modification" );
        break;

        case SM_LOG_COMMUNICATION_FAILURE:
            return( "communication-failure" );
        break;

        default:
            return ( "???" );
        break;
    }

    return( "???" );
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Get Fault Management Log Identifier
// ================================================
static SmErrorT sm_log_thread_get_fm_log_id( SmLogT log,
    const SmLogNodeNameT node_name, const SmLogDomainNameT domain_name,
    const SmLogServiceGroupNameT service_group_name, 
    const SmLogEntityNameT entity_name, char fm_log_id[],
    char fm_entity_type_id[], char fm_entity_instance_id[] )
{
    switch( log )
    {
        case SM_LOG_SERVICE_GROUP_STATE:
            snprintf( fm_log_id, FM_MAX_BUFFER_LENGTH, 
                      SM_LOG_SERVICE_GROUP_STATE_LOG_ID );
            snprintf( fm_entity_type_id, FM_MAX_BUFFER_LENGTH,
                      "service_domain.service_group.host" );
            snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH,
                      "service_domain=%s.service_group=%s.host=%s",
                      domain_name, entity_name, node_name );
        break;

        case SM_LOG_SERVICE_GROUP_REDUNDANCY:
            snprintf( fm_log_id, FM_MAX_BUFFER_LENGTH, 
                      SM_LOG_SERVICE_GROUP_REDUNDANCY_LOG_ID );
            snprintf( fm_entity_type_id, FM_MAX_BUFFER_LENGTH,
                      "service_domain.service_group" );
            snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH,
                      "service_domain=%s.service_group=%s",
                      domain_name, entity_name );
        break;

        case SM_LOG_NODE_STATE:
            snprintf( fm_log_id, FM_MAX_BUFFER_LENGTH,
                      SM_LOG_NODE_STATE_LOG_ID );
            snprintf( fm_entity_type_id, FM_MAX_BUFFER_LENGTH, "host" );
            snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH,
                      "host=%s", node_name );
        break;

        case SM_LOG_SOFTWARE_MODIFICATION:
            snprintf( fm_log_id, FM_MAX_BUFFER_LENGTH,
                      SM_LOG_SOFTWARE_MODIFICATION_LOG_ID );
            snprintf( fm_entity_type_id, FM_MAX_BUFFER_LENGTH, "host" );
            snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH,
                      "host=%s", node_name );
        break;

        case SM_LOG_COMMUNICATION_FAILURE:
            snprintf( fm_log_id, FM_MAX_BUFFER_LENGTH,
                      SM_LOG_COMMUNICATION_FAILURE_LOG_ID );
            snprintf( fm_entity_type_id, FM_MAX_BUFFER_LENGTH, 
                      "host.network" );
            snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH,
                      "host=%s.network=%s", node_name,
                      entity_name );
        break;

        case SM_LOG_SERVICE_STATE:
            snprintf( fm_log_id, FM_MAX_BUFFER_LENGTH,
                      SM_LOG_SERVICE_STATE_LOG_ID );
            snprintf( fm_entity_type_id, FM_MAX_BUFFER_LENGTH,
                      "service_domain.service_group.service.host" );
            snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH,
                      "service_domain=%s.service_group=%s.service=%s.host=%s",
                      domain_name, service_group_name, entity_name, node_name );
        break;
        
        default:
            DPRINTFE( "Unknown log (%i).", log );
            return( SM_FAILED );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Get Fault Management Log Type
// ==========================================
static EFmAlarmTypeT sm_log_thread_get_fm_log_type( SmLogEventTypeT event_type )
{
    switch( event_type )
    {
        case SM_LOG_EVENT_TYPE_UNKNOWN:
            return( FM_ALARM_TYPE_UNKNOWN );
        break;

        case SM_LOG_EVENT_TYPE_COMMUNICATIONS_ALARM:
            return( FM_ALARM_COMM );
        break;

        case SM_LOG_EVENT_TYPE_PROCESSING_ERROR_ALARM:
            return( FM_ALARM_PROCESSING_ERROR );
        break;

        case SM_LOG_EVENT_TYPE_ENVIRONMENTAL_ALARM:
            return( FM_ALARM_ENVIRONMENTAL );
        break;

        case SM_LOG_EVENT_TYPE_QUALITY_OF_SERVICE_ALARM:
            return( FM_ALARM_QOS );
        break;

        case SM_LOG_EVENT_TYPE_EQUIPMENT_ALARM:
            return( FM_ALARM_EQUIPMENT );
        break;

        case SM_LOG_EVENT_TYPE_INTEGRITY_VIOLATION:
            return( FM_ALARM_INTERGRITY );
        break;

        case SM_LOG_EVENT_TYPE_OPERATIONAL_VIOLATION:
            return( FM_ALARM_OPERATIONAL );
        break;

        case SM_LOG_EVENT_TYPE_PHYSICAL_VIOLATION:
            return( FM_ALARM_PHYSICAL );
        break;

        case SM_LOG_EVENT_TYPE_SECURITY_SERVICE_VIOLATION:
            return( FM_ALARM_SECURITY );
        break;

        case SM_LOG_EVENT_TYPE_MECHANISM_VIOLATION:
            return( FM_ALARM_SECURITY );
        break;

        case SM_LOG_EVENT_TYPE_TIME_DOMAIN_VIOLATION:
            return( FM_ALARM_TIME );
        break;

        default:
            DPRINTFI( "Unknown event type (%i).", event_type );
            return( FM_ALARM_TYPE_UNKNOWN );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Log Brief Header
// =============================
static void sm_log_thread_log_brief_header( void )
{
    // Output the brief log header every so many logs.
    if( 0 == ( _customer_log_id % 512 ) )
    {
        fprintf( _customer_log, "+-------------------------+------------+----------------------+----------------------------------+----------------------------------+----------------------------------+---------------------------------------------------------------------------\n" );
        fprintf( _customer_log, "| timestamp               | log-id     | log-type             | entity-name                      | from-state                       | to-state                         | reason                                                                    \n" );
        fprintf( _customer_log, "+-------------------------+------------+----------------------+----------------------------------+----------------------------------+----------------------------------+---------------------------------------------------------------------------\n" );
    }
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Log Brief
// ======================
static void sm_log_thread_log_brief( const char time[], const char type[],
    const char entity_name[], const char prev_state_change[], 
    const char state_change[],
    char reason_text[] )
{
    fprintf( _customer_log, "| %s | %10" PRIu64 " | %-20s | %-32s | "
             "%-32s | %-32s | %-s \n", time, ++_customer_log_id, type,
             entity_name, prev_state_change, state_change, reason_text );
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Log Reboot
// =======================
static void sm_log_thread_log_reboot( const char reboot_type[],
    SmLogThreadMsgRebootLogT* reboot )
{
    char time_str[80];
    char date_str[32];
    struct tm t_real;

    if( NULL == localtime_r( &(reboot->ts_real.tv_sec), &t_real ) )
    {
        snprintf( time_str, sizeof(time_str), "YYYY:MM:DD HH:MM:SS.xxx" );
    } else {
        strftime( date_str, sizeof(date_str), "%FT%T", &t_real );
        snprintf( time_str, sizeof(time_str), "%s.%03ld", date_str,
                  reboot->ts_real.tv_nsec/1000000 );
    }

#ifdef SM_LOG_WRITE_BRIEF_LOGS
    sm_log_thread_log_brief_header();

    sm_log_thread_log_brief( time_str, reboot_type, reboot->entity_name,
                             "", "", reboot->reason_text );
#else
    fprintf( _customer_log, "log-id:      %" PRIu64 "\n", ++_customer_log_id );
    fprintf( _customer_log, "log-type:    reboot\n" );
    fprintf( _customer_log, "timestamp:   %s\n", time_str );
    fprintf( _customer_log, "entity-name: %s\n", reboot->entity_name );
    fprintf( _customer_log, "reason-text: %s\n\n", reboot->reason_text );
#endif // SM_LOG_WRITE_BRIEF_LOGS

    fflush( _customer_log );
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Log State Change
// =============================
static void sm_log_thread_log_state_change( const char state_change_type[],
    SmLogThreadMsgStateChangeLogT* state_change )
{
    char time_str[80];
    char prev_state_change_str[80];
    char state_change_str[80];
    char date_str[32];
    struct tm t_real;

    if( NULL == localtime_r( &(state_change->ts_real.tv_sec), &t_real ) )
    {
        snprintf( time_str, sizeof(time_str), "YYYY:MM:DD HH:MM:SS.xxx" );
    } else {
        strftime( date_str, sizeof(date_str), "%FT%T", &t_real );
        snprintf( time_str, sizeof(time_str), "%s.%03ld", date_str,
                  state_change->ts_real.tv_nsec/1000000 );
    }

    snprintf( prev_state_change_str, sizeof(prev_state_change_str), "%s%s%s",
              state_change->prev_state,
              ('\0' == state_change->prev_status[0]) ? "" : "-",
              ('\0' == state_change->prev_status[0]) ? "" : state_change->prev_status );

    snprintf( state_change_str, sizeof(state_change_str), "%s%s%s",
              state_change->state,
              ('\0' == state_change->status[0]) ? "" : "-",
              ('\0' == state_change->status[0]) ? "" : state_change->status );

#ifdef SM_LOG_WRITE_BRIEF_LOGS
    sm_log_thread_log_brief_header();

    sm_log_thread_log_brief( time_str, state_change_type,
                             state_change->entity_name,
                             prev_state_change_str, state_change_str,
                             state_change->reason_text );
#else
    fprintf( _customer_log, "log-id:          %" PRIu64 "\n", ++_customer_log_id );
    fprintf( _customer_log, "log-type:        state-change\n" );
    fprintf( _customer_log, "timestamp:       %s\n", time_str );
    fprintf( _customer_log, "entity-name:     %s\n", state_change->entity_name );
    fprintf( _customer_log, "previous-state:  %s\n", state_change->prev_state );
    fprintf( _customer_log, "previous-status: %s\n",
             ('\0' == state_change->prev_status[0]) ? "" : state_change->prev_status );
    fprintf( _customer_log, "current-state:   %s\n", state_change->state );
    fprintf( _customer_log, "current-status:  %s\n",
             ('\0' == state_change->status[0]) ? "" : state_change->status );
    fprintf( _customer_log, "reason-text:     %s\n\n", state_change->reason_text );
#endif // SM_LOG_WRITE_BRIEF_LOGS

    fflush( _customer_log );
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Log Generate 
// =========================
static void sm_log_thread_log_generate( SmLogT log, SmLogNodeNameT node_name,
    SmLogDomainNameT domain_name, SmLogServiceGroupNameT service_group_name, 
    SmLogEntityNameT entity_name, SmLogEventTypeT event_type,
    const char log_text[] )
{
    struct timeval tv;
    SmLogEntryT* log_entry;
    SFmAlarmDataT* fm_log_data;
    SmErrorT error;

    memset(&tv,0,sizeof(tv));

    unsigned int log_entry_i;
    for( log_entry_i=0; SM_LOGS_MAX > log_entry_i; ++log_entry_i )
    {
        log_entry = &(_log_storage[log_entry_i]);

        if( SM_LOG_ENTRY_INUSE != log_entry->inuse )
            break;
    }

    if( SM_LOGS_MAX <= log_entry_i )
    {
        SmListT* entry = NULL;
        SmListEntryDataPtrT entry_data;

        SM_LIST_FIRST( _logs, entry, entry_data  );

        DPRINTFI( "Max logs (%i) buffered has been reached, deleting oldest",
                  SM_LOGS_MAX );

        SM_LIST_REMOVE( _logs, (SmListEntryDataPtrT) entry_data );
        log_entry = (SmLogEntryT*) entry_data;
    }

    memset( log_entry, 0, sizeof(SmLogEntryT) );
    fm_log_data = &(log_entry->fm_log_data);

    error = sm_log_thread_get_fm_log_id( log, node_name, domain_name,
                service_group_name, entity_name, fm_log_data->alarm_id,
                fm_log_data->entity_type_id, fm_log_data->entity_instance_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get log (%s) data for node (%s) domain (%s) "
                  "service-group (%s) entity (%s), error=%s.",
                  sm_log_thread_log_str( log ), node_name, domain_name,
                  service_group_name, entity_name, sm_error_str( error ) );
        return;
    }
    
    fm_log_data->alarm_state = FM_ALARM_STATE_MSG;
    fm_log_data->severity = FM_ALARM_SEVERITY_CRITICAL;

    snprintf( fm_log_data->reason_text, sizeof(fm_log_data->reason_text),
              "%s", log_text );

    if( isalpha( fm_log_data->reason_text[0] ) )
    {
        fm_log_data->reason_text[0] = toupper( fm_log_data->reason_text[0] );
    }

    if( 0 == gettimeofday( &tv, NULL ) )
    {
        fm_log_data->timestamp = ((((FMTimeT)tv.tv_sec) * 1000000) +
                                  ((FMTimeT)tv.tv_usec));
    } else {
        DPRINTFE( "Failed to get time of day, errno=%s.", strerror( errno ) );
    }

    fm_log_data->alarm_type = sm_log_thread_get_fm_log_type( event_type );
    fm_log_data->probable_cause = FM_ALARM_UNSPECIFIED_REASON;
    fm_log_data->proposed_repair_action[0] = '\0';
    fm_log_data->service_affecting = FM_TRUE;
    fm_log_data->suppression = FM_TRUE;
    fm_log_data->inhibit_alarms = FM_FALSE;

    log_entry->inuse = SM_LOG_ENTRY_INUSE;
    SM_LIST_APPEND( _logs, (SmListEntryDataPtrT) log_entry );
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Report
// ===================
static void sm_log_thread_report( void )
{
    SmListT* entry = NULL;
    SmListT* next = NULL;
    SmListEntryDataPtrT entry_data;
    SmLogEntryT* log_entry;
    fm_uuid_t fm_uuid;
    EFmErrorT fm_error;

    SM_LIST_FOREACH_SAFE( _logs, entry, next, entry_data )
    {
        log_entry = (SmLogEntryT*) entry_data;

        fm_error = fm_set_fault_wrapper( &(log_entry->fm_log_data), &fm_uuid );
        if( FM_ERR_OK == fm_error )
        {
            DPRINTFD( "Set log (%s) for entity (%s), reason_text=%s.",
                      log_entry->fm_log_data.alarm_id,
                      log_entry->fm_log_data.entity_instance_id,
                      log_entry->fm_log_data.reason_text );
        } else {
            DPRINTFE( "Failed to set log (%s) for entity (%s), error=%i",
                      log_entry->fm_log_data.alarm_id,
                      log_entry->fm_log_data.entity_instance_id, fm_error );
            return;
        }

        memset( log_entry, 0, sizeof(SmLogEntryT) );
        SM_LIST_REMOVE( _logs,(SmListEntryDataPtrT) log_entry );
    }
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Dispatch
// =====================
static void sm_log_thread_dispatch( int selobj, int64_t user_data )
{
    bool report = false;
    char prev_state_change_str[80];
    char state_change_str[80];
    char log_text[1024] = "";
    int bytes_read;
    SmLogThreadMsgT msg;
    SmLogThreadMsgRebootLogT* reboot = &(msg.u.reboot);
    SmLogThreadMsgStateChangeLogT* state_change = &(msg.u.state_change);

    memset( &msg, 0, sizeof(msg) );

    int retry_count;
    for( retry_count = 5; retry_count != 0; --retry_count )
    {
        bytes_read = recv( selobj, &msg, sizeof(msg), 0 );
        if( 0 < bytes_read )
        {
            break;

        } else if( 0 == bytes_read ) {
            // For connection oriented sockets, this indicates that the peer 
            // has performed an orderly shutdown.
            return;

        } else if(( 0 > bytes_read )&&( EINTR != errno )) {
            DPRINTFE( "Failed to receive message, errno=%s.",
                      strerror( errno ) );
            return;
        }

        DPRINTFD( "Interrupted while receiving message, retry=%d, errno=%s.",
                   retry_count, strerror( errno ) );
    }

    switch( msg.type )
    {
        case SM_LOG_THREAD_MSG_NODE_REBOOT_LOG:
            sm_log_thread_log_reboot( "node-reboot", reboot );
        break;

        case SM_LOG_THREAD_MSG_NODE_REBOOT_FORCE_LOG:
            sm_log_thread_log_reboot( "node-reboot-force", reboot );
        break;

        case SM_LOG_THREAD_MSG_NODE_STATE_CHANGE_LOG:
            sm_log_thread_log_state_change( "node-scn", state_change );

            if( 0 == strcmp( "swact", state_change->state ) )
            {
                snprintf( log_text, sizeof(log_text), "Swact %s",
                          state_change->reason_text );

                sm_log_thread_log_generate( SM_LOG_NODE_STATE,
                    state_change->node_name, state_change->domain_name,
                    state_change->service_group_name, state_change->entity_name,
                    SM_LOG_EVENT_TYPE_PROCESSING_ERROR_ALARM, log_text );

                report = true;

            } else if( 0 == strcmp( "swact-force", state_change->state ) ) {
                snprintf( log_text, sizeof(log_text), "Swact-Force %s",
                          state_change->reason_text );

                sm_log_thread_log_generate( SM_LOG_NODE_STATE,
                    state_change->node_name, state_change->domain_name,
                    state_change->service_group_name, state_change->entity_name,
                    SM_LOG_EVENT_TYPE_PROCESSING_ERROR_ALARM, log_text );

                report = true;
            }
        break;

        case SM_LOG_THREAD_MSG_INTERFACE_STATE_CHANGE_LOG:
            sm_log_thread_log_state_change( "interface-scn", state_change );
        break;

        case SM_LOG_THREAD_MSG_COMMUNICATION_STATE_CHANGE_LOG:
            sm_log_thread_log_generate( SM_LOG_COMMUNICATION_FAILURE,
                state_change->node_name, state_change->domain_name,
                state_change->service_group_name, state_change->entity_name,
                SM_LOG_EVENT_TYPE_PROCESSING_ERROR_ALARM,
                state_change->reason_text );

            report = true;
        break;

        case SM_LOG_THREAD_MSG_NEIGHBOR_STATE_CHANGE_LOG:
            sm_log_thread_log_state_change( "neighbor-scn", state_change );
        break;

        case SM_LOG_THREAD_MSG_SERVICE_DOMAIN_STATE_CHANGE_LOG:
            sm_log_thread_log_state_change( "service-domain-scn", state_change );
        break;

        case SM_LOG_THREAD_MSG_SERVICE_GROUP_REDUNDANCY_CHANGE_LOG:
            sm_log_thread_log_generate( SM_LOG_SERVICE_GROUP_REDUNDANCY,
                state_change->node_name, state_change->domain_name,
                state_change->service_group_name, state_change->entity_name,
                SM_LOG_EVENT_TYPE_PROCESSING_ERROR_ALARM,
                state_change->reason_text );

            report = true;
        break;

        case SM_LOG_THREAD_MSG_SERVICE_GROUP_STATE_CHANGE_LOG:
            sm_log_thread_log_state_change( "service-group-scn", state_change );

            snprintf( prev_state_change_str, sizeof(prev_state_change_str),
                      "%s%s%s", state_change->prev_state,
                      ('\0' == state_change->prev_status[0]) ? "" : "-",
                      ('\0' == state_change->prev_status[0]) ? "" :
                      state_change->prev_status );

            snprintf( state_change_str, sizeof(state_change_str), "%s%s%s",
                      state_change->state,
                      ('\0' == state_change->status[0]) ? "" : "-",
                      ('\0' == state_change->status[0]) ? "" :
                      state_change->status );

            if(( 0 == strcmp( sm_service_group_status_str(
                                SM_SERVICE_GROUP_STATUS_WARN ),
                                state_change->status ) ) ||
               ( 0 == strcmp( sm_service_group_status_str(
                                SM_SERVICE_GROUP_STATUS_DEGRADED ),
                                state_change->status ) ) ||
               ( 0 == strcmp( sm_service_group_status_str(
                                SM_SERVICE_GROUP_STATUS_FAILED ),
                                state_change->status ) ))
            {
                snprintf( log_text, sizeof(log_text),
                          "Service group %s state change from %s to %s on "
                          "host %s; %s", state_change->entity_name,
                          prev_state_change_str, state_change_str,
                          state_change->node_name, state_change->reason_text );
            } else {
                snprintf( log_text, sizeof(log_text),
                          "Service group %s state change from %s to %s on "
                          "host %s",
                          state_change->entity_name, prev_state_change_str,
                          state_change_str, state_change->node_name );
            }

            sm_log_thread_log_generate( SM_LOG_SERVICE_GROUP_STATE,
                state_change->node_name, state_change->domain_name,
                state_change->service_group_name, state_change->entity_name,
                SM_LOG_EVENT_TYPE_PROCESSING_ERROR_ALARM, log_text );

            report = true;
        break;

        case SM_LOG_THREAD_MSG_SERVICE_STATE_CHANGE_LOG:
            sm_log_thread_log_state_change( "service-scn", state_change );
        break;

        default:
            DPRINTFE( "Unknown message (%i) received.", msg.type );
            return;
        break;
    }

    if( report )
        sm_log_thread_report();
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Report Timer
// =========================
static bool sm_log_thread_report_timer( SmTimerIdT timer_id,
    int64_t user_data )
{
    sm_log_thread_report();
    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Initialize Thread
// ==============================
static SmErrorT sm_log_thread_initialize_thread( void )
{
    SmErrorT error;

    _server_fd_registered = false;

    error = sm_selobj_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize selection object module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    if( -1 < _server_fd )
    {
        error = sm_selobj_register( _server_fd, sm_log_thread_dispatch, 0 );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to register selection object, error=%s.",
                      sm_error_str( error ) );
            return( error );
        }    

        _server_fd_registered = true;
    }

    error = sm_timer_initialize( SM_LOG_THREAD_TICK_INTERVAL_IN_MS );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize timer module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_timer_register( "log report",
                               SM_LOG_THREAD_LOG_REPORT_TIMER_IN_MS,
                               sm_log_thread_report_timer, 0,
                               &_log_report_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create log report timer, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    if( NULL != _customer_log )
    {
        fflush( _customer_log );
        fclose( _customer_log );
        _customer_log = NULL;
    }

    _customer_log = fopen( SM_LOG_CUSTOMER_LOG_FILE, "a" );
    if( NULL == _customer_log )
    {
        DPRINTFE( "Failed to open customer log file (%s).", 
                  SM_LOG_CUSTOMER_LOG_FILE );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Finalize Thread
// ============================
static SmErrorT sm_log_thread_finalize_thread( void )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != _log_report_timer_id )
    {
        error = sm_timer_deregister( _log_report_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel log report timer, error=%s.",
                      sm_error_str( error ) );
        }

        _log_report_timer_id = SM_TIMER_ID_INVALID;
    }

    error = sm_timer_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize timer module, error=%s.",
                  sm_error_str( error ) );
    }
    
    if( _server_fd_registered )
    {
        error = sm_selobj_deregister( _server_fd );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object, error=%s.",
                      sm_error_str( error ) );
        }

        _server_fd_registered = false;
    }

    error = sm_selobj_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize selection object module, error=%s.",
                  sm_error_str( error ) );
    }

    if( NULL != _customer_log )
    {
        fflush( _customer_log );
        fclose( _customer_log );
        _customer_log = NULL;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Main
// =================
static void* sm_log_thread_main( void* arguments )
{
    SmErrorT error;

    pthread_setname_np( pthread_self(), SM_LOG_THREAD_NAME );
    sm_debug_set_thread_info();
    sm_trap_set_thread_info();

    DPRINTFI( "Starting" );

    // Warn after 60 seconds, fail after 16 minutes.
    error = sm_thread_health_register( SM_LOG_THREAD_NAME, 60000, 1000000 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register log thread, error=%s.",
                  sm_error_str( error ) );
        pthread_exit( NULL );
    }

    error = sm_log_thread_initialize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize log thread, error=%s.",
                  sm_error_str( error ) );
        pthread_exit( NULL );
    }

    DPRINTFI( "Started." );

    while( _stay_on )
    {
        error = sm_selobj_dispatch( SM_LOG_THREAD_TICK_INTERVAL_IN_MS );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Selection object dispatch failed, error=%s.",
                      sm_error_str(error) );
            break;
        }

        error = sm_thread_health_update( SM_LOG_THREAD_NAME );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to update log thread health, error=%s.",
                      sm_error_str(error) );
        }
    }

    DPRINTFI( "Shutting down." );

    error = sm_log_thread_finalize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize log thread, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_thread_health_deregister( SM_LOG_THREAD_NAME );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister log thread, error=%s.",
                  sm_error_str( error ) );
    }

    DPRINTFI( "Shutdown complete." );

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Start
// ==================
SmErrorT sm_log_thread_start( int server_fd )
{
    int result;

    _logs = NULL;
    memset( _log_storage, 0, sizeof(_log_storage) );

    _stay_on = 1;
    _thread_created = false;
    _server_fd = server_fd;

    result = pthread_create( &_log_thread, NULL, sm_log_thread_main, NULL );
    if( 0 != result )
    {
        DPRINTFE( "Failed to start log thread, error=%s.",
                  strerror(result) );
        return( SM_FAILED );
    }

    _thread_created = true;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Log Thread - Stop
// =================
SmErrorT sm_log_thread_stop( void )
{
    _stay_on = 0;
    
    if( _thread_created )
    {
        long ms_expired;
        SmTimeT time_prev;
        int result;

        sm_time_get( &time_prev );

        while( true )
        {
            result = pthread_tryjoin_np( _log_thread, NULL );
            if(( 0 != result )&&( EBUSY != result ))
            {
                if(( ESRCH != result )&&( EINVAL != result ))
                {
                    DPRINTFE( "Failed to wait for log thread exit, "
                              "sending kill signal, error=%s.",
                              strerror(result) );
                    pthread_kill( _log_thread, SIGKILL );
                }
                break;
            }

            ms_expired = sm_time_get_elapsed_ms( &time_prev );
            if( 5000 <= ms_expired )
            {
                DPRINTFE( "Failed to stop log thread, sending "
                          "kill signal." );
                pthread_kill( _log_thread, SIGKILL );
                break;
            }

            usleep( 250000 ); // 250 milliseconds.
        }
    }

    _server_fd = -1;
    _thread_created = false;

    memset( _log_storage, 0, sizeof(_log_storage) );
    SM_LIST_CLEANUP_ALL( _logs );

    return( SM_OKAY );
}
// ****************************************************************************
