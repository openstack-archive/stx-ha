//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_FSM_H__
#define __SM_SERVICE_GROUP_FSM_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmServiceGroupFsmCallbackT) (char service_group_name[],
        SmServiceGroupStateT state,SmServiceGroupStatusT status,
        SmServiceGroupConditionT condition, int64_t health,
        const char reason_text[] );

// ****************************************************************************
// Service Group FSM - Register Callback
// =====================================
extern SmErrorT sm_service_group_fsm_register_callback(
    SmServiceGroupFsmCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Deregister Callback
// =======================================
extern SmErrorT sm_service_group_fsm_deregister_callback(
    SmServiceGroupFsmCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Set State
// =============================
extern SmErrorT sm_service_group_fsm_set_state( char service_group_name[],
    SmServiceGroupStateT state );
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Get Next State
// ==================================
extern SmErrorT sm_service_group_fsm_get_next_state( char service_group_name[],
    SmServiceGroupEventT event, SmServiceGroupStateT* state );
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Event Handler
// =================================
extern SmErrorT sm_service_group_fsm_event_handler( char service_group_name[],
    SmServiceGroupEventT event, void* event_data[], const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Recover
// ===========================
extern SmErrorT sm_service_group_fsm_recover( char service_group_name[],
    bool clear_fatal_condition );
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Initialize
// ==============================
extern SmErrorT sm_service_group_fsm_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group FSM - Finalize
// ============================
extern SmErrorT sm_service_group_fsm_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_FSM_H__
