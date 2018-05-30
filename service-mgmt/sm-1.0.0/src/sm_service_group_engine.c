//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_engine.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/eventfd.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_selobj.h"
#include "sm_timer.h"
#include "sm_service_group_fsm.h"

#define SM_SERVICE_GROUP_ENGINE_AUDIT_TIMER_IN_MS            30000
#define SM_SERVICE_GROUP_ENGINE_TIMER_IN_MS                   1250
#define SM_SERVICE_GROUP_ENGINE_THROTTLE_SIGNALS                 0

static int _engine_fd = -1;
static SmTimerIdT _engine_timer_id = SM_TIMER_ID_INVALID;
static SmTimerIdT _engine_audit_timer_id = SM_TIMER_ID_INVALID;
static uint64_t _dispatches_per_interval = 0;

// ****************************************************************************
// Service Group Engine - Audit
// ============================
static void sm_service_group_engine_audit( void* user_data[],
    SmServiceGroupT* service_group )
{
    SmServiceGroupEventT event = SM_SERVICE_GROUP_EVENT_AUDIT;
    SmErrorT error;

    error = sm_service_group_fsm_event_handler( service_group->name, event,
                                                NULL, "periodic audit" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Event (%s) not handled for service group (%s).",
                  sm_service_group_event_str( event ),
                  service_group->name );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Engine - Audit Timeout
// ====================================
static bool sm_service_group_engine_audit_timeout( SmTimerIdT timer_id,
    int64_t user_data )
{
    sm_service_group_table_foreach( NULL, sm_service_group_engine_audit );

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Engine - Timeout
// ==============================
static bool sm_service_group_engine_timeout( SmTimerIdT timer_id,
    int64_t user_data )
{
    uint64_t count = 1;

    _dispatches_per_interval = 0;

    if( 0 > write( _engine_fd, &count, sizeof(count) ) )
    {
        DPRINTFE( "Failed to signal service groups, error=%s",
                  strerror( errno ) );
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Engine - Signal
// =============================
SmErrorT sm_service_group_engine_signal( SmServiceGroupT* service_group )
{
    if( service_group->desired_state != service_group->state )
    {
        if( SM_SERVICE_GROUP_ENGINE_THROTTLE_SIGNALS > _dispatches_per_interval )
        {
            uint64_t count = 1;

            if( 0 > write( _engine_fd, &count, sizeof(count) ) )
            {
                DPRINTFE( "Failed to signal service group (%s), error=%s",
                          service_group->name, strerror( errno ) );
                return( SM_FAILED );
            }
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Engine - Signal Handler
// =====================================
static void sm_service_group_engine_signal_handler( void* user_data[],
    SmServiceGroupT* service_group )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceGroupEventT event = SM_SERVICE_GROUP_EVENT_UNKNOWN;
    SmErrorT error;

    if( service_group->desired_state == service_group->state )
    {
        DPRINTFD( "Service group (%s) already in desired state (%s).",
                  service_group->name,
                  sm_service_group_state_str( service_group->desired_state ) );
        return;
    }

    DPRINTFD( "Signal handler called for service group (%s), "
              "desired-state=%s, state=%s", service_group->name, 
              sm_service_group_state_str( service_group->desired_state ),
              sm_service_group_state_str( service_group->state ) );

    switch( service_group->desired_state )
    {
        case SM_SERVICE_GROUP_STATE_INITIAL:
            return;
        break;

        case SM_SERVICE_GROUP_STATE_ACTIVE:
            event = SM_SERVICE_GROUP_EVENT_GO_ACTIVE;
            snprintf( reason_text, sizeof(reason_text), 
                      "active state requested" );
        break;

        case SM_SERVICE_GROUP_STATE_STANDBY:
            event = SM_SERVICE_GROUP_EVENT_GO_STANDBY;
            snprintf( reason_text, sizeof(reason_text), 
                      "standby state requested" );
        break;

        case SM_SERVICE_GROUP_STATE_DISABLED:
            event = SM_SERVICE_GROUP_EVENT_DISABLE;
            snprintf( reason_text, sizeof(reason_text), 
                      "disabled state requested" );
        break;

        default:
            DPRINTFE( "Service group (%s) has unknown desired state (%s).",
                      service_group->name,
                      sm_service_group_state_str( service_group->desired_state ) );
        break;
    }

    if( SM_SERVICE_GROUP_EVENT_UNKNOWN != event )
    {
        error = sm_service_group_fsm_event_handler( service_group->name, 
                                                    event, NULL, reason_text );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Event (%s) not handled for service group (%s).",
                      sm_service_group_event_str( event ),
                      service_group->name );
        }
    } else {
        error = sm_service_group_engine_signal( service_group );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to signal service group (%s), error=%s.",
                      service_group->name, sm_error_str( error ) );
            return;
        }               
    }

    return;
}
// ****************************************************************************

// ****************************************************************************
// Service Group Engine - Dispatch
// ===============================
static void sm_service_group_engine_dispatch( int selobj, int64_t user_data )
{
    uint64_t count;

    read( _engine_fd, &count, sizeof(count) );

    ++_dispatches_per_interval;

    sm_service_group_table_foreach( NULL, 
                                    sm_service_group_engine_signal_handler );
}
// ****************************************************************************

// ****************************************************************************
// Service Groups Engine - Initialize
// ==================================
SmErrorT sm_service_group_engine_initialize( void )
{
    SmErrorT error;

    _engine_fd = eventfd( 0, EFD_CLOEXEC | EFD_NONBLOCK  );
    if( 0 > _engine_fd )
    {
        DPRINTFE( "Failed to open file descriptor,error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    error = sm_selobj_register( _engine_fd, sm_service_group_engine_dispatch,
                                0 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register selection object, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    _dispatches_per_interval = 0;

    error = sm_timer_register( "service group engine",
                               SM_SERVICE_GROUP_ENGINE_TIMER_IN_MS,
                               sm_service_group_engine_timeout,
                               0, &_engine_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create engine timer, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }
    
    error = sm_timer_register( "service group audit",
                               SM_SERVICE_GROUP_ENGINE_AUDIT_TIMER_IN_MS,
                               sm_service_group_engine_audit_timeout,
                               0, &_engine_audit_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create engine audit timer, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }
    
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Engine - Finalize
// ===============================
SmErrorT sm_service_group_engine_finalize( void )
{
    SmErrorT error;

    _dispatches_per_interval = 0;

    if( SM_TIMER_ID_INVALID != _engine_audit_timer_id )
    {
        error = sm_timer_deregister( _engine_audit_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel engine audit timer, error=%s.",
                      sm_error_str( error ) );
        }
        _engine_audit_timer_id = SM_TIMER_ID_INVALID;
    }

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
