//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_ENABLE_H__
#define __SM_SERVICE_ENABLE_H__

#include "sm_types.h"
#include "sm_service_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// add more cores for enabling service
// ==============
extern void sm_service_enable_throttle_add_cores( bool additional_cores );
// ****************************************************************************

// ****************************************************************************
// get a ticket to enable service
// ==============
extern bool sm_service_enable_throttle_check( void );
// ****************************************************************************

// ****************************************************************************
// return a ticket after enabling service completed
// ==============
extern void sm_service_enable_throttle_uncheck( void );
// ****************************************************************************

// ****************************************************************************
// Service Enable
// ==============
extern SmErrorT sm_service_enable( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Enable - Abort
// ======================
extern SmErrorT sm_service_enable_abort( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Enable - Exists
// =======================
extern SmErrorT sm_service_enable_exists( SmServiceT* service, bool* exists );
// ****************************************************************************

// ****************************************************************************
// Service Enable - Initialize
// ===========================
extern SmErrorT sm_service_enable_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Enable - Finalize
// =========================
extern SmErrorT sm_service_enable_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_ENABLE_H__
