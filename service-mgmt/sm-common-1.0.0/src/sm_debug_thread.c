//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_debug_thread.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <pthread.h>

#include "sm_types.h"
#include "sm_time.h"
#include "sm_selobj.h"
#include "sm_trap.h"

#define SM_DEBUG_THREAD_NAME                                        "sm_debug"
#define SM_DEBUG_THREAD_TICK_INTERVAL_IN_MS                               1000
#define SM_DEBUG_THREAD_SCHED_LOG_FILE             "/var/log/sm-scheduler.log"

#define SM_SYSLOG( format, args... ) \
    syslog( LOG_LOCAL3 | LOG_DEBUG, format "\n", ##args )

#define SM_WRITE_SYSLOG( format, args... ) \
    syslog( LOG_LOCAL3 | LOG_DEBUG, format "\n", ##args )

#define SM_WRITE_SCHEDLOG( format, args... ) \
    fprintf( _sched_log, format "\n", ##args ); \
    fflush( _sched_log )

static sig_atomic_t _stay_on;
static bool _thread_created = false;
static pthread_t _debug_thread;
static int _server_fd = -1;
static bool _server_fd_registered = false;
static FILE * _sched_log = NULL;

// ****************************************************************************
// Debug Thread - Dispatch
// =======================
static void sm_debug_thread_dispatch( int selobj, int64_t user_data )
{
    int bytes_read;
    long ms_expired;
    SmTimeT time_prev, time_now;
    SmDebugThreadMsgT msg;
    char time_str[80];
    char date_str[32];
    struct tm t_real;

    memset( &msg, 0, sizeof(msg) );

    int retry_count;
    for( retry_count = 5; retry_count != 0; --retry_count )
    {
        bytes_read = recv( selobj, &msg, sizeof(msg), 0 );
        if( 0 < bytes_read )
        {
            break;

        } else if( 0 == bytes_read ) {
            // For connection oriented sockets, this indicates that the peer 
            // has performed an orderly shutdown.
            return;

        } else if(( 0 > bytes_read )&&( EINTR != errno )) {
            SM_SYSLOG( "Failed to receive message, errno=%s.",
                       strerror( errno ) );
            return;
        }
    }

    switch( msg.type )
    {
        case SM_DEBUG_THREAD_MSG_LOG:

            sm_time_get( &time_prev );

            SM_WRITE_SYSLOG( "time[%ld.%03ld] log<%" PRIu64 "> %s",
                             (long) msg.u.log.ts_mono.tv_sec, 
                             (long) msg.u.log.ts_mono.tv_nsec/1000000,
                             msg.u.log.seqnum, msg.u.log.data );

            sm_time_get( &time_now );
            ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );

            if( 1000 <= ms_expired )
                SM_SYSLOG( "Syslog logging took %li ms.", ms_expired );
        break;

        case SM_DEBUG_THREAD_MSG_SCHED_LOG:

            sm_time_get( &time_prev );

            if( NULL == localtime_r( &(msg.u.log.ts_real.tv_sec), &t_real ) )
            {
                snprintf( time_str, sizeof(time_str),
                          "YYYY:MM:DD HH:MM:SS.xxx" );
            } else {
                strftime( date_str, sizeof(date_str), "%FT%T",
                          &t_real );
                snprintf( time_str, sizeof(time_str), "%s.%03ld", date_str,
                          msg.u.log.ts_real.tv_nsec/1000000 );
            }

            SM_WRITE_SCHEDLOG( "%s sm: time[%ld.%03ld] %s", time_str,
                               (long) msg.u.log.ts_mono.tv_sec,
                               (long) msg.u.log.ts_mono.tv_nsec/1000000,
                               msg.u.log.data );

            sm_time_get( &time_now );
            ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );

            if( 1000 <= ms_expired )
                SM_SYSLOG( "Scheduler logging took %li ms.", ms_expired );
        break;

        default:
            SM_SYSLOG( "Unknown message (%i) received.", msg.type );
            return;
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Debug Thread - Initialize Thread
// ================================
static SmErrorT sm_debug_thread_initialize_thread( void )
{
    SmErrorT error;

    _server_fd_registered = false;

    error = sm_selobj_initialize();
    if( SM_OKAY != error )
    {
        SM_SYSLOG( "Failed to initialize selection object module, error=%s.",
                   sm_error_str( error ) );
        return( SM_FAILED );
    }

    if( -1 < _server_fd )
    {
        error = sm_selobj_register( _server_fd, sm_debug_thread_dispatch, 0 );
        if( SM_OKAY != error )
        {
            SM_SYSLOG( "Failed to register selection object, error=%s.",
                       sm_error_str( error ) );
            return( error );
        }    

        _server_fd_registered = true;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Debug Thread - Finalize Thread
// ==============================
static SmErrorT sm_debug_thread_finalize_thread( void )
{
    SmErrorT error;

    if( _server_fd_registered )
    {
        error = sm_selobj_deregister( _server_fd );
        if( SM_OKAY != error )
        {
            SM_SYSLOG( "Failed to deregister selection object, error=%s.",
                       sm_error_str( error ) );
        }

        _server_fd_registered = false;
    }

    error = sm_selobj_finalize();
    if( SM_OKAY != error )
    {
        SM_SYSLOG( "Failed to finialize selection object module, error=%s.",
                   sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Debug Thread - Main
// ===================
static void* sm_debug_thread_main( void* arguments )
{
    SmErrorT error;

    pthread_setname_np( pthread_self(), SM_DEBUG_THREAD_NAME );
    sm_trap_set_thread_info();

    SM_SYSLOG( "Starting" );

    error = sm_debug_thread_initialize_thread();
    if( SM_OKAY != error )
    {
        SM_SYSLOG( "Failed to initialize debug thread, error=%s.",
                   sm_error_str( error ) );
        pthread_exit( NULL );
    }

    SM_SYSLOG( "Started." );

    while( _stay_on )
    {
        error = sm_selobj_dispatch( SM_DEBUG_THREAD_TICK_INTERVAL_IN_MS );
        if( SM_OKAY != error )
        {
            SM_SYSLOG( "Selection object dispatch failed, error=%s.",
                      sm_error_str(error) );
            break;
        }
    }

    SM_SYSLOG( "Shutting down." );

    error = sm_debug_thread_finalize_thread();
    if( SM_OKAY != error )
    {
        SM_SYSLOG( "Failed to finalize debug thread, error=%s.",
                   sm_error_str( error ) );
    }

    SM_SYSLOG( "Shutdown complete." );

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Debug Thread - Start
// ====================
SmErrorT sm_debug_thread_start( int server_fd )
{
    int result;

    _stay_on = 1;
    _thread_created = false;
    _server_fd = server_fd;

    openlog( NULL, LOG_NDELAY, LOG_LOCAL3 );

    _sched_log = fopen( SM_DEBUG_THREAD_SCHED_LOG_FILE, "a" );
    if( NULL == _sched_log )
    {
        printf( "Failed to open scheduler log file (%s).\n",
                SM_DEBUG_THREAD_SCHED_LOG_FILE );
        return( SM_FAILED );
    }

    result = pthread_create( &_debug_thread, NULL, sm_debug_thread_main, NULL );
    if( 0 != result )
    {
        printf( "Failed to start debug thread, error=%s.\n",
                strerror(result) );
        return( SM_FAILED );
    }

    _thread_created = true;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Debug Thread - Stop
// ===================
SmErrorT sm_debug_thread_stop( void )
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
            result = pthread_tryjoin_np( _debug_thread, NULL );
            if(( 0 != result )&&( EBUSY != result ))
            {
                if(( ESRCH != result )&&( EINVAL != result ))
                {
                    printf( "Failed to wait for debug thread exit, "
                            "sending kill signal, error=%s.\n",
                            strerror(result) );
                    pthread_kill( _debug_thread, SIGKILL );
                }
                break;
            }

            ms_expired = sm_time_get_elapsed_ms( &time_prev );
            if( 5000 <= ms_expired )
            {
                printf( "Failed to stop debug thread, sending "
                        "kill signal.\n" );
                pthread_kill( _debug_thread, SIGKILL );
                break;
            }

            usleep( 250000 ); // 250 milliseconds.
        }
    }

    if( NULL != _sched_log )
    {
        fflush( _sched_log );
        fclose( _sched_log );
        _sched_log = NULL;
    }

    _server_fd = -1;
    _thread_created = false;

    closelog();

    return( SM_OKAY );
}
// ****************************************************************************
