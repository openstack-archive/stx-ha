//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_enabled_go_standby_state.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_fsm.h"
#include "sm_service_go_standby.h"
#include "sm_service_audit.h"

#define SM_SERVICE_GO_STANDBY_TIMEOUT_IN_MS                              300000

// ****************************************************************************
// Service Enabled Go-Standby State - Timeout Timer
// ================================================
static bool sm_service_enabled_go_standby_state_timeout_timer(
    SmTimerIdT timer_id, int64_t user_data )
{
    int64_t id = user_data;
    SmServiceT* service;
    SmErrorT error;

    service = sm_service_table_read_by_id( id );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service, error=%s.",
                  sm_error_str(SM_NOT_FOUND) );
        return( true );
    }

    error = sm_service_fsm_event_handler( service->name,
                                          SM_SERVICE_EVENT_GO_STANDBY_TIMEOUT,
                                          NULL, "overall go-standby timeout" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to signal overall go-standby timeout to service "
                  "(%s), error=%s.", service->name, sm_error_str( error ) );
        return( true );
    }

    DPRINTFI( "Service (%s) go-standby overall timeout.", service->name );

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Standby State - Entry
// ========================================
SmErrorT sm_service_enabled_go_standby_state_entry( SmServiceT* service )
{
    char timer_name[80] = "";
    SmTimerIdT action_state_timer_id;
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != service->action_state_timer_id )
    {
        error = sm_timer_deregister( service->action_state_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel go-standby overall timer, error=%s.",
                      sm_error_str( error ) );
        }

        service->action_state_timer_id = SM_TIMER_ID_INVALID;
    }

    snprintf( timer_name, sizeof(timer_name), "%s go-standby overall timeout",
              service->name );

    error = sm_timer_register( timer_name, 
                               SM_SERVICE_GO_STANDBY_TIMEOUT_IN_MS,
                               sm_service_enabled_go_standby_state_timeout_timer,
                               service->id, &action_state_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create go-standby overall timer for service "
                  "(%s), error=%s.", service->name, sm_error_str( error ) );
        return( error );
    }

    service->action_state_timer_id = action_state_timer_id;

    error = sm_service_go_standby( service );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to go-standby on service (%s), error=%s",
                  service->name, sm_error_str( error ) );
        return( error );
    }
    
    service->action_fail_count = 0;
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Standby State - Exit
// =======================================
SmErrorT sm_service_enabled_go_standby_state_exit( SmServiceT* service )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != service->action_state_timer_id )
    {
        error = sm_timer_deregister( service->action_state_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel go-standby overall timer, error=%s.",
                      sm_error_str( error ) );
        }

        service->action_state_timer_id = SM_TIMER_ID_INVALID;
    }

    if( SM_SERVICE_ACTION_GO_STANDBY == service->action_running )
    {
        error = sm_service_go_standby_abort( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to abort action (%s) of service (%s), error=%s.",
                      sm_service_action_str( service->action_running ),
                      service->name, sm_error_str( error ) );
            return( error );
        }
    }
    else if( SM_SERVICE_ACTION_AUDIT_ENABLED == service->action_running )
    {
        error = sm_service_audit_enabled_abort( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to abort action (%s) of service (%s), error=%s.",
                      sm_service_action_str( service->action_running ),
                      service->name, sm_error_str( error ) );
            return( error );
        }
    }

    service->action_fail_count = 0;
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Standby State - Transition
// =============================================
SmErrorT sm_service_enabled_go_standby_state_transition( SmServiceT* service,
    SmServiceStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Standby State - Event Handler
// ================================================
SmErrorT sm_service_enabled_go_standby_state_event_handler(
    SmServiceT* service, SmServiceEventT event, void* event_data[] )
{
    SmServiceStateT state;
    SmErrorT error;

    switch( event )
    {
        case SM_SERVICE_EVENT_GO_ACTIVE:
            if(( SM_SERVICE_ACTION_GO_STANDBY != service->action_running )&&
               !(( SM_SERVICE_STATUS_FAILED == service->status )&&
                 ( SM_SERVICE_CONDITION_FATAL_FAILURE == service->condition ))&&
               ( service->go_active_action_exists ))
            {
                error = sm_service_fsm_set_state( service, 
                                              SM_SERVICE_STATE_ENABLED_GO_ACTIVE );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Set state (%s) of service (%s) failed, error=%s.",
                              sm_service_state_str(
                                  SM_SERVICE_STATE_ENABLED_GO_ACTIVE ),
                              service->name, sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_EVENT_GO_STANDBY:
            if( service->transition_fail_count >= service->max_transition_failures )
            {
                if( SM_SERVICE_CONDITION_FATAL_FAILURE != service->condition )
                {
                    service->status = SM_SERVICE_STATUS_FAILED;
                    service->condition = SM_SERVICE_CONDITION_FATAL_FAILURE;

                    DPRINTFI( "Service (%s) is failed and has reached max "
                              "transition failures (%i).", service->name,
                              service->max_transition_failures );
                }
            }
            else if( service->action_fail_count >= service->max_action_failures )
            {
                if( SM_SERVICE_CONDITION_ACTION_FAILURE != service->condition )
                {
                    service->status = SM_SERVICE_STATUS_FAILED;
                    service->condition = SM_SERVICE_CONDITION_ACTION_FAILURE;

                    DPRINTFI( "Service (%s) is failed and has reached max "
                              "action failures (%i).", service->name,
                              service->max_action_failures );
                }
            } else {
                if(( SM_SERVICE_ACTION_GO_STANDBY    != service->action_running )&&
                   ( SM_SERVICE_ACTION_AUDIT_ENABLED != service->action_running ))
                {
                    error = sm_service_go_standby( service );
                    if( SM_OKAY != error )
                    {
                        DPRINTFE( "Failed to go-standby on service (%s), "
                                  "error=%s", service->name,
                                  sm_error_str( error ) );
                        return( error );
                    }
                }
            }
        break;

        case SM_SERVICE_EVENT_GO_STANDBY_SUCCESS:
            error = sm_service_fsm_set_state( service,
                                              SM_SERVICE_STATE_ENABLED_STANDBY );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service (%s) state (%s), "
                          "error=%s.", service->name, 
                          sm_service_state_str( SM_SERVICE_STATE_ENABLED_STANDBY ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_EVENT_GO_STANDBY_FAILED:
        case SM_SERVICE_EVENT_GO_STANDBY_TIMEOUT:
            ++(service->action_fail_count);
            ++(service->transition_fail_count);

            if( service->transition_fail_count >= service->max_transition_failures )
            {
                if( SM_SERVICE_CONDITION_FATAL_FAILURE != service->condition )
                {
                    service->status = SM_SERVICE_STATUS_FAILED;
                    service->condition = SM_SERVICE_CONDITION_FATAL_FAILURE;

                    DPRINTFI( "Service (%s) is failed and has reached max "
                              "transition failures (%i).", service->name,
                              service->max_transition_failures );
                }
            }
            else if( service->action_fail_count >= service->max_action_failures )
            {
                if( SM_SERVICE_CONDITION_ACTION_FAILURE != service->condition )
                {
                    service->status = SM_SERVICE_STATUS_FAILED;
                    service->condition = SM_SERVICE_CONDITION_ACTION_FAILURE;

                    DPRINTFI( "Service (%s) is failed and has reached max "
                              "action failures (%i).", service->name,
                              service->max_action_failures );
                }
            }

            // Do an audit to make sure the process is still running.
            error = sm_service_audit_enabled( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to audit service (%s), error=%s",
                          service->name, sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_EVENT_AUDIT_SUCCESS:
            // Service engine will request an enable-go-standby again.
            DPRINTFD( "Service (%s) audit success.", service->name );
        break;

        case SM_SERVICE_EVENT_AUDIT_MISMATCH:
            state = *(SmServiceStateT*) 
                    event_data[SM_SERVICE_EVENT_DATA_STATE];

            DPRINTFI( "State mismatch for service (%s), expected=%s, "
                      "result=%s.", service->name,
                      sm_service_state_str(SM_SERVICE_STATE_ENABLED_ACTIVE),
                      sm_service_state_str(state) );

            error = sm_service_fsm_set_state( service, state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service (%s) state (%s), error=%s.",
                          service->name, sm_service_state_str( state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_EVENT_AUDIT_FAILED:
        case SM_SERVICE_EVENT_AUDIT_TIMEOUT:
            // Ignore.
        break;

        case SM_SERVICE_EVENT_SHUTDOWN:
            error = sm_service_fsm_set_state( service,
                                              SM_SERVICE_STATE_SHUTDOWN );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service (%s) state (%s), error=%s.",
                          service->name,
                          sm_service_state_str(SM_SERVICE_STATE_SHUTDOWN),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFI( "Service (%s) ignoring event (%s).", service->name,
                      sm_service_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Standby State - Initialize
// =============================================
SmErrorT sm_service_enabled_go_standby_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Standby State - Finalize
// ===========================================
SmErrorT sm_service_enabled_go_standby_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
