//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_NODE_DISABLED_STATE_H__
#define __SM_NODE_DISABLED_STATE_H__

#include "sm_types.h"
#include "sm_db.h"
#include "sm_db_nodes.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Node Disabled State - Entry
// ===========================
extern SmErrorT sm_node_disabled_state_entry( SmDbNodeT* node );
// ****************************************************************************

// ****************************************************************************
// Node Disabled State - Exit
// ==========================
extern SmErrorT sm_node_disabled_state_exit( SmDbNodeT* node );
// ****************************************************************************

// ****************************************************************************
// Node Disabled State - Transition
// ================================
extern SmErrorT sm_node_disabled_state_transition( SmDbNodeT* node,
    SmNodeReadyStateT from_state );
// ****************************************************************************

// ****************************************************************************
// Node Disabled State - Event Handler
// ===================================
extern SmErrorT sm_node_disabled_state_event_handler( SmDbNodeT* node,
    SmNodeEventT event, void* event_data[] );
// ****************************************************************************

// ****************************************************************************
// Node Disabled State - Initialize
// ================================
extern SmErrorT sm_node_disabled_state_initialize( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Node Disabled State - Finalize
// ==============================
extern SmErrorT sm_node_disabled_state_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_NODE_DISABLED_STATE_H__
