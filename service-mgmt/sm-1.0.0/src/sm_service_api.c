//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_api.h"

#include <stdio.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_timer.h"
#include "sm_service_table.h"
#include "sm_service_dependency.h"
#include "sm_service_engine.h"
#include "sm_service_fsm.h"
#include "sm_service_go_active.h"
#include "sm_service_go_standby.h"
#include "sm_service_action.h"

static SmListT* _callbacks = NULL;

// ****************************************************************************
// Service API - Register Callback
// ===============================
SmErrorT sm_service_api_register_callback( SmServiceApiCallbackT callback )
{
    SM_LIST_PREPEND( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service API - Deregister Callback
// =================================
SmErrorT sm_service_api_deregister_callback( SmServiceApiCallbackT callback )
{
    SM_LIST_REMOVE( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service API - Service Callback
// ==============================
static void sm_service_api_service_callback( char service_name[],
    SmServiceStateT state, SmServiceStatusT status,
    SmServiceConditionT condition )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceApiCallbackT callback;
    SmServiceT* service;
    SmErrorT error;

    service = sm_service_table_read( service_name );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  service_name, sm_error_str(SM_NOT_FOUND) );
        return;
    }

    if(( SM_SERVICE_STATE_ENABLED_STANDBY == service->desired_state )&&
       ( SM_SERVICE_STATE_DISABLED == service->state ) &&
       ( SM_SERVICE_STATUS_NONE == service->status ))
    {
        bool exists;

        error = sm_service_go_standby_exists( service, &exists );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to read service (%s), error=%s.",
                      service_name, sm_error_str( error ) );
            return;
        }

        if( !exists )
        {
            // No standby service action exists.  Notify higher
            // layers that the service is in the standby state.
            state = SM_SERVICE_STATE_ENABLED_STANDBY;
        }
    }

    SM_LIST_FOREACH( _callbacks, entry, entry_data )
    {
        callback = (SmServiceApiCallbackT) entry_data;

        callback( service_name, state, status, condition );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service API - Abort Action
// ==========================
static void sm_service_api_abort_action( void* user_data[], 
    SmServiceT* service )
{
    SmErrorT error;

    if(( SM_SERVICE_ACTION_NIL     == service->action_running )||
       ( SM_SERVICE_ACTION_UNKNOWN == service->action_running )|| 
       ( SM_SERVICE_ACTION_NONE    == service->action_running ))
    {
        DPRINTFD( "Service (%s) not running an action.", service->name );
        return;
    }

    error = sm_service_action_abort( service->name, service->action_pid );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to abort service (%s) action (%s, pid=%i), "
                  "error=%s.", service->name, 
                  sm_service_action_str( service->action_running ),
                  service->action_pid, sm_error_str( error ) );
        return;
    }

    if( SM_TIMER_ID_INVALID != service->action_timer_id )
    {
        error = sm_timer_deregister( service->action_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to stop timer for action (%s) of "
                      "service (%s), error=%s.",
                      sm_service_action_str( service->action_running ),
                       service->name, sm_error_str( error ) );
            return;
        }

        DPRINTFI( "Timer stopped for action (%s) of service (%s).",
                  sm_service_action_str( service->action_running ),
                  service->name );
    }

    service->action_running = SM_SERVICE_ACTION_NONE;
    service->action_pid = -1;
    service->action_timer_id = SM_TIMER_ID_INVALID;
    service->action_attempts = -1;
    return;
}
// ****************************************************************************

// ****************************************************************************
// Service API - Abort Actions
// ===========================
SmErrorT sm_service_api_abort_actions( void )
{
    sm_service_table_foreach( NULL, sm_service_api_abort_action );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service API - Go-Active
// =======================
SmErrorT sm_service_api_go_active( char service_name[] )
{
    SmServiceT* service;
    SmErrorT error;

    service = sm_service_table_read( service_name );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  service_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    if( SM_SERVICE_STATE_ENABLED_ACTIVE != service->desired_state )
    {
        service->desired_state = SM_SERVICE_STATE_ENABLED_ACTIVE;

        error = sm_service_table_persist( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service (%s) data, error=%s.",
                      service->name, sm_error_str( error ) );
            return( error );
        }

        error = sm_service_engine_signal( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to signal service (%s), error=%s.",
                      service->name, sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service API - Go-Standby 
// ========================
SmErrorT sm_service_api_go_standby( char service_name[] )
{
    SmServiceT* service;
    SmErrorT error;

    service = sm_service_table_read( service_name );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  service_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    if( SM_SERVICE_STATE_ENABLED_STANDBY != service->desired_state )
    {
        service->desired_state = SM_SERVICE_STATE_ENABLED_STANDBY;

        error = sm_service_table_persist( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service (%s) data, error=%s.",
                      service->name, sm_error_str( error ) );
            return( error );
        }

        error = sm_service_engine_signal( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to signal service (%s), error=%s.",
                      service->name, sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service API - Disable 
// =====================
SmErrorT sm_service_api_disable( char service_name[] )
{
    SmServiceT* service;
    SmErrorT error;

    service = sm_service_table_read( service_name );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  service_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    if( SM_SERVICE_STATE_DISABLED != service->desired_state )
    {
        service->desired_state = SM_SERVICE_STATE_DISABLED;

        error = sm_service_table_persist( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service (%s) data, error=%s.",
                      service->name, sm_error_str( error ) );
            return( error );
        }

        error = sm_service_engine_signal( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to signal service (%s), error=%s.",
                      service->name, sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service API - Recover
// =====================
SmErrorT sm_service_api_recover( char service_name[],
    bool clear_fatal_condition )
{
    SmServiceT* service;

    service = sm_service_table_read( service_name );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  service_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    if(( 0 < service->fail_count )||( 0 < service->action_fail_count ))
    {
        service->recover = true;
        DPRINTFI( "Recovery of service (%s) requested.", service->name );
    }

    if(( clear_fatal_condition )&&( 0 < service->transition_fail_count ))
    {
        service->clear_fatal_condition = true;
        DPRINTFI( "Recovery of service (%s) from fatal condition requested.",
                  service->name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service API - Restart 
// =====================
SmErrorT sm_service_api_restart( char service_name[], int flag )
{
    SmServiceT* service;
    SmServiceEventT event = SM_SERVICE_EVENT_DISABLE;
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR];
    SmErrorT error;

    service = sm_service_table_read( service_name );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  service_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    if ( flag & SM_SVC_RESTART_NO_DEP )
    {
        snprintf( reason_text, sizeof(reason_text), "restart safe requested");
        service->disable_check_dependency = false;
        service->disable_skip_dependent = true;
    } else
    {
        snprintf( reason_text, sizeof(reason_text), "restart requested");
    }

    error = sm_service_fsm_event_handler( service->name, event, NULL,
                                          reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Event (%s) not handled for service (%s).",
                  sm_service_event_str( event ), service->name );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************


// ****************************************************************************
// Service API - Audit
// ===================
SmErrorT sm_service_api_audit( char service_name[] )
{
    SmServiceT* service;
    SmErrorT error;

    service = sm_service_table_read( service_name );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  service_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    error = sm_service_fsm_event_handler( service->name,
                                          SM_SERVICE_EVENT_AUDIT, NULL,
                                          "forced audit" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to perform audit on service (%s), error=%s.",
                  service->name, sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service API - Initialize 
// ========================
SmErrorT sm_service_api_initialize( void )
{
    SmErrorT error;

    error = sm_service_table_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_dependency_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service dependencies, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_engine_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to intialize service engine, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_fsm_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service fsm, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_fsm_register_callback( sm_service_api_service_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for service fsm state changes, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service API - Finalize 
// ======================
SmErrorT sm_service_api_finalize( void )
{
    SmErrorT error;

    error = sm_service_fsm_deregister_callback( sm_service_api_service_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister for service fsm state changes, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_fsm_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service fsm, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_engine_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service engine, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_dependency_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service dependency, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_table_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service table, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
