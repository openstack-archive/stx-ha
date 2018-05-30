//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_WATCHDOG_PROCESS_H__
#define __SM_WATCHDOG_PROCESS_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Watchdog Process - Main
// =======================
extern SmErrorT sm_watchdog_process_main( int argc, char *argv[], char *envp[] );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_WATCHDOG_PROCESS_H__
