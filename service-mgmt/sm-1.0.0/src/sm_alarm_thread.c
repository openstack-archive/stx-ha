//
// Copyright (c) 2014 -2016 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_alarm_thread.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h> 
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <pthread.h>

#include "fm_api_wrapper.h"

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_time.h"
#include "sm_debug.h"
#include "sm_trap.h"
#include "sm_timer.h"
#include "sm_selobj.h"
#include "sm_node_utils.h"
#include "sm_thread_health.h"

#define SM_ALARM_THREAD_NAME                                         "sm_alarm"
#define SM_ALARM_THREAD_ALARM_REPORT_TIMER_IN_MS                           5000
#define SM_ALARM_THREAD_ALARM_AUDIT_TIMER_IN_MS                           60000
#define SM_ALARM_THREAD_TICK_INTERVAL_IN_MS                                5000
#define SM_ALARM_CUSTOMER_LOG_FILE                 "/var/log/sm-customer.alarm"

#define SM_ALARM_ENTRY_INUSE                                         0xFDFDFDFD
#define SM_ALARM_DOMAIN_ENTRY_INUSE                                  0xFDFDFDFD

typedef enum
{
    SM_ALARM_STATE_UNKNOWN,
    SM_ALARM_STATE_INITIAL,
    SM_ALARM_STATE_CLEARING,
    SM_ALARM_STATE_CLEARED,
    SM_ALARM_STATE_RAISING,
    SM_ALARM_STATE_RAISED,
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
    SmAlarmDataT alarm_data;

    // Alarm Timestamps
    struct timespec alarm_raised_time;
    struct timespec alarm_changed_time;
    struct timespec alarm_cleared_time;

    // External FM Alarm Data
    fm_uuid_t fm_uuid;
    SFmAlarmDataT fm_alarm_data;
} SmAlarmEntryT;

typedef struct
{
    uint32_t inuse;
    bool manage;
    SmAlarmDomainNameT alarm_domain_name;
} SmAlarmDomainEntryT;

static sig_atomic_t _stay_on;
static bool _thread_created = false;
static pthread_t _alarm_thread;
static SmTimerIdT _alarm_report_timer_id = SM_TIMER_ID_INVALID;
static SmTimerIdT _alarm_audit_timer_id = SM_TIMER_ID_INVALID;
static int _server_fd = -1;
static bool _server_fd_registered = false;
static SmAlarmEntryT _alarms[SM_ALARMS_MAX];
static SmAlarmDomainEntryT _alarm_domains[SM_ALARM_DOMAINS_MAX];
static uint64_t _customer_alarm_log_id = 0;
static FILE * _customer_alarm_log = NULL;
 
// ****************************************************************************
// Alarm Thread - Event Type String
// ================================
static const char * sm_alarm_thread_event_type_str(
    SmAlarmEventTypeT event_type )
{
    switch( event_type )
    {
        case SM_ALARM_EVENT_TYPE_UNKNOWN:
            return( "unknown" );
        break;

        case SM_ALARM_EVENT_TYPE_COMMUNICATIONS_ALARM:
            return( "communications alarm" );
        break;

        case SM_ALARM_EVENT_TYPE_PROCESSING_ERROR_ALARM:
            return( "processing error alarm" );
        break;

        case SM_ALARM_EVENT_TYPE_ENVIRONMENTAL_ALARM:
            return( "environmental alarm" );
        break;

        case SM_ALARM_EVENT_TYPE_QUALITY_OF_SERVICE_ALARM:
            return( "quality of service alarm" );
        break;

        case SM_ALARM_EVENT_TYPE_EQUIPMENT_ALARM:
            return( "equipment alarm" );
        break;

        case SM_ALARM_EVENT_TYPE_INTEGRITY_VIOLATION:
            return( "integrity violation" );
        break;

        case SM_ALARM_EVENT_TYPE_OPERATIONAL_VIOLATION:
            return( "operational violation" );
        break;

        case SM_ALARM_EVENT_TYPE_PHYSICAL_VIOLATION:
            return( "physical violation" );
        break;

        case SM_ALARM_EVENT_TYPE_SECURITY_SERVICE_VIOLATION:
            return( "security service violation" );
        break;

        case SM_ALARM_EVENT_TYPE_MECHANISM_VIOLATION:
            return( "mechanism violation" );
        break;

        case SM_ALARM_EVENT_TYPE_TIME_DOMAIN_VIOLATION:
            return( "time domain violation" );
        break;

        default:
            return ( "???" );
        break;
    }

    return( "???" );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Probable Cause String
// ====================================
static const char * sm_alarm_thread_probable_cause_str(
    SmAlarmProbableCauseT probable_cause )
{
    switch( probable_cause )
    {
        case SM_ALARM_PROBABLE_CAUSE_UNKNOWN:
            return( "unknown" );
        break;

        case SM_ALARM_PROBABLE_CAUSE_INDETERMINATE:
            return( "indeterminate" );
        break;

        case SM_ALARM_PROBABLE_CAUSE_SOFTWARE_ERROR:
            return( "software error" );
        break;

        case SM_ALARM_PROBABLE_CAUSE_SOFTWARE_PROGRAM_ERROR:
            return( "software program error" );
        break;

        case SM_ALARM_PROBABLE_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE:
            return( "underlying resource unavailable" );
        break;

        case SM_ALARM_PROBABLE_CAUSE_KEY_EXPIRED:
            return( "key expired" );
        break;

        default:
            return ( "???" );
        break;
    }

    return( "???" );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Severity String
// ==============================
static const char * sm_alarm_thread_severity_str( SmAlarmSeverityT severity )
{
    switch( severity )
    {
        case SM_ALARM_SEVERITY_UNKNOWN:
            return( "unknown" );
        break;

        case SM_ALARM_SEVERITY_CLEARED:
            return( "cleared" );
        break;

        case SM_ALARM_SEVERITY_INDETERMINATE:
            return( "indeterminate" );
        break;

        case SM_ALARM_SEVERITY_WARNING:
            return( "warning" );
        break;

        case SM_ALARM_SEVERITY_MINOR:
            return( "minor" );
        break;

        case SM_ALARM_SEVERITY_MAJOR:
            return( "major" );
        break;

        case SM_ALARM_SEVERITY_CRITICAL:
            return( "critical" );
        break;

        default:
            return ( "???" );
        break;
    }

    return( "???" );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Trend String
// ===========================
static const char * sm_alarm_thread_trend_str( SmAlarmTrendIndicationT trend )
{
    switch( trend )
    {
        case SM_ALARM_TREND_INDICATION_UNKNOWN:
            return( "unknown" );
        break;

        case SM_ALARM_TREND_INDICATION_LESS_SEVERE:
            return( "less severe" );
        break;

        case SM_ALARM_TREND_INDICATION_NO_CHANGE:
            return( "no change" );
        break;

        case SM_ALARM_TREND_INDICATION_MORE_SEVERE:
            return( "more severe" );
        break;

        default:
            return ( "???" );
        break;
    }

    return( "???" );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Alarm String
// ===========================
static const char * sm_alarm_thread_alarm_str( SmAlarmT alarm )
{
    switch( alarm )
    {
        case SM_ALARM_UNKNOWN:
            return( "unknown" );
        break;

        case SM_ALARM_SERVICE_GROUP_STATE:
            return( "service-group-state" );
        break;

        case SM_ALARM_SERVICE_GROUP_REDUNDANCY:
            return( "service-group-redundancy" );
        break;

        case SM_ALARM_SOFTWARE_MODIFICATION:
            return( "software-modification" );
        break;

        case SM_ALARM_COMMUNICATION_FAILURE:
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
// Alarm Thread - Log
// ==================
static void sm_alarm_thread_log( SmAlarmEntryT* entry )
{
    char date_str[32];
    struct tm t_real;

    fprintf( _customer_alarm_log, "alarm-log-id:                  %" PRIu64 "\n",
             ++_customer_alarm_log_id );
    fprintf( _customer_alarm_log, "alarm-type:                    %s\n",
             sm_alarm_thread_alarm_str( entry->alarm ) );

    if( '\0' != entry->alarm_node_name[0] )
    {
        fprintf( _customer_alarm_log, "alarm-node:                    %s\n",
                 entry->alarm_node_name );
    }

    fprintf( _customer_alarm_log, "alarm-domain:                  %s\n",
             entry->alarm_domain_name );
    fprintf( _customer_alarm_log, "alarm-entity:                  %s\n",
             entry->alarm_entity_name );
    fprintf( _customer_alarm_log, "alarm-event-type:              %s\n",
             sm_alarm_thread_event_type_str( entry->alarm_data.event_type ) );
    fprintf( _customer_alarm_log, "alarm-probable-cause:          %s\n",
             sm_alarm_thread_probable_cause_str( entry->alarm_data.probable_cause ) );

    if( '\0' != entry->alarm_data.specific_problem_text[0] )
    {
        fprintf( _customer_alarm_log, "alarm-specific-problem-text:   %s\n",
                 entry->alarm_data.specific_problem_text );
    }

    fprintf( _customer_alarm_log, "alarm-perceived-severity:      %s\n",
             sm_alarm_thread_severity_str( entry->alarm_data.perceived_severity ) );

    if( SM_ALARM_SEVERITY_CLEARED != entry->alarm_data.perceived_severity )
    {
        fprintf( _customer_alarm_log, "alarm-trend-indication:        %s\n",
                 sm_alarm_thread_trend_str( entry->alarm_data.trend_indication ) );
    }

    if( entry->alarm_data.state_info.applicable )
    {
        fprintf( _customer_alarm_log, "alarm-entity-state:            %s\n", 
                 entry->alarm_data.state_info.state );
        fprintf( _customer_alarm_log, "alarm-entity-status:           %s\n",
                 entry->alarm_data.state_info.status );
        fprintf( _customer_alarm_log, "alarm-entity-condition:        %s\n",
                 entry->alarm_data.state_info.condition );
    }

    if( entry->alarm_data.threshold_info.applicable )
    {
        fprintf( _customer_alarm_log, "alarm-threshold-value:         %i\n",
                 entry->alarm_data.threshold_info.threshold_value );
        fprintf( _customer_alarm_log, "alarm-observed_value:          %i\n",
                 entry->alarm_data.threshold_info.observed_value );
    }

    if( '\0' != entry->alarm_data.proposed_repair_action[0] )
    {
        fprintf( _customer_alarm_log, "alarm-proposed-repair-action:  %s\n",
                 entry->alarm_data.proposed_repair_action );
    }

    if( '\0' != entry->alarm_data.additional_text[0] )
    {
        fprintf( _customer_alarm_log, "alarm-additional-text:         %s\n",
                 entry->alarm_data.additional_text );
    }

    if( SM_ALARM_SEVERITY_CLEARED != entry->alarm_data.perceived_severity )
    {
        fprintf( _customer_alarm_log, "alarm-service-affecting:       %s\n",
                 entry->alarm_data.service_affecting ? "yes" : "no" );
        fprintf( _customer_alarm_log, "alarm-suppression-allowed:     %s\n",
                 entry->alarm_data.suppression_allowed ? "yes" : "no" );
    }

    if( 0 != entry->alarm_raised_time.tv_sec )
    {
        if( NULL == localtime_r( &(entry->alarm_raised_time.tv_sec), &t_real ) )
        {
            fprintf( _customer_alarm_log, "alarm-raised-time:             "
                     "YYYY:MM:DD HH:MM:SS.xxx\n" );
        } else {
            strftime( date_str, sizeof(date_str), "%FT%T", &t_real );
            fprintf( _customer_alarm_log, "alarm-raised-time:             "
                     "%s.%03ld\n", date_str,
                     entry->alarm_raised_time.tv_nsec/1000000 );
        }
    }

    if( 0 != entry->alarm_changed_time.tv_sec )
    {
        if( NULL == localtime_r( &(entry->alarm_changed_time.tv_sec), &t_real ) )
        {
            fprintf( _customer_alarm_log, "alarm-changed-time:            "
                     "YYYY:MM:DD HH:MM:SS.xxx\n" );
        } else {
            strftime( date_str, sizeof(date_str), "%FT%T", &t_real );
            fprintf( _customer_alarm_log, "alarm-changed-time:            "
                     "%s.%03ld\n", date_str,
                     entry->alarm_changed_time.tv_nsec/1000000 );
        }
    }

    if( 0 != entry->alarm_cleared_time.tv_sec )
    {
        if( NULL == localtime_r( &(entry->alarm_cleared_time.tv_sec), &t_real ) )
        {
            fprintf( _customer_alarm_log, "alarm-cleared-time:            "
                     "YYYY:MM:DD HH:MM:SS.xxx\n" );
        } else {
            strftime( date_str, sizeof(date_str), "%FT%T", &t_real );
            fprintf( _customer_alarm_log, "alarm-cleared-time:            "
                     "%s.%03ld\n", date_str,
                     entry->alarm_cleared_time.tv_nsec/1000000 );
        }
    }

    fprintf( _customer_alarm_log, "\n" );
    fflush( _customer_alarm_log );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Find Empty Domain
// ================================
static SmAlarmDomainEntryT* sm_alarm_thread_find_empty_domain( void )
{
    SmAlarmDomainEntryT* domain_entry;

    int domain_i;
    for( domain_i=0; SM_ALARM_DOMAINS_MAX > domain_i; ++domain_i )
    {
        domain_entry = &(_alarm_domains[domain_i]);

        if( SM_ALARM_DOMAIN_ENTRY_INUSE != domain_entry->inuse )
        {
            return( domain_entry );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Find Domain
// ==========================
static SmAlarmDomainEntryT* sm_alarm_thread_find_domain(
    SmAlarmDomainNameT domain_name )
{
    SmAlarmDomainEntryT* domain_entry;

    int domain_i;
    for( domain_i=0; SM_ALARM_DOMAINS_MAX > domain_i; ++domain_i )
    {
        domain_entry = &(_alarm_domains[domain_i]);

        if( SM_ALARM_DOMAIN_ENTRY_INUSE == domain_entry->inuse )
        {
            if( 0 == strcmp( domain_name, domain_entry->alarm_domain_name ) )
            {
                return( domain_entry );
            }
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Domain Managed
// =============================
static bool sm_alarm_thread_domain_managed( SmAlarmT alarm )
{
    switch( alarm )
    {
        case SM_ALARM_SERVICE_GROUP_STATE:
            return( true );
        break;

        case SM_ALARM_SERVICE_GROUP_REDUNDANCY:
            return( true );
        break;

        case SM_ALARM_SOFTWARE_MODIFICATION:
            return( false );
        break;

        case SM_ALARM_COMMUNICATION_FAILURE:
            return( false );
        break;

        default:
            DPRINTFE( "Unknown alarm (%i).", alarm );
            return( false );
        break;
    }
    return( false );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Find Empty
// =========================
static SmAlarmEntryT* sm_alarm_thread_find_empty( void )
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
// Alarm Thread - Find
// ===================
static SmAlarmEntryT* sm_alarm_thread_find( SmAlarmT alarm,
    SmAlarmNodeNameT node_name, SmAlarmDomainNameT domain_name,
    SmAlarmEntityNameT entity_name )
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
// Alarm Thread - Add
// ==================
static void sm_alarm_thread_add( struct timespec* ts_real, SmAlarmT alarm,
    SmAlarmNodeNameT node_name, SmAlarmDomainNameT domain_name,
    SmAlarmEntityNameT entity_name, SmAlarmDataT* data )
{
    bool raise_alarm = false;
    SmAlarmEntryT* entry;

    entry = sm_alarm_thread_find( alarm, node_name, domain_name, entity_name );
    if( NULL == entry )
    {
        entry = sm_alarm_thread_find_empty();
        if( NULL == entry )
        {
            DPRINTFE( "Failed to store alarm, no more room." );
            return;

        } else {
            // New alarm entry.
            memset( entry, 0, sizeof(SmAlarmEntryT) );

            entry->inuse = SM_ALARM_ENTRY_INUSE;
            entry->alarm = alarm;
            entry->alarm_state = SM_ALARM_STATE_INITIAL;
            snprintf( entry->alarm_node_name, sizeof(entry->alarm_node_name),
                      "%s", node_name );
            snprintf( entry->alarm_domain_name, sizeof(entry->alarm_domain_name),
                      "%s", domain_name );
            snprintf( entry->alarm_entity_name, sizeof(entry->alarm_entity_name),
                      "%s", entity_name );
        }
    }

    switch( entry->alarm_state )
    {
        case SM_ALARM_STATE_INITIAL:
        case SM_ALARM_STATE_CLEARING:
        case SM_ALARM_STATE_CLEARED:
            memcpy( &(entry->alarm_raised_time), ts_real, sizeof(struct timespec) );
            memset( &(entry->alarm_changed_time), 0, sizeof(struct timespec) );
            memset( &(entry->alarm_cleared_time), 0, sizeof(struct timespec) );
            raise_alarm = true;
        break;

        case SM_ALARM_STATE_RAISING:
        case SM_ALARM_STATE_RAISED:
            if( 0 != memcmp( &(entry->alarm_data), data, sizeof(SmAlarmDataT) ) )
            {
                memcpy( &(entry->alarm_changed_time), ts_real, sizeof(struct timespec) );
                memset( &(entry->alarm_cleared_time), 0, sizeof(struct timespec) );
                raise_alarm = true;
            }
        break;

        default:
            // Ignore.
        break;
    }

    if( raise_alarm )
    {
        entry->alarm_state = SM_ALARM_STATE_RAISING;
        memcpy( &(entry->alarm_data), data, sizeof(entry->alarm_data) );
        DPRINTFI( "Raising alarm (%s) for node (%s) domain (%s) entity (%s).",
                  sm_alarm_thread_alarm_str( entry->alarm ),
                  entry->alarm_node_name, entry->alarm_domain_name,
                  entry->alarm_entity_name );
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Delete
// =====================
static void sm_alarm_thread_delete( struct timespec* ts_real, SmAlarmT alarm,
    SmAlarmNodeNameT node_name, SmAlarmDomainNameT domain_name,
    SmAlarmEntityNameT entity_name )
{
    SmAlarmEntryT* entry;

    entry = sm_alarm_thread_find( alarm, node_name, domain_name, entity_name );
    if( NULL == entry )
    {
        DPRINTFD( "No alarm (%s) raised for node (%s) domain (%s) entity (%s) "
                  "found.", sm_alarm_thread_alarm_str( alarm ), node_name,
                  domain_name, entity_name );
        return;
    }

    if( SM_ALARM_STATE_CLEARED != entry->alarm_state )
    {
        memcpy( &(entry->alarm_cleared_time), ts_real, sizeof(struct timespec) );
        entry->alarm_state = SM_ALARM_STATE_CLEARING;
        DPRINTFI( "Clearing alarm (%s) for node (%s) domain (%s) entity (%s).",
                  sm_alarm_thread_alarm_str( entry->alarm ),
                  entry->alarm_node_name, entry->alarm_domain_name,
                  entry->alarm_entity_name );
    }else
    {
        DPRINTFI( "Removed cleared alarm (%s) for node (%s) domain (%s) entity (%s).",
                  sm_alarm_thread_alarm_str( entry->alarm ),
                  entry->alarm_node_name, entry->alarm_domain_name,
                  entry->alarm_entity_name );
        entry->inuse = 0; //This alarm has reached the end of its lifecycle
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Get Alarm Identifier
// ===================================
static void sm_alarm_thread_get_alarm_id( SFmAlarmDataT* fm_alarm,
    SmAlarmT* alarm, SmAlarmNodeNameT alarm_node_name,
    SmAlarmDomainNameT alarm_domain_name, SmAlarmEntityNameT alarm_entity_name )
{
    char fm_entity_instance_id[FM_MAX_BUFFER_LENGTH];
    int len;

    *alarm = SM_ALARM_UNKNOWN;
    alarm_node_name[0]   = '\0';
    alarm_domain_name[0] = '\0';
    alarm_entity_name[0] = '\0';

    len = snprintf( fm_entity_instance_id, sizeof(fm_entity_instance_id),
                    "%s", fm_alarm->entity_instance_id );

    int char_i;
    for( char_i=0; len > char_i; ++char_i )
    {
        if( '.' == fm_entity_instance_id[char_i] )
        {
            fm_entity_instance_id[char_i] = ' ';
        }
    }

    if( 0 == strcmp( fm_alarm->alarm_id,
                     SM_ALARM_SERVICE_GROUP_STATE_ALARM_ID ) )
    {
        *alarm = SM_ALARM_SERVICE_GROUP_STATE;
        sscanf( fm_entity_instance_id,
                "service_domain=%s service_group=%s host=%s",
                alarm_domain_name, alarm_entity_name, alarm_node_name );
        return;
    }

    if( 0 == strcmp( fm_alarm->alarm_id, 
                     SM_ALARM_SERVICE_GROUP_REDUNDANCY_ALARM_ID ) )
    {
        *alarm = SM_ALARM_SERVICE_GROUP_REDUNDANCY;
        sscanf( fm_entity_instance_id,
                "service_domain=%s service_group=%s",
                alarm_domain_name, alarm_entity_name );
        return;
    }

    if( 0 == strcmp( fm_alarm->alarm_id,
                     SM_ALARM_SOFTWARE_MODIFICATION_ALARM_ID ) )
    {
        *alarm = SM_ALARM_SOFTWARE_MODIFICATION;
        sscanf( fm_entity_instance_id, "host=%s", alarm_node_name );
        return;
    }

    if( 0 == strcmp( fm_alarm->alarm_id,
                     SM_ALARM_COMMUNICATION_FAILURE_ALARM_ID ) )
    {
        *alarm = SM_ALARM_COMMUNICATION_FAILURE;
        sscanf( fm_entity_instance_id, "host=%s network=%s",
                alarm_node_name, alarm_entity_name );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Get Fault Management Alarm Identifier
// ====================================================
static SmErrorT sm_alarm_thread_get_fm_alarm_id( SmAlarmT alarm,
    const SmAlarmNodeNameT alarm_node_name,
    const SmAlarmDomainNameT alarm_domain_name,
    const SmAlarmEntityNameT alarm_entity_name, char fm_alarm_id[],
    char fm_entity_type_id[], char fm_entity_instance_id[] )
{
    switch( alarm )
    {
        case SM_ALARM_SERVICE_GROUP_STATE:
            snprintf( fm_alarm_id, FM_MAX_BUFFER_LENGTH, 
                      SM_ALARM_SERVICE_GROUP_STATE_ALARM_ID );
            snprintf( fm_entity_type_id, FM_MAX_BUFFER_LENGTH,
                      "service_domain.service_group.host" );
            snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH,
                      "service_domain=%s.service_group=%s.host=%s",
                      alarm_domain_name, alarm_entity_name, alarm_node_name );
        break;

        case SM_ALARM_SERVICE_GROUP_REDUNDANCY:
            snprintf( fm_alarm_id, FM_MAX_BUFFER_LENGTH, 
                      SM_ALARM_SERVICE_GROUP_REDUNDANCY_ALARM_ID );
            snprintf( fm_entity_type_id, FM_MAX_BUFFER_LENGTH,
                      "service_domain.service_group" );
            snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH,
                      "service_domain=%s.service_group=%s",
                      alarm_domain_name, alarm_entity_name );
        break;

        case SM_ALARM_SOFTWARE_MODIFICATION:
            snprintf( fm_alarm_id, FM_MAX_BUFFER_LENGTH,
                      SM_ALARM_SOFTWARE_MODIFICATION_ALARM_ID );
            snprintf( fm_entity_type_id, FM_MAX_BUFFER_LENGTH, "host" );
            snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH,
                      "host=%s", alarm_node_name );
        break;

        case SM_ALARM_COMMUNICATION_FAILURE:
            snprintf( fm_alarm_id, FM_MAX_BUFFER_LENGTH,
                      SM_ALARM_COMMUNICATION_FAILURE_ALARM_ID );
            snprintf( fm_entity_type_id, FM_MAX_BUFFER_LENGTH, 
                      "host.network" );
            snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH,
                      "host=%s.network=%s", alarm_node_name,
                      alarm_entity_name );
        break;

        default:
            DPRINTFE( "Unknown alarm (%i).", alarm );
            return( SM_FAILED );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Get Fault Management Alarm Type
// ==============================================
static EFmAlarmTypeT sm_alarm_thread_get_fm_alarm_type( SmAlarmEntryT* entry )
{
    switch( entry->alarm_data.event_type )
    {
        case SM_ALARM_EVENT_TYPE_UNKNOWN:
            return( FM_ALARM_TYPE_UNKNOWN );
        break;

        case SM_ALARM_EVENT_TYPE_COMMUNICATIONS_ALARM:
            return( FM_ALARM_COMM );
        break;

        case SM_ALARM_EVENT_TYPE_PROCESSING_ERROR_ALARM:
            return( FM_ALARM_PROCESSING_ERROR );
        break;

        case SM_ALARM_EVENT_TYPE_ENVIRONMENTAL_ALARM:
            return( FM_ALARM_ENVIRONMENTAL );
        break;

        case SM_ALARM_EVENT_TYPE_QUALITY_OF_SERVICE_ALARM:
            return( FM_ALARM_QOS );
        break;

        case SM_ALARM_EVENT_TYPE_EQUIPMENT_ALARM:
            return( FM_ALARM_EQUIPMENT );
        break;

        case SM_ALARM_EVENT_TYPE_INTEGRITY_VIOLATION:
            return( FM_ALARM_INTERGRITY );
        break;

        case SM_ALARM_EVENT_TYPE_OPERATIONAL_VIOLATION:
            return( FM_ALARM_OPERATIONAL );
        break;

        case SM_ALARM_EVENT_TYPE_PHYSICAL_VIOLATION:
            return( FM_ALARM_PHYSICAL );
        break;

        case SM_ALARM_EVENT_TYPE_SECURITY_SERVICE_VIOLATION:
            return( FM_ALARM_SECURITY );
        break;

        case SM_ALARM_EVENT_TYPE_MECHANISM_VIOLATION:
            return( FM_ALARM_SECURITY );
        break;

        case SM_ALARM_EVENT_TYPE_TIME_DOMAIN_VIOLATION:
            return( FM_ALARM_TIME );
        break;

        default:
            DPRINTFI( "Unknown event type (%i) for alarm (%s) "
                      "entity (%s).", entry->alarm_data.event_type,
                      sm_alarm_thread_alarm_str( entry->alarm ),
                      entry->alarm_entity_name );
            return( FM_ALARM_TYPE_UNKNOWN );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Get Fault Management Severity
// ============================================
static EFmAlarmSeverityT sm_alarm_thread_get_fm_severity( SmAlarmEntryT* entry )
{
    switch( entry->alarm_data.perceived_severity )
    {
        case SM_ALARM_SEVERITY_CLEARED:
            return( FM_ALARM_SEVERITY_CLEAR );
        break;

        case SM_ALARM_SEVERITY_INDETERMINATE:
            DPRINTFI( "Indeterminate severity for alarm (%s) entity (%s) "
                      "mapped to critical.",
                      sm_alarm_thread_alarm_str( entry->alarm ), 
                      entry->alarm_entity_name );
            return( FM_ALARM_SEVERITY_CRITICAL );
        break;

        case SM_ALARM_SEVERITY_WARNING:
            return( FM_ALARM_SEVERITY_WARNING );
        break;

        case SM_ALARM_SEVERITY_MINOR:
            return( FM_ALARM_SEVERITY_MINOR );
        break;

        case SM_ALARM_SEVERITY_MAJOR:
            return( FM_ALARM_SEVERITY_MAJOR );
        break;

        case SM_ALARM_SEVERITY_CRITICAL:
            return( FM_ALARM_SEVERITY_CRITICAL );
        break;

        default:
            DPRINTFI( "Unknown severity (%i) for alarm (%s) entity (%s) "
                      "mapped to warning.",
                      entry->alarm_data.perceived_severity,
                      sm_alarm_thread_alarm_str( entry->alarm ),
                      entry->alarm_entity_name );
            return( FM_ALARM_SEVERITY_WARNING );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm - Get Fault Management Probable Cause
// ===========================================
static EFmAlarmProbableCauseT sm_alarm_thread_get_fm_probable_cause(
    SmAlarmEntryT* entry )
{
    switch( entry->alarm_data.probable_cause )
    {
        case SM_ALARM_PROBABLE_CAUSE_UNKNOWN:
            return( FM_ALARM_CAUSE_UNKNOWN );
        break;

        case SM_ALARM_PROBABLE_CAUSE_INDETERMINATE:
            return( FM_ALARM_CAUSE_UNKNOWN );
        break;

        case SM_ALARM_PROBABLE_CAUSE_SOFTWARE_ERROR:
            return( FM_ALARM_SOFTWARE_ERROR );
        break;

        case SM_ALARM_PROBABLE_CAUSE_SOFTWARE_PROGRAM_ERROR:
            return( FM_ALARM_PROGRAM_ERROR );
        break;

        case SM_ALARM_PROBABLE_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE:
            return( FM_ALARM_UNDERLYING_RESOURCE_UNAVAILABLE );
        break;

        case SM_ALARM_PROBABLE_CAUSE_KEY_EXPIRED:
            return( FM_ALARM_KEY_EXPIRED );
        break;

        default:
            DPRINTFI( "Unknown probable cause (%i) for alarm (%s) "
                      "entity (%s).", entry->alarm_data.probable_cause,
                      sm_alarm_thread_alarm_str( entry->alarm ),
                      entry->alarm_entity_name );
            return( FM_ALARM_CAUSE_UNKNOWN );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Build Fault Management Alarm Data
// ================================================
static SmErrorT sm_alarm_thread_build_fm_alarm_data( SmAlarmEntryT* entry )
{
    SFmAlarmDataT* fm_alarm_data = &(entry->fm_alarm_data);
    SmErrorT error;

    error = sm_alarm_thread_get_fm_alarm_id( entry->alarm,
                        entry->alarm_node_name, entry->alarm_domain_name,
                        entry->alarm_entity_name, fm_alarm_data->alarm_id,
                        fm_alarm_data->entity_type_id, 
                        fm_alarm_data->entity_instance_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get alarm (%s) data for node (%s) domain (%s) "
                  "entity (%s), error=%s.",
                  sm_alarm_thread_alarm_str( entry->alarm ), 
                  entry->alarm_node_name, entry->alarm_domain_name,
                  entry->alarm_entity_name, sm_error_str( error ) );
        return( error );
    }

    fm_alarm_data->alarm_state = FM_ALARM_STATE_SET;

    fm_alarm_data->severity = sm_alarm_thread_get_fm_severity( entry );

    // FM only has a reason text field.  Need to concatenate the
    // specific_problem_text and addition text fields. 
    // Capitalize the first character.
    if( '\0' == entry->alarm_data.additional_text[0] )
    {
        snprintf( fm_alarm_data->reason_text,
                  sizeof(fm_alarm_data->reason_text), "%s",
                  entry->alarm_data.specific_problem_text );
    } else {
        snprintf( fm_alarm_data->reason_text,
                  sizeof(fm_alarm_data->reason_text), "%s; %s",
                  entry->alarm_data.specific_problem_text,
                  entry->alarm_data.additional_text );
    }

    if( isalpha( fm_alarm_data->reason_text[0] ) )
    {
        fm_alarm_data->reason_text[0]
            = toupper( fm_alarm_data->reason_text[0] );
    }

    fm_alarm_data->alarm_type = sm_alarm_thread_get_fm_alarm_type( entry );
    fm_alarm_data->probable_cause = sm_alarm_thread_get_fm_probable_cause( entry );

    // Capitalize the first character.
    snprintf( fm_alarm_data->proposed_repair_action,
              sizeof(fm_alarm_data->proposed_repair_action),
              "%s", entry->alarm_data.proposed_repair_action );

    if( isalpha( fm_alarm_data->proposed_repair_action[0] ) )
    {
        fm_alarm_data->proposed_repair_action[0]
            = toupper( fm_alarm_data->proposed_repair_action[0] );
    }
    
    if( entry->alarm_data.service_affecting )
    {
        fm_alarm_data->service_affecting = FM_TRUE;
    } else {
        fm_alarm_data->service_affecting = FM_FALSE;
    }

    if( entry->alarm_data.suppression_allowed )
    {
        fm_alarm_data->suppression = FM_TRUE;
    } else {
        fm_alarm_data->suppression = FM_FALSE;
    }

    fm_alarm_data->inhibit_alarms = FM_FALSE;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Raise Alarm
// ==========================
static void sm_alarm_thread_raise_alarm( SmAlarmEntryT* entry )
{
    EFmErrorT fm_error;
    SmErrorT error;

    if( SM_ALARM_SEVERITY_CLEARED == entry->alarm_data.perceived_severity )
    {
        DPRINTFE( "Attempting to raise a cleared alarm." );
        return;
    }

    error = sm_alarm_thread_build_fm_alarm_data( entry );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to build alarm (%s) data for node (%s) domain (%s) "
                  "entity (%s), error=%s.",
                  sm_alarm_thread_alarm_str( entry->alarm ),
                  entry->alarm_node_name, entry->alarm_domain_name,
                  entry->alarm_entity_name, sm_error_str( error ) );
        return;
    }

    fm_error = fm_set_fault_wrapper( &(entry->fm_alarm_data),
                                     &(entry->fm_uuid) );
    if( FM_ERR_OK != fm_error )
    {
        DPRINTFE( "Failed to set alarm (%s) for node (%s) domain (%s) "
                  "entity (%s), error=%i",
                  sm_alarm_thread_alarm_str( entry->alarm ),
                  entry->alarm_node_name, entry->alarm_domain_name,
                  entry->alarm_entity_name, fm_error );
        return;
    }

    DPRINTFI( "Raised alarm (%s) for node (%s) domain (%s) entity (%s), "
              "fm_uuid=%s.", sm_alarm_thread_alarm_str( entry->alarm ),
              entry->alarm_node_name, entry->alarm_domain_name,
              entry->alarm_entity_name, entry->fm_uuid );

    entry->alarm_state = SM_ALARM_STATE_RAISED;

    sm_alarm_thread_log( entry );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Clear Alarm
// ==========================
static void sm_alarm_thread_clear_alarm( SmAlarmEntryT* entry )
{
    AlarmFilter fm_filter;
    char fm_entity_type_id[FM_MAX_BUFFER_LENGTH];
    EFmErrorT fm_error;
    SmErrorT error;

    error = sm_alarm_thread_get_fm_alarm_id( entry->alarm,
                            entry->alarm_node_name, entry->alarm_domain_name,
                            entry->alarm_entity_name, fm_filter.alarm_id,
                            fm_entity_type_id, fm_filter.entity_instance_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get alarm (%s) data for entity (%s), "
                  "error=%s.", sm_alarm_thread_alarm_str( entry->alarm ),
                  entry->alarm_entity_name, sm_error_str( error ) );
        return;
    }

    fm_error = fm_clear_fault_wrapper( &fm_filter );
    if(( FM_ERR_OK != fm_error )&&( FM_ERR_ENTITY_NOT_FOUND != fm_error ))
    {
        DPRINTFE( "Failed to clear alarm (%s) for node (%s) domain (%s) "
                  "entity (%s), error=%i",
                  sm_alarm_thread_alarm_str( entry->alarm ),
                  entry->alarm_node_name, entry->alarm_domain_name,
                  entry->alarm_entity_name, fm_error );
        return;
    }

    DPRINTFI( "Cleared alarm (%s) for node (%s) domain (%s) entity (%s), "
              "fm_uuid=%s.", sm_alarm_thread_alarm_str( entry->alarm ),
              entry->alarm_node_name, entry->alarm_domain_name,
              entry->alarm_entity_name, entry->fm_uuid );

    entry->alarm_state = SM_ALARM_STATE_CLEARED;
    entry->alarm_data.perceived_severity = SM_ALARM_SEVERITY_CLEARED;
    entry->alarm_data.state_info.applicable = false;
    entry->alarm_data.threshold_info.applicable = false;
    entry->alarm_data.proposed_repair_action[0] = '\0';
    entry->alarm_data.additional_text[0] = '\0';

    sm_alarm_thread_log( entry );
    entry->inuse = 0; //This alarm has reached the end of its lifecycle
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Clear All
// ========================
static void sm_alarm_thread_clear_all( struct timespec* ts_real, 
    SmAlarmDomainNameT domain_name )
{
    char date_str[32];
    struct tm t_real;
    char fm_entity_instance_id[FM_MAX_BUFFER_LENGTH];
    EFmErrorT fm_error;
    SmAlarmEntryT* entry = NULL;

    snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH,
              "service_domain=%s", domain_name );

    fm_error = fm_clear_all_wrapper( &fm_entity_instance_id );
    if(( FM_ERR_OK != fm_error )&&( FM_ERR_ENTITY_NOT_FOUND != fm_error ))
    {
        DPRINTFE( "Failed to clear service domain (%s) alarms, error=%i",
                  domain_name, fm_error );
    }

    DPRINTFI( "Cleared all alarms." );

    unsigned int entry_i;
    for( entry_i=0; SM_ALARMS_MAX > entry_i; ++entry_i )
    {
        entry = &(_alarms[entry_i]);

        if( SM_ALARM_ENTRY_INUSE != entry->inuse )
            continue;

        if( 0 == strcmp( domain_name, entry->alarm_domain_name ) )
        {
            memset( entry, 0, sizeof(SmAlarmEntryT) );
        }
    }

    if( NULL == localtime_r( &(ts_real->tv_sec), &t_real ) )
    {
        fprintf( _customer_alarm_log,
                 "----- all alarms cleared for %s -----\n\n",
                 domain_name );
    } else {
        strftime( date_str, sizeof(date_str), "%FT%T", &t_real );
        fprintf( _customer_alarm_log, "----- all alarms cleared for %s at "
                 "%s.%03ld -----\n\n", domain_name, date_str,
                 ts_real->tv_nsec/1000000 );
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Manage Domain Alarms
// ===================================
static void sm_alarm_thread_manage_domain_alarms(
    SmAlarmDomainNameT domain_name, bool manage )
{
    SmAlarmEntryT* entry;
    SmAlarmDomainEntryT* domain_entry;

    domain_entry = sm_alarm_thread_find_domain( domain_name );
    if( NULL == domain_entry )
    {
        domain_entry = sm_alarm_thread_find_empty_domain();
        if( NULL == domain_entry )
        {
            DPRINTFE( "Alarm domain allocation failure, no free alarm "
                      "domain entries." );
            return;
        }

        domain_entry->inuse = SM_ALARM_DOMAIN_ENTRY_INUSE;
        domain_entry->manage = manage;
        snprintf( domain_entry->alarm_domain_name,
                  sizeof(domain_entry->alarm_domain_name), "%s",
                  domain_name );
    } else {
        domain_entry->manage = manage;
    }

    if( manage )
    {
        DPRINTFI( "Managing alarms for domain (%s).", domain_name );
                  
    } else {
        DPRINTFI( "No longer managing alarms for domain (%s).", domain_name );

        unsigned int entry_i;
        for( entry_i=0; SM_ALARMS_MAX > entry_i; ++entry_i )
        {
            entry = &(_alarms[entry_i]);

            if( SM_ALARM_ENTRY_INUSE == entry->inuse )
            {
                if( sm_alarm_thread_domain_managed( entry->alarm ) )
                {
                    memset( entry, 0, sizeof(SmAlarmEntryT) );
                }
            }
        }    
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Report
// =====================
static void sm_alarm_thread_report( void )
{
    SmAlarmEntryT* entry;

    unsigned int entry_i;
    for( entry_i=0; SM_ALARMS_MAX > entry_i; ++entry_i )
    {
        entry = &(_alarms[entry_i]);

        if( SM_ALARM_ENTRY_INUSE != entry->inuse )
            continue;
        
        // Call Fault Management.
        if( SM_ALARM_STATE_CLEARING == entry->alarm_state )
        {
            sm_alarm_thread_clear_alarm( entry );

        } else if( SM_ALARM_STATE_RAISING == entry->alarm_state ) {
            sm_alarm_thread_raise_alarm( entry );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Audit
// ====================
static void sm_alarm_thread_audit( const char entity_instance[] )
{
    bool found;
    char fm_entity_instance_id[FM_MAX_BUFFER_LENGTH];
    SFmAlarmDataT fm_alarm_data[SM_ALARMS_MAX];
    unsigned fm_total_alarms = SM_ALARMS_MAX;
    SFmAlarmDataT* fm_alarm;
    EFmErrorT fm_error;
    SmAlarmT alarm;
    SmAlarmNodeNameT node_name;
    SmAlarmDomainNameT domain_name;
    SmAlarmEntityNameT entity_name;
    SmAlarmEntryT* entry;
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmErrorT error;

    error = sm_node_utils_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return;
    }

    snprintf( fm_entity_instance_id, FM_MAX_BUFFER_LENGTH, "%s",
              entity_instance );

    fm_error = fm_get_faults_wrapper( &fm_entity_instance_id, fm_alarm_data,
                                      &fm_total_alarms );
    if(( FM_ERR_OK != fm_error )&&( FM_ERR_ENTITY_NOT_FOUND != fm_error ))
    {
        DPRINTFE( "Failed to get all alarms, error=%i", fm_error );
        return;
    }

    unsigned int fm_alarm_i;
    for( fm_alarm_i=0; fm_total_alarms > fm_alarm_i; ++fm_alarm_i )
    {
        found = false;
        fm_alarm = &(fm_alarm_data[fm_alarm_i]);

        sm_alarm_thread_get_alarm_id( fm_alarm, &alarm, node_name,
                                      domain_name, entity_name );

        DPRINTFD( "Looking at fm_alarm_id=%s, fm_entity_instance_id=%s, "
                  "alarm=%i, node_name=%s, domain_name=%s, entity_name=%s.",
                  fm_alarm->alarm_id, fm_alarm->entity_instance_id, alarm,
                  node_name, domain_name, entity_name );

        if( SM_ALARM_UNKNOWN == alarm )
        {
            continue;
        }

        if( sm_alarm_thread_domain_managed( alarm ) )
        {
            SmAlarmDomainEntryT* domain_entry;

            domain_entry = sm_alarm_thread_find_domain( domain_name );
            if( NULL == domain_entry )
            {
                DPRINTFD( "Alarm domain (%s) not found.", domain_name );
                continue;
            }

            if( !domain_entry->manage )
            {
                DPRINTFD( "Not managing alarm domain (%s).", domain_name );
                continue;
            }
        } else if( 0 != strcmp( hostname, node_name ) ) {
            DPRINTFD( "Not our alarm, node_name=%s.", node_name );
            continue;
        }

        entry = sm_alarm_thread_find( alarm, node_name, domain_name,
                                      entity_name );
        if( NULL != entry )
        {
            if( SM_ALARM_STATE_RAISING == entry->alarm_state
                || SM_ALARM_STATE_RAISED == entry->alarm_state )
            {
                // only when a matching record locally to be considered as "found"
                found = true;
                entry->alarm_state = SM_ALARM_STATE_RAISED;
            }else
            {
                // found a record, but not in raising/raised mode, alarm need to be cleared
            }
        }

        if( !found )
        {
            AlarmFilter fm_filter;

            snprintf( fm_filter.alarm_id, sizeof(fm_filter.alarm_id), "%s",
                      fm_alarm->alarm_id );
            snprintf( fm_filter.entity_instance_id, 
                      sizeof(fm_filter.entity_instance_id), "%s",
                      fm_alarm->entity_instance_id );

            fm_error = fm_clear_fault_wrapper( &fm_filter );
            if(( FM_ERR_OK != fm_error )&&( FM_ERR_ENTITY_NOT_FOUND != fm_error ))
            {
                DPRINTFE( "Failed to clear stale alarm (fm_alarm_id=%s, "
                          "fm_entity_instance_id=%s), error=%i",
                          fm_alarm->alarm_id, fm_alarm->entity_instance_id,
                          fm_error );
            }

            if ( NULL != entry )
            {
                //alarm is cleared, retire the record
                entry->inuse = 0;
                DPRINTFI( "Removed cleared alarm, fm_alarm_id=%s, "
                      "fm_entity_instance_id=%s, fm_uuid=%s.",
                      fm_alarm->alarm_id, fm_alarm->entity_instance_id,
                      fm_alarm->uuid );
            }else
            {
                DPRINTFI( "Deleted stale alarm, fm_alarm_id=%s, "
                          "fm_entity_instance_id=%s, fm_uuid=%s.",
                          fm_alarm->alarm_id, fm_alarm->entity_instance_id,
                          fm_alarm->uuid );
            }
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Report Timer
// ===========================
static bool sm_alarm_thread_report_timer( SmTimerIdT timer_id,
    int64_t user_data )
{
    sm_alarm_thread_report();
    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Audit Timer
// ==========================
static bool sm_alarm_thread_audit_timer( SmTimerIdT timer_id,
    int64_t user_data )
{
    SmAlarmEntryT* entry;

    // Set raised alarms to raising, the audit will set them
    // to raised, if FM really has them.
    unsigned int entry_i;
    for( entry_i=0; SM_ALARMS_MAX > entry_i; ++entry_i )
    {
        entry = &(_alarms[entry_i]);

        if( SM_ALARM_ENTRY_INUSE != entry->inuse )
            continue;
        
        if( SM_ALARM_STATE_RAISED == entry->alarm_state )
        {
            entry->alarm_state = SM_ALARM_STATE_RAISING;
        }
    }

    sm_alarm_thread_audit( "service_domain" );
    sm_alarm_thread_audit( "host"  );
    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Dispatch
// =======================
static void sm_alarm_thread_dispatch( int selobj, int64_t user_data )
{
    int bytes_read;
    SmAlarmThreadMsgT msg;
    SmAlarmThreadMsgRaiseAlarmT* raise_alarm = &(msg.u.raise_alarm);
    SmAlarmThreadMsgClearAlarmT* clear_alarm = &(msg.u.clear_alarm);
    SmAlarmThreadMsgClearAllAlarmsT* clear_all_alarms = &(msg.u.clear_all_alarms);
    SmAlarmThreadMsgManageDomainAlarmsT* manage_alarms = &(msg.u.manage_alarms);
    bool report_alarm = false;

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
        case SM_ALARM_THREAD_MSG_RAISE_ALARM:
            sm_alarm_thread_add( &(raise_alarm->ts_real), raise_alarm->alarm,
                          raise_alarm->node_name, raise_alarm->domain_name,
                          raise_alarm->entity_name, &(raise_alarm->data) );
            report_alarm = true;
        break;

        case SM_ALARM_THREAD_MSG_CLEAR_ALARM:
            sm_alarm_thread_delete( &(clear_alarm->ts_real), clear_alarm->alarm,
                          clear_alarm->node_name, clear_alarm->domain_name,
                          clear_alarm->entity_name );
            report_alarm = true;
        break;

        case SM_ALARM_THREAD_MSG_CLEAR_ALL_ALARMS:
            sm_alarm_thread_clear_all( &(clear_alarm->ts_real),
                                       clear_all_alarms->domain_name );
        break;

        case SM_ALARM_THREAD_MSG_MANAGE_DOMAIN_ALARMS:
            sm_alarm_thread_manage_domain_alarms( manage_alarms->domain_name,
                                                  manage_alarms->manage );
        break;

        default:
            DPRINTFE( "Unknown message (%i) received.", msg.type );
            return;
        break;
    }

    if( report_alarm )
        sm_alarm_thread_report();
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Initialize Thread
// ================================
static SmErrorT sm_alarm_thread_initialize_thread( void )
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
        error = sm_selobj_register( _server_fd, sm_alarm_thread_dispatch, 0 );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to register selection object, error=%s.",
                      sm_error_str( error ) );
            return( error );
        }    

        _server_fd_registered = true;
    }

    error = sm_timer_initialize( SM_ALARM_THREAD_TICK_INTERVAL_IN_MS );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize timer module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_timer_register( "alarm report",
                               SM_ALARM_THREAD_ALARM_REPORT_TIMER_IN_MS,
                               sm_alarm_thread_report_timer, 0,
                               &_alarm_report_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create alarm report timer, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_timer_register( "alarm audit",
                               SM_ALARM_THREAD_ALARM_AUDIT_TIMER_IN_MS,
                               sm_alarm_thread_audit_timer, 0,
                               &_alarm_audit_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create alarm audit timer, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    if( NULL != _customer_alarm_log )
    {
        fflush( _customer_alarm_log );
        fclose( _customer_alarm_log );
        _customer_alarm_log = NULL;
    }

    _customer_alarm_log = fopen( SM_ALARM_CUSTOMER_LOG_FILE, "a" );
    if( NULL == _customer_alarm_log )
    {
        DPRINTFE( "Failed to open customer alarm log file (%s).", 
                  SM_ALARM_CUSTOMER_LOG_FILE );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Finalize Thread
// ==============================
static SmErrorT sm_alarm_thread_finalize_thread( void )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != _alarm_report_timer_id )
    {
        error = sm_timer_deregister( _alarm_report_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel alarm report timer, error=%s.",
                      sm_error_str( error ) );
        }

        _alarm_report_timer_id = SM_TIMER_ID_INVALID;
    }

    if( SM_TIMER_ID_INVALID != _alarm_audit_timer_id )
    {
        error = sm_timer_deregister( _alarm_audit_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel alarm audit timer, error=%s.",
                      sm_error_str( error ) );
        }

        _alarm_audit_timer_id = SM_TIMER_ID_INVALID;
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

    if( NULL != _customer_alarm_log )
    {
        fflush( _customer_alarm_log );
        fclose( _customer_alarm_log );
        _customer_alarm_log = NULL;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Main
// ===================
static void* sm_alarm_thread_main( void* arguments )
{
    SmErrorT error;

    pthread_setname_np( pthread_self(), SM_ALARM_THREAD_NAME );
    sm_debug_set_thread_info();
    sm_trap_set_thread_info();

    DPRINTFI( "Starting" );

    // Warn after 2 minutes, fail after 16 minutes.
    error = sm_thread_health_register( SM_ALARM_THREAD_NAME, 120000, 1000000 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register alarm thread, error=%s.",
                  sm_error_str( error ) );
        pthread_exit( NULL );
    }

    error = sm_alarm_thread_initialize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize alarm thread, error=%s.",
                  sm_error_str( error ) );
        pthread_exit( NULL );
    }

    DPRINTFI( "Started." );

    while( _stay_on )
    {
        error = sm_selobj_dispatch( SM_ALARM_THREAD_TICK_INTERVAL_IN_MS );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Selection object dispatch failed, error=%s.",
                      sm_error_str(error) );
            break;
        }

        error = sm_thread_health_update( SM_ALARM_THREAD_NAME );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to update alarm thread health, error=%s.",
                      sm_error_str(error) );
        }
    }

    DPRINTFI( "Shutting down." );

    error = sm_alarm_thread_finalize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize alarm thread, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_thread_health_deregister( SM_ALARM_THREAD_NAME );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister alarm thread, error=%s.",
                  sm_error_str( error ) );
    }

    DPRINTFI( "Shutdown complete." );

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Start
// ====================
SmErrorT sm_alarm_thread_start( int server_fd )
{
    int result;

    memset( _alarms, 0, sizeof(_alarms) );
    memset( _alarm_domains, 0, sizeof(_alarm_domains) );

    _stay_on = 1;
    _thread_created = false;
    _server_fd = server_fd;
    
    result = pthread_create( &_alarm_thread, NULL, sm_alarm_thread_main, NULL );
    if( 0 != result )
    {
        DPRINTFE( "Failed to start alarm thread, error=%s.",
                  strerror(result) );
        return( SM_FAILED );
    }

    _thread_created = true;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Stop
// ===================
SmErrorT sm_alarm_thread_stop( void )
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
            result = pthread_tryjoin_np( _alarm_thread, NULL );
            if(( 0 != result )&&( EBUSY != result ))
            {
                if(( ESRCH != result )&&( EINVAL != result ))
                {
                    DPRINTFE( "Failed to wait for alarm thread exit, "
                              "sending kill signal, error=%s.",
                              strerror(result) );
                    pthread_kill( _alarm_thread, SIGKILL );
                }
                break;
            }

            ms_expired = sm_time_get_elapsed_ms( &time_prev );
            if( 5000 <= ms_expired )
            {
                DPRINTFE( "Failed to stop alarm thread, sending "
                          "kill signal." );
                pthread_kill( _alarm_thread, SIGKILL );
                break;
            }

            usleep( 250000 ); // 250 milliseconds.
        }
    }

    _server_fd = -1;
    _thread_created = false;

    memset( _alarms, 0, sizeof(_alarms) );
    memset( _alarm_domains, 0, sizeof(_alarm_domains) );

    return( SM_OKAY );
}
// ****************************************************************************
