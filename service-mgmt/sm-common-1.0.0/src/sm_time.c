//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_time.h"
#include <stdio.h>

#include "sm_debug.h"

#define MS_TO_NSEC  1000000
#define SEC_TO_NSEC 1000000000
static SmTimeT _monotonic_time_offset = {0};

// ****************************************************************************
// Time - Get
// ==========
void sm_time_get( SmTimeT* time )
{
    clock_gettime( CLOCK_MONOTONIC_RAW, time );
}
// ****************************************************************************

// ****************************************************************************
// Time - Get Elapsed Milliseconds
// ===============================
long sm_time_get_elapsed_ms( SmTimeT* time )
{
    SmTimeT now;

    clock_gettime( CLOCK_MONOTONIC_RAW, &now );

    if( NULL == time )
    {
        return( (now.tv_sec*1000) + (now.tv_nsec/MS_TO_NSEC) );

    } else {
        return( sm_time_delta_in_ms( &now, time ) );
    }
}
// ****************************************************************************

// ****************************************************************************
// Time - Delta in Milliseconds
// ============================
long sm_time_delta_in_ms( SmTimeT* end, SmTimeT* start )
{
    long start_in_ms = (start->tv_sec*1000) + (start->tv_nsec/MS_TO_NSEC);
    long end_in_ms = (end->tv_sec*1000) + (end->tv_nsec/MS_TO_NSEC);

    return( end_in_ms - start_in_ms );
}
// ****************************************************************************

// ****************************************************************************
// Time - Convert Milliseconds
// ===========================
void sm_time_convert_ms( long ms, SmTimeT* time )
{
    time->tv_sec = ms / 1000;
    time->tv_nsec = (ms % 1000) * MS_TO_NSEC;
}
// ****************************************************************************


// ****************************************************************************
// Time - format realtime
// ===========================
char* sm_time_format_realtime( SmTimeT *time, char *buffer, int buffer_len )
{
    struct tm t;
    localtime_r(&time->tv_sec, &t);
    char temp[20];
    strftime(temp, sizeof(temp), "%FT%X", &t);

    snprintf (buffer, buffer_len, "%s.%03d\n", temp, (int)(time->tv_nsec/MS_TO_NSEC));
    return buffer;
}
// ****************************************************************************

// ****************************************************************************
// Time - format monotonic time
// ===========================
char* sm_time_format_monotonic_time( SmTimeT *time, char *buffer, int buffer_len )
{
    if( 0 == _monotonic_time_offset.tv_sec && 0 == _monotonic_time_offset.tv_nsec)
    {
        SmTimeT monotonic_time;
        clock_gettime( CLOCK_MONOTONIC_RAW, &monotonic_time );
        clock_gettime( CLOCK_REALTIME, &_monotonic_time_offset);
        _monotonic_time_offset.tv_sec -= monotonic_time.tv_sec;
        _monotonic_time_offset.tv_nsec -= monotonic_time.tv_nsec;
        if(_monotonic_time_offset.tv_nsec < 0 )
        {
            _monotonic_time_offset.tv_sec --;
            _monotonic_time_offset.tv_nsec += SEC_TO_NSEC;
        }
    }

    SmTimeT realtime = _monotonic_time_offset;
    realtime.tv_sec += time->tv_sec;
    realtime.tv_nsec += time->tv_nsec;
    if(realtime.tv_nsec > SEC_TO_NSEC)
    {
        realtime.tv_nsec -= SEC_TO_NSEC;
        realtime.tv_sec ++;
    }
    return sm_time_format_realtime( &realtime, buffer, buffer_len );
}
// ****************************************************************************