//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DEBUG_THREAD_H__
#define __SM_DEBUG_THREAD_H__

#include <stdint.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SM_DEBUG_THREAD_MSG_LOG,
    SM_DEBUG_THREAD_MSG_SCHED_LOG,
} SmDebugThreadMsgTypeT;

#define SM_DEBUG_THREAD_LOG_MAX_CHARS       512

typedef struct
{
    uint64_t seqnum;
    struct timespec ts_mono;    
    struct timespec ts_real;    
    char data[SM_DEBUG_THREAD_LOG_MAX_CHARS];
} SmDebugThreadMsgLogT;

typedef struct
{
    SmDebugThreadMsgTypeT type;

    union
    {
        SmDebugThreadMsgLogT log;
    } u;
} SmDebugThreadMsgT;

// ****************************************************************************
// Debug Thread - Start
// ====================
extern SmErrorT sm_debug_thread_start( int server_fd );
// ****************************************************************************

// ****************************************************************************
// Debug Thread - Stop
// ===================
extern SmErrorT sm_debug_thread_stop( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DEBUG_THREAD_H__
