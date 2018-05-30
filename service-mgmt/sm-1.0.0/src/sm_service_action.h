//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_ACTION_H__
#define __SM_SERVICE_ACTION_H__

#include <stdbool.h>
#include <unistd.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Action - Exist
// ======================
extern SmErrorT sm_service_action_exist( char service_name[], 
    SmServiceActionT action );
// ****************************************************************************

// ****************************************************************************
// Service Action - Interval
// =========================
extern SmErrorT sm_service_action_interval( char service_name[],
    SmServiceActionT action, int* interval_in_ms );
// ****************************************************************************

// ****************************************************************************
// Service Action - Maximum Retries
// ================================
extern SmErrorT sm_service_action_max_retries( char service_name[],
    SmServiceActionT action, int* max_failure_retries, 
    int* max_timeout_retries, int* max_total_retries );
// ****************************************************************************

// ****************************************************************************
// Service Action - Result
// =======================
extern SmErrorT sm_service_action_result( int exit_code, char service_name[],
    SmServiceActionT action, SmServiceActionResultT* action_result,
    SmServiceStateT* service_state, SmServiceStatusT* service_status,
    SmServiceConditionT* service_condition, char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service Action - Abort
// ======================
extern SmErrorT sm_service_action_abort( char service_name[], int process_id );
// ****************************************************************************

// ****************************************************************************
// Service Action - Run
// ====================
extern SmErrorT sm_service_action_run( char service_name[],
    char instance_name[], char instance_params[], SmServiceActionT action,
    int* process_id, int* timeout_in_ms );
// ****************************************************************************

// ****************************************************************************
// Service Action - Initialize
// ===========================
extern SmErrorT sm_service_action_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Action - Finalize
// =========================
extern SmErrorT sm_service_action_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_ACTION_H__
