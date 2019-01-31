//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_initial_state.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_timer.h"
#include "sm_heartbeat.h"
#include "sm_msg.h"
#include "sm_service_domain_utils.h"
#include "sm_service_domain_fsm.h"
#include "sm_failover_fsm.h"

// ****************************************************************************
// Service Domain Initial State - Hello Timer
// ==========================================
static bool sm_service_domain_initial_state_hello_timer( SmTimerIdT timer_id,
    int64_t user_data )
{
    int64_t id = user_data;
    SmServiceDomainT* domain;
    SmErrorT error;

    domain = sm_service_domain_table_read_by_id( id );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain, error=%s.",
                  sm_error_str(SM_NOT_FOUND) );
        return( true );
    }

    error = sm_service_domain_utils_send_hello( domain->name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send hello for service domain (%s), error=%s.",
                  domain->name, sm_error_str( error ) );
        return( true );
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Initial State - Entry
// ====================================
SmErrorT sm_service_domain_initial_state_entry( SmServiceDomainT* domain )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != domain->hello_timer_id )
    {
        error = sm_timer_deregister( domain->hello_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel hello timer for service domain (%s), "
                      "error=%s.", domain->name, sm_error_str( error ) );
        }

        domain->hello_timer_id = SM_TIMER_ID_INVALID;
    }

    domain->designation = SM_DESIGNATION_TYPE_UNKNOWN;

    sm_msg_disable();

    sm_heartbeat_disable();

    error = sm_service_domain_utils_service_domain_disable_self(
                                                        domain->name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to disable own assignments for service domain "
                  "(%s), error=%s.", domain->name, sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Initial State - Exit
// ===================================
SmErrorT sm_service_domain_initial_state_exit( SmServiceDomainT* domain )
{
    char timer_name[80] = "";
    SmTimerIdT hello_timer_id;
    SmErrorT error;

    snprintf( timer_name, sizeof(timer_name), "%s hello", domain->name );

    error = sm_timer_register( timer_name, domain->hello_interval,
                               sm_service_domain_initial_state_hello_timer,
                               domain->id, &hello_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create hello timer for service domain (%s), "
                  "error=%s.", domain->name, sm_error_str( error ) );
        return( error );
    }

    domain->designation = SM_DESIGNATION_TYPE_UNKNOWN;
    domain->hello_timer_id = hello_timer_id;

    error = sm_service_domain_utils_update_own_assignments( domain->name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update own assignments for service domain "
                  "(%s), error=%s.", domain->name, sm_error_str( error ) );
        return( error );
    }
   
    sm_msg_enable();

    sm_heartbeat_enable();

    SmFailoverFSM::get_fsm().send_event(SM_FAILOVER_EVENT_HEARTBEAT_ENABLED, NULL);
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Initial State - Transition
// =========================================
SmErrorT sm_service_domain_initial_state_transition( SmServiceDomainT* domain,
    SmServiceDomainStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Initial State - Event Handler
// ============================================
SmErrorT sm_service_domain_initial_state_event_handler(
    SmServiceDomainT* domain, SmServiceDomainEventT event, void* event_data[] )
{
    bool enabled;
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceDomainStateT state;
    SmErrorT error;

    error = sm_service_domain_utils_service_domain_enabled( domain->name,
                                                            &enabled );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to determine if service domain (%s) is enabled, "
                  "error=%s.", domain->name, sm_error_str( error ) );
        return( error );
    }

    switch( event )
    {
        case SM_SERVICE_DOMAIN_EVENT_HELLO_MSG:
        case SM_SERVICE_DOMAIN_EVENT_NEIGHBOR_AGEOUT:
            // Ignore.
        break;

        case SM_SERVICE_DOMAIN_EVENT_INTERFACE_ENABLED:
            if( enabled )
            {
                if( 0 == domain->priority )
                {
                    state = SM_SERVICE_DOMAIN_STATE_OTHER;

                    snprintf( reason_text, sizeof(reason_text), 
                              "interface enabled, priority is zero" );
                } else {
                    state = SM_SERVICE_DOMAIN_STATE_WAITING;

                    snprintf( reason_text, sizeof(reason_text), 
                              "interface enabled" );
                }

                error = sm_service_domain_fsm_set_state( domain->name, state,
                                                         reason_text );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Set state (%s) of service domain (%s) failed, "
                              "error=%s.", sm_service_domain_state_str( state ),
                              domain->name, sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_DOMAIN_EVENT_INTERFACE_DISABLED:
        case SM_SERVICE_DOMAIN_EVENT_WAIT_EXPIRED:
            // Ignore.
        break;

        case SM_SERVICE_DOMAIN_EVENT_CHANGING_LEADER:
            DPRINTFE("Received unexpected %s event", sm_service_domain_event_str(event));
        break;

        default:
            DPRINTFD( "Service Domain (%s) ignoring event (%s).", 
                      domain->name, sm_service_domain_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Initial State - Initialize
// =========================================
SmErrorT sm_service_domain_initial_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Initial State - Finalize
// =======================================
SmErrorT sm_service_domain_initial_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
