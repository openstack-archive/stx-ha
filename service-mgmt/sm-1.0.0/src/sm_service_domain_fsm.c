//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_fsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_msg.h"
#include "sm_service_domain_table.h"
#include "sm_service_domain_utils.h"
#include "sm_service_domain_initial_state.h"
#include "sm_service_domain_waiting_state.h"
#include "sm_service_domain_other_state.h"
#include "sm_service_domain_backup_state.h"
#include "sm_service_domain_leader_state.h"
#include "sm_service_domain_neighbor_fsm.h"
#include "sm_service_domain_scheduler.h"
#include "sm_alarm.h"
#include "sm_log.h"
#include "sm_node_utils.h"

static SmMsgCallbacksT _msg_callbacks = {0};
static char _reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";

// ****************************************************************************
// Service Domain FSM - Enter State
// ================================
static SmErrorT sm_service_domain_fsm_enter_state( SmServiceDomainT* domain ) 
{
    SmErrorT error;

    switch( domain->state )
    {
        case SM_SERVICE_DOMAIN_STATE_INITIAL:
            error = sm_service_domain_initial_state_entry( domain );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to enter state (%s), "
                          "error=%s.", domain->name,
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_WAITING:
            error = sm_service_domain_waiting_state_entry( domain );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to enter state (%s), "
                          "error=%s.", domain->name,
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_OTHER:
            error = sm_service_domain_other_state_entry( domain );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to enter state (%s), "
                          "error=%s.", domain->name,
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_BACKUP:
            error = sm_service_domain_backup_state_entry( domain );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to enter state (%s), "
                          "error=%s.", domain->name,
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_LEADER:
            error = sm_service_domain_leader_state_entry( domain );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to enter state (%s), "
                          "error=%s.", domain->name,
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown service domain (%s) state (%s).", 
                      domain->name,
                      sm_service_domain_state_str( domain->state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - Exit State
// ===============================
static SmErrorT sm_service_domain_fsm_exit_state( SmServiceDomainT* domain )
{
    SmErrorT error;

    switch( domain->state )
    {
        case SM_SERVICE_DOMAIN_STATE_INITIAL:
            error = sm_service_domain_initial_state_exit( domain );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to exit state (%s), "
                          "error=%s.", domain->name,
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_WAITING:
            error = sm_service_domain_waiting_state_exit( domain );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to exit state (%s), "
                          "error=%s.", domain->name,
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_OTHER:
            error = sm_service_domain_other_state_exit( domain );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to exit state (%s), "
                          "error=%s.", domain->name,
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_BACKUP:
            error = sm_service_domain_backup_state_exit( domain );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to exit state (%s), "
                          "error=%s.", domain->name,
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_LEADER:
            error = sm_service_domain_leader_state_exit( domain );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to exit state (%s), "
                          "error=%s.", domain->name,
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown service domain (%s) state (%s).",
                      domain->name, 
                      sm_service_domain_state_str( domain->state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - Transition State
// =====================================
static SmErrorT sm_service_domain_fsm_transition_state( 
    SmServiceDomainT* domain, SmServiceDomainStateT from_state )
{
    SmErrorT error;

    switch( domain->state )
    {
        case SM_SERVICE_DOMAIN_STATE_INITIAL:
            error = sm_service_domain_initial_state_transition( 
                                                domain, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.",
                          domain->name,
                          sm_service_domain_state_str( from_state ),
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_WAITING:
            error = sm_service_domain_waiting_state_transition( 
                                                domain, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.",
                          domain->name,
                          sm_service_domain_state_str( from_state ),
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_OTHER:
            error = sm_service_domain_other_state_transition( 
                                                domain, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.",
                          domain->name,
                          sm_service_domain_state_str( from_state ),
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_BACKUP:
            error = sm_service_domain_backup_state_transition( 
                                                domain, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.",
                          domain->name,
                          sm_service_domain_state_str( from_state ),
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_LEADER:
            error = sm_service_domain_leader_state_transition( 
                                                domain, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.",
                          domain->name,
                          sm_service_domain_state_str( from_state ),
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown service_domain (%s) state (%s).", 
                      domain->name,
                      sm_service_domain_state_str( domain->state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - Set State
// ==============================
SmErrorT sm_service_domain_fsm_set_state( char service_domain_name[],
    SmServiceDomainStateT state, const char reason_text[] ) 
{
    SmServiceDomainStateT prev_state;
    SmServiceDomainT* domain;
    SmErrorT error, error2;

    domain = sm_service_domain_table_read( service_domain_name );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service_domain (%s), error=%s.",
                  service_domain_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    prev_state = domain->state;

    DPRINTFI("Set state %s->%s", sm_service_domain_state_str( domain->state ),
        sm_service_domain_state_str( state ));
    error = sm_service_domain_fsm_exit_state( domain );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to exit state (%s) service_domain (%s), error=%s.",
                  sm_service_domain_state_str( domain->state ),
                  domain->name, sm_error_str( error ) );
        return( error );
    }

    domain->state = state;

    error = sm_service_domain_fsm_transition_state( domain, prev_state );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to transition to state (%s) service domain (%s), "
                  "error=%s.", sm_service_domain_state_str( domain->state ),
                  domain->name, sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }

    error = sm_service_domain_fsm_enter_state( domain );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to enter state (%s) service domain (%s), error=%s.",
                  sm_service_domain_state_str( domain->state ),
                  domain->name, sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }

    if( '\0' != reason_text[0] )
    {
        snprintf( _reason_text, sizeof(_reason_text), "%s", reason_text );
    }

    error = sm_service_domain_table_persist( domain );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to persist service domain (%s), error=%s.",
                  domain->name, sm_error_str( error ) );
    }

    return( SM_OKAY );

STATE_CHANGE_ERROR:
    error2 = sm_service_domain_fsm_exit_state( domain ); 
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to exit state (%s) service domain (%s), error=%s.",
                  sm_service_domain_state_str( domain->state ),
                  domain->name, sm_error_str( error2 ) );
        abort();
    }

    domain->state = prev_state;

    error2 = sm_service_domain_fsm_transition_state( domain, state );
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to transition to state (%s) service domain (%s), "
                  "error=%s.", sm_service_domain_state_str( domain->state ),
                  domain->name, sm_error_str( error2 ) );
        abort();
    }

    error2 = sm_service_domain_fsm_enter_state( domain ); 
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to enter state (%s) service domain (%s), error=%s.",
                  sm_service_domain_state_str( domain->state ), 
                  domain->name, sm_error_str( error2 ) );
        abort();
    }

    return( error );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - Event Handler
// ==================================
SmErrorT sm_service_domain_fsm_event_handler( char service_domain_name[],
    SmServiceDomainEventT event, void* event_data[], const char reason_text[] )
{
    SmServiceDomainStateT prev_state;
    SmServiceDomainT* domain;
    SmErrorT error;

    snprintf( _reason_text, sizeof(_reason_text), "%s", reason_text );

    domain = sm_service_domain_table_read( service_domain_name );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain (%s), error=%s.",
                  service_domain_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    prev_state = domain->state;

    switch( domain->state )
    {
        case SM_SERVICE_DOMAIN_STATE_INITIAL:
            error = sm_service_domain_initial_state_event_handler( 
                                            domain, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", domain->name,
                          sm_service_domain_event_str( event ),
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_WAITING:
            error = sm_service_domain_waiting_state_event_handler( 
                                            domain, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", domain->name,
                          sm_service_domain_event_str( event ),
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_OTHER:
            error = sm_service_domain_other_state_event_handler( 
                                            domain, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", domain->name,
                          sm_service_domain_event_str( event ),
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_BACKUP:
            error = sm_service_domain_backup_state_event_handler( 
                                            domain, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", domain->name,
                          sm_service_domain_event_str( event ),
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_STATE_LEADER:
            error = sm_service_domain_leader_state_event_handler( 
                                            domain, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service Domain (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", domain->name,
                          sm_service_domain_event_str( event ),
                          sm_service_domain_state_str( domain->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown state (%s) for service domain (%s).",
                      sm_service_domain_state_str( domain->state ),
                      domain->name );
        break;
    }

    if( prev_state != domain->state )
    {
        DPRINTFI( "Service Domain (%s) received event (%s) was in the %s "
                  "state and is now in the %s state.", domain->name,
                  sm_service_domain_event_str( event ),
                  sm_service_domain_state_str( prev_state ),
                  sm_service_domain_state_str( domain->state ) );

        error = sm_service_domain_table_persist( domain );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service domain (%s) data, error=%s.",
                      service_domain_name, sm_error_str( error ) );
            return( error );
        }

        sm_log_service_domain_state_change( domain->name,
                            sm_service_domain_state_str( prev_state ),
                            sm_service_domain_state_str( domain->state ),
                            _reason_text );

        if( SM_SERVICE_DOMAIN_STATE_LEADER == domain->state )
        {
            sm_alarm_manage_domain_alarms( domain->name, true );
            sm_service_domain_scheduler_schedule_service_domain( domain );
        } else {
            sm_alarm_manage_domain_alarms( domain->name, false );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - Hello Message Callback
// ===========================================
static void sm_service_domain_fsm_hello_msg_callback(
    SmNetworkAddressT* network_address, int network_port, int version,
    int revision, char service_domain[], char node_name[], char orchestration[],
    char designation[], int generation, int priority, int hello_interval,
    int dead_interval, int wait_interval, int exchange_interval, char leader[] )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceDomainEventT event = SM_SERVICE_DOMAIN_EVENT_HELLO_MSG;
    void* event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MAX] = {0};
    SmErrorT error;

    event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_NODE_NAME] = node_name;
    event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_GENERATION] = &generation;
    event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_PRIORITY]  = &priority;
    event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_LEADER]    = leader;

    snprintf( reason_text, sizeof(reason_text), "hello received from %s",
              node_name );

    error = sm_service_domain_fsm_event_handler( service_domain, event,
                                                 event_data, reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Event (%s) not handled for service domain (%s).",
                  sm_service_domain_event_str( event ), service_domain );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - Neighbor State Callback
// ============================================
static void sm_service_domain_fsm_neighbor_state_callback( 
    char service_domain_name[], char node_name[], 
    SmServiceDomainNeighborStateT state )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceDomainEventT event;
    SmErrorT error;

    if( SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN == state )
    {
        void* event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MAX] = {0};

        DPRINTFI( "Neighbor (%s) down for service domain (%s).", node_name,
                  service_domain_name );

        error = sm_service_domain_utils_service_domain_neighbor_cleanup( 
                    service_domain_name, node_name );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cleanup neighbor (%s) for service "
                      "domain (%s), error=%s.", node_name,
                      service_domain_name, sm_error_str(error) );
            return;
        }

        event = SM_SERVICE_DOMAIN_EVENT_NEIGHBOR_AGEOUT;

        event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_NODE_NAME] = node_name;

        snprintf( reason_text, sizeof(reason_text), "%s neighbor ageout",
                  node_name );

        error = sm_service_domain_fsm_event_handler( service_domain_name,
                                                     event, event_data,
                                                     reason_text );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Service domain (%s) failed to handle event (%s), "
                      "error=%s.", service_domain_name,
                      sm_service_domain_event_str(event),
                      sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Failover - dump state
// ======================
void sm_service_domain_dump_state(FILE* fp)
{
    char host_name[SM_NODE_NAME_MAX_CHAR];
    SmErrorT error = sm_node_utils_get_hostname( host_name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        strncpy(host_name, "error:unknown", SM_NODE_NAME_MAX_CHAR);
    }

    char service_domain_name[] = "controller";
    fprintf( fp, "Service Domain %s\n\n", service_domain_name );

    fprintf( fp, "  local host: %s\n", host_name );
    fprintf( fp, ".........................\n" );
    SmServiceDomainT* domain;
    domain = sm_service_domain_table_read( service_domain_name );
    fprintf( fp, "   state: %s ", sm_service_domain_state_str( domain->state ));

//    fprintf( fp, "    neighbor: %s\n", host_name );
//    fprintf( fp, ".........................\n" );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - Initialize
// ===============================
SmErrorT sm_service_domain_fsm_initialize( void )
{
    SmErrorT error;

    error = sm_service_domain_initial_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain initial state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_waiting_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain waiting state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_other_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain other state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_backup_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain backup state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_leader_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain leader state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    _msg_callbacks.hello = sm_service_domain_fsm_hello_msg_callback;

    error = sm_msg_register_callbacks( &_msg_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register message callbacks, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_neighbor_fsm_register_callback(
                        sm_service_domain_fsm_neighbor_state_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for neighbor state callbacks, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - Finalize
// =============================
SmErrorT sm_service_domain_fsm_finalize( void )
{
    SmErrorT error;

    error = sm_service_domain_neighbor_fsm_deregister_callback(
                        sm_service_domain_fsm_neighbor_state_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister for neighbor state callbacks, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_msg_deregister_callbacks( &_msg_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister message callbacks, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_domain_initial_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain initial state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_waiting_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain waiting state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_other_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain other state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_backup_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain backup state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_leader_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain leader state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
