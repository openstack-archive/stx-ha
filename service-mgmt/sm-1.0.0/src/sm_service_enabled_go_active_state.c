//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_enabled_go_active_state.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_fsm.h"
#include "sm_service_go_active.h"

#define SM_SERVICE_GO_ACTIVE_TIMEOUT_IN_MS                               300000


// ****************************************************************************
// handling special upgrade case.
// THIS IS A HACK TO TEMPORALLY OVERCOME ISCSI ENABLE TIMEOUT AFTER UPGRADE
// THIS CODE SHOULD BE REMOVED in Release 17.xx
// ===============================
static int handle_special_upgrade_case(int* timeout_ms)
{
    FILE* fp;
    char flag_file[] = "/tmp/.extend_sm_iscsi_enable_timeout";
    int reading_sec;
    fp = fopen( flag_file, "r" );
    if ( NULL == fp)
        return 0;

    fscanf( fp, "%d", &reading_sec );
    fclose(fp);

    if ( reading_sec <= *timeout_ms )
    {
        reading_sec = 960; // make it 16 minutes
    }

    DPRINTFI( "Extending go-active timeout from %d ms to %d ms",
        *timeout_ms, reading_sec * 1000);
    *timeout_ms = reading_sec * 1000;

    return 1;
}

// ****************************************************************************
// Service Enabled Go-Active State - Timeout Timer
// ===============================================
static bool sm_service_enabled_go_active_state_timeout_timer(
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
                                          SM_SERVICE_EVENT_GO_ACTIVE_TIMEOUT,
                                          NULL, "overall go-active timeout" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to signal overall go-active timeout to service "
                  "(%s), error=%s.", service->name, sm_error_str( error ) );
        return( true );
    }

    DPRINTFI( "Service (%s) go-active overall timeout.", service->name );

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Active State - Entry
// =======================================
SmErrorT sm_service_enabled_go_active_state_entry( SmServiceT* service )
{
    char timer_name[80] = "";
    SmTimerIdT action_state_timer_id;
    SmErrorT error;
    int timeout = SM_SERVICE_GO_ACTIVE_TIMEOUT_IN_MS;

    if( SM_TIMER_ID_INVALID != service->action_state_timer_id )
    {
        error = sm_timer_deregister( service->action_state_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel go-active overall timer, error=%s.",
                      sm_error_str( error ) );
        }

        service->action_state_timer_id = SM_TIMER_ID_INVALID;
    }

    snprintf( timer_name, sizeof(timer_name), "%s go-active overall timeout",
              service->name );

    handle_special_upgrade_case(&timeout);
    error = sm_timer_register( timer_name,
                               timeout,
                               sm_service_enabled_go_active_state_timeout_timer,
                               service->id, &action_state_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create go-active overall timer for service "
                  "(%s), error=%s.", service->name, sm_error_str( error ) );
        return( error );
    }

    service->action_state_timer_id = action_state_timer_id;

    error = sm_service_go_active( service );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to go-active on service (%s), error=%s",
                  service->name, sm_error_str( error ) );
        return( error );
    }
    
    service->action_fail_count = 0;
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Active State - Exit
// ======================================
SmErrorT sm_service_enabled_go_active_state_exit( SmServiceT* service )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != service->action_state_timer_id )
    {
        error = sm_timer_deregister( service->action_state_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel go-active overall timer, error=%s.",
                      sm_error_str( error ) );
        }

        service->action_state_timer_id = SM_TIMER_ID_INVALID;
    }

    if( SM_SERVICE_ACTION_GO_ACTIVE == service->action_running )
    {
        error = sm_service_go_active_abort( service );
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
// Service Enabled Go-Active State - Transition
// ============================================
SmErrorT sm_service_enabled_go_active_state_transition( SmServiceT* service,
    SmServiceStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Active State - Event Handler
// ===============================================
SmErrorT sm_service_enabled_go_active_state_event_handler( SmServiceT* service,
    SmServiceEventT event, void* event_data[] )
{
    SmErrorT error;

    switch( event )
    {
        case SM_SERVICE_EVENT_GO_ACTIVE:
            if( SM_SERVICE_ACTION_GO_ACTIVE != service->action_running )
            {
                error = sm_service_go_active( service );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to go-active on service (%s), error=%s",
                              service->name, sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_EVENT_GO_ACTIVE_SUCCESS:
            error = sm_service_fsm_set_state( service,
                                              SM_SERVICE_STATE_ENABLED_ACTIVE );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service (%s) state (%s), "
                          "error=%s.", service->name, 
                          sm_service_state_str( SM_SERVICE_STATE_ENABLED_ACTIVE ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_EVENT_GO_ACTIVE_FAILED:
        case SM_SERVICE_EVENT_GO_ACTIVE_TIMEOUT:
            ++(service->fail_count);
            ++(service->action_fail_count);
            ++(service->transition_fail_count);

        case SM_SERVICE_EVENT_GO_STANDBY:
        case SM_SERVICE_EVENT_DISABLE:
            error = sm_service_fsm_set_state( service,
                                        SM_SERVICE_STATE_ENABLED_GO_STANDBY );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service (%s) state (%s), "
                          "error=%s.", service->name,
                          sm_service_state_str( 
                              SM_SERVICE_STATE_ENABLED_GO_STANDBY  ),
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
// Service Enabled Go-Active State - Initialize
// ============================================
SmErrorT sm_service_enabled_go_active_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Active State - Finalize
// ==========================================
SmErrorT sm_service_enabled_go_active_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
