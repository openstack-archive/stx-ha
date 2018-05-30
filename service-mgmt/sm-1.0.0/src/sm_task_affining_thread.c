//
// Copyright (c) 2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_task_affining_thread.h"
#include "sm_swact_state.h"
#include "sm_time.h"
#include "sm_debug.h"
#include "sm_trap.h"
#include "sm_selobj.h"
#include "sm_thread_health.h"
#include "sm_timer.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

static const char sm_task_affining_thread_name[] = "sm_ta";
static const int  sm_task_affining_tick_interval_in_ms = 500;

static sig_atomic_t _stay_on = 1;
static bool _thread_created = false;
static pthread_t _task_affining_thread;

// ****************************************************************************
// Task Affining Thread - Affine Tasks
// ============================================
void affine_tasks( void )
{
    switch (sm_get_swact_state()) 
    {
        case SM_SWACT_STATE_START:
	    DPRINTFI("Invoking system call, affining to idle cores...");
	    system("source /etc/init.d/task_affinity_functions.sh; affine_tasks_to_idle_cores 2>/dev/null");
            DPRINTFI("Done affining tasks to idle cores, set state to none");
            sm_set_swact_state( SM_SWACT_STATE_NONE );
            return;
        case SM_SWACT_STATE_END:
  	    DPRINTFI("Invoking system call, affining to platform cores...");
            system("source /etc/init.d/task_affinity_functions.sh; affine_tasks_to_platform_cores 2>/dev/null");
	    DPRINTFI("Done affining tasks to platform cores, set state to none");
            sm_set_swact_state( SM_SWACT_STATE_NONE );
	    return;
	default:
	    // SM_SWACT_STATE_NONE and everything else...
	    return;
    }
}
	
// ****************************************************************************
// Task Affining Thread - Initialize Thread
// ============================================
SmErrorT sm_task_affining_thread_initialize_thread( void )
{
    SmErrorT error;

    error = sm_selobj_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize selection object module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_timer_initialize( sm_task_affining_tick_interval_in_ms );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize timer module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************


// ****************************************************************************
// Task Affining Thread - Finalize Thread
// ==========================================
SmErrorT sm_task_affining_thread_finalize_thread( void )
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
// Task Affining Thread - Main
// ===============================
static void* sm_task_affining_thread_main( void* arguments )
{
    SmErrorT error;

    pthread_setname_np( pthread_self(), sm_task_affining_thread_name );
    sm_debug_set_thread_info();
    sm_trap_set_thread_info();

    DPRINTFI( "Starting" );

    error = sm_task_affining_thread_initialize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Task affining thread initialization failed, error=%s.",
                  sm_error_str( error ) );
        pthread_exit( NULL );
    }

    DPRINTFI( "Started." );

    while( _stay_on )
    {
        error = sm_selobj_dispatch( sm_task_affining_tick_interval_in_ms );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Selection object dispatch failed, error=%s.",
                      sm_error_str(error) );
            break;
        }
        affine_tasks();
    }

    DPRINTFI( "Shutting down." );

    error = sm_task_affining_thread_finalize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Task affining thread finalization failed, error=%s.",
                  sm_error_str( error ) );
    }

    DPRINTFI( "Shutdown complete." );

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Task Affining Thread - Start
// ================================
SmErrorT sm_task_affining_thread_start( void )
{
    int result;

    _stay_on = 1;
    _thread_created = false;

    result = pthread_create( &_task_affining_thread, NULL,
                             sm_task_affining_thread_main, NULL );
    if( 0 != result )
    {
        DPRINTFE( "Failed to start task affining thread, error=%s.",
                  strerror(result) );
        return( SM_FAILED );
    }

    _thread_created = true;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Task Affining Thread - Stop
// ===============================
SmErrorT sm_task_affining_thread_stop( void )
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
            result = pthread_tryjoin_np( _task_affining_thread, NULL );
            if(( 0 != result )&&( EBUSY != result ))
            {
                if(( ESRCH != result )&&( EINVAL != result ))
                {
                    DPRINTFE( "Failed to wait for task affining thread "
                              "exit, sending kill signal, error=%s.",
                              strerror(result) );
                    pthread_kill( _task_affining_thread, SIGKILL );
                }
                break;
            }

            ms_expired = sm_time_get_elapsed_ms( &time_prev );
            if( 12000 <= ms_expired )
            {
                DPRINTFE( "Failed to stop task affining thread, sending "
                          "kill signal." );
                pthread_kill( _task_affining_thread, SIGKILL );
                break;
            }

            usleep( 250000 ); // 250 milliseconds.
        }
    }

    _thread_created = false;

    return( SM_OKAY );
}
// ****************************************************************************
