//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_neighbor_fsm.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_time.h"
#include "sm_timer.h"
#include "sm_msg.h"
#include "sm_node_api.h"
#include "sm_heartbeat.h"
#include "sm_service_domain_table.h"
#include "sm_service_domain_neighbor_down_state.h"
#include "sm_service_domain_neighbor_exchange_start_state.h"
#include "sm_service_domain_neighbor_exchange_state.h"
#include "sm_service_domain_neighbor_full_state.h"
#include "sm_log.h"
#include "sm_failover.h"

static SmMsgCallbacksT _msg_callbacks = {0};
static SmListT* _callbacks = NULL;

static SmServiceDomainNeighborStateT _dump_state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_NIL;
static SmTimeT _dump_state_last_changed = {0};

// ****************************************************************************
// Service Domain Neighbor FSM - Register Callback
// ===============================================
SmErrorT sm_service_domain_neighbor_fsm_register_callback(
    SmServiceDomainNeighborFsmCallbackT callback )
{
    SM_LIST_PREPEND( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Deregister Callback
// =================================================
SmErrorT sm_service_domain_neighbor_fsm_deregister_callback(
    SmServiceDomainNeighborFsmCallbackT callback )
{
    SM_LIST_REMOVE( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Notify
// ====================================
static void sm_service_domain_neighbor_fsm_notify( 
    SmServiceDomainNeighborT* neighbor )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainNeighborFsmCallbackT callback;

    SM_LIST_FOREACH( _callbacks, entry, entry_data )
    {
        callback = (SmServiceDomainNeighborFsmCallbackT) entry_data;

        callback( neighbor->service_domain, neighbor->name, neighbor->state );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Pause Timeout
// ===========================================
static bool sm_service_domain_neighbor_fsm_pause_timeout( SmTimerIdT timer_id,
    int64_t user_data )
{
    int64_t id = user_data;
    SmServiceDomainNeighborT* neighbor;

    neighbor = sm_service_domain_neighbor_table_read_by_id( id );
    if( NULL == neighbor )
    {
        DPRINTFE( "Failed to read neighbor (%" PRIi64 "), error=%s.", id,
                  sm_error_str(SM_NOT_FOUND) );
        return( true );
    }

    neighbor->pause_timer_id = SM_TIMER_ID_INVALID;

    DPRINTFI( "Neighbor (%s) for service domain (%s) pause interval expired.",
              neighbor->name, neighbor->service_domain );

    return( false );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Start Pause Timer
// ===============================================
static SmErrorT sm_service_domain_neighbor_fsm_start_pause_timer(
    int pause_interval, SmServiceDomainNeighborT* neighbor )
{
    char timer_name[80] = "";
    SmTimerIdT pause_timer_id;
    SmErrorT error;

    DPRINTFI( "Start pause timer for neighbor (%s), pause=%i ms.",
              neighbor->name, pause_interval );

    snprintf( timer_name, sizeof(timer_name), "%s neighbor %s pause",
              neighbor->service_domain, neighbor->name );

    error = sm_timer_register( timer_name, pause_interval,
                               sm_service_domain_neighbor_fsm_pause_timeout,
                               neighbor->id, &pause_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create pause timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    neighbor->pause_timer_id = pause_timer_id;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Stop Pause Timer
// ==============================================
static SmErrorT sm_service_domain_neighbor_fsm_stop_pause_timer(
    SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != neighbor->pause_timer_id )
    {
        DPRINTFI( "Stop pause timer for neighbor (%s).", neighbor->name );

        error = sm_timer_deregister( neighbor->pause_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel pause timer for service domain (%s) "
                      "neighbor (%s), error=%s.", neighbor->service_domain,
                      neighbor->name, sm_error_str( error ) );
        }

        neighbor->pause_timer_id = SM_TIMER_ID_INVALID;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Restart Pause Timer
// =================================================
static SmErrorT sm_service_domain_neighbor_fsm_restart_pause_timer( 
    int pause_interval, SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    DPRINTFI( "Restart pause timer for neighbor (%s).", neighbor->name );

    error = sm_service_domain_neighbor_fsm_stop_pause_timer( neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to stop pause timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_neighbor_fsm_start_pause_timer( pause_interval,
                                                              neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to start pause timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Dead Timeout
// ==========================================
static bool sm_service_domain_neighbor_fsm_dead_timeout( SmTimerIdT timer_id,
    int64_t user_data )
{
    int64_t id = user_data;
    SmServiceDomainNeighborT* neighbor;

    neighbor = sm_service_domain_neighbor_table_read_by_id( id );
    if( NULL == neighbor )
    {
        DPRINTFE( "Failed to read neighbor (%" PRIi64 "), error=%s.", id,
                  sm_error_str(SM_NOT_FOUND) );
        return( true );
    }

    if( !sm_timer_scheduling_on_time() )
    {
        DPRINTFI( "Not scheduling on time, would have declared neighbor (%s) "
                  "for service domain (%s) dead.", neighbor->name,
                  neighbor->service_domain );
        return( true );
    }

    if( sm_heartbeat_peer_alive_in_period( neighbor->name,
                                           neighbor->dead_interval/2 ) )
    {
        DPRINTFI( "Heartbeat still indicates peer is alive, would have "
                  "declared neighbor (%s) for service domain (%s) dead.",
                  neighbor->name, neighbor->service_domain );
        return( true );
    }

    if( SM_TIMER_ID_INVALID != neighbor->pause_timer_id )
    {
        DPRINTFI( "Pause timer still in effect, would have declared neighbor "
                  "(%s) for service domain (%s) dead.", neighbor->name,
                  neighbor->service_domain );
        return( true );
    }

    sm_failover_lost_hello_msg();

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Start Dead Timer
// ==============================================
SmErrorT sm_service_domain_neighbor_fsm_start_dead_timer(
    SmServiceDomainNeighborT* neighbor )
{
    char timer_name[80] = "";
    SmTimerIdT dead_timer_id;
    SmErrorT error;

    DPRINTFD( "Start dead timer for neighbor (%s).", neighbor->name );

    snprintf( timer_name, sizeof(timer_name), "%s neighbor %s dead",
              neighbor->service_domain, neighbor->name );

    error = sm_timer_register( timer_name, neighbor->dead_interval,
                               sm_service_domain_neighbor_fsm_dead_timeout,
                               neighbor->id, &dead_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create dead timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    neighbor->dead_timer_id = dead_timer_id;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Stop Dead Timer
// =============================================
SmErrorT sm_service_domain_neighbor_fsm_stop_dead_timer(
    SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != neighbor->dead_timer_id )
    {
        DPRINTFD( "Stop dead timer for neighbor (%s).", neighbor->name );

        error = sm_timer_deregister( neighbor->dead_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel dead timer for service domain (%s) "
                      "neighbor (%s), error=%s.", neighbor->service_domain,
                      neighbor->name, sm_error_str( error ) );
        }

        neighbor->dead_timer_id = SM_TIMER_ID_INVALID;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Restart Dead Timer
// ================================================
SmErrorT sm_service_domain_neighbor_fsm_restart_dead_timer(
    SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    DPRINTFD( "Restart dead timer for neighbor (%s).", neighbor->name );

    error = sm_service_domain_neighbor_fsm_stop_dead_timer( neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to stop dead timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_neighbor_fsm_start_dead_timer( neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to start dead timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Enter State
// =========================================
static SmErrorT sm_service_domain_neighbor_fsm_enter_state(
    SmServiceDomainNeighborT* neighbor ) 
{
    SmErrorT error;

    switch( neighbor->state )
    {
        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN:
            error = sm_service_domain_neighbor_down_state_entry( neighbor );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to enter state (%s), error=%s.",
                          neighbor->name, 
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE_START:
            error = sm_service_domain_neighbor_exchange_start_state_entry( neighbor );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to enter state (%s), error=%s.",
                          neighbor->name, 
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE:
            error = sm_service_domain_neighbor_exchange_state_entry( neighbor );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to enter state (%s), error=%s.",
                          neighbor->name, 
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_FULL:
            error = sm_service_domain_neighbor_full_state_entry( neighbor );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to enter state (%s), error=%s.",
                          neighbor->name, 
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown neighbor (%s) state (%s).", neighbor->name,
                      sm_service_domain_neighbor_state_str( neighbor->state ) );
        break;
    }

    _dump_state = neighbor->state;
    clock_gettime( CLOCK_REALTIME, &_dump_state_last_changed );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Exit State
// ========================================
static SmErrorT sm_service_domain_neighbor_fsm_exit_state(
    SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    switch( neighbor->state )
    {
        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN:
            error = sm_service_domain_neighbor_down_state_exit( neighbor );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to exit state (%s), error=%s.",
                          neighbor->name, 
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE_START:
            error = sm_service_domain_neighbor_exchange_start_state_exit( neighbor );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to exit state (%s), error=%s.",
                          neighbor->name, 
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE:
            error = sm_service_domain_neighbor_exchange_state_exit( neighbor );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to exit state (%s), error=%s.",
                          neighbor->name, 
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_FULL:
            error = sm_service_domain_neighbor_full_state_exit( neighbor );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to exit state (%s), error=%s.",
                          neighbor->name, 
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown neighbor (%s) state (%s).", neighbor->name,
                      sm_service_domain_neighbor_state_str( neighbor->state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Transition State
// ==============================================
static SmErrorT sm_service_domain_neighbor_fsm_transition_state(
    SmServiceDomainNeighborT* neighbor,
    SmServiceDomainNeighborStateT from_state )
{
    SmErrorT error;

    switch( neighbor->state )
    {
        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN:
            error = sm_service_domain_neighbor_down_state_transition(
                                                        neighbor, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", neighbor->name,
                          sm_service_domain_neighbor_state_str( from_state ),
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE_START:
            error = sm_service_domain_neighbor_exchange_start_state_transition(
                                                        neighbor, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", neighbor->name,
                          sm_service_domain_neighbor_state_str( from_state ),
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE:
            error = sm_service_domain_neighbor_exchange_state_transition(
                                                        neighbor, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", neighbor->name,
                          sm_service_domain_neighbor_state_str( from_state ),
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_FULL:
            error = sm_service_domain_neighbor_full_state_transition(
                                                        neighbor, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", neighbor->name,
                          sm_service_domain_neighbor_state_str( from_state ),
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown neighbor (%s) state (%s).", neighbor->name,
                      sm_service_domain_neighbor_state_str( neighbor->state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Set State
// =======================================
SmErrorT sm_service_domain_neighbor_fsm_set_state( char neighbor_name[], 
    char service_domain[], SmServiceDomainNeighborStateT state ) 
{
    SmServiceDomainNeighborStateT prev_state;
    SmServiceDomainNeighborT* neighbor;
    SmErrorT error, error2;

    neighbor = sm_service_domain_neighbor_table_read( neighbor_name,
                                                      service_domain );
    if( NULL == neighbor )
    {
        DPRINTFE( "Failed to read neighbor (%s), error=%s.", neighbor_name,
                  sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    prev_state = neighbor->state;

    error = sm_service_domain_neighbor_fsm_exit_state( neighbor ); 
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to exit state (%s) neighbor (%s), error=%s.",
                  sm_service_domain_neighbor_state_str( neighbor->state ),
                  neighbor_name, sm_error_str( error ) );
        return( error );
    }

    neighbor->state = state;

    error = sm_service_domain_neighbor_fsm_transition_state( neighbor,
                                                             prev_state );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to transition to state (%s) neighbor (%s), "
                  "error=%s.",
                  sm_service_domain_neighbor_state_str( neighbor->state ),
                  neighbor_name, sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }

    error = sm_service_domain_neighbor_fsm_enter_state( neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to enter state (%s) neighbor (%s), error=%s.",
                  sm_service_domain_neighbor_state_str( neighbor->state ),
                  neighbor_name, sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }

    return( SM_OKAY );

STATE_CHANGE_ERROR:
    error2 = sm_service_domain_neighbor_fsm_exit_state( neighbor );
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to exit state (%s) neighbor (%s), error=%s.",
                  sm_service_domain_neighbor_state_str( neighbor->state ),
                  neighbor_name, sm_error_str( error2 ) );
        abort();
    }

    neighbor->state = prev_state;

    error2 = sm_service_domain_neighbor_fsm_transition_state( neighbor,
                                                              state );
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to transition neighbor (%s) to state (%s), "
                  "error=%s.", neighbor_name,
                  sm_service_domain_neighbor_state_str( neighbor->state ),
                  sm_error_str( error2 ) );
        abort();
    }

    error2 = sm_service_domain_neighbor_fsm_enter_state( neighbor ); 
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to enter state (%s) neighbor (%s), error=%s.",
                  sm_service_domain_neighbor_state_str( neighbor->state ),
                  neighbor_name, sm_error_str( error2 ) );
        abort();
    }

    return( error );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Event Handler
// ===========================================
SmErrorT sm_service_domain_neighbor_fsm_event_handler( char neighbor_name[], 
    char service_domain[], SmServiceDomainNeighborEventT event,
    void* event_data[], const char reason_text[] )
{
    SmServiceDomainNeighborStateT prev_state;
    SmServiceDomainNeighborT* neighbor;
    SmErrorT error;

    neighbor = sm_service_domain_neighbor_table_read( neighbor_name, 
                                                      service_domain );
    if( NULL == neighbor )
    {
        DPRINTFE( "Failed to read neighbor (%s), error=%s.",
                  neighbor_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    prev_state = neighbor->state;

    switch( neighbor->state )
    {
        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN:
            error = sm_service_domain_neighbor_down_state_event_handler(
                                                neighbor, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", neighbor_name,
                          sm_service_domain_neighbor_event_str( event ),
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE_START:
            error = sm_service_domain_neighbor_exchange_start_state_event_handler(
                                                neighbor, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", neighbor_name,
                          sm_service_domain_neighbor_event_str( event ),
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE:
            error = sm_service_domain_neighbor_exchange_state_event_handler(
                                                neighbor, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", neighbor_name,
                          sm_service_domain_neighbor_event_str( event ),
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_STATE_FULL:
            error = sm_service_domain_neighbor_full_state_event_handler(
                                                neighbor, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Neighbor (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", neighbor_name,
                          sm_service_domain_neighbor_event_str( event ),
                          sm_service_domain_neighbor_state_str( neighbor->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown state (%s) for neighbor (%s).",
                      sm_service_domain_neighbor_state_str( neighbor->state ),
                      neighbor_name );
        break;
    }

    if( prev_state != neighbor->state )
    {
        DPRINTFI( "Neighbor (%s) received event (%s) was in the %s state and "
                  "is now in the %s.", neighbor_name,
                  sm_service_domain_neighbor_event_str( event ),
                  sm_service_domain_neighbor_state_str( prev_state ),
                  sm_service_domain_neighbor_state_str( neighbor->state ) );

        error = sm_service_domain_neighbor_table_persist( neighbor );
        if( SM_OKAY != error )
        {
                DPRINTFE( "Failed to update database for neighbor (%s), "
                          "error=%s.", neighbor_name, sm_error_str( error ) );
            return( error );
        }

        sm_log_neighbor_state_change( neighbor_name, 
                    sm_service_domain_neighbor_state_str( prev_state ),
                    sm_service_domain_neighbor_state_str( neighbor->state ),
                    reason_text );

        sm_service_domain_neighbor_fsm_notify( neighbor );
    }

    return( SM_OKAY );
}
// ****************************************************************************

SmNodeAvailStatusT sm_node_get_availability( char node_name[])
{
    SmDbNodeT node;
    SmErrorT error = sm_failover_get_node(node_name, node);
    if( SM_OKAY == error )
    {
        return node.avail_status;
    }
    return SM_NODE_AVAIL_STATUS_UNKNOWN;
}

// ****************************************************************************
// Service Domain Neighbor FSM - Hello Message Callback
// ====================================================
static void sm_service_domain_neighbor_fsm_hello_msg_callback(
    SmNetworkAddressT* network_address, int network_port, int version,
    int revision, char service_domain_name[], char node_name[],
    char orchestration[], char designation[], int generation, int priority, 
    int hello_interval, int dead_interval, int wait_interval,
    int exchange_interval, char leader[] )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmServiceDomainT* domain;
    SmServiceDomainNeighborT* neighbor;
    SmServiceDomainNeighborEventT event;
    SmErrorT error;

    domain = sm_service_domain_table_read( service_domain_name );
    if( NULL == domain )
    {
        DPRINTFD( "Service domain (%s) does not exist, error=%s.",
                  service_domain_name, sm_error_str(SM_NOT_FOUND) );
        return;
    }

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return;
    }

    neighbor = sm_service_domain_neighbor_table_read( node_name,
                                                      service_domain_name );
    if( NULL == neighbor )
    {
        error = sm_service_domain_neighbor_table_insert( node_name, 
                        service_domain_name, orchestration, designation,
                        generation, priority, hello_interval, dead_interval,
                        wait_interval, exchange_interval );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to insert neighbor (%s) into database, "
                      "error=%s.", node_name, sm_error_str( error ) );
            return;
        }

        neighbor = sm_service_domain_neighbor_table_read( node_name,
                                                          service_domain_name );
        if( NULL == neighbor )
        {
            DPRINTFE( "Failed to read neighbor (%s), error=%s.",
                      node_name, sm_error_str(SM_NOT_FOUND) );
            return;
        }

        neighbor->exchange_master = false;

        if( generation > domain->generation )
        {
            neighbor->exchange_master = true;

        } else if( generation == domain->generation ) {
            if( priority < domain->priority )
            {
                neighbor->exchange_master = true;

            } else if( priority == domain->priority ) {
                SmNodeAvailStatusT host_avail_status =
                    sm_node_get_availability( hostname );
                SmNodeAvailStatusT neighbor_avail_status =
                    sm_node_get_availability( neighbor->name );

                int weight_table[SM_NODE_AVAIL_STATUS_MAX] = {0};
                weight_table[SM_NODE_AVAIL_STATUS_AVAILABLE] = 10;
                weight_table[SM_NODE_AVAIL_STATUS_DEGRADED] = 5;
                weight_table[SM_NODE_AVAIL_STATUS_FAILED] = -1;
                int host_weight = weight_table[host_avail_status];
                int neighbor_weight = weight_table[neighbor_avail_status];

                if( host_weight > neighbor_weight )
                {
                    DPRINTFD("%s is more available than %s", hostname, neighbor->name);
                    neighbor->exchange_master = true;
                }
                else if( host_weight == neighbor_weight )
                {
                    if( 0 < strcmp( hostname, neighbor->name ) )
                    {
                        neighbor->exchange_master = true;
                    }
                }
                else
                {
                    DPRINTFD("%s is less available than %s", hostname, neighbor->name);
                }
            }
        }
    } else {
        neighbor->exchange_master = false;

        if( generation > domain->generation )
        {
            neighbor->exchange_master = true;

        } else if( generation == domain->generation ) {
            if( priority < domain->priority )
            {
                neighbor->exchange_master = true;

            } else if( priority == domain->priority ) {
                SmNodeAvailStatusT host_avail_status =
                    sm_node_get_availability( hostname );
                SmNodeAvailStatusT neighbor_avail_status =
                    sm_node_get_availability( neighbor->name );

                int weight_table[SM_NODE_AVAIL_STATUS_MAX] = {0};
                weight_table[SM_NODE_AVAIL_STATUS_AVAILABLE] = 10;
                weight_table[SM_NODE_AVAIL_STATUS_DEGRADED] = 5;
                weight_table[SM_NODE_AVAIL_STATUS_FAILED] = -1;
                int host_weight = weight_table[host_avail_status];
                int neighbor_weight = weight_table[neighbor_avail_status];

                if( host_weight > neighbor_weight )
                {
                    DPRINTFD("%s is more available than %s", hostname, neighbor->name);
                    neighbor->exchange_master = true;
                }
                else if( host_weight == neighbor_weight )
                {
                    if( 0 < strcmp( hostname, neighbor->name ) )
                    {
                        neighbor->exchange_master = true;
                    }
                }
                else
                {
                    DPRINTFD("%s is less available than %s", hostname, neighbor->name);
                }
            }
        }

        snprintf( neighbor->designation, sizeof(neighbor->designation),
                  "%s", designation );

        error = sm_service_domain_neighbor_table_persist( neighbor );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to update neighbor (%s) into database, "
                      "error=%s.", node_name, sm_error_str( error ) );
            return;
        }
    }

    error = sm_service_domain_neighbor_fsm_stop_pause_timer( neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to cancel pause timer for neighbor (%s), "
                  "error=%s.", node_name, sm_error_str( error ) );
        return;
    }

    event = SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_HELLO_MSG;

    snprintf( reason_text, sizeof(reason_text), "hello received for %s",
              service_domain_name );

    error = sm_service_domain_neighbor_fsm_event_handler( node_name,
                                                          service_domain_name,
                                                          event, NULL,
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
// Service Domain Neighbor FSM - Pause Message Callback
// ====================================================
static void sm_service_domain_neighbor_fsm_pause_msg_callback(
    SmNetworkAddressT* network_address, int network_port, int version,
    int revision, char service_domain_name[], char node_name[],
    int pause_interval )
{
    SmServiceDomainNeighborT* neighbor;
    SmErrorT error;

    neighbor = sm_service_domain_neighbor_table_read( node_name,
                                                      service_domain_name );
    if( NULL != neighbor )
    {
        error = sm_service_domain_neighbor_fsm_restart_pause_timer(
                                                pause_interval, neighbor );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Service domain (%s) neighbor (%s) unable start "
                      "pause timer, error=%s.", service_domain_name,
                      node_name, sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Exchange Start Message Callback
// =============================================================
static void sm_service_domain_neighbor_fsm_exchange_start_msg_callback(
    SmNetworkAddressT* network_address, int network_port, int version,
    int revision, char service_domain_name[], char node_name[],
    int exchange_seq )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceDomainNeighborT* neighbor;
    SmServiceDomainNeighborEventT event;
    void* event_data[SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DATA_MAX] = {0};
    SmErrorT error;

    neighbor = sm_service_domain_neighbor_table_read( node_name,
                                                      service_domain_name );
    if( NULL == neighbor )
    {
        DPRINTFE( "Failed to read neighbor (%s) for service domain (%s), "
                  "error=%s.", node_name, service_domain_name,
                  sm_error_str(SM_NOT_FOUND) );
        return;
    }

    event = SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_MSG;

    event_data[SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DATA_MSG_EXCHANGE_SEQ]
        = &exchange_seq;

    snprintf( reason_text, sizeof(reason_text), "exchange-start received for %s",
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

void sm_domain_neighbor_fsm_dump(FILE* fp)
{
    SmRealTimeStrT realtime;
    sm_time_format_realtime(&_dump_state_last_changed, realtime, sizeof(realtime));
    fprintf(fp, "Domain Neighbor FSM state: %s, last changed %s\n",
            sm_service_domain_neighbor_state_str(_dump_state),
            realtime
        );
}

// ****************************************************************************
// Service Domain Neighbor FSM - Initialize
// ========================================
SmErrorT sm_service_domain_neighbor_fsm_initialize( void )
{
    SmErrorT error;

    error = sm_service_domain_neighbor_down_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize neighbor down state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_neighbor_exchange_start_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize neighbor exchange start state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_neighbor_exchange_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize neighbor exchange state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_neighbor_full_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize neighbor full state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    _msg_callbacks.hello
        = sm_service_domain_neighbor_fsm_hello_msg_callback;
    _msg_callbacks.pause
        = sm_service_domain_neighbor_fsm_pause_msg_callback;
    _msg_callbacks.exchange_start 
        = sm_service_domain_neighbor_fsm_exchange_start_msg_callback;

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
// Service Domain Neighbor FSM - Finalize
// ======================================
SmErrorT sm_service_domain_neighbor_fsm_finalize( void )
{
    SmErrorT error;

    error = sm_msg_deregister_callbacks( &_msg_callbacks ); 
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister message callbacks, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_domain_neighbor_down_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize neighbor down state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_neighbor_exchange_start_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize neighbor exchange start state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_neighbor_exchange_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize neighbor exchange state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_neighbor_full_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize neighbor full state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
