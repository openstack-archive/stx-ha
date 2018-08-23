//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_fsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_time.h"
#include "sm_list.h"
#include "sm_selobj.h"
#include "sm_service_group_table.h"
#include "sm_service_group_member_table.h"
#include "sm_service_table.h"
#include "sm_service_group_engine.h"
#include "sm_service_group_initial_state.h"
#include "sm_service_group_active_state.h"
#include "sm_service_group_go_standby_state.h"
#include "sm_service_group_go_active_state.h"
#include "sm_service_group_standby_state.h"
#include "sm_service_group_disabling_state.h"
#include "sm_service_group_disabled_state.h"
#include "sm_service_group_shutdown_state.h"
#include "sm_service_group_enable.h"
#include "sm_service_group_go_active.h"
#include "sm_service_group_go_standby.h"
#include "sm_service_group_disable.h"
#include "sm_service_group_audit.h"
#include "sm_service_api.h"
#include "sm_log.h"
#include "sm_node_utils.h"
#include "sm_node_swact_monitor.h"
#include "sm_failover_utils.h"
#include "sm_swact_state.h"

static SmListT* _callbacks = NULL;

// ****************************************************************************
// Service Group FSM - Register Callback
// =====================================
SmErrorT sm_service_group_fsm_register_callback(
    SmServiceGroupFsmCallbackT callback )
{
    SM_LIST_PREPEND( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Deregister Callback
// =======================================
SmErrorT sm_service_group_fsm_deregister_callback(
    SmServiceGroupFsmCallbackT callback )
{
    SM_LIST_REMOVE( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Notify
// ==========================
static void sm_service_group_fsm_notify( SmServiceGroupT* service_group,
    const char reason_text[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceGroupFsmCallbackT callback;

    SM_LIST_FOREACH( _callbacks, entry, entry_data )
    {
        callback = (SmServiceGroupFsmCallbackT) entry_data;

        callback( service_group->name, service_group->state,
                  service_group->status, service_group->condition,
                  service_group->health, reason_text );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Update Service Group Member
// ===============================================
static void sm_service_group_fsm_update_service_group_member(
    void* user_data[], SmServiceGroupMemberT* service_group_member )
{
    SmServiceT* service;

    service = sm_service_table_read( service_group_member->service_name );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service group member (%s), error=%s.",
                  service_group_member->service_name, 
                  sm_error_str(SM_NOT_FOUND) );
        return;
    }

    if(( service->state != service_group_member->service_state )||
       ( service->status != service_group_member->service_status ))
    {
        service_group_member->service_state = service->state;
        service_group_member->service_status = service->status;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Update Service Group Members
// ================================================
static SmErrorT sm_service_group_fsm_update_service_group_members( void )
{
    sm_service_group_member_table_foreach( NULL, 
                            sm_service_group_fsm_update_service_group_member );
    return( SM_OKAY );
}
// ****************************************************************************

static void sm_service_group_state_check( void* user_data[],
    SmServiceGroupT* service_group )
{
    bool *all_good = (bool*)user_data[0];
    if( service_group->desired_state != service_group->state )
    {
        *all_good = false;
    }
}

// ****************************************************************************
// Service Group FSM - Enter State
// ===============================
static SmErrorT sm_service_group_fsm_enter_state( SmServiceGroupT* service_group ) 
{
    SmErrorT error;

    switch( service_group->state )
    {
        case SM_SERVICE_GROUP_STATE_INITIAL:
            error = sm_service_group_initial_state_entry( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to enter state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_ACTIVE:
            error = sm_service_group_active_state_entry( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to enter state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_GO_ACTIVE:
            error = sm_service_group_go_active_state_entry( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to enter state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_GO_STANDBY:
            error = sm_service_group_go_standby_state_entry( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to enter state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_STANDBY:
            error = sm_service_group_standby_state_entry( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to enter state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_DISABLING:
            error = sm_service_group_disabling_state_entry( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to enter state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_DISABLED:
            // This is a workaround for lack of sm message that
            // signifies the "end-of-swact" state on both controller nodes.
            //
            // As all service groups have to transition to disabled state
            // during swact regardless of the outcome (success or failure), it
            // is a suitable spot to reaffine tasks back to the platform cores.
            // Controller services group is often the last group to reach 
            // any state. Task reaffining is only applicable to AIO duplex. 
            bool duplex;
            sm_node_utils_is_aio_duplex(&duplex);
            if (( duplex ) && ( 0 == strcmp( "controller-services", service_group->name )))
            {
                DPRINTFI( "Reaffining tasks back to platform cores..." );
		sm_set_swact_state(SM_SWACT_STATE_END);
            }
            error = sm_service_group_disabled_state_entry( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to enter state (%s), "
                          "error=%s.", service_group->name, 
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_SHUTDOWN:
            error = sm_service_group_shutdown_state_entry( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to enter state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown service group (%s) state (%s).",
                      service_group->name,
                      sm_service_group_state_str( service_group->state ) );
        break;
    }

    if( SM_SERVICE_GROUP_STATE_STANDBY == service_group->state ||
        SM_SERVICE_GROUP_STATE_ACTIVE == service_group->state )
    {
        bool all_good = true;
        void* user_data[] = {&all_good};
        sm_service_group_table_foreach( user_data, sm_service_group_state_check );
        if( all_good )
        {
            char hostname[SM_NODE_NAME_MAX_CHAR];
            error = sm_node_utils_get_hostname( hostname );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to get hostname, error=%s.",
                          sm_error_str( error ) );
                hostname[0] = '\0';
                return error;
            }

            SmNodeScheduleStateT controller_state = sm_get_controller_state(hostname);
            if( SM_NODE_STATE_ACTIVE == controller_state )
            {
                SmNodeSwactMonitor::SwactUpdate(hostname, SM_NODE_STATE_ACTIVE );
            }
            else if ( SM_NODE_STATE_STANDBY == controller_state )
            {
                SmNodeSwactMonitor::SwactUpdate(hostname, SM_NODE_STATE_STANDBY );
            }else
            {
                SmNodeSwactMonitor::SwactUpdate(hostname, SM_NODE_STATE_FAILED );
            }
        }
    }
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Exit State
// ==============================
static SmErrorT sm_service_group_fsm_exit_state( SmServiceGroupT* service_group )
{
    SmErrorT error;

    switch( service_group->state )
    {
        case SM_SERVICE_GROUP_STATE_INITIAL:
            error = sm_service_group_initial_state_exit( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to exit state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_ACTIVE:
            error = sm_service_group_active_state_exit( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to exit state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_GO_ACTIVE:
            error = sm_service_group_go_active_state_exit( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to exit state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_GO_STANDBY:
            error = sm_service_group_go_standby_state_exit( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to exit state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_STANDBY:
            error = sm_service_group_standby_state_exit( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to exit state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_DISABLING:
            error = sm_service_group_disabling_state_exit( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to exit state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_DISABLED:
            error = sm_service_group_disabled_state_exit( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to exit state (%s), "
                          "error=%s.", service_group->name, 
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_SHUTDOWN:
            error = sm_service_group_shutdown_state_exit( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to exit state (%s), "
                          "error=%s.", service_group->name,
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown service group (%s) state (%s).",
                      service_group->name,
                      sm_service_group_state_str( service_group->state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Transition State
// ====================================
static SmErrorT sm_service_group_fsm_transition_state( 
    SmServiceGroupT* service_group, SmServiceGroupStateT from_state ) 
{
    SmErrorT error;

    switch( service_group->state )
    {
        case SM_SERVICE_GROUP_STATE_INITIAL:
            error = sm_service_group_initial_state_transition( service_group,
                                                               from_state  );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.", 
                          service_group->name,
                          sm_service_group_state_str( from_state ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_ACTIVE:
            error = sm_service_group_active_state_transition( service_group,
                                                              from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.", 
                          service_group->name,
                          sm_service_group_state_str( from_state ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_GO_ACTIVE:
            error = sm_service_group_go_active_state_transition( service_group,
                                                                 from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.", 
                          service_group->name,
                          sm_service_group_state_str( from_state ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_GO_STANDBY:
            error = sm_service_group_go_standby_state_transition( service_group,
                                                                  from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.", 
                          service_group->name,
                          sm_service_group_state_str( from_state ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_STANDBY:
            error = sm_service_group_standby_state_transition( service_group,
                                                               from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.", 
                          service_group->name,
                          sm_service_group_state_str( from_state ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_DISABLING:
            error = sm_service_group_disabling_state_transition( service_group,
                                                                 from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.", 
                          service_group->name,
                          sm_service_group_state_str( from_state ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_DISABLED:
            error = sm_service_group_disabled_state_transition( service_group,
                                                                from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.", 
                          service_group->name,
                          sm_service_group_state_str( from_state ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_SHUTDOWN:
            error = sm_service_group_shutdown_state_transition( service_group,
                                                                from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to transition from "
                          "state (%s) to state (%s), error=%s.", 
                          service_group->name,
                          sm_service_group_state_str( from_state ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown service group (%s) state (%s).",
                      service_group->name,
                      sm_service_group_state_str( service_group->state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Set State
// =============================
SmErrorT sm_service_group_fsm_set_state( char service_group_name[],
    SmServiceGroupStateT state ) 
{
    SmServiceGroupStateT prev_state;
    SmServiceGroupT* service_group;
    SmErrorT error, error2;

    service_group = sm_service_group_table_read( service_group_name );
    if( NULL == service_group )
    {
        DPRINTFE( "Failed to read service group (%s), error=%s.",
                  service_group_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    prev_state = service_group->state;

    error = sm_service_group_fsm_exit_state( service_group ); 
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to exit state (%s) service group (%s), error=%s.",
                  sm_service_group_state_str( service_group->state ),
                  service_group_name, sm_error_str( error ) );
        return( error );
    }

    service_group->state = state;

    error = sm_service_group_fsm_transition_state( service_group, 
                                                   prev_state );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to transition to state (%s) service group (%s), "
                  "error=%s.", sm_service_group_state_str( service_group->state ),
                  service_group_name, sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }

    error = sm_service_group_fsm_enter_state( service_group ); 
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to enter state (%s) service group (%s), error=%s.",
                  sm_service_group_state_str( service_group->state ),
                  service_group_name, sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }


    return( SM_OKAY );

STATE_CHANGE_ERROR:
    error2 = sm_service_group_fsm_exit_state( service_group ); 
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to exit state (%s) service group (%s), error=%s.",
                  sm_service_group_state_str( service_group->state ),
                  service_group_name, sm_error_str( error2 ) );
        abort();
    }

    service_group->state = prev_state;

    error2 = sm_service_group_fsm_transition_state( service_group, state );
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to transition to state (%s) service group (%s), "
                  "error=%s.", sm_service_group_state_str( service_group->state ),
                  service_group_name, sm_error_str( error2 ) );
        abort();
    }

    error2 = sm_service_group_fsm_enter_state( service_group ); 
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to enter state (%s) service group (%s), error=%s.",
                  sm_service_group_state_str( service_group->state ),
                  service_group_name, sm_error_str( error2 ) );
        abort();
    }

    return( error );
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Get Next State
// ==================================
SmErrorT sm_service_group_fsm_get_next_state( char service_group_name[],
    SmServiceGroupEventT event, SmServiceGroupStateT* state )
{
    bool complete;
    SmServiceGroupT* service_group;
    SmErrorT error;

    service_group = sm_service_group_table_read( service_group_name );
    if( NULL == service_group )
    {
        DPRINTFE( "Failed to read service group (%s), error=%s.",
                  service_group_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    error = sm_service_group_fsm_update_service_group_members();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update service group (%s) members, "
                  "error=%s.", service_group->name, 
                  sm_error_str( error ) );
        return( error );
    }

    switch( event )
    {
        case SM_SERVICE_GROUP_EVENT_GO_ACTIVE:
            error = sm_service_group_go_active_complete( service_group,
                                                         &complete );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to determine if go-active complete on "
                          "service group (%s), error=%s", 
                          service_group->name, sm_error_str( error ) );
                return( error );
            }

            if( complete )
            {
                *state = SM_SERVICE_GROUP_STATE_ACTIVE;
            } else {
                *state = SM_SERVICE_GROUP_STATE_GO_ACTIVE;
            }
        break;

        case SM_SERVICE_GROUP_EVENT_GO_STANDBY:
            error = sm_service_group_go_standby_complete( service_group,
                                                          &complete );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to determine if go-standby complete on "
                          "service group (%s), error=%s", 
                          service_group->name, sm_error_str( error ) );
                return( error );
            }

            if( complete )
            {
                *state = SM_SERVICE_GROUP_STATE_STANDBY;
            } else {
                *state = SM_SERVICE_GROUP_STATE_GO_STANDBY;
            }
        break;

        case SM_SERVICE_GROUP_EVENT_DISABLE:
            error = sm_service_group_disable_complete( service_group,
                                                       &complete );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to determine if disable complete on "
                          "service group (%s), error=%s",
                          service_group->name, sm_error_str( error ) );
                return( error );
            }

            if( complete )
            {
                *state = SM_SERVICE_GROUP_STATE_DISABLED;
            } else {
                *state = SM_SERVICE_GROUP_STATE_DISABLING;
            }
        break;

        default:
            // Ignore.
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Event Handler
// =================================
SmErrorT sm_service_group_fsm_event_handler( char service_group_name[], 
    SmServiceGroupEventT event, void* event_data[], const char reason_text[] )
{
    SmServiceGroupStateT prev_state;
    SmServiceGroupStatusT prev_status;
    SmServiceGroupConditionT prev_condition;
    char prev_reason_text[SM_SERVICE_GROUP_REASON_TEXT_MAX_CHAR];
    SmServiceGroupT* service_group;
    SmErrorT error;

    service_group = sm_service_group_table_read( service_group_name );
    if( NULL == service_group )
    {
        DPRINTFE( "Failed to read service group (%s), error=%s.",
                  service_group_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    prev_state = service_group->state;
    prev_status = service_group->status;
    prev_condition = service_group->condition;

    snprintf( prev_reason_text, sizeof(prev_reason_text), "%s",
              service_group->reason_text );

    switch( service_group->state )
    {
        case SM_SERVICE_GROUP_STATE_INITIAL:
            error = sm_service_group_initial_state_event_handler(
                                        service_group, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_group_name,
                          sm_service_group_event_str( event ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_ACTIVE:
            error = sm_service_group_active_state_event_handler(
                                        service_group, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_group_name,
                          sm_service_group_event_str( event ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_GO_ACTIVE:
            error = sm_service_group_go_active_state_event_handler(
                                        service_group, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_group_name,
                          sm_service_group_event_str( event ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_GO_STANDBY:
            error = sm_service_group_go_standby_state_event_handler(
                                        service_group, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_group_name,
                          sm_service_group_event_str( event ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_STANDBY:
            error = sm_service_group_standby_state_event_handler(
                                        service_group, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_group_name,
                          sm_service_group_event_str( event ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_DISABLING:
            error = sm_service_group_disabling_state_event_handler(
                                        service_group, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_group_name,
                          sm_service_group_event_str( event ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_DISABLED:
            error = sm_service_group_disabled_state_event_handler(
                                        service_group, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_group_name,
                          sm_service_group_event_str( event ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_STATE_SHUTDOWN:
            error = sm_service_group_shutdown_state_event_handler(
                                        service_group, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service group (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", service_group_name,
                          sm_service_group_event_str( event ),
                          sm_service_group_state_str( service_group->state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown state (%s) for service group (%s).",
                      sm_service_group_state_str( service_group->state ),
                      service_group_name );
        break;
    }

    if(( prev_state != service_group->state )||
       ( prev_status != service_group->status )||
       ( prev_condition != service_group->condition ))
    {
        DPRINTFI( "Service group (%s) was in the %s%s%s state and is now "
                  "in the %s%s%s state%s%s.",
                  service_group_name,
                  sm_service_group_state_str( prev_state ),
                  SM_SERVICE_GROUP_STATUS_NONE == prev_status
                  ? "" : "-",
                  SM_SERVICE_GROUP_STATUS_NONE == prev_status
                  ? "" : sm_service_group_status_str(prev_status),
                  sm_service_group_state_str( service_group->state ),
                  SM_SERVICE_GROUP_STATUS_NONE == service_group->status
                  ? "" : "-",
                  SM_SERVICE_GROUP_STATUS_NONE == service_group->status
                  ? "" : sm_service_group_status_str(service_group->status),
                  SM_SERVICE_GROUP_CONDITION_NONE == service_group->condition
                  ? "" : ", condition=",
                  SM_SERVICE_GROUP_CONDITION_NONE == service_group->condition
                  ? "" : sm_service_group_condition_str( service_group->condition ) );

        error = sm_service_group_table_persist( service_group );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service group (%s) data, error=%s.",
                      service_group->name, sm_error_str(error) );
            return( error );
        }

        if(( prev_state != service_group->state )||
           ( prev_status != service_group->status ))
        { 
            sm_log_service_group_state_change( service_group_name,
                             sm_service_group_state_str( prev_state ),
                             sm_service_group_status_str( prev_status ),
                             sm_service_group_state_str( service_group->state ),
                             sm_service_group_status_str( service_group->status ),
                             service_group->reason_text );
        }

        sm_service_group_fsm_notify( service_group,
                                     service_group->reason_text );

    } else if( SM_SERVICE_GROUP_EVENT_AUDIT == event ) {
        if(( SM_SERVICE_GROUP_STATE_ACTIVE   == service_group->state )||
           ( SM_SERVICE_GROUP_STATE_STANDBY  == service_group->state )||
           ( SM_SERVICE_GROUP_STATE_DISABLED == service_group->state ))
        {
            sm_service_group_fsm_notify( service_group, 
                                         service_group->reason_text );
        }

    } else if( 0 != strcmp( prev_reason_text, service_group->reason_text ) ) {

        sm_service_group_fsm_notify( service_group, 
                                     service_group->reason_text );
    }

    if( service_group->desired_state != service_group->state )
    {
        error = sm_service_group_engine_signal( service_group );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to signal service group (%s), error=%s.",
                      service_group_name, sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Recover Member
// ==================================
static void sm_service_group_fsm_recover_member( void* user_data[],
    SmServiceGroupMemberT* service_group_member )
{
    bool clear_fatal_condition  = *(bool*) user_data[0];
    SmErrorT error;
    
    error = sm_service_api_recover( service_group_member->service_name,
                                    clear_fatal_condition );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to recover member (%s), error=%s.",
                  service_group_member->service_name,
                  sm_error_str( error ) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Recover
// ===========================
SmErrorT sm_service_group_fsm_recover( char service_group_name[],
    bool clear_fatal_condition )
{
    void* user_data[] = {&clear_fatal_condition};

    sm_service_group_member_table_foreach_member( service_group_name, user_data,
                                        sm_service_group_fsm_recover_member);
    
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Service State Change Notification
// =====================================================
static void sm_service_group_fsm_service_scn( char service_name[],
    SmServiceStateT state, SmServiceStatusT status,
    SmServiceConditionT condition )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceGroupMemberT* service_group_member;
    void* event_data[SM_SERVICE_GROUP_EVENT_DATA_MAX] = {0};    
    SmErrorT error;

    event_data[SM_SERVICE_GROUP_EVENT_DATA_SERVICE_NAME] = service_name;
    event_data[SM_SERVICE_GROUP_EVENT_DATA_SERVICE_STATE] = &state;

    service_group_member = sm_service_group_member_table_read_by_service(
                                                                service_name );
    if( NULL == service_group_member )
    {
        DPRINTFE( "Failed to query service group member by service (%s), "
                  "error=%s.", service_name, sm_error_str(SM_NOT_FOUND) );
        return;
    }

    service_group_member->service_state = state;
    service_group_member->service_status = status;
    service_group_member->service_condition = condition;

    if( SM_SERVICE_STATUS_FAILED == status )
    {
        service_group_member->service_failure_timestamp
                                    = sm_time_get_elapsed_ms( NULL );
    }

    if( SM_SERVICE_STATUS_NONE == status )
    {
        snprintf( reason_text, sizeof(reason_text), "%s service state "
                  "change, state=%s", service_name, sm_service_state_str(state) );
    } else {
        if( SM_SERVICE_CONDITION_NONE == condition )
        {
            snprintf( reason_text, sizeof(reason_text), "%s service state "
                      "change, state=%s, status=%s", service_name,
                      sm_service_state_str(state),
                      sm_service_status_str(status) );
        } else {
            snprintf( reason_text, sizeof(reason_text), "%s service state "
                      "change, state=%s, status=%s, condition=%s",
                      service_name, sm_service_state_str(state),
                      sm_service_status_str(status),
                      sm_service_condition_str(condition) );
        }
    }

    error = sm_service_group_fsm_event_handler( service_group_member->name,
                                    SM_SERVICE_GROUP_EVENT_SERVICE_SCN,
                                    event_data, reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service (%s) scn not handled for service group (%s), "
                  "error=%s.", service_group_member->service_name,
                  service_group_member->name, sm_error_str( error ) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Initialize
// ==============================
SmErrorT sm_service_group_fsm_initialize( void )
{
    SmErrorT error;

    error = sm_service_group_initial_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group initial state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_active_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group active state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_go_active_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group go-active state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_go_standby_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group go-standby "
                  "state module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_standby_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group standby state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_disabling_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group disabling state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_disabled_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group disabled state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_shutdown_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group shutdown state "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_enable_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group enable module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_go_active_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group go-active module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_go_standby_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group go-standby module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_disable_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group disable module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_audit_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group audit module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_api_register_callback( sm_service_group_fsm_service_scn );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for service state changes, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Finalize
// ============================
SmErrorT sm_service_group_fsm_finalize( void )
{
    SmErrorT error;

    error = sm_service_api_deregister_callback( sm_service_group_fsm_service_scn );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister for service state changes, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_initial_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group initial state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_active_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group active state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_go_active_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group go-active state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_go_standby_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group go-standby state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_standby_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group standby state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_disabling_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group disabling state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_disabled_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group disabled state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_shutdown_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group shutdown state "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_enable_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group enable module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_go_active_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group go-active module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_go_standby_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group go-standby module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_disable_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group disable module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_audit_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group audit module, "
                  "error=%s.", sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
