//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_enabled_standby_state.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_timer.h"
#include "sm_service_dependency.h"
#include "sm_service_fsm.h"
#include "sm_service_disable.h"
#include "sm_service_go_active.h"
#include "sm_service_audit.h"
#include "sm_service_heartbeat_api.h"

// ****************************************************************************
// Service Enabled Standby State - Audit Timer
// ===========================================
static bool sm_service_enabled_standby_state_audit_timer( SmTimerIdT timer_id,
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
// Service Enabled Standby State - Entry
// =====================================
SmErrorT sm_service_enabled_standby_state_entry( SmServiceT* service )
{
    char timer_name[80] = "";
    int audit_interval;
    SmTimerIdT audit_timer_id;
    SmErrorT error;

    error = sm_service_fsm_process_failure_register( service );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for process failure for "
                  "service (%s), error=%s.", service->name,
                  sm_error_str( error ) );
    }

    error = sm_service_heartbeat_api_start_heartbeat( service->name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to start service (%s) heartbeat, error=%s.",
                  service->name, sm_error_str( error ) );
    }

    error = sm_service_fsm_start_fail_countdown_timer( service );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create fail countdown timer for service (%s), "
                  "error=%s.", service->name, sm_error_str( error ) );
        return( error );
    }

    if( service->audit_enabled_exists )
    {
        error = sm_service_audit_enabled_interval( service, &audit_interval );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to get service (%s) audit interval, error=%s.",
                      service->name, sm_error_str( error ) );
            return( error );
        }

        snprintf( timer_name, sizeof(timer_name), "%s enabled-standby audit",
                  service->name );

        error = sm_timer_register( timer_name, audit_interval,
                                   sm_service_enabled_standby_state_audit_timer,
                                   service->id, &audit_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to create audit-enabled timer for service (%s), "
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
// Service Enabled Standby State - Exit
// ====================================
SmErrorT sm_service_enabled_standby_state_exit( SmServiceT* service )
{
    SmErrorT error;

    error = sm_service_heartbeat_api_stop_heartbeat( service->name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to stop service (%s) heartbeat, error=%s.",
                  service->name, sm_error_str( error ) );
    }

    error = sm_service_fsm_stop_fail_countdown_timer( service );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to stop fail countdown timer for service (%s), "
                  "error=%s.", service->name, sm_error_str( error ) );
        return( error );
    }

    if( SM_SERVICE_ACTION_AUDIT_ENABLED == service->action_running )
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

    if( SM_TIMER_ID_INVALID != service->audit_timer_id )
    {
        error = sm_timer_deregister( service->audit_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel audit-enabled timer, error=%s.",
                      sm_error_str( error ) );
        }

        service->audit_timer_id = SM_TIMER_ID_INVALID;
    }

    service->transition_fail_count = 0;
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Standby State - Transition
// ==========================================
SmErrorT sm_service_enabled_standby_state_transition( SmServiceT* service,
    SmServiceStateT from_state )
{
    if( SM_SERVICE_STATE_ENABLING == from_state )
    {
        service->status = SM_SERVICE_STATUS_NONE;
        service->condition = SM_SERVICE_CONDITION_NONE;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Standby State - Event Handler
// =============================================
SmErrorT sm_service_enabled_standby_state_event_handler( SmServiceT* service,
    SmServiceEventT event, void* event_data[] )
{
    bool state_dependency_met;
    SmServiceStateT state;
    SmErrorT error;

    switch( event )
    {
        case SM_SERVICE_EVENT_HEARTBEAT_OKAY:
            service->status = SM_SERVICE_STATUS_NONE;
        break;

        case SM_SERVICE_EVENT_HEARTBEAT_WARN:
            service->status = SM_SERVICE_STATUS_WARN;
        break;

        case SM_SERVICE_EVENT_HEARTBEAT_DEGRADE:
            service->status = SM_SERVICE_STATUS_DEGRADED;
        break;

        case SM_SERVICE_EVENT_HEARTBEAT_FAIL:
            ++(service->fail_count);

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

        case SM_SERVICE_EVENT_PROCESS_FAILURE:
            ++(service->fail_count);

            error = sm_service_fsm_set_state( service, 
                                              SM_SERVICE_STATE_DISABLED );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Set state (%s) of service (%s) failed, error=%s.",
                          sm_service_state_str( SM_SERVICE_STATE_DISABLED ),
                          service->name, sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_EVENT_GO_ACTIVE:
            if(( service->fail_count < service->max_failures )&&
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
            } else if( service->fail_count >= service->max_failures ) {
                DPRINTFI( "Service (%s) has reached max failures (%i).",
                          service->name, service->max_failures );

                if( service->disable_action_exists )
                {
                    error = sm_service_fsm_set_state( service,
                                                      SM_SERVICE_STATE_DISABLING );
                    if( SM_OKAY != error )
                    {
                        DPRINTFE( "Set state (%s) of service (%s) failed, error=%s.",
                                  sm_service_state_str(
                                      SM_SERVICE_STATE_DISABLING ),
                                  service->name, sm_error_str( error ) );
                        return( error );
                    }
                }
            }
        break;

        case SM_SERVICE_EVENT_AUDIT:
            error = sm_service_fsm_process_failure_register( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to register for process failure for "
                          "service (%s), error=%s.", service->name,
                          sm_error_str( error ) );
            }

            error = sm_service_heartbeat_api_start_heartbeat( service->name );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to start service (%s) heartbeat, error=%s.",
                          service->name, sm_error_str( error ) );
            }

            error = sm_service_dependency_enabled_standby_state_met( service,
                                                    &state_dependency_met );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to check dependency state for service (%s), "
                          "error=%s", service->name, sm_error_str( error ) );
                return( error );
            }

            if(( state_dependency_met )&&( service->audit_enabled_exists ))
            {
                error = sm_service_audit_enabled( service );
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

        case SM_SERVICE_EVENT_AUDIT_MISMATCH:
            ++(service->fail_count);

            state = *(SmServiceStateT*)
                    event_data[SM_SERVICE_EVENT_DATA_STATE];

            DPRINTFI( "State mismatch for service (%s), expected=%s, "
                      "result=%s.", service->name,
                      sm_service_state_str(SM_SERVICE_STATE_ENABLED_STANDBY),
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
            ++(service->fail_count);

        case SM_SERVICE_EVENT_DISABLE:
            if( service->disable_action_exists )
            {
                error = sm_service_fsm_set_state( service, 
                                                  SM_SERVICE_STATE_DISABLING );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Set state (%s) of service (%s) failed, error=%s.",
                              sm_service_state_str( SM_SERVICE_STATE_DISABLING ),
                              service->name, sm_error_str( error ) );
                    return( error );
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
// Service Enabled Standby State - Initialize
// ==========================================
SmErrorT sm_service_enabled_standby_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Standby State - Finalize
// ========================================
SmErrorT sm_service_enabled_standby_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
