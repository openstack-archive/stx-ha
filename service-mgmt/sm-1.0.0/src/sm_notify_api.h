//
// Copyright (c) 2015 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_NOTIFY_API_H__
#define __SM_NOTIFY_API_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SM_NOTIFY_SERVICE_EVENT_UNKNOWN,
    SM_NOTIFY_SERVICE_EVENT_SYNC_START,
    SM_NOTIFY_SERVICE_EVENT_SYNC_END,
    SM_NOTIFY_SERVICE_EVENT_MAX
} SmNotifyServiceEventT;

typedef void (*SmNotifyApiServiceEventCallbackT) ( char service_name[],
        SmNotifyServiceEventT event );

typedef struct
{
    SmNotifyApiServiceEventCallbackT service_event;
} SmNotifyApiCallbacksT;

// ****************************************************************************
// Notify API - Register Callbacks
// ===============================
extern SmErrorT sm_notify_api_register_callbacks( 
    SmNotifyApiCallbacksT* callbacks );
// ****************************************************************************

// ****************************************************************************
// Notify API - Deregister Callbacks
// =================================
extern SmErrorT sm_notify_api_deregister_callbacks(
    SmNotifyApiCallbacksT* callbacks );
// ****************************************************************************

// ***************************************************************************
// Notify API - Initialize
// =======================
extern SmErrorT sm_notify_api_initialize( void );
// ***************************************************************************

// ***************************************************************************
// Notify API - Finalize
// =====================
extern SmErrorT sm_notify_api_finalize( void );
// ***************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_NOTIFY_API_H__
