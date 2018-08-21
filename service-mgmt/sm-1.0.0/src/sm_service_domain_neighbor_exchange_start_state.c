//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_neighbor_exchange_start_state.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_domain_neighbor_fsm.h"
#include "sm_service_domain_utils.h"
#include "sm_service_domain_assignment_table.h"
#include "sm_failover.h"

static const int MIN_EXCHANGE_START_INTERVAL_MS = 100;
// ****************************************************************************
// Service Domain Neighbor Exchange Start - Clear Assignments
// ==========================================================
static void sm_service_domain_neighbor_exchange_start_clear_assignments(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    assignment->exchanged = false;
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange Start - Timeout
// ================================================
static bool sm_service_domain_neighbor_exchange_start_timeout(
    SmTimerIdT timer_id, int64_t user_data )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    int64_t id = user_data;
    SmServiceDomainNeighborT* neighbor;
    SmServiceDomainNeighborEventT event;
    SmErrorT error;

    neighbor = sm_service_domain_neighbor_table_read_by_id( id );
    if( NULL == neighbor )
    {
        DPRINTFE( "Failed to read neighbor (%" PRIi64 "), error=%s.", id,
                  sm_error_str(SM_NOT_FOUND) );
        return( true );
    }

    event = SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_TIMER;

    snprintf( reason_text, sizeof(reason_text), "exchange-start timeout for %s",
              neighbor->service_domain );

    error = sm_service_domain_neighbor_fsm_event_handler( neighbor->name,
                                    neighbor->service_domain, event, NULL,
                                    reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) neighbor (%s) unable to handle "
                  "event (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_service_domain_neighbor_event_str( event ),
                  sm_error_str( error ) );
        return( true );
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange Start State - Entry
// ====================================================
SmErrorT sm_service_domain_neighbor_exchange_start_state_entry(
    SmServiceDomainNeighborT* neighbor )
{
    char timer_name[80] = "";
    SmTimerIdT exchange_timer_id;
    SmErrorT error;

    error = sm_service_domain_utils_update_own_assignments(
                                                neighbor->service_domain );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update own assignments for service domain "
                  "(%s), error=%s.", neighbor->service_domain,
                  sm_error_str( error ) );
        return( error );
    }

    sm_service_domain_assignment_table_foreach_node_in_service_domain(
            neighbor->service_domain, neighbor->name, NULL,
            sm_service_domain_neighbor_exchange_start_clear_assignments );

    snprintf( timer_name, sizeof(timer_name), "neighbor %s exchange start",
              neighbor->name );

    if( neighbor->exchange_master )
    {
        error = sm_timer_register( timer_name, neighbor->exchange_interval,
                                   sm_service_domain_neighbor_exchange_start_timeout,
                                   neighbor->id, &exchange_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to create exchange start timer for service "
                      "domain (%s) neighbor (%s), error=%s.",
                      neighbor->service_domain, neighbor->name,
                      sm_error_str( error ) );
            return( error );
        }

        ++neighbor->exchange_seq;
        neighbor->exchange_timer_id = exchange_timer_id;

        error = sm_service_domain_utils_send_exchange_start(
                    neighbor->service_domain, neighbor, neighbor->exchange_seq );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send exchange start for service domain (%s) "
                      "neighbor (%s), error=%s.", neighbor->service_domain,
                      neighbor->name, sm_error_str( error ) );
            return( error );
        }
    } else {
        error = sm_timer_register( timer_name, neighbor->exchange_interval*4,
                                   sm_service_domain_neighbor_exchange_start_timeout,
                                   neighbor->id, &exchange_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to create exchange start timer for service "
                      "domain (%s) neighbor (%s), error=%s.",
                      neighbor->service_domain, neighbor->name,
                      sm_error_str( error ) );
            return( error );
        }

        neighbor->exchange_seq = 0;
        neighbor->exchange_timer_id = exchange_timer_id;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange Start State - Exit
// ===================================================
SmErrorT sm_service_domain_neighbor_exchange_start_state_exit( 
    SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != neighbor->exchange_timer_id )
    {
        error = sm_timer_deregister( neighbor->exchange_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel exchange start timer for service "
                      "domain (%s) neighbor (%s), error=%s.", 
                      neighbor->service_domain, neighbor->name, 
                      sm_error_str( error ) );
        }

        neighbor->exchange_timer_id = SM_TIMER_ID_INVALID;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange Start State - Transition
// =========================================================
SmErrorT sm_service_domain_neighbor_exchange_start_state_transition(
    SmServiceDomainNeighborT* neighbor,
    SmServiceDomainNeighborStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange Start State - Event Handler
// ============================================================
SmErrorT sm_service_domain_neighbor_exchange_start_state_event_handler(
    SmServiceDomainNeighborT* neighbor, SmServiceDomainNeighborEventT event,
    void* event_data[] )
{
    int exchange_seq;
    SmServiceDomainNeighborStateT state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_NIL;
    SmErrorT error;

    switch( event )
    {
        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_HELLO_MSG:
            sm_failover_hello_msg_restore();
            error = sm_service_domain_neighbor_fsm_restart_dead_timer( neighbor );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to restart dead timer for service domain (%s) "
                          "neighbor (%s), error=%s.", neighbor->service_domain,
                          neighbor->name, sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_MSG:
            exchange_seq = *(int*)
              event_data[SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DATA_MSG_EXCHANGE_SEQ];

            if( neighbor->exchange_master )
            {
                if( exchange_seq == neighbor->exchange_seq )
                {
                    state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE;

                } else {
                    error = sm_service_domain_utils_send_exchange_start_throttle(
                                                        neighbor->service_domain,
                                                        neighbor,
                                                        neighbor->exchange_seq,
                                                        MIN_EXCHANGE_START_INTERVAL_MS);
                    if( SM_OKAY != error )
                    {
                        DPRINTFE( "Failed to send exchange start for service "
                                  "domain (%s) neighbor (%s), error=%s.",
                                  neighbor->service_domain, neighbor->name,
                                  sm_error_str( error ) );
                        return( error );
                    }
                }
            } else {
                state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE;

                error = sm_service_domain_utils_send_exchange_start(
                                                        neighbor->service_domain,
                                                        neighbor, exchange_seq );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to send exchange start for service "
                              "domain (%s) neighbor (%s), error=%s.",
                              neighbor->service_domain, neighbor->name,
                              sm_error_str( error ) );
                    return( error );
                }
            }

            if( SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE == state )
            {
                error = sm_service_domain_neighbor_fsm_set_state( neighbor->name,
                                                neighbor->service_domain, state );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Service domain neighbor (%s) set state (%s) "
                              "failed, error=%s.", neighbor->name,
                              sm_service_domain_neighbor_state_str( state ),
                              sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_TIMER:
            if( neighbor->exchange_master )
            {
                ++neighbor->exchange_seq;

                error = sm_service_domain_utils_send_exchange_start(
                                                        neighbor->service_domain,
                                                        neighbor,
                                                        neighbor->exchange_seq );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to send exchange start for service "
                              "domain (%s) neighbor (%s), error=%s.",
                              neighbor->service_domain, neighbor->name,
                              sm_error_str( error ) );
                    return( error );
                }
            } else {
                error = sm_service_domain_utils_send_exchange_start(
                                                        neighbor->service_domain,
                                                        neighbor, 0 );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to send exchange start for service "
                              "domain (%s) neighbor (%s), error=%s.",
                              neighbor->service_domain, neighbor->name,
                              sm_error_str( error ) );
                    return( error );
                }
            }                
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_MSG:
        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_TIMEOUT:
            // Ignore.
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DOWN:
            state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN;

            error = sm_service_domain_neighbor_fsm_set_state( neighbor->name,
                                            neighbor->service_domain, state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain neighbor (%s) set state (%s) "
                          "failed, error=%s.", neighbor->name,
                          sm_service_domain_neighbor_state_str( state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFD( "Neighbor (%s) ignoring event (%s).", neighbor->name,
                      sm_service_domain_neighbor_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange Start State - Initialize
// =========================================================
SmErrorT sm_service_domain_neighbor_exchange_start_state_initialize(
    void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange Start State - Finalize
// =======================================================
SmErrorT sm_service_domain_neighbor_exchange_start_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
