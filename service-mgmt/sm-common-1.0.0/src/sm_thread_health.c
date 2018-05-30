//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_thread_health.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_time.h"

#define SM_THREAD_HEALTH_CHECK_INTERVAL_IN_MS   1000

typedef struct
{
    bool inuse;
    char thread_name[SM_THREAD_NAME_MAX_CHAR];
    long warn_after_elapsed_ms;
    long fail_after_elapsed_ms;
    SmTimeT last_health_update; 
} SmThreadHealthT;

static SmThreadHealthT _threads[SM_THREADS_MAX];
static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

// ****************************************************************************
// Thread Health - Register
// ========================
SmErrorT sm_thread_health_register( const char thread_name[],
    long warn_after_elapsed_ms, long fail_after_elapsed_ms )
{
    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    SmThreadHealthT* entry;

    unsigned int entry_i;
    for( entry_i=0; SM_THREADS_MAX > entry_i; ++entry_i )
    {
        entry = &(_threads[entry_i]);

        if( !(entry->inuse) )
        {
            entry->inuse = true;
            snprintf( entry->thread_name, sizeof(entry->thread_name), "%s",
                      thread_name );
            entry->warn_after_elapsed_ms = warn_after_elapsed_ms;
            entry->fail_after_elapsed_ms = fail_after_elapsed_ms;
            sm_time_get( &(entry->last_health_update) );
            break;
        }
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Thread Health - Deregister
// ==========================
SmErrorT sm_thread_health_deregister( const char thread_name[] )
{
    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    SmThreadHealthT* entry;

    unsigned int entry_i;
    for( entry_i=0; SM_THREADS_MAX > entry_i; ++entry_i )
    {
        entry = &(_threads[entry_i]);

        if( entry->inuse )
        {
            if( 0 == strcmp( thread_name, entry->thread_name ) )
            {
                memset( entry, 0, sizeof(SmThreadHealthT) );
                break;
            }
        }
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Thread Health - Update
// ======================
SmErrorT sm_thread_health_update( const char thread_name[] )
{
    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    SmThreadHealthT* entry;

    unsigned int entry_i;
    for( entry_i=0; SM_THREADS_MAX > entry_i; ++entry_i )
    {
        entry = &(_threads[entry_i]);

        if( entry->inuse )
        {
            if( 0 == strcmp( thread_name, entry->thread_name ) )
            {
                sm_time_get( &(entry->last_health_update) );
                break;
            }
        }
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Thread Health - Check
// =====================
SmErrorT sm_thread_health_check( bool* healthy )
{
    static SmTimeT last_check = {0};

    long ms_elapsed;
    SmThreadHealthT* entry;

    *healthy = true;

    ms_elapsed = sm_time_get_elapsed_ms( &last_check );

    if( SM_THREAD_HEALTH_CHECK_INTERVAL_IN_MS > ms_elapsed )
    {
        DPRINTFD( "Not enough time has elapsed to do thread health check." );
        return( SM_OKAY );
    }

    sm_time_get( &last_check );

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    unsigned int entry_i;
    for( entry_i=0; SM_THREADS_MAX > entry_i; ++entry_i )
    {
        entry = &(_threads[entry_i]);

        if( !(entry->inuse) )
            continue;

        ms_elapsed = sm_time_get_elapsed_ms( &(entry->last_health_update) );
        if( ms_elapsed >= entry->fail_after_elapsed_ms )
        {
            *healthy = false;
            DPRINTFI( "FAILED: thread (%s) delayed for %li ms, maximum delay "
                      "%li ms.", entry->thread_name, ms_elapsed,
                      entry->fail_after_elapsed_ms );

        } else if( ms_elapsed >= entry->warn_after_elapsed_ms ) {
            DPRINTFI( "WARNING: thread (%s) delayed for %li",
                      entry->thread_name, ms_elapsed );
        } else {
            DPRINTFD( "Thread (%s) healthy, ms_elapsed=%li.",
                      entry->thread_name, ms_elapsed );
        }
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Thread Health - Initialize
// ==========================
SmErrorT sm_thread_health_initialize( void )
{
    memset( _threads, 0, sizeof(_threads) );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Thread Health - Finalize
// ========================
SmErrorT sm_thread_health_finalize( void )
{
    memset( _threads, 0, sizeof(_threads) );
    return( SM_OKAY );
}
// ****************************************************************************
