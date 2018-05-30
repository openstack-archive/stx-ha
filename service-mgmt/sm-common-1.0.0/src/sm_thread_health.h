//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_THREAD_HEALTH_H__
#define __SM_THREAD_HEALTH_H__

#include <stdbool.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Thread Health - Register
// ========================
extern SmErrorT sm_thread_health_register( const char thread_name[],
    long warn_after_elapsed_ms, long fail_after_elapsed_ms );
// ****************************************************************************

// ****************************************************************************
// Thread Health - Deregister
// ==========================
extern SmErrorT sm_thread_health_deregister( const char thread_name[] );
// ****************************************************************************

// ****************************************************************************
// Thread Health - Update
// ======================
extern SmErrorT sm_thread_health_update( const char thread_name[] );
// ****************************************************************************

// ****************************************************************************
// Thread Health - Check
// =====================
extern SmErrorT sm_thread_health_check( bool* healthy );
// ****************************************************************************

// ****************************************************************************
// Thread Health - Initialize
// ==========================
extern SmErrorT sm_thread_health_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Thread Health - Finalize
// ========================
extern SmErrorT sm_thread_health_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_THREAD_HEALTH_H__
