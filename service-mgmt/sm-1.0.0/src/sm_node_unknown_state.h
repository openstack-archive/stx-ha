//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_NODE_UNKNOWN_STATE_H__
#define __SM_NODE_UNKNOWN_STATE_H__

#include "sm_types.h"
#include "sm_db.h"
#include "sm_db_nodes.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Node Unknown State - Entry
// ==========================
extern SmErrorT sm_node_unknown_state_entry( SmDbNodeT* node );
// ****************************************************************************

// ****************************************************************************
// Node Unknown State - Exit
// =========================
extern SmErrorT sm_node_unknown_state_exit( SmDbNodeT* node );
// ****************************************************************************

// ****************************************************************************
// Node Unknown State - Transition
// ===============================
extern SmErrorT sm_node_unknown_state_transition( SmDbNodeT* node,
    SmNodeReadyStateT from_state );
// ****************************************************************************

// ****************************************************************************
// Node Unknown State - Event Handler
// ==================================
extern SmErrorT sm_node_unknown_state_event_handler( SmDbNodeT* node,
    SmNodeEventT event, void* event_data[] );
// ****************************************************************************

// ****************************************************************************
// Node Unknown State - Initialize
// ===============================
extern SmErrorT sm_node_unknown_state_initialize( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Node Unknown State - Finalize
// =============================
extern SmErrorT sm_node_unknown_state_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_NODE_UNKNOWN_STATE_H__
