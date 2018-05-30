//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_API_H__
#define __SM_SERVICE_GROUP_API_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmServiceGroupApiCallbackT) (char service_group_name[],
        SmServiceGroupStateT state, SmServiceGroupStatusT status,
        SmServiceGroupConditionT condition, int64_t health,
        const char reason_text[]);

// ****************************************************************************
// Service Group API - Register Callback
// =====================================
extern SmErrorT sm_service_group_api_register_callback( 
    SmServiceGroupApiCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Group API - Deregister Callback
// =======================================
extern SmErrorT sm_service_group_api_deregister_callback( 
        SmServiceGroupApiCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Group API - Go-Active
// =============================
extern SmErrorT sm_service_group_api_go_active( char service_group_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Group API - Go-Standby
// ==============================
extern SmErrorT sm_service_group_api_go_standby( char service_group_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Group API - Disable
// ===========================
extern SmErrorT sm_service_group_api_disable( char service_group_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Group API - Recover
// ===========================
extern SmErrorT sm_service_group_api_recover( char service_group_name[],
    bool clear_fatal_condition, bool escalate_recovery );
// ****************************************************************************

// ****************************************************************************
// Service Group API - Initialize
// ==============================
extern SmErrorT sm_service_group_api_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group API - Finalize
// ============================
extern SmErrorT sm_service_group_api_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_API_H__
