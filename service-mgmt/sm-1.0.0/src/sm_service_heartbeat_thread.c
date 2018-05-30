//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_heartbeat_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <libgen.h>

#include "sm_types.h"
#include "sm_time.h"
#include "sm_debug.h"
#include "sm_trap.h"
#include "sm_utils.h"
#include "sm_selobj.h"
#include "sm_timer.h"
#include "sm_thread_health.h"
#include "sm_db.h"
#include "sm_db_service_heartbeat.h"
#include "sm_service_heartbeat_api.h"
#include "sm_service_heartbeat.h"

#define SM_SERVICE_HEARTBEAT_THREAD_NAME                        "sm_service_hb"
#define SM_SERVICE_HEARTBEAT_TICK_INTERVAL_IN_MS                            100

static sig_atomic_t _stay_on = 1;
static bool _thread_created = false;
static pthread_t _heartbeat_thread;

// ****************************************************************************
// Service Heartbeat Thread - Initialize Thread
// ============================================
SmErrorT sm_service_heartbeat_thread_initialize_thread( void )
{
    SmErrorT error;

    error = sm_selobj_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize selection object module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_timer_initialize( SM_SERVICE_HEARTBEAT_TICK_INTERVAL_IN_MS );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize timer module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_db_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize database module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_service_heartbeat_api_initialize( false );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service heartbeat api module, "
                  "error=%s.", sm_error_str( error ) );
        return( SM_FAILED );
    }  
    
    error = sm_db_service_heartbeat_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service heartbeat database module, "
                  "error=%s.", sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_service_heartbeat_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service heartbeat module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }  
    
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat Thread - Finalize Thread
// ==========================================
SmErrorT sm_service_heartbeat_thread_finalize_thread( void )
{
    SmErrorT error;

    error = sm_service_heartbeat_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service heartbeat module, error=%s.",
                  sm_error_str( error ) );
    }  

    error = sm_db_service_heartbeat_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service heartbeat database module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_heartbeat_api_finalize( false );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service heartbeat api module, "
                  "error=%s.", sm_error_str( error ) );
    }  

    error = sm_db_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize database module, error=%s.",
                  sm_error_str( error ) );
    }

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
// Service Heartbeat Thread - Main
// ===============================
static void* sm_service_heartbeat_thread_main( void* arguments )
{
    SmErrorT error;

    pthread_setname_np( pthread_self(), SM_SERVICE_HEARTBEAT_THREAD_NAME );
    sm_debug_set_thread_info();
    sm_trap_set_thread_info();

    DPRINTFI( "Starting" );

    // Warn after 1 second, fail after 3 seconds.
    error = sm_thread_health_register( SM_SERVICE_HEARTBEAT_THREAD_NAME,
                                       1000, 3000 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register service heartbeat thread, error=%s.",
                  sm_error_str( error ) );
        pthread_exit( NULL );
    }

    error = sm_service_heartbeat_thread_initialize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service heartbeat thread initialize failed, error=%s.",
                  sm_error_str( error ) );
        pthread_exit( NULL );
    }

    DPRINTFI( "Started." );

    while( _stay_on )
    {
        error = sm_selobj_dispatch( SM_SERVICE_HEARTBEAT_TICK_INTERVAL_IN_MS );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Selection object dispatch failed, error=%s.",
                      sm_error_str(error) );
            break;
        }

        error = sm_thread_health_update( SM_SERVICE_HEARTBEAT_THREAD_NAME );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to update service heartbeat thread health, "
                      "error=%s.", sm_error_str(error) );
        }
    }

    DPRINTFI( "Shutting down." );

    error = sm_service_heartbeat_thread_finalize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service heartbeat finalize thread failed, error=%s.",
                  sm_error_str( error ) );
    }

    DPRINTFI( "Shutdown complete." );

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat Thread - Start
// ================================
SmErrorT sm_service_heartbeat_thread_start( void )
{
    int result;

    _stay_on = 1;
    _thread_created = false;
    
    result = pthread_create( &_heartbeat_thread, NULL,
                             sm_service_heartbeat_thread_main, NULL );
    if( 0 != result )
    {
        DPRINTFE( "Failed to start service heartbeat thread, error=%s.",
                  strerror(result) );
        return( SM_FAILED );
    }

    _thread_created = true;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat Thread - Stop
// ===============================
SmErrorT sm_service_heartbeat_thread_stop( void )
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
            result = pthread_tryjoin_np( _heartbeat_thread, NULL );
            if(( 0 != result )&&( EBUSY != result ))
            {
                if(( ESRCH != result )&&( EINVAL != result ))
                {
                    DPRINTFE( "Failed to wait for service heartbeat thread "
                              "exit, sending kill signal, error=%s.",
                              strerror(result) );
                    pthread_kill( _heartbeat_thread, SIGKILL );
                }
                break;
            }

            ms_expired = sm_time_get_elapsed_ms( &time_prev );
            if( 5000 <= ms_expired )
            {
                DPRINTFE( "Failed to stop service heartbeat thread, sending "
                          "kill signal." );
                pthread_kill( _heartbeat_thread, SIGKILL );
                break;
            }

            usleep( 250000 ); // 250 milliseconds.
        }
    }

    _thread_created = false;

    return( SM_OKAY );
}
// ****************************************************************************
