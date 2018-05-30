//
// Copyright (c) 2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_failover_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

#include "sm_failover.h"
#include "sm_types.h"
#include "sm_time.h"
#include "sm_debug.h"
#include "sm_trap.h"
#include "sm_selobj.h"
#include "sm_thread_health.h"
#include "sm_hw.h"
#include "sm_timer.h"

#define SM_FAILOVER_THREAD_TICK_INTERVAL_IN_MS 1000

static const char sm_failover_thread_name[] = "sm_failover";
static const int  sm_failover_tick_interval_in_ms = 1000;

static sig_atomic_t _stay_on = 1;
static bool _thread_created = false;
static pthread_t _failover_thread;

static void sm_failover_interface_change_callback(
    SmHwInterfaceChangeDataT* if_change )
{
    switch ( if_change->interface_state )
    {
        case SM_INTERFACE_STATE_DISABLED:
            DPRINTFI("Interface %s is down", if_change->interface_name);
            sm_failover_interface_down(if_change->interface_name);
            break;
        case SM_INTERFACE_STATE_ENABLED:
            DPRINTFI("Interface %s is up", if_change->interface_name);
            sm_failover_interface_up(if_change->interface_name);
            break;
        default:
            DPRINTFI("Interface %s state changed to %d",
                if_change->interface_name, if_change->interface_state);
            break;
    }
}

// ****************************************************************************
// Failover Thread - Initialize Thread
// ============================================
SmErrorT sm_failover_thread_initialize_thread( void )
{
    SmErrorT error;
    SmHwCallbacksT callbacks;

    error = sm_selobj_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize selection object module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_timer_initialize( SM_FAILOVER_THREAD_TICK_INTERVAL_IN_MS );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize timer module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    memset( &callbacks, 0, sizeof(callbacks) );
    callbacks.interface_change = sm_failover_interface_change_callback;
    error = sm_hw_initialize( &callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize hardware module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************


// ****************************************************************************
// Failover Thread - Finalize Thread
// ==========================================
SmErrorT sm_failover_thread_finalize_thread( void )
{
    SmErrorT error;

    error = sm_timer_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize timer module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_selobj_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize selection object module, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************


// ****************************************************************************
// Failover Thread - Main
// ===============================
static void* sm_failover_thread_main( void* arguments )
{
    SmErrorT error;

    pthread_setname_np( pthread_self(), sm_failover_thread_name );
    sm_debug_set_thread_info();
    sm_trap_set_thread_info();

    DPRINTFI( "Starting" );

    error = sm_failover_thread_initialize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failover thread initialize failed, error=%s.",
                  sm_error_str( error ) );
        pthread_exit( NULL );
    }

    DPRINTFI( "Started." );

    while( _stay_on )
    {
        error = sm_selobj_dispatch( sm_failover_tick_interval_in_ms );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Selection object dispatch failed, error=%s.",
                      sm_error_str(error) );
            break;
        }

        sm_failover_action();
    }

    DPRINTFI( "Shutting down." );

    error = sm_failover_thread_finalize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failover finalize thread failed, error=%s.",
                  sm_error_str( error ) );
    }

    DPRINTFI( "Shutdown complete." );

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Failover Thread - Start
// ================================
SmErrorT sm_failover_thread_start( void )
{
    int result;

    _stay_on = 1;
    _thread_created = false;

    result = pthread_create( &_failover_thread, NULL,
                             sm_failover_thread_main, NULL );
    if( 0 != result )
    {
        DPRINTFE( "Failed to start failover thread, error=%s.",
                  strerror(result) );
        return( SM_FAILED );
    }

    _thread_created = true;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Failover Thread - Stop
// ===============================
SmErrorT sm_failover_thread_stop( void )
{
    _stay_on = 0;

    if( _thread_created )
    {
        long ms_expired;
        SmTimeT time_prev;
        int result;

        sm_time_get( &time_prev );

        while( true )
        {
            result = pthread_tryjoin_np( _failover_thread, NULL );
            if(( 0 != result )&&( EBUSY != result ))
            {
                if(( ESRCH != result )&&( EINVAL != result ))
                {
                    DPRINTFE( "Failed to wait for failover thread "
                              "exit, sending kill signal, error=%s.",
                              strerror(result) );
                    pthread_kill( _failover_thread, SIGKILL );
                }
                break;
            }

            ms_expired = sm_time_get_elapsed_ms( &time_prev );
            if( 12000 <= ms_expired )
            {
                DPRINTFE( "Failed to stop failover thread, sending "
                          "kill signal." );
                pthread_kill( _failover_thread, SIGKILL );
                break;
            }

            usleep( 250000 ); // 250 milliseconds.
        }
    }

    _thread_created = false;

    return( SM_OKAY );
}
// ****************************************************************************