//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_node_unknown_state.h"

#include <stdio.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_hw.h"
#include "sm_db.h"
#include "sm_db_nodes.h"
#include "sm_node_utils.h"
#include "sm_node_fsm.h"

static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Node Unknown State - Entry
// ==========================
SmErrorT sm_node_unknown_state_entry( SmDbNodeT* node )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Unknown State - Exit
// =========================
SmErrorT sm_node_unknown_state_exit( SmDbNodeT* node )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Unknown State - Transition
// ===============================
SmErrorT sm_node_unknown_state_transition( SmDbNodeT* node,
    SmNodeReadyStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Unknown State - Event Handler
// ==================================
SmErrorT sm_node_unknown_state_event_handler( SmDbNodeT* node,
    SmNodeEventT event, void* event_data[] )
{
    bool enabled;
    SmNodeReadyStateT state;
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    switch( event )
    {
        case SM_NODE_EVENT_ENABLED:
            error = sm_node_fsm_set_state( node->name,
                                           SM_NODE_READY_STATE_ENABLED,
                                           "node ready" );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Set state (%s) of node (%s) failed, error=%s.",
                          sm_node_ready_state_str( SM_NODE_READY_STATE_ENABLED ),
                          node->name, sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_NODE_EVENT_DISABLED:
            error = sm_node_fsm_set_state( node->name, 
                                           SM_NODE_READY_STATE_DISABLED,
                                           "node not ready" );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Set state (%s) of node (%s) failed, error=%s.",
                          sm_node_ready_state_str( SM_NODE_READY_STATE_DISABLED ),
                          node->name, sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_NODE_EVENT_AUDIT:
            error = sm_node_utils_enabled( &enabled, reason_text );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to determine if node (%s) is enabled, "
                          "error=%s.", node->name, sm_error_str( error ) );
                return( error );
            }

            if( enabled )
            {
                state = SM_NODE_READY_STATE_ENABLED;
            } else {
                state = SM_NODE_READY_STATE_DISABLED;
            }

            error = sm_node_fsm_set_state( node->name, state, reason_text );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Set state (%s) of node (%s) failed, error=%s.",
                          sm_node_ready_state_str( state ), node->name,
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFD( "Node (%s) ignoring event (%s).", node->name,
                      sm_node_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Unknown State - Initialize
// ===============================
SmErrorT sm_node_unknown_state_initialize( SmDbHandleT* sm_db_handle )
{
    _sm_db_handle = sm_db_handle;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Unknown State - Finalize
// =============================
SmErrorT sm_node_unknown_state_finalize( void )
{
    _sm_db_handle = NULL;

    return( SM_OKAY );
}
// ****************************************************************************
