//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_neighbor_exchange_state.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_msg.h"
#include "sm_node_api.h"
#include "sm_service_domain_neighbor_fsm.h"
#include "sm_service_domain_utils.h"
#include "sm_service_domain_assignment_table.h"
#include "sm_failover.h"

static SmMsgCallbacksT _msg_callbacks = {0};

// ****************************************************************************
// Service Domain Neighbor Exchange - Timeout
// ==========================================
static bool sm_service_domain_neighbor_exchange_timeout( SmTimerIdT timer_id,
    int64_t user_data )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    int64_t id = user_data;
    SmServiceDomainNeighborT* neighbor;
    SmErrorT error;

    neighbor = sm_service_domain_neighbor_table_read_by_id( id );
    if( NULL == neighbor )
    {
        DPRINTFE( "Failed to read neighbor (%" PRIi64 "), error=%s.", id,
                  sm_error_str(SM_NOT_FOUND) );
        return( true );
    }

    if( neighbor->exchange_master )
    {
        SmServiceDomainNeighborEventT event;

        event = SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_TIMEOUT;

        snprintf( reason_text, sizeof(reason_text), "exchange timeout for %s",
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
        }
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange - Start Timer
// ==============================================
static SmErrorT sm_service_domain_neighbor_exchange_start_timer(
    SmServiceDomainNeighborT* neighbor )
{
    char timer_name[80] = "";
    SmTimerIdT exchange_timer_id;
    SmErrorT error;

    snprintf( timer_name, sizeof(timer_name), "neighbor %s exchange",
              neighbor->name );

    error = sm_timer_register( timer_name, neighbor->exchange_interval,
                               sm_service_domain_neighbor_exchange_timeout,
                               neighbor->id, &exchange_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create exchange timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    neighbor->exchange_timer_id = exchange_timer_id;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange - Stop Timer
// =============================================
static SmErrorT sm_service_domain_neighbor_exchange_stop_timer(
    SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != neighbor->exchange_timer_id )
    {
        error = sm_timer_deregister( neighbor->exchange_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel exchange timer for service domain (%s) "
                      "neighbor (%s), error=%s.", neighbor->service_domain,
                      neighbor->name, sm_error_str( error ) );
        }

        neighbor->exchange_timer_id = SM_TIMER_ID_INVALID;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange - Restart Timer
// ================================================
static SmErrorT sm_service_domain_neighbor_exchange_restart_timer(
    SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    error = sm_service_domain_neighbor_exchange_stop_timer( neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to stop exchange timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_neighbor_exchange_start_timer( neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to start exchange timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange - Send
// =======================================
static SmErrorT sm_service_domain_neighbor_exchange_send(
    SmServiceDomainNeighborT* neighbor, bool* exchange_complete )
{
    bool more_members;
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmServiceDomainAssignmentT* assignment;
    SmServiceDomainAssignmentT* next_assignment;
    SmErrorT error;

    *exchange_complete = false;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    assignment = sm_service_domain_assignment_table_get_next_node( 
                    neighbor->service_domain, hostname,
                    neighbor->exchange_last_sent_id );
    if( NULL == assignment )
    {
        assignment = sm_service_domain_assignment_table_get_last_node(
                        neighbor->service_domain, hostname,
                        neighbor->exchange_last_sent_id );
        if( NULL == assignment )
        {
            DPRINTFE( "Failed to find last sent assignment." );
            return( SM_NOT_FOUND );
        }

        next_assignment = NULL;
    } else {
        next_assignment = sm_service_domain_assignment_table_get_next_node(
                                        neighbor->service_domain, hostname,
                                        assignment->id );
    }

    more_members = (NULL != next_assignment);

    error = sm_service_domain_utils_send_exchange( neighbor->service_domain,
            neighbor, neighbor->exchange_seq+1, assignment->id,
            assignment->service_group_name, assignment->desired_state,
            assignment->state, assignment->status, assignment->condition,
            assignment->health, assignment->reason_text,
            more_members, neighbor->exchange_last_recvd_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send service domain (%s) exchange message, "
                  "error=%s.", neighbor->service_domain,
                  sm_error_str(error) );
        return( error );
    }

    DPRINTFI( "Service domain (%s) sent service group (%s) to "
              "node (%s), more_members=%s.", neighbor->service_domain,
              assignment->service_group_name, neighbor->name,
              more_members ? "yes" : "no" );

    ++(neighbor->exchange_seq);
    neighbor->exchange_last_sent_id = assignment->id;

    *exchange_complete = (!more_members);

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange - Receive
// ==========================================
static void sm_service_domain_neighbor_exchange_receive(
    SmNetworkAddressT* network_address, int network_port, int version,
    int revision, char service_domain_name[], char node_name[], int exchange_seq,
    int64_t member_id, char member_name[], SmServiceGroupStateT member_desired_state,
    SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
    SmServiceGroupConditionT member_condition, int64_t member_health,
    char member_reason_text[], bool more_members, int64_t last_received_member_id )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceDomainNeighborT* neighbor;
    SmServiceDomainAssignmentT* assignment;
    SmServiceDomainNeighborEventT event;
    void* event_data[SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DATA_MAX] = {0};
    SmErrorT error;

    event_data[SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DATA_MSG_MORE_MEMBERS] 
        = &more_members;

    neighbor = sm_service_domain_neighbor_table_read( node_name,
                                                      service_domain_name );
    if( NULL == neighbor )
    {
        DPRINTFE( "Failed to read neighbor (%s), error=%s.", node_name,
                  sm_error_str(SM_NOT_FOUND) );
        return;
    }

    if( last_received_member_id != neighbor->exchange_last_sent_id )
    {
        DPRINTFE( "Member id mismatch from neighbor (%s) for service domain "
                  "(%s), received=%" PRIi64 ", expected=%" PRIi64 ".",
                  node_name, service_domain_name, last_received_member_id,
                  neighbor->exchange_last_sent_id );
        return;
    }

    neighbor->exchange_last_recvd_id = member_id;

    DPRINTFI( "Service domain (%s) received service group (%s) from "
              "node (%s), more_members=%s.", service_domain_name, member_name,
              node_name, more_members ? "yes" : "no" );

    if( '\0' != member_name[0] )
    {
        error = sm_service_domain_utils_update_assignment( service_domain_name,
                    node_name, member_name, member_desired_state,
                    member_state, member_status, member_condition, member_health,
                    member_reason_text );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Service domain (%s) neighbor (%s) unable to update "
                      "assignment (%s), error=%s.", service_domain_name, 
                      node_name, member_name, sm_error_str( error ) );
            return;
        }

        assignment = sm_service_domain_assignment_table_read(
                            service_domain_name, node_name, member_name );
        if( NULL != assignment )
        {
            assignment->exchanged = true;
        }
    }

    event = SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_MSG;
    
    // If we do transition because of this event, it is because the exchange 
    // is complete on both ends.
    snprintf( reason_text, sizeof(reason_text), "exchange complete for %s",
              service_domain_name );

    error = sm_service_domain_neighbor_fsm_event_handler( node_name,
                                                          service_domain_name,
                                                          event, event_data,
                                                          reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) neighbor (%s) unable to handle "
                  "event (%s), error=%s.", service_domain_name, node_name,
                  sm_service_domain_neighbor_event_str( event ),
                  sm_error_str( error ) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange State - Entry
// ==============================================
SmErrorT sm_service_domain_neighbor_exchange_state_entry(
    SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    error = sm_service_domain_neighbor_exchange_start_timer( neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to start exchange timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    neighbor->exchange_last_sent_id = 0;
    neighbor->exchange_last_recvd_id = 0;

    if( neighbor->exchange_master )
    {
        bool exchange_complete;

        error = sm_service_domain_neighbor_exchange_send( neighbor,
                                                          &exchange_complete );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send exchange message to neighbor (%s) for "
                      "service domain (%s), error=%s.", neighbor->name,
                      neighbor->service_domain, sm_error_str( error ) );
            return( error );
        }
    }

    error = sm_msg_register_callbacks( &_msg_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register message callbacks, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange State - Exit
// =============================================
SmErrorT sm_service_domain_neighbor_exchange_state_exit( 
    SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    error = sm_msg_deregister_callbacks( &_msg_callbacks ); 
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister message callbacks, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_neighbor_exchange_stop_timer( neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to stop exchange timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    neighbor->exchange_last_sent_id = 0;
    neighbor->exchange_last_recvd_id = 0;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange State - Transition
// ===================================================
SmErrorT sm_service_domain_neighbor_exchange_state_transition(
    SmServiceDomainNeighborT* neighbor,
    SmServiceDomainNeighborStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange State - Event Handler
// ======================================================
SmErrorT sm_service_domain_neighbor_exchange_state_event_handler(
    SmServiceDomainNeighborT* neighbor,
    SmServiceDomainNeighborEventT event, void* event_data[] )
{
    bool exchange_complete;
    bool neighbor_more_members;
    bool neighbor_exchange_complete;
    SmServiceDomainNeighborStateT state;
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
            state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE_START;

            error = sm_service_domain_neighbor_fsm_set_state( neighbor->name,
                                            neighbor->service_domain, state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) neighbor (%s) unable to set "
                          "state (%s), error=%s.", neighbor->service_domain,
                          neighbor->name,
                          sm_service_domain_neighbor_state_str( state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_TIMER:
            // Ignore.
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_MSG:

            neighbor_more_members = *(bool*) 
              event_data[SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DATA_MSG_MORE_MEMBERS];

            neighbor_exchange_complete = !neighbor_more_members;

            error = sm_service_domain_neighbor_exchange_restart_timer( neighbor );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to start exchange timer for service domain (%s) "
                          "neighbor (%s), error=%s.", neighbor->service_domain,
                          neighbor->name, sm_error_str( error ) );
                return( error );
            }

            error = sm_service_domain_neighbor_exchange_send( neighbor,
                                                              &exchange_complete );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to send exchange message to neighbor (%s) for "
                          "service domain (%s), error=%s.", neighbor->name,
                          neighbor->service_domain, sm_error_str( error ) );
                return( error );
            }

            if(( exchange_complete )&&( neighbor_exchange_complete ))
            {
                state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_FULL;

                error = sm_service_domain_neighbor_fsm_set_state( neighbor->name,
                                                neighbor->service_domain, state );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Service domain (%s) neighbor (%s) unable to set "
                              "state (%s), error=%s.", neighbor->service_domain,
                              neighbor->name,
                              sm_service_domain_neighbor_state_str( state ),
                              sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_TIMEOUT:
            state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE_START;

            error = sm_service_domain_neighbor_fsm_set_state( neighbor->name,
                                            neighbor->service_domain, state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) neighbor (%s) unable to set "
                          "state (%s), error=%s.", neighbor->service_domain,
                          neighbor->name,
                          sm_service_domain_neighbor_state_str( state ),
                          sm_error_str( error ) );
                return( error );
            }
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
// Service Domain Neighbor Exchange State - Initialize
// ===================================================
SmErrorT sm_service_domain_neighbor_exchange_state_initialize( void )
{
    _msg_callbacks.exchange = sm_service_domain_neighbor_exchange_receive;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Exchange State - Finalize
// =================================================
SmErrorT sm_service_domain_neighbor_exchange_state_finalize( void )
{
    _msg_callbacks.exchange = NULL;

    return( SM_OKAY );
}
// ****************************************************************************
