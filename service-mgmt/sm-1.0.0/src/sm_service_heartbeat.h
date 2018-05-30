//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_HEARTBEAT_H__
#define __SM_SERVICE_HEARTBEAT_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Heartbeat - Initialize
// ==============================
extern SmErrorT sm_service_heartbeat_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat - Finalize
// ============================
extern SmErrorT sm_service_heartbeat_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_HEARTBEAT_H__
