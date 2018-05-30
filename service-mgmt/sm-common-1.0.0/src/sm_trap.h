//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_TRAP_H__
#define __SM_TRAP_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ***************************************************************************
// Trap - Set Thread Information
// =============================
extern void sm_trap_set_thread_info( void );
// ***************************************************************************

// ***************************************************************************
// Trap - Initialize
// =================
extern SmErrorT sm_trap_initialize( const char process_name[] );
// ***************************************************************************

// ***************************************************************************
// Trap - Finalize
// ===============
extern SmErrorT sm_trap_finalize( void );
// ***************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_TRAP_H__
