//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_NODE_FSM_H__
#define __SM_NODE_FSM_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmNodeFsmCallbackT) (char node_name[],
        SmNodeReadyStateT ready_state );

// ****************************************************************************
// Node FSM - Register Callback
// ============================
extern SmErrorT sm_node_fsm_register_callback( SmNodeFsmCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Node FSM - Deregister Callback
// ==============================
extern SmErrorT sm_node_fsm_deregister_callback( SmNodeFsmCallbackT callback );
// ****************************************************************************
    
// ****************************************************************************
// Node FSM - Set State
// ====================
extern SmErrorT sm_node_fsm_set_state( char node_name[],
    SmNodeReadyStateT state, const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Node FSM - Event Handler
// ========================
extern SmErrorT sm_node_fsm_event_handler( char node_name[], 
    SmNodeEventT event, void* event_data[], const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Node FSM - Initialize
// =====================
extern SmErrorT sm_node_fsm_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Node FSM - Finalize
// ===================
extern SmErrorT sm_node_fsm_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_NODE_FSM_H__
