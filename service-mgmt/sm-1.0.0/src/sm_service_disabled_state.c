//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_disabled_state.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_timer.h"
#include "sm_service_fsm.h"
#include "sm_service_enable.h"
#include "sm_service_disable.h"
#include "sm_service_audit.h"
#include "sm_service_heartbeat_api.h"

// ****************************************************************************
// Service Disabled State - Audit Timer
// ====================================
static bool sm_service_disabled_state_audit_timer( SmTimerIdT timer_id,
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
                                          SM_SERVICE_EVENT_AUDIT, NULL, 
                                          "periodic audit" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to perform audit on service (%s), error=%s.",
                  service->name, sm_error_str( error ) );
        return( true );
    }

    DPRINTFD( "Audit on service (%s) requested.", service->name );

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Disabled State - Entry
// ==============================
SmErrorT sm_service_disabled_state_entry( SmServiceT* service )
{
    char timer_name[80] = "";
    int audit_interval;
    SmTimerIdT audit_timer_id;
    SmErrorT error;

    error = sm_service_fsm_process_failure_deregister( service );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister for process failure for "
                  "service (%s), error=%s.", service->name,
                  sm_error_str( error ) );
    }

    if( service->audit_disabled_exists )
    {
        error = sm_service_audit_disabled_interval( service, &audit_interval );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to get service (%s) audit interval, error=%s.",
                      service->name, sm_error_str( error ) );
            return( error );
        }

        snprintf( timer_name, sizeof(timer_name), "%s disabled audit",
                  service->name );

        error = sm_timer_register( timer_name, audit_interval,
                                   sm_service_disabled_state_audit_timer,
                                   service->id, &audit_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to create audit-disabled timer for service (%s), "
                      "error=%s.", service->name, sm_error_str( error ) );
            return( error );
        }

        service->audit_timer_id = audit_timer_id;

    } else {
        DPRINTFI( "No audit for service (%s) exists.", service->name );
    }

    service->transition_fail_count = 0;
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Disabled State - Exit
// =============================
SmErrorT sm_service_disabled_state_exit( SmServiceT* service )
{
    SmErrorT error;

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

    if( SM_SERVICE_ACTION_AUDIT_DISABLED == service->action_running )
    {
        error = sm_service_audit_disabled_abort( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to abort action (%s) of service (%s), error=%s.",
                      sm_service_action_str( service->action_running ),
                      service->name, sm_error_str( error ) );
            return( error );
        }
    }

    if( SM_TIMER_ID_INVALID != service->audit_timer_id )
    {
        error = sm_timer_deregister( service->audit_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel audit-disabled timer, error=%s.",
                      sm_error_str( error ) );
        }

        service->audit_timer_id = SM_TIMER_ID_INVALID;
    }

    service->disable_check_dependency = true;
    service->disable_skip_dependent = false;
    service->transition_fail_count = 0;
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Disabled State - Transition
// ===================================
SmErrorT sm_service_disabled_state_transition( SmServiceT* service,
    SmServiceStateT from_state )
{
    SmErrorT error;
    if(( SM_SERVICE_STATE_UNKNOWN == from_state )||
       ( SM_SERVICE_STATE_DISABLING == from_state ))
    {
        service->disable_check_dependency = true;
    }

    if(service->disable_skip_dependent)
    {
        service->disable_check_dependency = false;
    }

    if ( !service->disable_check_dependency )
    {
        error = sm_service_disable( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to disable service (%s), error=%s",
                      service->name, sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Disabled State - Event Handler
// ======================================
SmErrorT sm_service_disabled_state_event_handler( SmServiceT* service,
    SmServiceEventT event, void* event_data[] )
{
    SmServiceStateT state;
    SmErrorT error;

    switch( event )
    {
        case SM_SERVICE_EVENT_ENABLE_THROTTLE:
            if(( SM_SERVICE_ACTION_DISABLE != service->action_running )&&
               ( service->fail_count < service->max_failures )&&
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
            } else if(( service->fail_count >= service->max_failures )&&
                      ( SM_SERVICE_STATUS_FAILED != service->status ))
            {
                service->status = SM_SERVICE_STATUS_FAILED;
                service->condition = SM_SERVICE_CONDITION_RECOVERY_FAILURE;

                DPRINTFI( "Service (%s) is failed and has reached max "
                          "failures (%i).", service->name,
                          service->max_failures );
                return( SM_OKAY );
            }
        break;

        case SM_SERVICE_EVENT_DISABLE_SUCCESS:
            DPRINTFI( "Service (%s) disable successfully completed.",
                      service->name );
        break;

        case SM_SERVICE_EVENT_DISABLE_FAILED:
        case SM_SERVICE_EVENT_DISABLE_TIMEOUT:
            error = sm_service_fsm_set_state( service, 
                                              SM_SERVICE_STATE_DISABLING );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service (%s) state (%s), "
                          "error=%s.", service->name,
                          sm_service_state_str( SM_SERVICE_STATE_DISABLING ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_EVENT_AUDIT:
            error = sm_service_heartbeat_api_stop_heartbeat( service->name );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to stop service (%s) heartbeat, error=%s.",
                          service->name, sm_error_str( error ) );
            }

            if(( SM_SERVICE_ACTION_DISABLE != service->action_running )&&
               ( service->audit_disabled_exists ))
            {
                error = sm_service_audit_disabled( service );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to audit service (%s), error=%s",
                              service->name, sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_EVENT_AUDIT_SUCCESS:
            DPRINTFD( "Service (%s) audit success.", service->name );
        break;

        case SM_SERVICE_EVENT_AUDIT_FAILED:
            DPRINTFD( "Service (%s) audit failed.", service->name );
        break;

        case SM_SERVICE_EVENT_AUDIT_TIMEOUT:
            DPRINTFD( "Service (%s) audit timeout.", service->name );
        break;

        case SM_SERVICE_EVENT_AUDIT_MISMATCH:
            state = *(SmServiceStateT*) 
                    event_data[SM_SERVICE_EVENT_DATA_STATE];

            DPRINTFI( "State mismatch for service (%s), expected=%s, "
                      "result=%s.", service->name,
                      sm_service_state_str(SM_SERVICE_STATE_DISABLED),
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
// Service Disabled State - Initialize
// ===================================
SmErrorT sm_service_disabled_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Disabled State - Finalize
// =================================
SmErrorT sm_service_disabled_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
