//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_FSM_H__
#define __SM_SERVICE_FSM_H__

#include "sm_types.h"
#include "sm_service_table.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmServiceFsmCallbackT) (char service_name[], 
        SmServiceStateT state, SmServiceStatusT status,
        SmServiceConditionT condition);

// ****************************************************************************
// Service FSM - Register Callback
// ===============================
extern SmErrorT sm_service_fsm_register_callback( SmServiceFsmCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service FSM - Deregister Callback
// =================================
extern SmErrorT sm_service_fsm_deregister_callback( SmServiceFsmCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service FSM - Set State
// =======================
extern SmErrorT sm_service_fsm_set_state( SmServiceT* service,
    SmServiceStateT state );
// ****************************************************************************

// ****************************************************************************
// Service FSM - Set Status
// ========================
extern SmErrorT sm_service_fsm_set_status( SmServiceT* service,
    SmServiceStatusT status, SmServiceConditionT condition );
// ****************************************************************************

// ****************************************************************************
// Service FSM - Event Handler
// ===========================
extern SmErrorT sm_service_fsm_event_handler( char service_name[],
    SmServiceEventT event, void* event_data[], const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service FSM - Action Complete Handler
// =====================================
extern SmErrorT sm_service_fsm_action_complete_handler( char service_name[],
    SmServiceActionT action, SmServiceActionResultT action_result,
    SmServiceStateT state, SmServiceStatusT status,
    SmServiceConditionT condition, const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service FSM - Start Fail Countdown Timer
// ========================================
extern SmErrorT sm_service_fsm_start_fail_countdown_timer( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service FSM - Stop Fail Countdown Timer
// =======================================
extern SmErrorT sm_service_fsm_stop_fail_countdown_timer( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service FSM - Process Failure Register
// ======================================
extern SmErrorT sm_service_fsm_process_failure_register(
    SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service FSM - Process Failure Deregister
// =======================================
extern SmErrorT sm_service_fsm_process_failure_deregister(
    SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service FSM - Initialize
// ========================
extern SmErrorT sm_service_fsm_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service FSM - Finalize
// ======================
extern SmErrorT sm_service_fsm_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_FSM_H__
