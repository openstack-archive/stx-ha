//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_watchdog_process.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h> 
#include <sched.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <getopt.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_utils.h"
#include "sm_selobj.h"
#include "sm_time.h"
#include "sm_timer.h"
#include "sm_node_stats.h"
#include "sm_watchdog_module.h"

#define SM_WATCHDOG_PROCESS_TICK_INTERVAL_IN_MS                     1000

static sig_atomic_t _stay_on = 1;

// ****************************************************************************
// Watchdog Process - Signal Handler
// =================================
static void sm_watchdog_process_signal_handler( int signum )
{
    switch( signum )
    {
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
            _stay_on = 0;
        break;

        case SIGCONT:
            DPRINTFD( "Ignoring signal SIGCONT (%i).", signum );
        break;

        default:
            DPRINTFD( "Signal (%i) ignored.", signum );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Watchdog Process - Setup Signal Handler
// =======================================
static void sm_watchdog_process_setup_signal_handler( void )
{
    struct sigaction sa;

    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = sm_watchdog_process_signal_handler;

    sigaction( SIGINT,  &sa, NULL );
    sigaction( SIGTERM, &sa, NULL );
    sigaction( SIGQUIT, &sa, NULL );
    sigaction( SIGCONT, &sa, NULL );

    signal( SIGCHLD, SIG_IGN );
}
// ****************************************************************************

// ****************************************************************************
// Watchdog Process - Initialize
// =============================
static SmErrorT sm_watchdog_process_initialize( void )
{
    SmErrorT error;

    error = sm_selobj_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize selection object module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_timer_initialize( SM_WATCHDOG_PROCESS_TICK_INTERVAL_IN_MS );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize timer module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_node_stats_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize node stats, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Watchdog Process - Finalize
// ===========================
static SmErrorT sm_watchdog_process_finalize( void )
{
    SmErrorT error;

    error = sm_node_stats_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize node stats, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_timer_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize timer module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_selobj_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize selection object module, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Watchdog Process - Main
// =======================
SmErrorT sm_watchdog_process_main( int argc, char *argv[], char *envp[] )
{
    long ms_expired;
    SmTimeT watchdog_heartbeat_time_prev;
    SmErrorT error;

    sm_watchdog_process_setup_signal_handler();

    DPRINTFI( "Starting" );

    if( sm_utils_process_running( SM_WATCHDOG_PROCESS_PID_FILENAME ) )
    {
        DPRINTFI( "Already running an instance of sm-watchdog." );
        return( SM_OKAY );
    }

    if( !sm_utils_set_pid_file( SM_WATCHDOG_PROCESS_PID_FILENAME ) )
    {
        DPRINTFE( "Failed to write pid file for sm-watchdog, error=%s.",
                  strerror(errno) );
        return( SM_FAILED );
    }

    error = sm_watchdog_process_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed initialize process, error=%s.", 
                  sm_error_str(error) );
        return( error );
    }

    error = sm_watchdog_module_load_all();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed load modules, error=%s.", 
                  sm_error_str(error) );
        return( error );
    }

    DPRINTFI( "Started." );

    sm_time_get( &watchdog_heartbeat_time_prev );
    sm_utils_watchdog_heartbeat();

    while( _stay_on )
    {
        error = sm_selobj_dispatch( SM_WATCHDOG_PROCESS_TICK_INTERVAL_IN_MS );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Selection object dispatch failed, error=%s.",
                      sm_error_str(error) );
            break;
        }

        ms_expired = sm_time_get_elapsed_ms( &watchdog_heartbeat_time_prev );
        if( SM_WATCHDOG_PROCESS_TICK_INTERVAL_IN_MS <= ms_expired )
        {
            if( sm_timer_scheduling_on_time() )
            {
                sm_utils_watchdog_heartbeat();
                sm_time_get( &watchdog_heartbeat_time_prev );
            }
        }
    }

    DPRINTFI( "Shutting down." );

    error = sm_watchdog_module_unload_all();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed unload modules, error=%s.",
                  sm_error_str(error) );
    }

    error = sm_watchdog_process_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize process, error=%s.",
                  sm_error_str( error ) );
    }

    DPRINTFI( "Shutdown complete." );

    return( SM_OKAY );
}
// ****************************************************************************
