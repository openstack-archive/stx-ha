//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_HEARTBEAT_THREAD_H__
#define __SM_SERVICE_HEARTBEAT_THREAD_H__

#include <stdbool.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Heartbeat Thread - Start
// ================================
extern SmErrorT sm_service_heartbeat_thread_start( void );
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat Thread - Stop
// ===============================
extern SmErrorT sm_service_heartbeat_thread_stop( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_HEARTBEAT_THREAD_H__
