//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_go_active.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_process_death.h"
#include "sm_timer.h"
#include "sm_service_dependency.h"
#include "sm_service_fsm.h"
#include "sm_service_action.h"

// ****************************************************************************
// Service Go-Active - Result Handler
// ==================================
static SmErrorT service_go_active_result_handler( SmServiceT* service,
    SmServiceActionT action_running, SmServiceActionResultT action_result,
    SmServiceStateT service_state, SmServiceStatusT service_status,
    SmServiceConditionT service_condition, const char reason_text[] )
{
    bool retries_exhausted = true;
    int max_failure_retries;
    int max_timeout_retries;
    int max_total_retries;
    SmErrorT error;

    if( SM_SERVICE_ACTION_RESULT_SUCCESS == action_result )
    {
        DPRINTFD( "Action (%s) of service (%s) completed with success.",
                  sm_service_action_str( action_running ), service->name );
        goto REPORT;
    }

    error = sm_service_action_max_retries( service->name, action_running,
                                           &max_failure_retries,
                                           &max_timeout_retries,
                                           &max_total_retries );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get max retries for action (%s) of service (%s), "
                  "error=%s.", sm_service_action_str( action_running ),
                  service->name, sm_error_str( error ) );
        goto REPORT;
    }

    if( SM_SERVICE_ACTION_RESULT_FAILED == action_result )
    {
        retries_exhausted =
            (( max_failure_retries <= service->action_attempts )||
             ( max_total_retries   <= service->action_attempts ));

    } else if( SM_SERVICE_ACTION_RESULT_TIMEOUT == action_result ) {
        retries_exhausted =
            (( max_timeout_retries <= service->action_attempts )||
             ( max_total_retries   <= service->action_attempts ));
    }

    if( retries_exhausted )
    {
        DPRINTFI( "Max retires met for action (%s) of service (%s), "
                  "attempts=%i.", sm_service_action_str( action_running ),
                  service->name, service->action_attempts );
        goto REPORT;
    } else {
        DPRINTFI( "Max retires not met for action (%s) of service (%s), "
                  "attempts=%i.", sm_service_action_str( action_running ),
                  service->name, service->action_attempts );

        error = sm_service_go_active( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to go-active on service (%s), error=%s.",
                      service->name, sm_error_str( error ) );
            goto REPORT;
        }
    }

    return( SM_OKAY );

REPORT:
    service->action_attempts = 0;

    error = sm_service_fsm_action_complete_handler( service->name,
                        action_running, action_result, service_state,
                        service_status, service_condition, reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to notify service (%s) fsm action complete, "
                  "error=%s.", service->name, sm_error_str( error ) );
        abort();
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Go-Active - Timeout
// ===========================
static bool sm_service_go_active_timeout( SmTimerIdT timer_id, 
    int64_t user_data )
{
    int64_t id = user_data;
    SmServiceT* service;    
    SmServiceActionT action_running;
    SmServiceActionResultT action_result;
    SmServiceStateT service_state;
    SmServiceStatusT service_status;
    SmServiceConditionT service_condition;
    char reason_text[SM_SERVICE_ACTION_RESULT_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    service = sm_service_table_read_by_id( id );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service, error=%s.",
                  sm_error_str(SM_NOT_FOUND) );
        return( false );
    }
    
    if( SM_SERVICE_ACTION_GO_ACTIVE != service->action_running )
    {
        DPRINTFE( "Service (%s) not running action (%s).", service->name,
                  sm_service_action_str( service->action_running ) );
        return( false );
    }

    action_running = service->action_running;

    error = sm_service_action_result( SM_SERVICE_ACTION_PLUGIN_TIMEOUT,
                                      service->name, service->action_running,
                                      &action_result, &service_state,
                                      &service_status, &service_condition,
                                      reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get action result for service (%s) defaulting "
                  "to failed, exit_code=%i, error=%s.", service->name,
                  SM_SERVICE_ACTION_PLUGIN_TIMEOUT, sm_error_str( error ) );
        action_result = SM_SERVICE_ACTION_RESULT_FAILED;
        service_state = SM_SERVICE_STATE_UNKNOWN;
        service_status = SM_SERVICE_STATUS_UNKNOWN;
        service_condition = SM_SERVICE_CONDITION_UNKNOWN;
    }

    DPRINTFI( "Action (%s) timeout with result (%s), state (%s), "
              "status (%s), and condition (%s) for service (%s), "
              "reason_text=%s, exit_code=%i.", 
              sm_service_action_str( action_running ),
              sm_service_action_result_str( action_result ),
              sm_service_state_str( service_state ),
              sm_service_status_str( service_status ),
              sm_service_condition_str( service_condition ),
              service->name, reason_text, SM_SERVICE_ACTION_PLUGIN_TIMEOUT );

    error = sm_service_action_abort( service->name, service->action_pid );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to abort action (%s) of service (%s), error=%s.",
                  sm_service_action_str( service->action_running ),
                  service->name, sm_error_str( error ) );
    }

    service->action_running = SM_SERVICE_ACTION_NONE;
    service->action_pid = -1;
    service->action_timer_id = SM_TIMER_ID_INVALID;

    error = service_go_active_result_handler( service, action_running,
                                              action_result, service_state,
                                              service_status, service_condition,
                                              reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to handle go-active result for service (%s), "
                  "error=%s.", service->name, sm_error_str( error ) );
        abort();
    }

    return( false );
}
// ****************************************************************************

// ****************************************************************************
// Service Go-Active - Complete
// ============================
static void sm_service_go_active_complete( pid_t pid, int exit_code,
    int64_t user_data )
{
    SmServiceT* service;    
    SmServiceActionT action_running;
    SmServiceActionResultT action_result;
    SmServiceStateT service_state;
    SmServiceStatusT service_status;
    SmServiceConditionT service_condition;
    char reason_text[SM_SERVICE_ACTION_RESULT_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    if( SM_PROCESS_FAILED == exit_code )
    {
        exit_code = SM_SERVICE_ACTION_PLUGIN_FAILURE;
    }

    service = sm_service_table_read_by_action_pid( (int) pid );
    if( NULL == service )
    {
        DPRINTFE( "Failed to query service based on pid (%i), error=%s.",
                  (int) pid, sm_error_str(SM_NOT_FOUND) );
        return;
    }
    
    if( SM_SERVICE_ACTION_GO_ACTIVE != service->action_running )
    {
        DPRINTFE( "Service (%s) not running go-active action.",
                  service->name );
        return;
    }

    if( SM_TIMER_ID_INVALID != service->action_timer_id )
    {
        error = sm_timer_deregister( service->action_timer_id );
        if( SM_OKAY == error )
        {
            DPRINTFD( "Timer stopped for action (%s) of service (%s).",
                      sm_service_action_str( service->action_running ),
                      service->name );
        } else {
            DPRINTFE( "Failed to stop timer for action (%s) of "
                      "service (%s), error=%s.",
                      sm_service_action_str( service->action_running ),
                      service->name, sm_error_str( error ) );
        }
    }

    action_running = service->action_running;

    if( SM_SERVICE_ACTION_PLUGIN_FORCE_SUCCESS == exit_code )
    {
        DPRINTFI( "Action (%s) for service (%s) is force-passed.",
                  sm_service_action_str( action_running ), service->name );

        action_result = SM_SERVICE_ACTION_RESULT_SUCCESS;
        service_state = SM_SERVICE_STATE_UNKNOWN;
        service_status = SM_SERVICE_STATUS_UNKNOWN;
        service_condition = SM_SERVICE_CONDITION_UNKNOWN;

    } else {
        error = sm_service_action_result( exit_code, service->name, 
                                          service->action_running, 
                                          &action_result, &service_state,
                                          &service_status, &service_condition,
                                          reason_text );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to get action result for service (%s) "
                      "defaulting to failed, exit_code=%i, error=%s.",
                      service->name, exit_code, sm_error_str( error ) );
            action_result = SM_SERVICE_ACTION_RESULT_FAILED;
            service_state = SM_SERVICE_STATE_UNKNOWN;
            service_status = SM_SERVICE_STATUS_UNKNOWN;
            service_condition = SM_SERVICE_CONDITION_UNKNOWN;
        }
    }

    DPRINTFI( "Action (%s) completed with result (%s), state (%s), "
              "status (%s), and condition (%s) for service (%s), "
              "reason_text=%s, exit_code=%i.", 
              sm_service_action_str( action_running ),
              sm_service_action_result_str( action_result ),
              sm_service_state_str( service_state ),
              sm_service_status_str( service_status ),
              sm_service_condition_str( service_condition ),
              service->name, reason_text, exit_code );

    service->action_running = SM_SERVICE_ACTION_NONE;
    service->action_pid = -1;
    service->action_timer_id = SM_TIMER_ID_INVALID;

    error = service_go_active_result_handler( service, action_running,
                                              action_result, service_state,
                                              service_status, service_condition,
                                              reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to handle go-active result for service (%s), "
                  "error=%s.", service->name, sm_error_str( error ) );
        abort();
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Go-Active
// =================
SmErrorT sm_service_go_active( SmServiceT* service )
{
    char timer_name[80] = "";
    int process_id = -1;
    int timeout = 0;
    bool dependency_met;
    SmTimerIdT timer_id = SM_TIMER_ID_INVALID;
    SmErrorT error;

    // Are go-active dependencies met?
    error =  sm_service_dependency_go_active_met( service, &dependency_met );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to check service (%s) dependencies, error=%s.",
                  service->name, sm_error_str( error ) );
        return( error );
    }

    if( !dependency_met )
    {
        DPRINTFD( "Some dependencies for service (%s) were not met.",
                  service->name );
        return( SM_OKAY );
    }

    DPRINTFD( "All dependencies for service (%s) were met.", service->name );

    // Run action.
    error = sm_service_action_run( service->name, 
                                   service->instance_name,
                                   service->instance_params,            
                                   SM_SERVICE_ACTION_GO_ACTIVE,
                                   &process_id, &timeout );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to run go-active action for service (%s), "
                  "error=%s.", service->name, sm_error_str( error ) );
        return( error );
    }
   
    // Register for action script exit notification.
    error = sm_process_death_register( process_id, true,
                                       sm_service_go_active_complete, 0 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for go-active action completion for "
                  "service (%s), error=%s.", service->name,
                  sm_error_str( error ) );
        abort();
    }

    // Create timer for action completion.
    snprintf( timer_name, sizeof(timer_name), "%s %s action",
              service->name, sm_service_action_str( SM_SERVICE_ACTION_GO_ACTIVE ) );

    error = sm_timer_register( timer_name, timeout, sm_service_go_active_timeout,
                               service->id, &timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create a timer for action (%s) for service (%s), "
                  "error=%s.", sm_service_action_str( SM_SERVICE_ACTION_GO_ACTIVE ),
                  service->name, sm_error_str( error ) );
        abort();
    }

    // Write to database that we are running an action.
    service->action_running = SM_SERVICE_ACTION_GO_ACTIVE;
    service->action_pid = process_id;
    service->action_timer_id = timer_id;
    service->action_attempts += 1;

    DPRINTFI( "Started go-active action (%i) for service (%s).", process_id,
              service->name );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Go-Active - Abort
// =========================
SmErrorT sm_service_go_active_abort( SmServiceT* service )
{
    SmErrorT error;

    DPRINTFI( "Aborting action (%s) of service (%s).",
              sm_service_action_str( service->action_running ),
              service->name );

    error = sm_service_action_abort( service->name, service->action_pid );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to abort action (%s) of service (%s), error=%s.",
                  sm_service_action_str( service->action_running ),
                  service->name, sm_error_str( error ) );
        return( error );
    }

    if( SM_TIMER_ID_INVALID != service->action_timer_id )
    {
        error = sm_timer_deregister( service->action_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Cancel timer for action (%s) of service (%s) failed, "
                      "error=%s.", service->name,
                      sm_service_action_str( service->action_running ),
                      sm_error_str( error ) );
            return( error );
        }
    }

    service->action_running = SM_SERVICE_ACTION_NONE;
    service->action_pid = -1;
    service->action_timer_id = SM_TIMER_ID_INVALID;
    service->action_attempts = 0;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Go-Active - Exists
// ==========================
SmErrorT sm_service_go_active_exists( SmServiceT* service, bool* exists )
{
    SmErrorT error;

    error = sm_service_action_exist( service->name, SM_SERVICE_ACTION_GO_ACTIVE );
    if(( SM_OKAY != error )&&( SM_NOT_FOUND != error ))
    {
        DPRINTFE( "Failed to determine if action (%s) exists for service (%s), "
                  "error=%s.", sm_service_action_str( SM_SERVICE_ACTION_GO_ACTIVE ),
                  service->name, sm_error_str( error ) );
        return( error );
    }

    if( SM_OKAY == error )
    {
        *exists = true;
    } else {
        *exists = false;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Go-Active - Initialize
// ==============================
SmErrorT sm_service_go_active_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Go-Active - Finalize
// ============================
SmErrorT sm_service_go_active_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
