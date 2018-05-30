//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_engine.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/eventfd.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_selobj.h"
#include "sm_time.h"
#include "sm_timer.h"
#include "sm_service_table.h"
#include "sm_service_dependency.h"
#include "sm_service_fsm.h"
#include "sm_service_go_active.h"
#include "sm_service_go_standby.h"

#define SM_SERVICE_ENGINE_TIMER_IN_MS                               500
#define SM_SERVICE_ENGINE_MIN_TIME_BETWEEN_SIGNALS_IN_MS            100

static int _engine_fd = -1;
static SmTimerIdT _engine_timer_id = SM_TIMER_ID_INVALID;

// ****************************************************************************
// Service Engine - Timeout
// ========================
static bool sm_service_engine_timeout( SmTimerIdT timer_id, int64_t user_data )
{
    uint64_t count = 1;

    if( 0 > write( _engine_fd, &count, sizeof(count) ) )
    {
        DPRINTFE( "Failed to signal services, error=%s", strerror( errno ) );
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Engine - Signal
// =======================
SmErrorT sm_service_engine_signal( SmServiceT* service )
{
    static SmTimeT time_last_signal = {0};

    long ms_expired;
    SmTimeT time_now;

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_last_signal );

    if( SM_SERVICE_ENGINE_MIN_TIME_BETWEEN_SIGNALS_IN_MS > ms_expired )
    {
        uint64_t count = 1;

        if( 0 > write( _engine_fd, &count, sizeof(count) ) )
        {
            DPRINTFE( "Failed to signal service (%s), error=%s",
                      service->name, strerror( errno ) );
            return( SM_FAILED );
        }

        time_last_signal = time_now;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Engine - Signal Handler
// ===============================
static void sm_service_engine_signal_handler( void* user_data[],
    SmServiceT* service )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    bool disable_required = false;
    bool state_dependency_met;
    SmServiceStateT desired_state;
    SmServiceEventT event = SM_SERVICE_EVENT_UNKNOWN;
    SmErrorT error;

    if ( service->disable_skip_dependent )
    {
        DPRINTFI( "service %s (state: %s, desired state: %s) to check dependency require disable, skipped",
            service->name, sm_service_state_str(service->state), sm_service_state_str(service->desired_state));
    }else
    {
        // DPRINTFI( "service %s to check dependency require disable", service->name);
        error = sm_service_dependency_dependents_require_disable(
            service,
            &disable_required
        );

        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to determine if dependents require disable for "
                      "service (%s), error=%s.", service->name,
                      sm_error_str( error ) );
            return;
        }
    }

    if( disable_required )
    {
        DPRINTFD( "Dependents require disable of service (%s).",
              service->name );
        desired_state = SM_SERVICE_STATE_DISABLED;
    } else if( SM_SERVICE_STATE_ENABLED_STANDBY == service->desired_state )
    {
        if( service->go_standby_action_exists )
        {
            desired_state = service->desired_state;
        } else {
            DPRINTFD( "No go-standby action exists for service (%s), "
                      "remain disabled.", service->name );
            desired_state = SM_SERVICE_STATE_DISABLED;
        }
    } else {
        desired_state = service->desired_state;
    }

    if(( service->clear_fatal_condition )&&
       ( SM_SERVICE_STATUS_FAILED == service->status )&&
       ( SM_SERVICE_CONDITION_FATAL_FAILURE == service->condition ))
    {
        service->clear_fatal_condition = false;
        service->transition_fail_count = 0;
        service->condition = SM_SERVICE_CONDITION_NONE;

        error = sm_service_table_persist( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service (%s) data, error=%s.",
                      service->name, sm_error_str(error) );
            return;
        }

        DPRINTFI( "Fail counts and status for service (%s) cleared "
                  "because clear-fatal-condition indicated.", service->name );
    } else {
        service->clear_fatal_condition = false;

        if(( SM_SERVICE_STATUS_FAILED == service->status )&&
           ( SM_SERVICE_CONDITION_FATAL_FAILURE == service->condition ))
        {
            DPRINTFI( "Service (%s) has had a fatal failure and is "
                      "unrecoverable.", service->name );
            return;
        }
    }

    switch( service->state )
    {
        case SM_SERVICE_STATE_INITIAL:
            if( SM_SERVICE_STATE_INITIAL != desired_state )
            {
                event = SM_SERVICE_EVENT_AUDIT;
                snprintf( reason_text, sizeof(reason_text), 
                          "audit requested" );
            }
        break;

        case SM_SERVICE_STATE_UNKNOWN:
            event = SM_SERVICE_EVENT_AUDIT;
            snprintf( reason_text, sizeof(reason_text), 
                      "audit requested, state is unknown" );
        break;

        case SM_SERVICE_STATE_ENABLED_ACTIVE:
            error = sm_service_dependency_enabled_active_state_met( service,
                                                    &state_dependency_met );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to check dependency state for service (%s), "
                          "error=%s", service->name, sm_error_str( error ) );
                return;
            }
            
            if( !state_dependency_met )
            {
                event = SM_SERVICE_EVENT_DISABLE;
                snprintf( reason_text, sizeof(reason_text), 
                          "enabled-active state dependency was not met" );
            }
            else if( SM_SERVICE_STATE_ENABLED_STANDBY == desired_state )
            {
                event = SM_SERVICE_EVENT_GO_STANDBY;
                snprintf( reason_text, sizeof(reason_text), 
                          "enabled-standby state requested" );
            } 
            else if( SM_SERVICE_STATE_DISABLED == desired_state ) 
            {
                event = SM_SERVICE_EVENT_DISABLE;
                snprintf( reason_text, sizeof(reason_text), 
                          "disable state requested" );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_ACTIVE:
            if( SM_SERVICE_STATE_ENABLED_ACTIVE == desired_state )
            {
                event = SM_SERVICE_EVENT_GO_ACTIVE;
                snprintf( reason_text, sizeof(reason_text), 
                          "enabled-active state requested" );
            } else {
                event = SM_SERVICE_EVENT_GO_STANDBY;
                snprintf( reason_text, sizeof(reason_text), 
                          "enabled-stanby state requested" );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_STANDBY:
            if(( service->recover )&&
               ( SM_SERVICE_STATUS_FAILED == service->status )&&
               ( SM_SERVICE_CONDITION_ACTION_FAILURE == service->condition ))
            {
                service->recover = false;
                service->action_fail_count = 0;
                service->condition = SM_SERVICE_CONDITION_NONE;

                error = sm_service_table_persist( service );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to persist service (%s) data, error=%s.",
                              service->name, sm_error_str(error) );
                    return;
                }

                DPRINTFI( "Fail counts and status for service (%s) cleared "
                          "in enabled-go-standby state.", service->name );

                if( SM_SERVICE_STATE_ENABLED_ACTIVE == desired_state )
                {
                    event = SM_SERVICE_EVENT_GO_ACTIVE;
                    snprintf( reason_text, sizeof(reason_text), 
                              "enabled-active state requested" );
                } else {
                    service->recover = false;
                    event = SM_SERVICE_EVENT_GO_STANDBY;
                    snprintf( reason_text, sizeof(reason_text),
                              "enabled-standby state requested" );
                }
            } else {
                service->recover = false;
                event = SM_SERVICE_EVENT_GO_STANDBY;
                snprintf( reason_text, sizeof(reason_text),
                          "enabled-standby state requested" );
            }
        break;

        case SM_SERVICE_STATE_ENABLED_STANDBY:
            error = sm_service_dependency_enabled_standby_state_met( service,
                                                    &state_dependency_met );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to check dependency state for service (%s), "
                          "error=%s", service->name, sm_error_str( error ) );
                return;
            }

            if( !state_dependency_met )
            {
                event = SM_SERVICE_EVENT_DISABLE;
                snprintf( reason_text, sizeof(reason_text), 
                          "enabled-standby state dependency was not met" );
            }
            else if( SM_SERVICE_STATE_ENABLED_ACTIVE == desired_state )
            {
                event = SM_SERVICE_EVENT_GO_ACTIVE;
                snprintf( reason_text, sizeof(reason_text), 
                          "enabled-active state requested" );
            }
            else if( SM_SERVICE_STATE_DISABLED == desired_state ) 
            {
                event = SM_SERVICE_EVENT_DISABLE;
                snprintf( reason_text, sizeof(reason_text), 
                          "disabled state requested" );
            }
        break;

        case SM_SERVICE_STATE_ENABLING_THROTTLE:
            if( SM_SERVICE_STATE_ENABLED_ACTIVE  == desired_state )
            {
                event = SM_SERVICE_EVENT_ENABLE_THROTTLE;
                snprintf( reason_text, sizeof(reason_text),
                          "enabled-active state requested" );
            }
            else if( SM_SERVICE_STATE_ENABLED_STANDBY == desired_state )
            {
                event = SM_SERVICE_EVENT_ENABLE_THROTTLE;
                snprintf( reason_text, sizeof(reason_text),
                          "enabled-standby state requested" );
            } else {
                event = SM_SERVICE_EVENT_DISABLE;
                snprintf( reason_text, sizeof(reason_text),
                          "disabled state requested" );
            }
        break;

        case SM_SERVICE_STATE_ENABLING:
            if( SM_SERVICE_STATE_ENABLED_ACTIVE  == desired_state )
            {
                event = SM_SERVICE_EVENT_ENABLE;
                snprintf( reason_text, sizeof(reason_text), 
                          "enabled-active state requested" );
            } 
            else if( SM_SERVICE_STATE_ENABLED_STANDBY == desired_state ) 
            {
                event = SM_SERVICE_EVENT_ENABLE;
                snprintf( reason_text, sizeof(reason_text), 
                          "enabled-standby state requested" );
            } else {
                event = SM_SERVICE_EVENT_DISABLE;
                snprintf( reason_text, sizeof(reason_text), 
                          "disabled state requested" );
            }
        break;

        case SM_SERVICE_STATE_DISABLING:
            if(( service->recover )&&
               ( SM_SERVICE_STATUS_FAILED == service->status )&&
               ( SM_SERVICE_CONDITION_ACTION_FAILURE == service->condition ))
            {
                service->recover = false;
                service->action_fail_count = 0;
                service->condition = SM_SERVICE_CONDITION_NONE;

                error = sm_service_table_persist( service );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to persist service (%s) data, error=%s.",
                              service->name, sm_error_str(error) );
                    return;
                }

                DPRINTFI( "Fail counts and status for service (%s) cleared "
                          "in disabling state.", service->name );

                if( SM_SERVICE_STATE_ENABLED_ACTIVE == desired_state )
                {
                    event = SM_SERVICE_EVENT_ENABLE_THROTTLE;
                    snprintf( reason_text, sizeof(reason_text), 
                              "enabled-active state requested" );
                }
                else if(( SM_SERVICE_STATE_ENABLED_STANDBY == desired_state )&&
                        ( service->go_active_action_exists ))
                {
                    event = SM_SERVICE_EVENT_ENABLE_THROTTLE;
                    snprintf( reason_text, sizeof(reason_text),
                              "enabled-standby state requested" );
                } else {
                    service->recover = false;
                    event = SM_SERVICE_EVENT_DISABLE;
                    snprintf( reason_text, sizeof(reason_text), "disabling" );
                }
            } else {
                service->recover = false;
                event = SM_SERVICE_EVENT_DISABLE;
                snprintf( reason_text, sizeof(reason_text), "disabling" );
            }
        break;

        case SM_SERVICE_STATE_DISABLED:
            if( service->recover )
            {
                service->recover = false;
                service->fail_count = 0;
                service->action_fail_count = 0;
                service->transition_fail_count = 0;
                service->condition = SM_SERVICE_CONDITION_NONE;

                error = sm_service_table_persist( service );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to persist service (%s) data, error=%s.",
                              service->name, sm_error_str(error) );
                    return;
                }

                DPRINTFI( "Fail counts and status for service (%s) cleared "
                          "in disabled state.", service->name );
            }

            if( SM_SERVICE_STATE_ENABLED_ACTIVE == desired_state )
            {
                event = SM_SERVICE_EVENT_ENABLE_THROTTLE;
                snprintf( reason_text, sizeof(reason_text),
                          "enabled-active state requested" );
            }
            else if(( SM_SERVICE_STATE_ENABLED_STANDBY == desired_state )&&
                    ( service->go_active_action_exists ))
            {
                event = SM_SERVICE_EVENT_ENABLE_THROTTLE;
                snprintf( reason_text, sizeof(reason_text),
                          "enabled-standby state requested" );

            } else if( SM_SERVICE_STATE_DISABLED == desired_state ) {
                service->recover = false;
                service->fail_count = 0;
                service->action_fail_count = 0;
                service->transition_fail_count = 0;
                service->status = SM_SERVICE_STATUS_NONE;
                service->condition = SM_SERVICE_CONDITION_NONE;

                error = sm_service_table_persist( service );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to persist service (%s) data, error=%s.",
                              service->name, sm_error_str(error) );
                    return;
                }
            }
        break;

        default:
            DPRINTFE( "Unknown service (%s) state (%s).", service->name,
                      sm_service_state_str( service->state ) );
        break;
    }

    if( SM_SERVICE_EVENT_UNKNOWN != event )
    {
        DPRINTFD( "Sending event (%s) to service (%s).",
                  sm_service_event_str( event ), service->name );
                  
        error = sm_service_fsm_event_handler( service->name, event, NULL,
                                              reason_text );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Event (%s) not handled for service (%s).",
                      sm_service_event_str( event ), service->name );
        }
    }

    return;
}
// ****************************************************************************

// ****************************************************************************
// Service Engine - Dispatch
// =========================
static void sm_service_engine_dispatch( int selobj, int64_t user_data )
{
    uint64_t count;

    read( _engine_fd, &count, sizeof(count) );

    sm_service_table_foreach( NULL, sm_service_engine_signal_handler );
}
// ****************************************************************************

// ****************************************************************************
// Service Engine - Initialize
// ===========================
SmErrorT sm_service_engine_initialize( void )
{
    SmErrorT error;

    _engine_fd = eventfd( 0, EFD_CLOEXEC | EFD_NONBLOCK  );
    if( 0 > _engine_fd )
    {
        DPRINTFE( "Failed to open file descriptor,error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    error = sm_selobj_register( _engine_fd, sm_service_engine_dispatch, 0 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register selection object, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_timer_register( "service engine",
                               SM_SERVICE_ENGINE_TIMER_IN_MS,
                               sm_service_engine_timeout, 0,
                               &_engine_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create engine timer, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Engine - Finalize
// =========================
SmErrorT sm_service_engine_finalize( void )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != _engine_timer_id )
    {
        error = sm_timer_deregister( _engine_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel engine timer, error=%s.",
                      sm_error_str( error ) );
        }
        _engine_timer_id = SM_TIMER_ID_INVALID;
    }

    if( 0 <= _engine_fd )
    {
        error = sm_selobj_deregister( _engine_fd );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object, error=%s.",
                      sm_error_str( error ) );
        }

        close( _engine_fd );
    }

    return( SM_OKAY );
}
// ****************************************************************************
