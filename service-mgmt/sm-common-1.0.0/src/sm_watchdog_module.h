//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_WATCHDOG_MODULE_H__
#define __SM_WATCHDOG_MODULE_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Watchdog Module - Load All
// ==========================
extern SmErrorT sm_watchdog_module_load_all( void );
// ****************************************************************************

// ****************************************************************************
// Watchdog Module - Unload All
// ============================
extern SmErrorT sm_watchdog_module_unload_all( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_WATCHDOG_MODULE_H__
