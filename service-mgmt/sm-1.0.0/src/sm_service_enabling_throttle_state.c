//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_enabling_throttle_state.h"

#include <stdio.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_fsm.h"
#include "sm_service_go_active.h"
#include "sm_service_enable.h"
#include "sm_service_dependency.h"

#define SM_SERVICE_ENABLING_THROTTLE_TIMEOUT_IN_MS  25

// ****************************************************************************
// Service Enabling Throttle State - Timeout Timer
// ======================================

static bool sm_service_enabling_throttle_state_timeout_timer( SmTimerIdT timer_id,
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
                                          SM_SERVICE_EVENT_ENABLE_THROTTLE,
                                          NULL, "wait for enabling throttle reopen" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to signal service enabling throttle "
                  "(%s), error=%s.", service->name, sm_error_str( error ) );
        return( true );
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabling Throttle State - Entry
// ==============================
SmErrorT sm_service_enabling_throttle_state_entry( SmServiceT* service )
{
    char timer_name[80] = "";
    SmTimerIdT action_state_timer_id;
    SmErrorT error;
    int timeout = SM_SERVICE_ENABLING_THROTTLE_TIMEOUT_IN_MS;

    if( SM_TIMER_ID_INVALID != service->action_state_timer_id )
    {
        error = sm_timer_deregister( service->action_state_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel enabling overall timer, error=%s.",
                      sm_error_str( error ) );
        }

        service->action_state_timer_id = SM_TIMER_ID_INVALID;
    }

    bool dependency_met;
    // Are enable dependencies met?
    error =  sm_service_dependency_enable_met( service, &dependency_met );
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

    DPRINTFD( "All dependencies for service (%s) were met.Attempt to enable service ",
                service->name );

    if ( sm_service_enable_throttle_check() )
    {
        error = sm_service_fsm_set_state( service,
                                          SM_SERVICE_STATE_ENABLING );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Set state (%s) of service (%s) failed, error=%s.",
                      sm_service_state_str( SM_SERVICE_STATE_ENABLING ),
                      service->name, sm_error_str( error ) );
            return( error );
        }
    }else
    {
        snprintf( timer_name, sizeof(timer_name), "%s wait for enabling",
                  service->name );

        error = sm_timer_register( timer_name,
                                   timeout,
                                   sm_service_enabling_throttle_state_timeout_timer,
                                   service->id, &action_state_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to create enabling throttle timer for service "
                      "(%s), error=%s.", service->name, sm_error_str( error ) );
            return( error );
        }

        service->action_state_timer_id = action_state_timer_id;
    }
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabling Throttle State - Exit
// =============================
SmErrorT sm_service_enabling_throttle_state_exit( SmServiceT* service )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != service->action_state_timer_id )
    {
        error = sm_timer_deregister( service->action_state_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel enabling overall timer, error=%s.",
                      sm_error_str( error ) );
        }

        service->action_state_timer_id = SM_TIMER_ID_INVALID;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabling Throttle State - Transition
// ===================================
SmErrorT sm_service_enabling_throttle_state_transition( SmServiceT* service,
    SmServiceStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabling Throttle State - Event Handler
// ======================================
SmErrorT sm_service_enabling_throttle_state_event_handler( SmServiceT* service,
    SmServiceEventT event, void* event_data[] )
{
    SmErrorT error;

    switch( event )
    {
        case SM_SERVICE_EVENT_ENABLE_THROTTLE:
            error = sm_service_fsm_set_state( service,
                                              SM_SERVICE_STATE_ENABLING_THROTTLE );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Set state (%s) of service (%s) failed, error=%s.",
                          sm_service_state_str( SM_SERVICE_STATE_ENABLING_THROTTLE ),
                          service->name, sm_error_str( error ) );
                return( error );
            }

        break;
        case SM_SERVICE_EVENT_ENABLE:
            error = sm_service_fsm_set_state( service,
                                              SM_SERVICE_STATE_ENABLING );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Set state (%s) of service (%s) failed, error=%s.",
                          sm_service_state_str( SM_SERVICE_STATE_ENABLING ),
                          service->name, sm_error_str( error ) );
                return( error );
            }

        break;

        case SM_SERVICE_EVENT_DISABLE:
            error = sm_service_fsm_set_state( service,
                                              SM_SERVICE_STATE_DISABLING );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Set state (%s) of service (%s) failed, error=%s.",
                          sm_service_state_str( SM_SERVICE_STATE_DISABLING ),
                          service->name, sm_error_str( error ) );
                return( error );
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
            DPRINTFD( "Service (%s) ignoring event (%s).", service->name,
                      sm_service_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabling Throttle State - Initialize
// ===================================
SmErrorT sm_service_enabling_throttle_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabling Throttle State - Finalize
// =================================
SmErrorT sm_service_enabling_throttle_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
