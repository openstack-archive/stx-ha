//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_ENABLING_THROTTLE_STATE_H__
#define __SM_SERVICE_ENABLING_THROTTLE_STATE_H__

#include "sm_types.h"
#include "sm_service_table.h"
#include "sm_service_fsm.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Enabling Throttle State - Entry
// ==============================
extern SmErrorT sm_service_enabling_throttle_state_entry( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Enabling Throttle State - Exit
// =============================
extern SmErrorT sm_service_enabling_throttle_state_exit( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Enabling Throttle State - Transition
// ===================================
extern SmErrorT sm_service_enabling_throttle_state_transition( SmServiceT* service,
    SmServiceStateT from_state );
// ****************************************************************************

// ****************************************************************************
// Service Enabling Throttle State - Event Handler
// ======================================
extern SmErrorT sm_service_enabling_throttle_state_event_handler( SmServiceT* service,
    SmServiceEventT event, void* event_data[] );
// ****************************************************************************

// ****************************************************************************
// Service Enabling Throttle State - Initialize
// ===================================
extern SmErrorT sm_service_enabling_throttle_state_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Enabling Throttle State - Finalize
// ==================================
extern SmErrorT sm_service_enabling_throttle_state_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_ENABLING_THROTTLE_STATE_H__
