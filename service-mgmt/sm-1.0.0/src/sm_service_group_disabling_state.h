//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_DISABLING_STATE_H__
#define __SM_SERVICE_GROUP_DISABLING_STATE_H__

#include "sm_types.h"
#include "sm_service_group_table.h"
#include "sm_service_group_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Group Disabling State - Entry
// =====================================
extern SmErrorT sm_service_group_disabling_state_entry(
    SmServiceGroupT* service_group );
// ****************************************************************************

// ****************************************************************************
// Service Group Disabling State - Exit
// ====================================
extern SmErrorT sm_service_group_disabling_state_exit(
    SmServiceGroupT* service_group );
// ****************************************************************************

// ****************************************************************************
// Service Group Disabling State - Transition
// ==========================================
extern SmErrorT sm_service_group_disabling_state_transition(
    SmServiceGroupT* service_group, SmServiceGroupStateT from_state );
// ****************************************************************************

// ****************************************************************************
// Service Group Disabling State - Event Handler
// =============================================
extern SmErrorT sm_service_group_disabling_state_event_handler(
    SmServiceGroupT* service_group, SmServiceGroupEventT event,
    void* event_data[] );
// ****************************************************************************

// ****************************************************************************
// Service Group Disabling State - Initialize
// ==========================================
extern SmErrorT sm_service_group_disabling_state_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Disabling State - Finalize
// ========================================
extern SmErrorT sm_service_group_disabling_state_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_DISABLING_STATE_H__
