//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_HEARTBEAT_API_H__
#define __SM_SERVICE_HEARTBEAT_API_H__

#include <stdbool.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmServiceHeartbeatStartCallbackT) (char service_name[] );
typedef void (*SmServiceHeartbeatStopCallbackT) (char service_name[] );
typedef void (*SmServiceHeartbeatOkayCallbackT) (char service_name[] );
typedef void (*SmServiceHeartbeatWarnCallbackT) (char service_name[] );
typedef void (*SmServiceHeartbeatDegradeCallbackT) (char service_name[] );
typedef void (*SmServiceHeartbeatFailCallbackT) (char service_name[] );

typedef struct
{
    SmServiceHeartbeatStartCallbackT start_callback;
    SmServiceHeartbeatStopCallbackT stop_callback;
    SmServiceHeartbeatOkayCallbackT okay_callback;
    SmServiceHeartbeatWarnCallbackT warn_callback;
    SmServiceHeartbeatDegradeCallbackT degrade_callback;
    SmServiceHeartbeatFailCallbackT fail_callback;
} SmServiceHeartbeatCallbacksT;

// ****************************************************************************
// Service Heartbeat API - Register Callbacks
// ==========================================
extern SmErrorT sm_service_heartbeat_api_register_callbacks( 
    SmServiceHeartbeatCallbacksT* callbacks );
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Deregister Callbacks
// ============================================
extern SmErrorT sm_service_heartbeat_api_deregister_callbacks( 
    SmServiceHeartbeatCallbacksT* callbacks );
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Start Heartbeat
// =======================================
extern SmErrorT sm_service_heartbeat_api_start_heartbeat( char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Stop Heartbeat
// ======================================
extern SmErrorT sm_service_heartbeat_api_stop_heartbeat( char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Okay Heartbeat
// ======================================
extern SmErrorT sm_service_heartbeat_api_okay_heartbeat( char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Warn Heartbeat
// ======================================
extern SmErrorT sm_service_heartbeat_api_warn_heartbeat( char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Degrade Heartbeat
// =========================================
extern SmErrorT sm_service_heartbeat_api_degrade_heartbeat( char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Fail Heartbeat
// ======================================
extern SmErrorT sm_service_heartbeat_api_fail_heartbeat( char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Initialize
// ==================================
extern SmErrorT sm_service_heartbeat_api_initialize( bool main_process );
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Finialize
// =================================
extern SmErrorT sm_service_heartbeat_api_finalize( bool main_process );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_HEARTBEAT_API_H__
