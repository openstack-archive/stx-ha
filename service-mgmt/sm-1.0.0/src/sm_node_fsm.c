//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_node_fsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_nodes.h"
#include "sm_node_unknown_state.h"
#include "sm_node_enabled_state.h"
#include "sm_node_disabled_state.h"
#include "sm_log.h"

static SmDbHandleT* _sm_db_handle = NULL;
static char _reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
static SmListT* _callbacks = NULL;

// ****************************************************************************
// Node FSM - Register Callback
// ============================
SmErrorT sm_node_fsm_register_callback( SmNodeFsmCallbackT callback )
{
    SM_LIST_PREPEND( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node FSM - Deregister Callback
// ==============================
SmErrorT sm_node_fsm_deregister_callback( SmNodeFsmCallbackT callback )
{
    SM_LIST_REMOVE( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node FSM - Notify
// =================
static void sm_node_fsm_notify( SmDbNodeT* node )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmNodeFsmCallbackT callback;

    SM_LIST_FOREACH( _callbacks, entry, entry_data )
    {
        callback = (SmNodeFsmCallbackT) entry_data;

        callback( node->name, node->ready_state );
    }
}
// ****************************************************************************

// ****************************************************************************
// Node FSM - Enter State
// ======================
static SmErrorT sm_node_fsm_enter_state( SmDbNodeT* node ) 
{
    SmErrorT error;

    switch( node->ready_state )
    {
        case SM_NODE_READY_STATE_UNKNOWN:
            error = sm_node_unknown_state_entry( node );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to enter state (%s), error=%s.",
                          node->name,
                          sm_node_ready_state_str( node->ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_NODE_READY_STATE_ENABLED:
            error = sm_node_enabled_state_entry( node );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to enter state (%s), error=%s.",
                          node->name,
                          sm_node_ready_state_str( node->ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_NODE_READY_STATE_DISABLED:
            error = sm_node_disabled_state_entry( node );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to enter state (%s), error=%s.",
                          node->name,
                          sm_node_ready_state_str( node->ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown node (%s) state (%s).", node->name,
                      sm_node_ready_state_str( node->ready_state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node FSM - Exit State
// =====================
static SmErrorT sm_node_fsm_exit_state( SmDbNodeT* node )
{
    SmErrorT error;

    switch( node->ready_state )
    {
        case SM_NODE_READY_STATE_UNKNOWN:
            error = sm_node_unknown_state_exit( node );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to exit state (%s), error=%s.",
                          node->name, 
                          sm_node_ready_state_str( node->ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_NODE_READY_STATE_ENABLED:
            error = sm_node_enabled_state_exit( node );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to exit state (%s), error=%s.",
                          node->name,
                          sm_node_ready_state_str( node->ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_NODE_READY_STATE_DISABLED:
            error = sm_node_disabled_state_exit( node );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to exit state (%s), error=%s.",
                          node->name,
                          sm_node_ready_state_str( node->ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown node (%s) state (%s).", node->name,
                      sm_node_ready_state_str( node->ready_state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node FSM - Transition State
// ===========================
static SmErrorT sm_node_fsm_transition_state( SmDbNodeT* node,
    SmNodeReadyStateT from_state )
{
    SmErrorT error;

    switch( node->ready_state )
    {
        case SM_NODE_READY_STATE_UNKNOWN:
            error = sm_node_unknown_state_transition( node, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", node->name,
                          sm_node_ready_state_str( from_state ),
                          sm_node_ready_state_str( node->ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_NODE_READY_STATE_ENABLED:
            error = sm_node_enabled_state_transition( node, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", node->name,
                          sm_node_ready_state_str( from_state ),
                          sm_node_ready_state_str( node->ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_NODE_READY_STATE_DISABLED:
            error = sm_node_disabled_state_transition( node, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to transition from state (%s) "
                          "to state (%s), error=%s.", node->name,
                          sm_node_ready_state_str( from_state ),
                          sm_node_ready_state_str( node->ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown node (%s) state (%s).", node->name,
                      sm_node_ready_state_str( node->ready_state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node FSM - Set State
// ====================
SmErrorT sm_node_fsm_set_state( char node_name[], SmNodeReadyStateT state,
    const char reason_text[] ) 
{
    SmNodeReadyStateT prev_state;
    SmDbNodeT node;
    SmErrorT error, error2;

    error = sm_db_nodes_read( _sm_db_handle, node_name, &node );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to read node (%s), error=%s.", node_name,
                  sm_error_str( error ) );
        return( error );
    }

    prev_state = node.ready_state;

    error = sm_node_fsm_exit_state( &node );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to exit state (%s) node (%s), error=%s.",
                  sm_node_ready_state_str( node.ready_state ), node.name,
                  sm_error_str( error ) );
        return( error );
    }

    node.ready_state = state;

    error = sm_db_nodes_update( _sm_db_handle, &node );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update node  (%s), error=%s.",
                  node.name, sm_error_str( error ) );
        return( error );
    }

    error = sm_node_fsm_transition_state( &node, prev_state );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to transition to state (%s) node (%s), "
                  "error=%s.", sm_node_ready_state_str( node.ready_state ),
                  node.name, sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }

    error = sm_node_fsm_enter_state( &node );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to enter state (%s) node (%s), error=%s.",
                  sm_node_ready_state_str( node.ready_state ), node.name,
                  sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }

    if( '\0' != reason_text[0] )
    {
        snprintf( _reason_text, sizeof(_reason_text), "%s", reason_text );
    }

    return( SM_OKAY );

STATE_CHANGE_ERROR:
    error2 = sm_node_fsm_exit_state( &node ); 
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to exit state (%s) node (%s), error=%s.",
                  sm_node_ready_state_str( node.ready_state ),
                  node.name, sm_error_str( error2 ) );
        abort();
    }

    node.ready_state = prev_state;

    error2 = sm_db_nodes_update( _sm_db_handle, &node );
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to update node (%s), error=%s.",
                  node.name, sm_error_str( error2 ) );
        abort();
    }

    error2 = sm_node_fsm_transition_state( &node, state );
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to transition to state (%s) node (%s), "
                  "error=%s.", sm_node_ready_state_str( node.ready_state ),
                  node.name, sm_error_str( error2 ) );
        abort();
    }

    error2 = sm_node_fsm_enter_state( &node ); 
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to enter state (%s) node (%s), error=%s.",
                  sm_node_ready_state_str( node.ready_state ), node.name,
                  sm_error_str( error2 ) );
        abort();
    }

    return( error );
}
// ****************************************************************************

// ****************************************************************************
// Node FSM - Event Handler
// ========================
SmErrorT sm_node_fsm_event_handler( char node_name[], SmNodeEventT event,
    void* event_data[], const char reason_text[] )
{
    SmNodeReadyStateT prev_state;
    SmDbNodeT node;
    SmErrorT error;

    snprintf( _reason_text, sizeof(_reason_text), "%s", reason_text );

    error = sm_db_nodes_read( _sm_db_handle, node_name, &node );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to read node (%s), error=%s.", node_name,
                  sm_error_str( error ) );
        return( error );
    }

    prev_state = node.ready_state;

    switch( node.ready_state )
    {
        case SM_NODE_READY_STATE_UNKNOWN:
            error = sm_node_unknown_state_event_handler( &node, event,
                                                         event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", node.name,
                          sm_node_event_str( event ),
                          sm_node_ready_state_str( node.ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_NODE_READY_STATE_ENABLED:
            error = sm_node_enabled_state_event_handler( &node, event,
                                                         event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", node.name,
                          sm_node_event_str( event ),
                          sm_node_ready_state_str( node.ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_NODE_READY_STATE_DISABLED:
            error = sm_node_disabled_state_event_handler( &node, event,
                                                          event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Node (%s) unable to handle event (%s) "
                          "in state (%s), error=%s.", node.name,
                          sm_node_event_str( event ),
                          sm_node_ready_state_str( node.ready_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown state (%s) for node (%s).",
                      sm_node_ready_state_str( node.ready_state ),
                      node.name );
        break;
    }

    error = sm_db_nodes_read( _sm_db_handle, node_name, &node );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to read node (%s), error=%s.", node_name,
                  sm_error_str( error ) );
        return( error );
    }

    if( prev_state != node.ready_state )
    {
        DPRINTFI( "Node (%s) received event (%s) was in the %s state and "
                  "is now in the %s.", node.name, sm_node_event_str( event ),
                  sm_node_ready_state_str( prev_state ),
                  sm_node_ready_state_str( node.ready_state ) );

        sm_log_node_state_change( node_name,
            sm_node_state_str( node.admin_state, prev_state ),
            sm_node_state_str( node.admin_state, node.ready_state ),
            _reason_text );

        sm_node_fsm_notify( &node );

    } else if( SM_NODE_EVENT_AUDIT == event ) {
        sm_node_fsm_notify( &node );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node FSM - Initialize
// =====================
SmErrorT sm_node_fsm_initialize( void )
{
    SmErrorT error;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_node_unknown_state_initialize( _sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize node unknown state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_node_enabled_state_initialize( _sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize node enabled state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_node_disabled_state_initialize( _sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize node disabled state module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node FSM - Finalize
// ===================
SmErrorT sm_node_fsm_finalize( void )
{
    SmErrorT error;

    error = sm_node_unknown_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize node unknown state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_node_enabled_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize node enabled state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_node_disabled_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize node disabled state module, "
                  "error=%s.", sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
