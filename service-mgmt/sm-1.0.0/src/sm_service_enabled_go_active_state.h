//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_ENABLED_GO_ACTIVE_STATE_H__
#define __SM_SERVICE_ENABLED_GO_ACTIVE_STATE_H__

#include "sm_types.h"
#include "sm_service_table.h"
#include "sm_service_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Enabled Go-Active State - Entry
// =======================================
extern SmErrorT sm_service_enabled_go_active_state_entry(
    SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Active State - Exit
// ======================================
extern SmErrorT sm_service_enabled_go_active_state_exit(
    SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Active State - Transition
// ============================================
extern SmErrorT sm_service_enabled_go_active_state_transition(
    SmServiceT* service, SmServiceStateT from_state );
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Active State - Event Handler
// ===============================================
extern SmErrorT sm_service_enabled_go_active_state_event_handler(
    SmServiceT* service, SmServiceEventT event, void* event_data[] );
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Active State - Initialize
// ============================================
extern SmErrorT sm_service_enabled_go_active_state_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Enabled Go-Active State - Finalize
// ==========================================
extern SmErrorT sm_service_enabled_go_active_state_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_ENABLED_GO_ACTIVE_STATE_H__
