//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_waiting_state.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_timer.h"
#include "sm_node_api.h"
#include "sm_service_domain_utils.h"
#include "sm_service_domain_fsm.h"

#define SM_SKEW_WAIT_TIME( priority ) ((( 256 - priority ) * 100) / 256) 

// ****************************************************************************
// Service Domain Waiting State - Wait Timer
// =========================================
static bool sm_service_domain_waiting_state_wait_timer( SmTimerIdT timer_id,
    int64_t user_data )
{
    int wait_interval;                 
    int64_t id = user_data;
    SmServiceDomainT* domain;
    SmServiceDomainEventT event = SM_SERVICE_DOMAIN_EVENT_WAIT_EXPIRED;
    SmErrorT error;

    domain = sm_service_domain_table_read_by_id( id );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain, error=%s.",
                  sm_error_str(SM_NOT_FOUND) );
        return( true );
    }

    wait_interval = domain->wait_interval + SM_SKEW_WAIT_TIME( domain->priority );

    if( sm_timer_scheduling_on_time_in_period( wait_interval ) )
    {
        error = sm_service_domain_fsm_event_handler( domain->name, event,
                                                     NULL, "wait expired" );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Service domain (%s) failed to handle event (%s), "
                      "error=%s.", domain->name,
                      sm_service_domain_event_str(event),
                      sm_error_str( error ) );
            return( true );
        }
    } else {
        DPRINTFI( "Not scheduling on time in the last %i ms, waiting "
                  "another %i ms for service domain (%s).", wait_interval,
                  wait_interval, domain->name );
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Waiting State - Entry
// ====================================
SmErrorT sm_service_domain_waiting_state_entry( SmServiceDomainT* domain )
{
    char timer_name[80] = "";
    int wait_interval;                 
    SmTimerIdT wait_timer_id;
    SmErrorT error;

    wait_interval = domain->wait_interval + SM_SKEW_WAIT_TIME( domain->priority );

    DPRINTFI( "Service domain (%s) wait interval set to %i ms.", domain->name,
              wait_interval );

    snprintf( timer_name, sizeof(timer_name), "%s wait", domain->name );

    error = sm_timer_register( timer_name, wait_interval,
                               sm_service_domain_waiting_state_wait_timer,
                               domain->id, &wait_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create wait timer for service domain (%s), "
                  "error=%s.", domain->name, sm_error_str( error ) );
        return( error );
    }

    domain->designation = SM_DESIGNATION_TYPE_UNKNOWN;
    domain->leader[0] = '\0';
    domain->wait_timer_id = wait_timer_id;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Waiting State - Exit
// ===================================
SmErrorT sm_service_domain_waiting_state_exit( SmServiceDomainT* domain )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != domain->wait_timer_id )
    {
        error = sm_timer_deregister( domain->wait_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel wait timer for service domain (%s), "
                      "error=%s.", domain->name, sm_error_str( error ) );
        }

        domain->wait_timer_id = SM_TIMER_ID_INVALID;
    }

    domain->designation = SM_DESIGNATION_TYPE_UNKNOWN;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Waiting State - Transition
// =========================================
SmErrorT sm_service_domain_waiting_state_transition( 
    SmServiceDomainT* domain, SmServiceDomainStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Waiting State - Event Handler
// ============================================
SmErrorT sm_service_domain_waiting_state_event_handler(
    SmServiceDomainT* domain, SmServiceDomainEventT event,
    void* event_data[] )
{
    bool enabled, elected, hold_election;
    char* leader_name = NULL;
    char leader[SM_NODE_NAME_MAX_CHAR];
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmServiceDomainStateT state;
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceDomainNeighborT* neighbor;
    SmErrorT error;

    error = sm_service_domain_utils_service_domain_enabled( domain->name,
                                                            &enabled );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to determine if service domain (%s) is enabled, "
                  "error=%s.", domain->name, sm_error_str( error ) );
        return( error );
    }

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    switch( event )
    {
        case SM_SERVICE_DOMAIN_EVENT_HELLO_MSG:
            leader_name
                = (char*) event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_LEADER];

            if( '\0' != leader_name[0] )
            {
                snprintf( domain->leader, SM_NODE_NAME_MAX_CHAR, "%s",
                          leader_name );

                error = sm_service_domain_table_persist( domain );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to persist service domain (%s) data, "
                              "error=%s.", domain->name,
                              sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_DOMAIN_EVENT_NEIGHBOR_AGEOUT:
        case SM_SERVICE_DOMAIN_EVENT_INTERFACE_ENABLED:
            // Ignore.
        break;

        case SM_SERVICE_DOMAIN_EVENT_INTERFACE_DISABLED:
            if( !enabled )
            {
                state = SM_SERVICE_DOMAIN_STATE_INITIAL;

                snprintf( reason_text, sizeof(reason_text),
                          "primary interface disabled" );

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

        case SM_SERVICE_DOMAIN_EVENT_WAIT_EXPIRED:
            elected = false;
            hold_election = false;

            if(( '\0' == domain->leader[0] )||( domain->preempt ))
            {
                hold_election = true;

            } else {
                neighbor = sm_service_domain_neighbor_table_read( 
                        domain->leader, domain->name );
                if( NULL != neighbor )
                {
                    if( domain->generation > neighbor->generation )
                    {
                        hold_election = true;
                    }
                } else {
                    DPRINTFE( "Service domain (%s) neighbor (%s) not found.",
                              domain->name, domain->leader );
                }
            }

            if( hold_election )
            {
                error = sm_service_domain_utils_hold_election( domain->name,
                                                            leader, &elected );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to hold election for service domain "
                              "(%s), error=%s.", domain->name,
                              sm_error_str( error ) );
                    abort();
                }

                snprintf( domain->leader, SM_NODE_NAME_MAX_CHAR, "%s", leader );

                error = sm_service_domain_table_persist( domain );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to update service domain (%s), "
                              "error=%s.", domain->name,
                              sm_error_str( error ) );
                    return( error );
                }
            } else {
                elected = (0 == strcmp( hostname, domain->leader ));
            }

            if( elected )
            {
                state = SM_SERVICE_DOMAIN_STATE_LEADER;

                snprintf( reason_text, sizeof(reason_text),
                          "wait expired, elected leader" );
            } else {
                state = SM_SERVICE_DOMAIN_STATE_BACKUP;

                snprintf( reason_text, sizeof(reason_text),
                          "wait expired, elected backup" );
            }

            error = sm_service_domain_fsm_set_state( domain->name, state,
                                                     reason_text );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Set state (%s) of service domain (%s) failed, "
                          "error=%s.", sm_service_domain_state_str( state ),
                          domain->name, sm_error_str( error ) );
                abort();
            }
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
// Service Domain Waiting State - Initialize
// =========================================
SmErrorT sm_service_domain_waiting_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Waiting State - Finalize
// =======================================
SmErrorT sm_service_domain_waiting_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
