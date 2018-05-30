//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_API_H__
#define __SM_SERVICE_API_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmServiceApiCallbackT) (char service_name[], 
        SmServiceStateT state, SmServiceStatusT status,
        SmServiceConditionT condition);

// ****************************************************************************
// Service API - Register Callback
// ===============================
extern SmErrorT sm_service_api_register_callback( SmServiceApiCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service API - Deregister Callback
// =================================
extern SmErrorT sm_service_api_deregister_callback( SmServiceApiCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service API - Abort Actions
// ===========================
extern SmErrorT sm_service_api_abort_actions( void );
// ****************************************************************************

// ****************************************************************************
// Service API - Go-Active
// =======================
extern SmErrorT sm_service_api_go_active( char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service API - Go-Standby 
// ========================
extern SmErrorT sm_service_api_go_standby( char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service API - Disable 
// =====================
extern SmErrorT sm_service_api_disable( char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service API - Recover
// =====================
extern SmErrorT sm_service_api_recover( char service_name[],
    bool clear_fatal_condition );
// ****************************************************************************

// ****************************************************************************
// Service API - Restart 
// =====================
extern SmErrorT sm_service_api_restart( char service_name[], int flag );
// ****************************************************************************

// ****************************************************************************
// Service API - Audit
// ===================
extern SmErrorT sm_service_api_audit( char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service API - Initialize 
// ========================
extern SmErrorT sm_service_api_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service API - Finalize 
// ======================
extern SmErrorT sm_service_api_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_API_H__
