//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
// The following module is thread safe.
//
#ifndef __SM_TIME_H__
#define __SM_TIME_H__

#include <time.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct timespec SmTimeT;
typedef char SmRealTimeStrT[24];

// ****************************************************************************
// Time - Get
// ==========
extern void sm_time_get( SmTimeT* time );
// ****************************************************************************

// ****************************************************************************
// Time - Get Elapsed Milliseconds
// ===============================
extern long sm_time_get_elapsed_ms( SmTimeT* time );
// ****************************************************************************

// ****************************************************************************
// Time - Delta in Milliseconds
// ============================
extern long sm_time_delta_in_ms( SmTimeT* end, SmTimeT* start );
// ****************************************************************************

// ****************************************************************************
// Time - Convert Milliseconds
// ===========================
extern void sm_time_convert_ms( long ms, SmTimeT* time );
// ****************************************************************************

// ****************************************************************************
// Time - format realtime
// ===========================
char* sm_time_format_realtime( SmTimeT *time, char *buffer, int buffer_len );
// ****************************************************************************

// ****************************************************************************
// Time - format monotonic time
// ===========================
char* sm_time_format_monotonic_time( SmTimeT *time, char *buffer, int buffer_len );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_TIMER_H__
