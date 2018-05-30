//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_PROCESS_DEATH_H__
#define __SM_PROCESS_DEATH_H__

#include <stdbool.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmProcessDeathCallbackT) (pid_t pid, int exit_code, 
        int64_t user_data);

// ****************************************************************************
// Process Death - Already Registered
// ==================================
extern bool sm_process_death_already_registered( pid_t pid,
    SmProcessDeathCallbackT death_callback );
// ****************************************************************************

// ****************************************************************************
// Process Death - Register
// ========================
extern SmErrorT sm_process_death_register( pid_t pid, bool child_process, 
    SmProcessDeathCallbackT death_callback, int64_t user_data );
// ****************************************************************************

// ****************************************************************************
// Process Death - Deregister
// ==========================
extern SmErrorT sm_process_death_deregister( pid_t pid );
// ****************************************************************************

// ****************************************************************************
// Process Death - Save
// ====================
extern SmErrorT sm_process_death_save( pid_t pid, int exit_code );
// ****************************************************************************

// ****************************************************************************
// Process Death - Initialize
// ==========================
extern SmErrorT sm_process_death_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Process Death - Finalize
// ========================
extern SmErrorT sm_process_death_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_PROCESS_DEATH_H__
