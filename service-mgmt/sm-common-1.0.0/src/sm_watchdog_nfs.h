//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_WATCHDOG_NFS_H__
#define __SM_WATCHDOG_NFS_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Watchdog NFS - Do Check
// =======================
extern void sm_watchdog_module_do_check( void );
// ****************************************************************************

// ****************************************************************************
// Watchdog NFS - Initialize
// =========================
extern bool sm_watchdog_module_initialize( int* do_check_in_ms );
// ****************************************************************************

// ****************************************************************************
// Watchdog NFS - Finalize
// =======================
extern bool sm_watchdog_module_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_WATCHDOG_NFS_H__
