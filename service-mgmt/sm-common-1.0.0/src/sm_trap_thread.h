//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_TRAP_THREAD_H__
#define __SM_TRAP_THREAD_H__

#include <stdint.h>
#include <time.h>
#include <ucontext.h>

#include "sm_limits.h"
#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_TRAP_THREAD_MSG_START_MARKER     0xA5A5A5A5
#define SM_TRAP_THREAD_MSG_END_MARKER       0xFDFDFDFD
#define SM_TRAP_MAX_ADDRESS_TRACE           32

typedef struct
{
    struct timespec ts_real;
    int process_id;    
    char process_name[SM_PROCESS_NAME_MAX_CHAR];
    int thread_id;
    char thread_name[SM_THREAD_NAME_MAX_CHAR];
    int si_num;
    int si_code;
    int si_errno;
    void* si_address;
    ucontext_t user_context;
    int address_trace_count;
    void* address_trace[SM_TRAP_MAX_ADDRESS_TRACE];
    // Starting point of symbol dump.
    char address_trace_symbols[0];
} SmTrapThreadMsgT;

// ****************************************************************************
// Trap Thread - Start
// ===================
extern SmErrorT sm_trap_thread_start( int trap_fd );
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Stop
// ==================
extern SmErrorT sm_trap_thread_stop( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_TRAP_THREAD_H__
