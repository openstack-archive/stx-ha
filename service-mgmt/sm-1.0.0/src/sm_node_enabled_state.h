//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_NODE_ENABLED_STATE_H__
#define __SM_NODE_ENABLED_STATE_H__

#include "sm_types.h"
#include "sm_db.h"
#include "sm_db_nodes.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Node Enabled State - Entry
// ==========================
extern SmErrorT sm_node_enabled_state_entry( SmDbNodeT* node );
// ****************************************************************************

// ****************************************************************************
// Node Enabled State - Exit
// =========================
extern SmErrorT sm_node_enabled_state_exit( SmDbNodeT* node );
// ****************************************************************************

// ****************************************************************************
// Node Enabled State - Transition
// ===============================
extern SmErrorT sm_node_enabled_state_transition( SmDbNodeT* node,
    SmNodeReadyStateT from_state );
// ****************************************************************************

// ****************************************************************************
// Node Enabled State - Event Handler
// ==================================
extern SmErrorT sm_node_enabled_state_event_handler( SmDbNodeT* node,
    SmNodeEventT event, void* event_data[] );
// ****************************************************************************

// ****************************************************************************
// Node Enabled State - Initialize
// ===============================
extern SmErrorT sm_node_enabled_state_initialize( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Node Enabled State - Finalize
// =============================
extern SmErrorT sm_node_enabled_state_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_NODE_ENABLED_STATE_H__
