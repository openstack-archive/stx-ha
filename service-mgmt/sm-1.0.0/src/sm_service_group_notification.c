//
// Copyright (c) 2014-2016 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_notification.h"


#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "sm_types.h"
#include "sm_utils.h"
#include "sm_debug.h"
#include "sm_process_death.h"
#include "sm_service_domain_member_table.h"
#include "sm_service_group_table.h"
#include "sm_service_group_fsm.h"

typedef struct 
{
    char seqnum_str[24];
    bool part_of_aggregate;
    char service_group_aggregate_name[SM_SERVICE_GROUP_AGGREGATE_NAME_MAX_CHAR];
    SmServiceGroupStateT service_group_aggregate_desired_state;
    SmServiceGroupStateT service_group_aggregate_state;
    char service_group_name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    SmServiceGroupStateT service_group_desired_state;
    SmServiceGroupStateT service_group_state;
    SmServiceGroupNotificationT service_group_notification;
} SmNotificationEnvT;

#define SM_NOTIFICATION_SCRIPT_MAX_DELAY_IN_SECS                     4
#define SM_NOTIFICATION_SCRIPT_TIMEOUT_IN_MS                     30000
#define SM_NOTIFICATION_SCRIPT_TIMER_SKEW_IN_MS                  60000
#define SM_NOTIFICATION_SCRIPT_SUCCESS                               0
#define SM_NOTIFICATION_SCRIPT_TIMEOUT                          -65534
#define SM_NOTIFICATION_SCRIPT_FAILURE                          -65535

static unsigned long _seqnum = 0;


// ****************************************************************************
// Service Group Notification - Do Abort
// =====================================
static SmErrorT sm_service_group_notification_do_abort(
    char service_group_name[], int process_id )
{
    if( -1 == process_id )
    {
        DPRINTFE( "Trying to abort a process (%i) for service group (%s) "
                  "notificaton but pid is invalid.", process_id,
                  service_group_name );
        return( SM_FAILED );
    }

    if( process_id == (int) getpid() )
    {
        DPRINTFE( "Trying to abort a process (%i) for service group (%s) "
                  "notification but pid is self.", process_id,
                  service_group_name );
        return( SM_FAILED );
    }

    DPRINTFI( "Aborting service group (%s) notification with kill signal, "
              "pid=%i.", service_group_name, process_id );

    if( 0 > kill( process_id, SIGKILL ) )
    {
        if( ESRCH == errno )
        {
            DPRINTFD( "Service group (%s) notification script not running.",
                      service_group_name );
            return( SM_OKAY );

        } else {
            DPRINTFE( "Failed to send kill signal to service group (%s) "
                      "notification script, error=%s.", service_group_name,
                      strerror( errno ) );
            return( SM_FAILED );
        }
    }

    DPRINTFD( "Kill signal sent to service groupt (%s) notification "
              "script.", service_group_name );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Service Group Aggregate
// ====================================================
static void sm_service_group_notification_service_group_aggregate(
    void* user_data[], SmServiceDomainMemberT* member )
{
    SmServiceGroupStateT* aggregate_desired_state;
    SmServiceGroupStateT* aggregate_state;
    SmServiceGroupStateT* transition_state;
    SmServiceGroupT* service_group;

    aggregate_desired_state = (SmServiceGroupStateT*) user_data[0];
    aggregate_state = (SmServiceGroupStateT*) user_data[1];
    transition_state = (SmServiceGroupStateT*) user_data[2];

    service_group = sm_service_group_table_read( member->service_group_name );
    if( NULL == service_group )
    {
        DPRINTFE( "Failed to read service group (%s), error=%s.",
                  member->service_group_name, sm_error_str(SM_NOT_FOUND) );
        return;
    }

    switch( service_group->desired_state )
    {
        case SM_SERVICE_GROUP_STATE_ACTIVE:
            if(( SM_SERVICE_GROUP_STATE_UNKNOWN  == *aggregate_desired_state )||
               ( SM_SERVICE_GROUP_STATE_STANDBY  != *aggregate_desired_state )||
               ( SM_SERVICE_GROUP_STATE_DISABLED != *aggregate_desired_state )||
               ( SM_SERVICE_GROUP_STATE_SHUTDOWN != *aggregate_desired_state ))
                *aggregate_desired_state = service_group->desired_state;
        break;

        case SM_SERVICE_GROUP_STATE_STANDBY:
            if(( SM_SERVICE_GROUP_STATE_UNKNOWN  == *aggregate_desired_state )||
               ( SM_SERVICE_GROUP_STATE_DISABLED != *aggregate_desired_state )||
               ( SM_SERVICE_GROUP_STATE_SHUTDOWN != *aggregate_desired_state ))
                *aggregate_desired_state = service_group->desired_state;
        break;

        case SM_SERVICE_GROUP_STATE_DISABLED:
            if(( SM_SERVICE_GROUP_STATE_UNKNOWN  == *aggregate_desired_state )||
               ( SM_SERVICE_GROUP_STATE_SHUTDOWN != *aggregate_desired_state ))
                *aggregate_desired_state = service_group->desired_state;
        break;

        case SM_SERVICE_GROUP_STATE_SHUTDOWN:
            *aggregate_desired_state = service_group->desired_state;
        break;

        default:
            // Ignore
        break;
    }

    switch( service_group->state )
    {
        case SM_SERVICE_GROUP_STATE_ACTIVE:
            if(( SM_SERVICE_GROUP_STATE_UNKNOWN  == *aggregate_state )||
               ( SM_SERVICE_GROUP_STATE_STANDBY  != *aggregate_state )||
               ( SM_SERVICE_GROUP_STATE_DISABLED != *aggregate_state )||
               ( SM_SERVICE_GROUP_STATE_SHUTDOWN != *aggregate_state ))
                *aggregate_state = service_group->state;
        break;

        case SM_SERVICE_GROUP_STATE_STANDBY:
            if(( SM_SERVICE_GROUP_STATE_UNKNOWN  == *aggregate_state )||
               ( SM_SERVICE_GROUP_STATE_DISABLED != *aggregate_state )||
               ( SM_SERVICE_GROUP_STATE_SHUTDOWN != *aggregate_state ))
                *aggregate_state = service_group->state;
        break;

        case SM_SERVICE_GROUP_STATE_DISABLED:
            if(( SM_SERVICE_GROUP_STATE_UNKNOWN  == *aggregate_state )||
               ( SM_SERVICE_GROUP_STATE_SHUTDOWN != *aggregate_state ))
                *aggregate_state = service_group->state;
        break;

        case SM_SERVICE_GROUP_STATE_SHUTDOWN:
            *aggregate_state = service_group->state;
        break;

        case SM_SERVICE_GROUP_STATE_GO_ACTIVE:
            if(( SM_SERVICE_GROUP_STATE_UNKNOWN    == *transition_state )||
               ( SM_SERVICE_GROUP_STATE_GO_STANDBY != *transition_state )||
               ( SM_SERVICE_GROUP_STATE_DISABLING  != *transition_state ))
                *transition_state = service_group->state;
        break;

        case SM_SERVICE_GROUP_STATE_GO_STANDBY:
            if(( SM_SERVICE_GROUP_STATE_UNKNOWN   == *transition_state )||
               ( SM_SERVICE_GROUP_STATE_DISABLING != *transition_state ))
                *transition_state = service_group->state;
        break;

        case SM_SERVICE_GROUP_STATE_DISABLING:
            *transition_state = service_group->state;
        break;

        default:
            // Ignore
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Get Environment
// ============================================
static SmErrorT sm_service_group_notification_get_env(
    char service_group_name[], SmServiceGroupNotificationT notification,
    SmNotificationEnvT* env )
{
    SmServiceGroupStateT aggregate_desired_state = SM_SERVICE_GROUP_STATE_UNKNOWN;
    SmServiceGroupStateT aggregate_state = SM_SERVICE_GROUP_STATE_UNKNOWN;
    SmServiceGroupStateT transition_state = SM_SERVICE_GROUP_STATE_UNKNOWN;
    void* user_data[] = {&aggregate_desired_state, &aggregate_state,
                         &transition_state};
    SmServiceDomainMemberT* service_domain_member = NULL;
    SmServiceGroupT* service_group = NULL;

    service_group = sm_service_group_table_read( service_group_name );
    if( NULL == service_group )
    {
        DPRINTFE( "Failed to read service group (%s), error=%s.",
                  service_group_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_FAILED );
    }

    _seqnum++;
    snprintf( &(env->seqnum_str[0]), sizeof(env->seqnum_str), "%lu", _seqnum );
    snprintf( &(env->service_group_name[0]), SM_SERVICE_GROUP_NAME_MAX_CHAR,
              "%s", service_group->name );
    env->service_group_desired_state = service_group->desired_state;
    env->service_group_state = service_group->state;
    env->service_group_notification = notification;

    service_domain_member = 
        sm_service_domain_member_table_read_service_group( service_group->name );

    if( NULL != service_domain_member )
    {
        if( '\0' != service_domain_member->service_group_aggregate[0] )
        {
            sm_service_domain_member_table_foreach_service_group_aggregate(
                    service_domain_member->name,
                    service_domain_member->service_group_aggregate, user_data,
                    sm_service_group_notification_service_group_aggregate );

            switch( aggregate_state )
            {
                case SM_SERVICE_GROUP_STATE_ACTIVE:
                    if(( SM_SERVICE_GROUP_STATE_GO_ACTIVE  == transition_state )||
                       ( SM_SERVICE_GROUP_STATE_GO_STANDBY == transition_state )||
                       ( SM_SERVICE_GROUP_STATE_DISABLING  == transition_state ))
                        aggregate_state = transition_state;
                break;

                case SM_SERVICE_GROUP_STATE_STANDBY:
                    if(( SM_SERVICE_GROUP_STATE_GO_STANDBY == transition_state )||
                       ( SM_SERVICE_GROUP_STATE_DISABLING  == transition_state ))
                        aggregate_state = transition_state;
                break;

                case SM_SERVICE_GROUP_STATE_DISABLED:
                    if( SM_SERVICE_GROUP_STATE_DISABLING  == transition_state )
                        aggregate_state = transition_state;
                break;

                case SM_SERVICE_GROUP_STATE_UNKNOWN:
                    aggregate_state = transition_state;
                break;

                default:
                    // Ignore
                break;
            }

            env->part_of_aggregate = true;
            snprintf( &(env->service_group_aggregate_name[0]),
                      SM_SERVICE_GROUP_AGGREGATE_NAME_MAX_CHAR, "%s",
                      service_domain_member->service_group_aggregate );
            env->service_group_aggregate_desired_state = aggregate_desired_state;
            env->service_group_aggregate_state = aggregate_state;
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Setup Environment
// ==============================================
static SmErrorT sm_service_group_notification_setup_env(
    SmNotificationEnvT* env )
{
    if( 0 > setenv( "NOTIFICATION_SEQNUM", env->seqnum_str, 1 ) )
    {
        DPRINTFE( "Failed to set environment variable (NOTIFICATION_SEQNUM), "
                  "error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }


    if( 0 > setenv( "SERVICE_GROUP_NAME", env->service_group_name, 1 ) )
    {
        DPRINTFE( "Failed to set environment variable (SERVICE_GROUP_NAME), "
                  "error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }

    if( 0 > setenv( "SERVICE_GROUP_DESIRED_STATE", 
                    sm_service_group_state_str(
                        env->service_group_desired_state), 1 ) )
    {
        DPRINTFE( "Failed to set environment variable "
                  "(SERVICE_GROUP_DESIRED_STATE), error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    if( 0 > setenv( "SERVICE_GROUP_STATE", 
                    sm_service_group_state_str(env->service_group_state), 1 ) )
    {
        DPRINTFE( "Failed to set environment variable (SERVICE_GROUP_STATE), "
                  "error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }

    if( 0 > setenv( "SERVICE_GROUP_NOTIFICATION",
                    sm_service_group_notification_str(
                        env->service_group_notification), 1 ) )
    {
        DPRINTFE( "Failed to set environment variable "
                  "(SERVICE_GROUP_NOTIFICATION), error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    if( env->part_of_aggregate )
    {
        if( 0 > setenv( "SERVICE_GROUP_AGGREGATE_NAME", 
                        env->service_group_aggregate_name, 1 ) )
        {
            DPRINTFE( "Failed to set environment variable "
                      "(SERVICE_GROUP_AGGREGATE_NAME), error=%s.",
                      strerror( errno ) );
            return ( SM_FAILED );
        }

        if( 0 > setenv( "SERVICE_GROUP_AGGREGATE_DESIRED_STATE", 
                        sm_service_group_state_str(
                            env->service_group_aggregate_desired_state), 1 ) )
        {
            DPRINTFE( "Failed to set environment variable "
                      "(SERVICE_GROUP_AGGREGATE_DESIRED_STATE), error=%s.",
                      strerror( errno ) );
            return ( SM_FAILED );
        }

        if( 0 > setenv( "SERVICE_GROUP_AGGREGATE_STATE", 
                        sm_service_group_state_str(
                            env->service_group_aggregate_state), 1 ) )
        {
            DPRINTFE( "Failed to set environment variable "
                      "(SERVICE_GROUP_AGGREGATE_STATE), error=%s.",
                      strerror( errno ) );
            return ( SM_FAILED );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Run
// ================================
static SmErrorT sm_service_group_notification_run( char service_group_name[],
    SmServiceGroupNotificationT notification, int* process_id )
{
    pid_t pid;
    struct stat stat_data;
    int result;
    char program_name[80];
    SmNotificationEnvT env;
    SmErrorT error;

    *process_id = -1;

    memset( &env, 0, sizeof(SmNotificationEnvT) );

    if( 0 > access( SM_NOTIFICATION_SCRIPT,  F_OK | X_OK ) )
    {
        DPRINTFE( "Service group notification script access failed, "
                  "error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }

    if( 0 > stat( SM_NOTIFICATION_SCRIPT, &stat_data ) )
    {
        DPRINTFE( "Service group notification script stat failed, "
                  "error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }

    if( 0 >= stat_data.st_size )
    {
        DPRINTFE( "Service group notification script has zero size." );
        return( SM_FAILED );
    }

    error = sm_service_group_notification_get_env( service_group_name,
                                                   notification, &env );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get environment for service group (%s) "
                  "notification (%s), error=%s.", service_group_name,
                  sm_service_group_notification_str(notification),
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    pid = fork();
    if( 0 > pid )
    {
        DPRINTFE( "Failed to fork notification process for service group "
                  "(%s), error=%s.", service_group_name, strerror( errno ) );
        return( SM_FAILED );

    } else if( 0 == pid ) {
        // Child process.
        struct rlimit file_limits;

        DPRINTFD( "Child notification process created for service group "
                  "(%s).", service_group_name );

        if( 0 > setpgid( 0, 0 ) )
        {
            DPRINTFE( "Failed to set notification process group id for "
                      "service group (%s) notification (%s), error=%s.",
                      service_group_name,
                      sm_service_group_notification_str(notification),
                      strerror( errno ) );
            exit( SM_NOTIFICATION_SCRIPT_FAILURE );
        }

        if( 0 > getrlimit( RLIMIT_NOFILE, &file_limits ) )
        {
            DPRINTFE( "Failed to get file limits for service group (%s) "
                      "notification (%s), error=%s.", service_group_name,
                      sm_service_group_notification_str(notification),
                      strerror( errno ) );
            exit( SM_NOTIFICATION_SCRIPT_FAILURE );
        }

        unsigned int fd_i;
        for( fd_i=0; fd_i < file_limits.rlim_cur; ++fd_i )
        {
            close( fd_i );
        }

        if( 0 > open( "/dev/null", O_RDONLY ) )
        {
            DPRINTFE( "Failed to open stdin to /dev/null for service group "
                      "(%s) notification (%s), error=%s.", service_group_name,
                      sm_service_group_notification_str(notification),
                      strerror( errno ) );
        }

        if( 0 > open( "/dev/null", O_WRONLY ) )
        {
            DPRINTFE( "Failed to open stdout to /dev/null for service group "
                      "(%s) notification (%s), error=%s.", service_group_name,
                      sm_service_group_notification_str(notification),
                      strerror( errno ) );
        }

        if( 0 > open( "/dev/null", O_WRONLY ) )
        {
            DPRINTFE( "Failed to open stderr to /dev/null for service group "
                      "(%s) notification (%s), error=%s.", service_group_name, 
                      sm_service_group_notification_str(notification),
                      strerror( errno ) );
        }

        result = setpriority( PRIO_PROCESS, getpid(), -1 );
        if( 0 > result )
        {
            DPRINTFE( "Failed to set priority of process, error=%s.",
                      strerror( errno ) );
            exit( SM_NOTIFICATION_SCRIPT_FAILURE );
        }

        error = sm_service_group_notification_setup_env( &env );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to setup environment for service group (%s) "
                      "notification (%s), error=%s.", service_group_name,
                      sm_service_group_notification_str(notification),
                      sm_error_str( error ) );
            exit( SM_NOTIFICATION_SCRIPT_FAILURE );
        }

        snprintf( program_name, sizeof(program_name), "%s",
                  SM_NOTIFICATION_SCRIPT );

        char* argv[] = {program_name, NULL};

        if( 0 > execv( program_name, argv ) )
        {
            DPRINTFE( "Failed to exec command for service group (%s) "
                      "notification (%s), error=%s.", service_group_name,
                      sm_service_group_notification_str(notification),
                      strerror( errno ) );
        }

        exit( SM_NOTIFICATION_SCRIPT_FAILURE );

    } else {
        // Parent process.
        *process_id = (int) pid;

        DPRINTFD( "Child notification process (%i) created for service "
                  "group (%s) notification (%s).", *process_id,
                  service_group_name, 
                  sm_service_group_notification_str(notification) );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Timeout
// ====================================
static bool sm_service_group_notification_timeout( SmTimerIdT timer_id,
    int64_t user_data )
{
    int64_t id = user_data;
    SmServiceGroupT* service_group;
    SmErrorT error;

    service_group = sm_service_group_table_read_by_id( id );
    if( NULL == service_group )
    {
        DPRINTFE( "Failed to read service group, error=%s.",
                  sm_error_str(SM_NOT_FOUND) );
        return( false );
    }
    
    if( -1 == service_group->notification_pid )
    {
        DPRINTFE( "Service group (%s) notification script not running.",
                  service_group->name );
        return( false );
    }

    DPRINTFI( "Notification (%s) timeout for service group (%s).",
              service_group->notification_str, service_group->name );

    error = sm_service_group_notification_do_abort(
            service_group->name, service_group->notification_pid );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to abort notification script for service group "
                  "(%s), error=%s.", service_group->name,
                  sm_error_str( error ) );
    }

    service_group->notification_pid = -1;
    service_group->notification_timer_id = SM_TIMER_ID_INVALID;
    service_group->notification_complete = false;
    service_group->notification_failed = true;
    service_group->notification_timeout = true;

    error = sm_service_group_fsm_event_handler( service_group->name,
            SM_SERVICE_GROUP_EVENT_NOTIFICATION_TIMEOUT, NULL, 
            "notification script timed out" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to notify service group (%s) fsm notification "
                  "timed out, error=%s.", service_group->name, 
                  sm_error_str( error ) );
        abort();
    }

    return( false );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Complete
// =====================================
static void sm_service_group_notification_complete( pid_t pid, int exit_code,
    int64_t user_data )
{
    SmServiceGroupT* service_group;
    SmServiceGroupEventT event;
    char reason_text[SM_SERVICE_GROUP_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    service_group = sm_service_group_table_read_by_notification_pid( (int) pid );
    if( NULL == service_group )
    {
        DPRINTFE( "Failed to query service group based on pid (%i), error=%s.",
                  (int) pid, sm_error_str(SM_NOT_FOUND) );
        return;
    }
    
    if( -1 == service_group->notification_pid )
    {
        DPRINTFE( "Service group (%s) not running notification script.",
                  service_group->name );
        return;
    }

    if( SM_TIMER_ID_INVALID != service_group->notification_timer_id )
    {
        error = sm_timer_deregister( service_group->notification_timer_id );
        if( SM_OKAY == error )
        {
            DPRINTFD( "Timer stopped for notification (%s) for service "
                      "group (%s).", service_group->notification_str,
                      service_group->name );
        } else {
            DPRINTFE( "Failed to stop timer for notification (%s) for "
                      "service group (%s), error=%s.",
                      service_group->notification_str, service_group->name,
                      sm_error_str( error ) );
        }
    }

    service_group->notification_pid = -1;
    service_group->notification_timer_id = SM_TIMER_ID_INVALID;
    
    if( SM_NOTIFICATION_SCRIPT_SUCCESS == exit_code )
    {
        DPRINTFI( "Notification (%s) for service group (%s) passed.",
                  service_group->notification_str, service_group->name );

        service_group->notification_complete = true;
        service_group->notification_failed = false;
        service_group->notification_timeout = false;

        event = SM_SERVICE_GROUP_EVENT_NOTIFICATION_SUCCESS;

    } else {
        DPRINTFI( "Notification (%s) for service group (%s) failed.",
                  service_group->notification_str, service_group->name );

        service_group->notification_complete = true;
        service_group->notification_failed = true;        
        service_group->notification_timeout = false;

        event = SM_SERVICE_GROUP_EVENT_NOTIFICATION_FAILED;
        snprintf( reason_text, sizeof(reason_text), "notification script "
                  "failed" );
    }

    error = sm_service_group_fsm_event_handler( service_group->name,
                                                event, NULL, reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to notify service group (%s) fsm, error=%s.",
                  service_group->name, sm_error_str( error ) );
        abort();
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Notify
// ===================================
SmErrorT sm_service_group_notification_notify( SmServiceGroupT* service_group,
    SmServiceGroupNotificationT notification )
{
    const char* notification_str;
    char timer_name[80] = "";
    int process_id = -1;
    int timeout_in_ms = SM_NOTIFICATION_SCRIPT_TIMEOUT_IN_MS;
    SmTimerIdT timer_id = SM_TIMER_ID_INVALID;
    SmErrorT error;

    notification_str = 
        sm_service_group_notification_str(notification);

    // Run notification script.
    error = sm_service_group_notification_run( service_group->name,
                                               notification, &process_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to run notification script for service group (%s), "
                  "error=%s.", service_group->name, sm_error_str( error ) );
        return( error );
    }
   
    // Register for notification script exit.
    error = sm_process_death_register( process_id, true,
                                       sm_service_group_notification_complete,
                                       0 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for notification completion for "
                  "service group (%s), error=%s.", service_group->name,
                  sm_error_str( error ) );
        abort();
    }

    // Create timer for notification completion.
    snprintf( timer_name, sizeof(timer_name), "%s %s notification ",
              service_group->name, notification_str );

    if( sm_utils_watchdog_delayed( SM_NOTIFICATION_SCRIPT_MAX_DELAY_IN_SECS ) )
    {
        DPRINTFI( "Notification timeout %d secs increased by %d ms, "
                  "sm-watchdog delayed.", timeout_in_ms,
                  SM_NOTIFICATION_SCRIPT_TIMER_SKEW_IN_MS );
        timeout_in_ms += SM_NOTIFICATION_SCRIPT_TIMER_SKEW_IN_MS;
    }

    error = sm_timer_register( timer_name, timeout_in_ms,
                               sm_service_group_notification_timeout,
                               service_group->id, &timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create a timer for notification for service "
                  "group (%s), error=%s.", service_group->name,
                  sm_error_str( error ) );
        abort();
    }

    service_group->notification = notification;
    service_group->notification_str = notification_str;
    service_group->notification_pid = process_id;
    service_group->notification_timer_id = timer_id;
    service_group->notification_complete = false;
    service_group->notification_failed = false;
    service_group->notification_timeout = false;

    DPRINTFI( "Started notification (%s) process (%i) for service group "
              "(%s).", service_group->notification_str, process_id,
              service_group->name );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Abort
// ==================================
SmErrorT sm_service_group_notification_abort( SmServiceGroupT* service_group )
{
    const char* notification_str;
    SmErrorT error;

    notification_str = 
        sm_service_group_notification_str(service_group->notification);

    DPRINTFI( "Aborting notification (%s) for service group (%s).",
              notification_str, service_group->name );

    if( -1 != service_group->notification_pid )
    {
        error = sm_service_group_notification_do_abort( 
                    service_group->name, service_group->notification_pid );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to abort notification (%s) for service group "
                      "(%s), error=%s.", notification_str, service_group->name,
                      sm_error_str( error ) );
            return( error );
        }
    }

    if( SM_TIMER_ID_INVALID != service_group->notification_timer_id )
    {
        error = sm_timer_deregister( service_group->notification_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Cancel notification timer for service group (%s) "
                      "failed, error=%s.", service_group->name,
                      sm_error_str( error ) );
            return( error );
        }
    }

    service_group->notification = SM_SERVICE_GROUP_NOTIFICATION_NIL;
    service_group->notification_str = NULL;
    service_group->notification_pid = -1;
    service_group->notification_timer_id = SM_TIMER_ID_INVALID;
    service_group->notification_complete = false;
    service_group->notification_failed = false;
    service_group->notification_timeout = false;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Initialize
// =======================================
SmErrorT sm_service_group_notification_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Finalize
// =====================================
SmErrorT sm_service_group_notification_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
