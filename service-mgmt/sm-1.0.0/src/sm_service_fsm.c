//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_fsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_selobj.h"
#include "sm_service_table.h"
#include "sm_service_engine.h"
#include "sm_service_initial_state.h"
#include "sm_service_unknown_state.h"
#include "sm_service_enabled_active_state.h"
#include "sm_service_enabled_go_standby_state.h"
#include "sm_service_enabled_go_active_state.h"
#include "sm_service_enabled_standby_state.h"
#include "sm_service_enabling_state.h"
#include "sm_service_enabling_throttle_state.h"
#include "sm_service_disabling_state.h"
#include "sm_service_disabled_state.h"
#include "sm_service_shutdown_state.h"
#include "sm_service_enable.h"
#include "sm_service_go_active.h"
#include "sm_service_go_standby.h"
#include "sm_service_disable.h"
#include "sm_service_audit.h"
#include "sm_service_heartbeat_api.h"
#include "sm_process_death.h"
#include "sm_log.h"

#define SM_SERVICE_FSM_PID_FILE_AUDIT_IN_MS         2000

static SmServiceHeartbeatCallbacksT _hb_callbacks;
static SmListT* _callbacks = NULL;

static SmErrorT sm_service_get_terminate_reason(SmServiceT* service,
    char reason_text[], int len);

static bool remove_pid_file(char pid_file[]);

// ****************************************************************************
// Service FSM - Register Callback
// ===============================
SmErrorT sm_service_fsm_register_callback( SmServiceFsmCallbackT callback )
{
    SM_LIST_PREPEND( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Deregister Callback
// =================================
SmErrorT sm_service_fsm_deregister_callback( SmServiceFsmCallbackT callback )
{
    SM_LIST_REMOVE( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Notify
// ====================
static void sm_service_fsm_notify( SmServiceT* service )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceFsmCallbackT callback;

    SM_LIST_FOREACH( _callbacks, entry, entry_data )
    {
        callback = (SmServiceFsmCallbackT) entry_data;

        callback( service->name, service->state, service->status,
                  service->condition );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Fail Countdown Timeout
// ====================================
static bool sm_service_fsm_fail_countdown_timeout( SmTimerIdT timer_id,
    int64_t user_data )
{
    int64_t id = user_data;
    SmServiceT* service;

    service = sm_service_table_read_by_id( id );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service, error=%s.", 
                  sm_error_str(SM_NOT_FOUND) );
        return( true );
    }

    if(( 0 < service->fail_count )&&
       ( SM_SERVICE_STATUS_FAILED != service->status ))
    {
        service->fail_count -= service->fail_countdown;
        if( 0 > service->fail_count )
        {
            service->fail_count = 0;

            DPRINTFI( "Fail count for service (%s) reset.",
                      service->name );
        }
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Start Fail Countdown Timer
// ========================================
SmErrorT sm_service_fsm_start_fail_countdown_timer( SmServiceT* service )
{
    char timer_name[80] = "";
    SmTimerIdT fail_countdown_timer_id;
    SmErrorT error;

    DPRINTFD( "Start fail countdown timer for service (%s).", service->name );

    snprintf( timer_name, sizeof(timer_name), "%s fail countdown",
              service->name );

    error = sm_timer_register( timer_name,
                               service->fail_countdown_interval_in_ms,
                               sm_service_fsm_fail_countdown_timeout,
                               service->id, &fail_countdown_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create fail countdown timer for "
                  "service (%s), error=%s.", service->name,
                  sm_error_str( error ) );
        return( error );
    }

    service->fail_countdown_timer_id = fail_countdown_timer_id;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Stop Fail Countdown Timer
// =======================================
SmErrorT sm_service_fsm_stop_fail_countdown_timer( SmServiceT* service )
{
    SmErrorT error;

    DPRINTFD( "Stop fail countdown timer for service (%s).", service->name );

    if( SM_TIMER_ID_INVALID != service->fail_countdown_timer_id )
    {
        error = sm_timer_deregister( service->fail_countdown_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel fail countdown timer for "
                      "service (%s), error=%s.", service->name,
                      sm_error_str( error ) );
        }

        service->fail_countdown_timer_id = SM_TIMER_ID_INVALID;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Enter State
// =========================
static SmErrorT sm_service_fsm_enter_state( SmServiceT* service ) 
{
    SmErrorT error;

    switch( service->state )
    {
        case SM_SERVICE_STATE_INITIAL:
            error = sm_service_initial_state_entry( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to enter state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_UNKNOWN:
            error = sm_service_unknown_state_entry( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to enter state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_ACTIVE:
            error = sm_service_enabled_active_state_entry( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to enter state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_ACTIVE:
            error = sm_service_enabled_go_active_state_entry( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to enter state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_STANDBY:
            error = sm_service_enabled_go_standby_state_entry( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to enter state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_STANDBY:
            error = sm_service_enabled_standby_state_entry( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to enter state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;


        case SM_SERVICE_STATE_ENABLING_THROTTLE:
            error = sm_service_enabling_throttle_state_entry( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to enter state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLING:
            error = sm_service_enabling_state_entry( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to enter state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_DISABLING:
            error = sm_service_disabling_state_entry( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to enter state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_DISABLED:
            error = sm_service_disabled_state_entry( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to enter state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_SHUTDOWN:
            error = sm_service_shutdown_state_entry( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to enter state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown service (%s) state (%s).", service->name,
                      sm_service_state_str( service->state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Exit State
// ========================
static SmErrorT sm_service_fsm_exit_state( SmServiceT* service )
{
    SmErrorT error;

    switch( service->state )
    {
        case SM_SERVICE_STATE_INITIAL:
            error = sm_service_initial_state_exit( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to exit state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_UNKNOWN:
            error = sm_service_unknown_state_exit( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to exit state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_ACTIVE:
            error = sm_service_enabled_active_state_exit( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to exit state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_ACTIVE:
            error = sm_service_enabled_go_active_state_exit( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to exit state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_STANDBY:
            error = sm_service_enabled_go_standby_state_exit( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to exit state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_STANDBY:
            error = sm_service_enabled_standby_state_exit( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to exit state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLING_THROTTLE:
            error = sm_service_enabling_throttle_state_exit( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to exit state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLING:
            error = sm_service_enabling_state_exit( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to exit state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_DISABLING:
            error = sm_service_disabling_state_exit( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to exit state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_DISABLED:
            error = sm_service_disabled_state_exit( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to exit state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_SHUTDOWN:
            error = sm_service_shutdown_state_exit( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to exit state (%s), error=%s.",
                          service->name, sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown service (%s) state (%s).", service->name,
                      sm_service_state_str( service->state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Transition State
// ==============================
static SmErrorT sm_service_fsm_transition_state( SmServiceT* service,
    SmServiceStateT from_state )
{
    SmErrorT error;

    switch( service->state )
    {
        case SM_SERVICE_STATE_INITIAL:
            error = sm_service_initial_state_transition( service, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", service->name,
                          sm_service_state_str( from_state ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_UNKNOWN:
            error = sm_service_unknown_state_transition( service, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", service->name,
                          sm_service_state_str( from_state ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_ACTIVE:
            error = sm_service_enabled_active_state_transition( service,
                                                                from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", service->name,
                          sm_service_state_str( from_state ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_ACTIVE:
            error = sm_service_enabled_go_active_state_transition( service,
                                                                   from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", service->name,
                          sm_service_state_str( from_state ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_STANDBY:
            error = sm_service_enabled_go_standby_state_transition( service,
                                                                    from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", service->name,
                          sm_service_state_str( from_state ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_STANDBY:
            error = sm_service_enabled_standby_state_transition( service,
                                                                 from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", service->name,
                          sm_service_state_str( from_state ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLING_THROTTLE:
            error = sm_service_enabling_throttle_state_transition( service, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", service->name,
                          sm_service_state_str( from_state ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLING:
            error = sm_service_enabling_state_transition( service, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", service->name,
                          sm_service_state_str( from_state ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_DISABLING:
            error = sm_service_disabling_state_transition( service, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", service->name,
                          sm_service_state_str( from_state ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_DISABLED:
            error = sm_service_disabled_state_transition( service, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", service->name,
                          sm_service_state_str( from_state ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_SHUTDOWN:
            error = sm_service_shutdown_state_transition( service, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", service->name,
                          sm_service_state_str( from_state ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown service (%s) state (%s).", service->name,
                      sm_service_state_str( service->state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Set State
// =======================
SmErrorT sm_service_fsm_set_state( SmServiceT* service, SmServiceStateT state ) 
{
    SmServiceStateT prev_state;
    SmErrorT error, error2;

    prev_state = service->state;

    error = sm_service_fsm_exit_state( service ); 
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to exit state (%s) service (%s), error=%s.",
                  sm_service_state_str( service->state ),
                  service->name, sm_error_str( error ) );
        return( error );
    }

    service->state = state;

    error = sm_service_fsm_transition_state( service, prev_state );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to transition to state (%s) service (%s), "
                  "error=%s.", sm_service_state_str( service->state ),
                  service->name, sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }

    error = sm_service_fsm_enter_state( service ); 
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to enter state (%s) service (%s), error=%s.",
                  sm_service_state_str( service->state ),
                  service->name, sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }

    return( SM_OKAY );

STATE_CHANGE_ERROR:
    error2 = sm_service_fsm_exit_state( service ); 
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to exit state (%s) service (%s), error=%s.",
                  sm_service_state_str( service->state ),
                  service->name, sm_error_str( error2 ) );
        abort();
    }

    service->state = prev_state;

    error2 = sm_service_fsm_transition_state( service, state );
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to transition to state (%s) service (%s), "
                  "error=%s.", sm_service_state_str( service->state ),
                  service->name, sm_error_str( error2 ) );
        abort();
    }

    error2 = sm_service_fsm_enter_state( service ); 
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to enter state (%s) service (%s), error=%s.",
                  sm_service_state_str( service->state ),
                  service->name, sm_error_str( error2 ) );
        abort();
    }

    return( error );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Set Status
// ========================
SmErrorT sm_service_fsm_set_status( SmServiceT* service,
    SmServiceStatusT status, SmServiceConditionT condition )
{  
    bool persist = false;

    switch( status )
    {
        case SM_SERVICE_STATUS_NIL:
        case SM_SERVICE_STATUS_UNKNOWN:
            // Ignore.
        break;

        case SM_SERVICE_STATUS_NONE:
            service->status = SM_SERVICE_STATUS_NONE;
            service->condition = SM_SERVICE_CONDITION_NONE;
            persist = true;
        break;

        case SM_SERVICE_STATUS_WARN:
            if(( SM_SERVICE_STATUS_DEGRADED != service->status )&&
               ( SM_SERVICE_STATUS_FAILED   != service->status ))
            {
                service->status = status;

                if(( SM_SERVICE_CONDITION_NIL     != condition )&&
                   ( SM_SERVICE_CONDITION_UNKNOWN != condition ))
                {
                    service->condition = condition;
                } else {
                    service->condition = SM_SERVICE_CONDITION_NONE;
                }
                persist = true;
            }
        break;

        case SM_SERVICE_STATUS_DEGRADED:
            if( SM_SERVICE_STATUS_FAILED != service->status )
            {
                service->status = status;

                if(( SM_SERVICE_CONDITION_NIL     != condition )&&
                   ( SM_SERVICE_CONDITION_UNKNOWN != condition ))
                {
                    service->condition = condition;
                } else {
                    service->condition = SM_SERVICE_CONDITION_NONE;
                }
                persist = true;
            }
        break;

        case SM_SERVICE_STATUS_FAILED:
            service->status = status;

            if(( SM_SERVICE_CONDITION_NIL     != condition )&&
               ( SM_SERVICE_CONDITION_UNKNOWN != condition ))
            {
                service->condition = condition;
            } else {
                service->condition = SM_SERVICE_CONDITION_NONE;
            }
            persist = true;
        break;

        default:
            DPRINTFE( "Uknown status (%i) for service (%s) given.", status,
                      service->name );
        break;
    }

    if( persist )
    {
        SmErrorT error;

        error = sm_service_table_persist( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service (%s) data, error=%s.",
                      service->name, sm_error_str(error) );
            return( error );
        }
    }  

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Event Handler
// ===========================
SmErrorT sm_service_fsm_event_handler( char service_name[],
    SmServiceEventT event, void* event_data[], const char reason_text[] )
{
    SmServiceStateT prev_state;
    SmServiceStatusT prev_status;
    SmServiceConditionT prev_condition;
    SmServiceT* service;
    SmErrorT error;

    service = sm_service_table_read( service_name );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  service_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    prev_state = service->state;
    prev_status = service->status;
    prev_condition = service->condition;

    if( NULL != event_data )
    {
        if( NULL != event_data[SM_SERVICE_EVENT_DATA_IS_ACTION] )
        {
            bool is_action = *(bool*) event_data[SM_SERVICE_EVENT_DATA_IS_ACTION];

            if( is_action )
            {
                SmServiceStatusT action_status;
                SmServiceConditionT action_condition;

                action_status    = *(SmServiceStatusT*)
                                    event_data[SM_SERVICE_EVENT_DATA_STATUS];
                action_condition = *(SmServiceConditionT*)
                                   event_data[SM_SERVICE_EVENT_DATA_CONDITION];

                error = sm_service_fsm_set_status( service, action_status,
                                                   action_condition );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to set status (%s) and condition (%s) "
                              "of service (%s), error=%s.",
                              sm_service_status_str(action_status),
                              sm_service_condition_str(action_condition),
                              service->name, sm_error_str( error ) );
                    return( error );
                }
            }
        }
    }

    switch( service->state )
    {
        case SM_SERVICE_STATE_INITIAL:
            error = sm_service_initial_state_event_handler( service, event,
                                                            event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_name,
                          sm_service_event_str( event ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_UNKNOWN:
            error = sm_service_unknown_state_event_handler( service, event,
                                                            event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_name,
                          sm_service_event_str( event ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_ACTIVE:
            error = sm_service_enabled_active_state_event_handler( service,
                                                            event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_name,
                          sm_service_event_str( event ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_ACTIVE:
            error = sm_service_enabled_go_active_state_event_handler( service,
                                                            event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_name,
                          sm_service_event_str( event ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_STANDBY:
            error = sm_service_enabled_go_standby_state_event_handler( service,
                                                            event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_name,
                          sm_service_event_str( event ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_STANDBY:
            error = sm_service_enabled_standby_state_event_handler( service, 
                                                            event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_name,
                          sm_service_event_str( event ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLING_THROTTLE:
            error = sm_service_enabling_throttle_state_event_handler( service, event,
                                                             event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_name,
                          sm_service_event_str( event ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_ENABLING:
            error = sm_service_enabling_state_event_handler( service, event,
                                                             event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_name,
                          sm_service_event_str( event ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;


        case SM_SERVICE_STATE_DISABLING:
            error = sm_service_disabling_state_event_handler( service, event,
                                                              event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_name,
                          sm_service_event_str( event ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_DISABLED:
            error = sm_service_disabled_state_event_handler( service, event,
                                                             event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_name,
                          sm_service_event_str( event ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_STATE_SHUTDOWN:
            error = sm_service_shutdown_state_event_handler( service, event,
                                                             event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_name,
                          sm_service_event_str( event ),
                          sm_service_state_str( service->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown state (%s) for service (%s).",
                      sm_service_state_str( service->state ),
                      service_name );
        break;
    }

    if(( prev_state != service->state )||( prev_status != service->status )||
       ( prev_condition != service->condition ))
    {
        DPRINTFI( "Service (%s) received event (%s) was in the %s%s%s state "
                  "and is now in the %s%s%s state%s%s.",
                  service_name,
                  sm_service_event_str( event ),
                  sm_service_state_str( prev_state ),
                  SM_SERVICE_STATUS_NONE == prev_status 
                  ? "" : "-",
                  SM_SERVICE_STATUS_NONE == prev_status 
                  ? "" : sm_service_status_str( prev_status ),
                  sm_service_state_str( service->state ),
                  SM_SERVICE_STATUS_NONE == service->status 
                  ? "" : "-",
                  SM_SERVICE_STATUS_NONE == service->status 
                  ? "" : sm_service_status_str( service->status ),
                  SM_SERVICE_CONDITION_NONE == service->condition
                  ? "" : ", condition=",
                  SM_SERVICE_CONDITION_NONE == service->condition
                  ? "" : sm_service_condition_str( service->condition ) );

        error = sm_service_table_persist( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service (%s) data, error=%s.",
                      service->name, sm_error_str(error) );
            return( error );
        }
       
        if(( prev_state != service->state )||( prev_status != service->status ))
        {
            sm_log_service_state_change( service_name,
                                     sm_service_state_str( prev_state ),
                                     sm_service_status_str( prev_status ),
                                     sm_service_state_str( service->state ),
                                     sm_service_status_str( service->status ),
                                     reason_text );
        }

        sm_service_fsm_notify( service );

        if( prev_state != service->state )
        {
            error = sm_service_engine_signal( service );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to signal service (%s), error=%s.",
                          service_name, sm_error_str( error ) );
                return( error );
            }
        }
    } else if( SM_SERVICE_EVENT_AUDIT == event ) {
        if(( SM_SERVICE_STATE_ENABLED_ACTIVE  == service->state )||
           ( SM_SERVICE_STATE_ENABLED_STANDBY == service->state )||
           ( SM_SERVICE_STATE_DISABLED        == service->state ))
        {
            sm_service_fsm_notify( service );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Action Complete Handler
// =====================================
SmErrorT sm_service_fsm_action_complete_handler( char service_name[],
    SmServiceActionT action, SmServiceActionResultT action_result,
    SmServiceStateT state, SmServiceStatusT status,
    SmServiceConditionT condition, const char reason_text[] )
{
    bool is_action = true;
    SmServiceT* service;
    SmServiceEventT event = SM_SERVICE_EVENT_UNKNOWN;
    void* event_data[SM_SERVICE_EVENT_DATA_MAX] = {0};
    char event_reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    service = sm_service_table_read( service_name );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  service_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    event_data[SM_SERVICE_EVENT_DATA_IS_ACTION] = &is_action;
    event_data[SM_SERVICE_EVENT_DATA_STATE] = &state;
    event_data[SM_SERVICE_EVENT_DATA_STATUS] = &status;
    event_data[SM_SERVICE_EVENT_DATA_CONDITION] = &condition;

    switch( action )
    {
        case SM_SERVICE_ACTION_ENABLE:
            switch( action_result )
            {
                case SM_SERVICE_ACTION_RESULT_SUCCESS:
                    event = SM_SERVICE_EVENT_ENABLE_SUCCESS;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "enable success" );
                break;

                case SM_SERVICE_ACTION_RESULT_FATAL:
                    event = SM_SERVICE_EVENT_SHUTDOWN;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "enable fatal, shutting down" );
                break;

                case SM_SERVICE_ACTION_RESULT_FAILED:
                    event = SM_SERVICE_EVENT_ENABLE_FAILED;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "enable failed" );
                break;

                case SM_SERVICE_ACTION_RESULT_TIMEOUT:
                    event = SM_SERVICE_EVENT_ENABLE_TIMEOUT;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "enable timeout" );
                break;

                default:
                    DPRINTFE( "Unknown action result (%s) for action (%s) of "
                              "service (%s).",
                              sm_service_action_result_str( action_result ),
                              sm_service_action_str( action ), service_name );
                break;
            }
        break;

        case SM_SERVICE_ACTION_GO_ACTIVE:
            switch( action_result )
            {
                case SM_SERVICE_ACTION_RESULT_SUCCESS:
                    event = SM_SERVICE_EVENT_GO_ACTIVE_SUCCESS;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "go-active success" );
                break;

                case SM_SERVICE_ACTION_RESULT_FATAL:
                    event = SM_SERVICE_EVENT_SHUTDOWN;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "go-active fatal, shutting down" );
                break;

                case SM_SERVICE_ACTION_RESULT_FAILED:
                    event = SM_SERVICE_EVENT_GO_ACTIVE_FAILED;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "go-active failed" );
                break;

                case SM_SERVICE_ACTION_RESULT_TIMEOUT:
                    event = SM_SERVICE_EVENT_GO_ACTIVE_TIMEOUT;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "go-active timeout" );
                break;

                default:
                    DPRINTFE( "Unknown action result (%s) for action (%s) of "
                              "service (%s).",
                              sm_service_action_result_str( action_result ),
                              sm_service_action_str( action ), service_name );
                break;
            }
        break;

        case SM_SERVICE_ACTION_GO_STANDBY:
            switch( action_result )
            {
                case SM_SERVICE_ACTION_RESULT_SUCCESS:
                    event = SM_SERVICE_EVENT_GO_STANDBY_SUCCESS;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "go-standby success" );
                break;

                case SM_SERVICE_ACTION_RESULT_FATAL:
                    event = SM_SERVICE_EVENT_SHUTDOWN;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "go-standy fatal, shutting down" );
                break;

                case SM_SERVICE_ACTION_RESULT_FAILED:
                    event = SM_SERVICE_EVENT_GO_STANDBY_FAILED;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "go-standby failed" );
                break;

                case SM_SERVICE_ACTION_RESULT_TIMEOUT:
                    event = SM_SERVICE_EVENT_GO_STANDBY_TIMEOUT;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "go-standby timeout" );
                break;

                default:
                    DPRINTFE( "Unknown action result (%s) for action (%s) of "
                              "service (%s).",
                              sm_service_action_result_str( action_result ),
                              sm_service_action_str( action ), service_name );
                break;
            }
        break;

        case SM_SERVICE_ACTION_DISABLE:
            switch( action_result )
            {
                case SM_SERVICE_ACTION_RESULT_SUCCESS:
                    event = SM_SERVICE_EVENT_DISABLE_SUCCESS;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "disable success" );
                break;

                case SM_SERVICE_ACTION_RESULT_FATAL:
                    event = SM_SERVICE_EVENT_SHUTDOWN;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "disable fatal, shutting down" );
                break;

                case SM_SERVICE_ACTION_RESULT_FAILED:
                    event = SM_SERVICE_EVENT_DISABLE_FAILED;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "disable failed" );
                break;

                case SM_SERVICE_ACTION_RESULT_TIMEOUT:
                    event = SM_SERVICE_EVENT_DISABLE_TIMEOUT;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "disable timeout" );
                break;

                default:
                    DPRINTFE( "Unknown action result (%s) for action (%s) of "
                              "service (%s).", 
                              sm_service_action_result_str( action_result ),
                              sm_service_action_str( action ), service_name );
                break;
            }
        break;

        case SM_SERVICE_ACTION_AUDIT_ENABLED:
        case SM_SERVICE_ACTION_AUDIT_DISABLED:
            switch( action_result )
            {
                case SM_SERVICE_ACTION_RESULT_SUCCESS:
                    switch( service->state )
                    {
                        case SM_SERVICE_STATE_INITIAL:
                        case SM_SERVICE_STATE_UNKNOWN:
                           event = SM_SERVICE_EVENT_AUDIT_SUCCESS;
                           snprintf( event_reason_text,
                                     sizeof(event_reason_text), 
                                     "audit success" );
                        break;

                        case SM_SERVICE_STATE_ENABLED_GO_STANDBY:
                            if( SM_SERVICE_STATE_ENABLED_ACTIVE == state )
                            {
                                event = SM_SERVICE_EVENT_AUDIT_SUCCESS;
                                snprintf( event_reason_text,
                                          sizeof(event_reason_text), 
                                          "audit success" );
                            } else {
                                event = SM_SERVICE_EVENT_AUDIT_MISMATCH;
                                snprintf( event_reason_text,
                                          sizeof(event_reason_text), 
                                          "audit state mismatch" );
                            }
                        break;

                        default:
                            if( state == service->state )
                            {
                                event = SM_SERVICE_EVENT_AUDIT_SUCCESS;
                                snprintf( event_reason_text,
                                          sizeof(event_reason_text), 
                                          "audit success" );
                            } else {
                                event = SM_SERVICE_EVENT_AUDIT_MISMATCH;
                                snprintf( event_reason_text, 
                                          sizeof(event_reason_text), 
                                          "audit state mismatch" );
                            }
                        break;
                    }
                break;

                case SM_SERVICE_ACTION_RESULT_FATAL:
                    event = SM_SERVICE_EVENT_SHUTDOWN;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "audit fatal, shutting down" );
                break;

                case SM_SERVICE_ACTION_RESULT_FAILED:
                    event = SM_SERVICE_EVENT_AUDIT_FAILED;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "audit failed" );
                break;

                case SM_SERVICE_ACTION_RESULT_TIMEOUT:
                    event = SM_SERVICE_EVENT_AUDIT_TIMEOUT;
                    snprintf( event_reason_text, sizeof(event_reason_text), 
                              "audit timeout" );
                break;

                default:
                    DPRINTFE( "Unknown action result (%s) for action (%s) of "
                              "service (%s).", 
                              sm_service_action_result_str( action_result ),
                              sm_service_action_str( action ), service_name );
                break;
            }
        break;

        default:
            DPRINTFE( "Unknown action (%s) for service (%s).",
                      sm_service_action_str( action ), service_name );
        break;
    }

    if( SM_SERVICE_EVENT_UNKNOWN != event )
    {
        error = sm_service_fsm_event_handler( service_name, event,
                                              event_data, event_reason_text );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Event (%s) not handled for service (%s).",
                      sm_service_event_str( event ), service_name );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Heartbeat Okay Callback
// ========================================
static void sm_service_fsm_heartbeat_okay_callback( char service_name[] )
{
    SmServiceEventT event = SM_SERVICE_EVENT_HEARTBEAT_OKAY;
    SmErrorT error;

    DPRINTFI( "Service (%s) heartbeat okay.", service_name );

    error = sm_service_fsm_event_handler( service_name, event, NULL,
                                          "heartbeat okay" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Event (%s) not handled for service (%s).",
                  sm_service_event_str( event ), service_name );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Heartbeat Warning Callback
// ========================================
static void sm_service_fsm_heartbeat_warn_callback( char service_name[] )
{
    SmServiceEventT event = SM_SERVICE_EVENT_HEARTBEAT_WARN;
    SmErrorT error;

    DPRINTFI( "Service (%s) heartbeat warning.", service_name );

    error = sm_service_fsm_event_handler( service_name, event, NULL,
                                          "heartbeat warning threshold crossed" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Event (%s) not handled for service (%s).",
                  sm_service_event_str( event ), service_name );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Heartbeat Degrade Callback
// ========================================
static void sm_service_fsm_heartbeat_degrade_callback( char service_name[] )
{
    SmServiceEventT event = SM_SERVICE_EVENT_HEARTBEAT_DEGRADE;
    SmErrorT error;

    DPRINTFI( "Service (%s) heartbeat degrade.", service_name );

    error = sm_service_fsm_event_handler( service_name, event, NULL,
                                          "heartbeat degrade threshold crossed" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Event (%s) not handled for service (%s).",
                  sm_service_event_str( event ), service_name );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Heartbeat Failure Callback
// ========================================
static void sm_service_fsm_heartbeat_fail_callback( char service_name[] )
{
    SmServiceEventT event = SM_SERVICE_EVENT_HEARTBEAT_FAIL;
    SmErrorT error;

    DPRINTFI( "Service (%s) heartbeat failure.", service_name );

    error = sm_service_fsm_event_handler( service_name, event, NULL,
                                          "heartbeat failure" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Event (%s) not handled for service (%s).",
                  sm_service_event_str( event ), service_name );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Process Failure Callback
// ======================================
static void sm_service_fsm_process_failure_callback( pid_t pid, int exit_code,
    int64_t user_data )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceT* service;    
    SmServiceEventT event = SM_SERVICE_EVENT_PROCESS_FAILURE;
    SmErrorT error;

    service = sm_service_table_read_by_pid( (int) pid );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service based on pid (%"PRIu64"), error=%s.",
                  user_data, sm_error_str(SM_NOT_FOUND) );
        return;
    }
    
    DPRINTFI( "Service (%s) process failure, pid=%i, exit_code=%i.",
              service->name, (int) pid, exit_code );

    sm_service_get_terminate_reason(service, reason_text, sizeof(reason_text));

    error = sm_service_fsm_event_handler( service->name, event, NULL,
                                          reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Event (%s) not handled for service (%s).",
                  sm_service_event_str( event ), service->name );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Process Audit PID File Timer
// ==========================================
static bool sm_service_fsm_process_audit_pid_file_timer( SmTimerIdT timer_id,
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
        goto EXIT;
    }

    service->pid_file_audit_timer_id = SM_TIMER_ID_INVALID;

    error = sm_service_fsm_process_failure_register( service );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for process failure for "
                  "service (%s), error=%s.", service->name,
                  sm_error_str( error ) );
        goto EXIT;
    }

EXIT:
    return( false );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Process Failure Register
// ======================================
SmErrorT sm_service_fsm_process_failure_register( SmServiceT* service )
{
    int pid;
    FILE* pid_file;
    bool audit_pid_file = false;
    char file_line[80] = "\0";
    SmErrorT error;

    if( '\0' != service->pid_file[0] )
    {
        pid_file = fopen ( service->pid_file, "r" );

        if( NULL == pid_file )
        {
            DPRINTFE( "Failed to open pid file (%s) for service (%s).",
                      service->pid_file, service->name );
            audit_pid_file = true;
            goto EXIT;
        }

        if( NULL == fgets( file_line, sizeof(file_line), pid_file ) )
        {
            DPRINTFE( "Failed to read pid from pid file (%s) for "
                      "service (%s).", service->pid_file, service->name );
            fclose( pid_file );

            remove_pid_file(service->pid_file);
            audit_pid_file = true;
            goto EXIT;
        }

        fclose( pid_file );

        if( 1 != sscanf( file_line, "%d", &pid ) )
        {
            DPRINTFE( "pid file (%s) for (%s) is invalid. content (%s).",
                      service->pid_file, service->name, file_line );

            remove_pid_file( service->pid_file );
            audit_pid_file = true;
            goto EXIT;
        }

        if( 0 > pid )
        {
            DPRINTFE( "Invalid pid (%i) converted from line (%s) from file "
                      "(%s) for service (%s).", pid, file_line,
                      service->pid_file, service->name );
            audit_pid_file = true;
            goto EXIT;
        }

        if ( 0 != kill(pid, 0) )
        {
            DPRINTFE( "pid (%i) from file (%s) for service (%s)"
                      " is no longer valid.", pid,
                      service->pid_file, service->name );
            remove_pid_file( service->pid_file );
            service->pid = -1;

            if( !sm_process_death_already_registered( pid,
                    sm_service_fsm_process_failure_callback ) )
            {
                sm_service_fsm_process_failure_deregister(service);
            }

            service->disable_check_dependency = false;
            service->disable_skip_dependent = true;
            SmServiceEventT event = SM_SERVICE_EVENT_DISABLE;
            char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR];
            snprintf(reason_text, sizeof(reason_text),
                "Service (%s) process failure, pid=%i, exit_code unknown",
                service->name,
                (int)pid);

            error = sm_service_fsm_event_handler( service->name, event, NULL,
                                                  reason_text );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Event (%s) not handled for service (%s).",
                          sm_service_event_str( event ), service->name );
            }
            goto EXIT;
        }

        service->pid = pid;

        if( sm_process_death_already_registered( pid,
                    sm_service_fsm_process_failure_callback ) )
        {
            DPRINTFD( "Already registered for process failure callback "
                      "for service (%s), pid=%i.", service->name, pid );
            goto EXIT;
        }

        error = sm_process_death_register( pid, false, 
                        sm_service_fsm_process_failure_callback, 
                        service->id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to register for process failure callback "
                      "for service (%s), pid=%i, error=%s.", service->name,
                      pid, sm_error_str( error ) );
            goto EXIT;
        }
    }

EXIT:
    if( audit_pid_file )
    {
        // Some services do not write their pid file right away when they
        // enable, so register an audit to attempt to register the pid again.
        char timer_name[80] = "";
        SmTimerIdT audit_timer_id;

        if( SM_TIMER_ID_INVALID != service->pid_file_audit_timer_id )
        {
            error = sm_timer_deregister( service->pid_file_audit_timer_id );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to cancel pid file audit timer, error=%s.",
                          sm_error_str( error ) );
            }

            service->pid_file_audit_timer_id = SM_TIMER_ID_INVALID;
        }

        snprintf( timer_name, sizeof(timer_name), "%s audit pid file",
                  service->name );

        error = sm_timer_register( timer_name,
                                   SM_SERVICE_FSM_PID_FILE_AUDIT_IN_MS,
                                   sm_service_fsm_process_audit_pid_file_timer,
                                   service->id, &audit_timer_id );
        if( SM_OKAY == error )
        {
            service->pid_file_audit_timer_id = audit_timer_id;

        } else {
            DPRINTFE( "Failed to create pid file audit timer for "
                      "service (%s), error=%s.", service->name,
                      sm_error_str( error ) );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Process Failure Deregister
// ========================================
SmErrorT sm_service_fsm_process_failure_deregister( SmServiceT* service )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != service->pid_file_audit_timer_id )
    {
        error = sm_timer_deregister( service->pid_file_audit_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel pid file audit timer, error=%s.",
                      sm_error_str( error ) );
        }

        service->pid_file_audit_timer_id = SM_TIMER_ID_INVALID;
    }

    if( 0 <= service->pid )
    {
        error = sm_process_death_deregister( service->pid );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister for process failure callback "
                      "for service (%s), pid=%i, error=%s.", service->name,
                      service->pid, sm_error_str( error ) );
        }

        service->pid = -1;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Initialize
// ========================
SmErrorT sm_service_fsm_initialize( void )
{
    SmErrorT error;

    memset( &_hb_callbacks, 0, sizeof(_hb_callbacks) );

    error = sm_service_initial_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service initial state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_unknown_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service unknown state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_enabled_active_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service enabled active state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_enabled_go_active_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service enabled go-active state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_enabled_go_standby_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service enabled go-standby state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_enabled_standby_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service enabled standby state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_enabling_throttle_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service enabling throttle state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_enabling_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service enabling state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_disabling_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service disabling state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_disabled_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service disabled state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_shutdown_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service shutdown state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_enable_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service enable module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_go_active_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service go-active module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_go_standby_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service go-standby module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_disable_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service disable module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_audit_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service audit module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    _hb_callbacks.okay_callback = sm_service_fsm_heartbeat_okay_callback;
    _hb_callbacks.warn_callback = sm_service_fsm_heartbeat_warn_callback;
    _hb_callbacks.degrade_callback = sm_service_fsm_heartbeat_degrade_callback;
    _hb_callbacks.fail_callback = sm_service_fsm_heartbeat_fail_callback;

    error = sm_service_heartbeat_api_register_callbacks( &_hb_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register service heartbeat callbacks, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - Finalize
// ======================
SmErrorT sm_service_fsm_finalize( void )
{
    SmErrorT error;

    error = sm_service_heartbeat_api_deregister_callbacks( &_hb_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister service heartbeat callbacks, "
                  "error=%s.", sm_error_str( error ) );
    }

    memset( &_hb_callbacks, 0, sizeof(_hb_callbacks) );

    error = sm_service_initial_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service initial state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_unknown_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service unknown state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_enabled_active_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service enabled active state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_enabled_go_active_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service enabled go-active state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_enabled_go_standby_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service enabled go-standby state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_enabled_standby_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service enabled standby state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_enabling_throttle_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service enabling throttle state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_enabling_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service enabling state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_disabling_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service disabling state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_disabled_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service disabled state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_shutdown_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service shutdown state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_enable_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service enable module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_go_active_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service go-active module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_go_standby_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service go-standby module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_disable_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service disable module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_audit_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service audit module, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service FSM - get service terminate reason
// ========================================
static SmErrorT sm_service_get_terminate_reason(SmServiceT* service,
    char reason_text[], int len)
{
    switch( service->state )
    {
        case SM_SERVICE_STATE_DISABLING:
        case SM_SERVICE_STATE_DISABLED:
        case SM_SERVICE_STATE_SHUTDOWN:
            snprintf( reason_text, len, "process (pid=%i) terminated",
                (int) service->pid );
            break;
        default:
            snprintf( reason_text, len, "process (pid=%i) failed",
                (int) service->pid );
            break;
    }

    return SM_OKAY;
}
// ****************************************************************************


// ****************************************************************************
// util function for deleting an invalid pid file
// ========================================
static bool remove_pid_file(char pid_file[])
{
    if ( 0 == remove( pid_file ) )
    {
        DPRINTFI( "pid file (%s) is deleted. ", pid_file );
        return true;
    }else
    {
        DPRINTFE( "Failed to delete pid file (%s). ", pid_file );
        return false;
    }
}
// ****************************************************************************