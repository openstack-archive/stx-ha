//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_disabling_state.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_fsm.h"
#include "sm_service_disable.h"

#define SM_SERVICE_DISABLING_TIMEOUT_IN_MS                               300000

// ****************************************************************************
// Service Disabling State - Timeout Timer
// =======================================
static bool sm_service_disabling_state_timeout_timer( SmTimerIdT timer_id,
    int64_t user_data )
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
                                          SM_SERVICE_EVENT_DISABLE_TIMEOUT,
                                          NULL, "overall disabling timeout" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to signal overall disabling timeout to service "
                  "(%s), error=%s.", service->name, sm_error_str( error ) );
        return( true );
    }

    DPRINTFI( "Service (%s) disabling overall timeout.", service->name );

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Disabling State - Entry
// ===============================
SmErrorT sm_service_disabling_state_entry( SmServiceT* service )
{
    char timer_name[80] = "";
    SmTimerIdT action_state_timer_id;
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != service->action_state_timer_id )
    {
        error = sm_timer_deregister( service->action_state_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel disabling overall timer, error=%s.",
                      sm_error_str( error ) );
        }

        service->action_state_timer_id = SM_TIMER_ID_INVALID;
    }

    snprintf( timer_name, sizeof(timer_name), "%s disabling overall timeout",
              service->name );

    error = sm_timer_register( timer_name, 
                               SM_SERVICE_DISABLING_TIMEOUT_IN_MS,
                               sm_service_disabling_state_timeout_timer,
                               service->id, &action_state_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create disabling overall timer for service "
                  "(%s), error=%s.", service->name, sm_error_str( error ) );
        return( error );
    }

    service->action_state_timer_id = action_state_timer_id;

    error = sm_service_disable( service );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to disable service (%s), error=%s",
                  service->name, sm_error_str( error ) );
        return( error );
    }
    
    service->action_fail_count = 0;
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Disabling State - Exit
// ==============================
SmErrorT sm_service_disabling_state_exit( SmServiceT* service )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != service->action_state_timer_id )
    {
        error = sm_timer_deregister( service->action_state_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel disabling overall timer, error=%s.",
                      sm_error_str( error ) );
        }

        service->action_state_timer_id = SM_TIMER_ID_INVALID;
    }

    if( SM_SERVICE_ACTION_DISABLE == service->action_running )
    {
        error = sm_service_disable_abort( service );
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
// Service Disabling State - Transition
// ====================================
SmErrorT sm_service_disabling_state_transition( SmServiceT* service,
    SmServiceStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Disabling State - Event Handler
// =======================================
SmErrorT sm_service_disabling_state_event_handler( SmServiceT* service,
    SmServiceEventT event, void* event_data[] )
{
    SmErrorT error;

    switch( event )
    {
        case SM_SERVICE_EVENT_ENABLE_THROTTLE:
            if(( SM_SERVICE_ACTION_DISABLE != service->action_running )&&
               !(( SM_SERVICE_STATUS_FAILED == service->status )&&
                 ( SM_SERVICE_CONDITION_FATAL_FAILURE == service->condition ))&&
               ( service->enable_action_exists ))
            {
                error = sm_service_fsm_set_state( service,
                                                  SM_SERVICE_STATE_ENABLING_THROTTLE );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Set state (%s) of service (%s) failed, error=%s.",
                              sm_service_state_str( SM_SERVICE_STATE_ENABLING_THROTTLE ),
                              service->name, sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_EVENT_DISABLE:
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
                if( SM_SERVICE_ACTION_DISABLE != service->action_running )
                {
                    error = sm_service_disable( service );
                    if( SM_OKAY != error )
                    {
                        DPRINTFE( "Failed to disable service (%s), error=%s",
                                  service->name, sm_error_str( error ) );
                        return( error );
                    }
                }
            }
        break;

        case SM_SERVICE_EVENT_PROCESS_FAILURE:
            DPRINTFD( "Service (%s) has reported process failure "
                      "when it is in 'disabling' state. It is considered disable success.",
                       service->name );
            //allow the script to complete further cleanup when applicable.
        break;
        case SM_SERVICE_EVENT_DISABLE_SUCCESS:
            error = sm_service_fsm_set_state( service, 
                                              SM_SERVICE_STATE_DISABLED );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service (%s) state (%s), error=%s.",
                          service->name,
                          sm_service_state_str( SM_SERVICE_STATE_DISABLED ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_EVENT_DISABLE_FAILED:
        case SM_SERVICE_EVENT_DISABLE_TIMEOUT:
            // Service engine will run the disable action again.
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
// Service Disabling State - Initialize
// ====================================
SmErrorT sm_service_disabling_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Disabling State - Finalize
// ==================================
SmErrorT sm_service_disabling_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
