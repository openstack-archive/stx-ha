//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_debug.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <pthread.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug_thread.h"

typedef struct
{
    bool inuse;
    uint64_t log_seqnum;
    char thread_name[SM_THREAD_NAME_MAX_CHAR];
    int thread_id;
    char thread_identifier[SM_THREAD_NAME_MAX_CHAR+10];
} SmDebugThreadInfoT;

static int _server_fd = -1;
static int _client_fd = -1;
static int _initialized;
static SmDebugLogLevelT _log_level = SM_DEBUG_LOG_LEVEL_INFO;
static SmDebugThreadInfoT _thread_info[SM_THREADS_MAX];
static pthread_key_t _thread_key;

#define SCHED_LOGS_MAX          128

typedef SmDebugThreadMsgT SmDebugSchedLogSetT[SCHED_LOGS_MAX];

static int _sched_log_set = 0;
static int _sched_log_num = 0;
static SmDebugSchedLogSetT _sched_logs[2];

// ****************************************************************************
// Debug - Log Level String
// ========================
const char* sm_debug_log_level_str( SmDebugLogLevelT level )
{
    switch( level )
    {
        case SM_DEBUG_LOG_LEVEL_ERROR:
            return( "ERROR" );
        break;

        case SM_DEBUG_LOG_LEVEL_INFO:
            return( "INFO" );
        break;

        case SM_DEBUG_LOG_LEVEL_DEBUG:
            return( "DEBUG" );
        break;

        case SM_DEBUG_LOG_LEVEL_VERBOSE:
            return( "VERBOSE" );
        break;

        default:
            return( "???" );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Debug - Set Log Level
// =====================
void sm_debug_set_log_level( SmDebugLogLevelT level )
{
    _log_level = level;
}
// ****************************************************************************

// ****************************************************************************
// Debug - Do Log
// ==============
bool sm_debug_do_log( const char* file_name, SmDebugLogLevelT level )
{
    return( level <= _log_level );
}
// ****************************************************************************

// ****************************************************************************
// Debug - Log
// ===========
void sm_debug_log( SmDebugLogTypeT type, const char* format, ... ) 
{
    SmDebugThreadInfoT* info = NULL;
    SmDebugThreadMsgT msg;
    va_list arguments;

    memset( &msg, 0, sizeof(msg) );

    if( !_initialized )
    {
        va_start( arguments, format );
        vsnprintf( msg.u.log.data, sizeof(msg.u.log.data), format, arguments );
        va_end( arguments );
        printf( "%s\n", msg.u.log.data );
        return;
    }

    info = (SmDebugThreadInfoT*) pthread_getspecific( _thread_key );

    if( SM_DEBUG_SCHED_LOG == type )
    {
        msg.type = SM_DEBUG_THREAD_MSG_SCHED_LOG;
        msg.u.log.seqnum = 0;

    } else {
        msg.type = SM_DEBUG_THREAD_MSG_LOG;

        if( NULL != info )
            msg.u.log.seqnum = ++(info->log_seqnum);
        else
            msg.u.log.seqnum = 0;
    }

    clock_gettime( CLOCK_MONOTONIC_RAW, &(msg.u.log.ts_mono) );
    clock_gettime( CLOCK_REALTIME, &(msg.u.log.ts_real) );

    va_start( arguments, format );
    vsnprintf( msg.u.log.data, sizeof(msg.u.log.data), format, arguments );
    va_end( arguments );

    if( -1 < _client_fd )
    {
        send( _client_fd, &msg, sizeof(msg), 0 );
    }
}
// ****************************************************************************

// ****************************************************************************
// Debug - Scheduler Log
// =====================
void sm_debug_sched_log( SmDebugLogTypeT type, const char* format, ... )
{
    SmDebugThreadMsgT* msg = &(_sched_logs[_sched_log_set][_sched_log_num]);
    va_list arguments;

    memset( msg, 0, sizeof(SmDebugThreadMsgT) );

    if( _initialized )
    {
        msg->type = SM_DEBUG_THREAD_MSG_SCHED_LOG;
        msg->u.log.seqnum = 0;

        clock_gettime( CLOCK_MONOTONIC_RAW, &(msg->u.log.ts_mono) );
        clock_gettime( CLOCK_REALTIME, &(msg->u.log.ts_real) );

        va_start( arguments, format );
        vsnprintf( msg->u.log.data, sizeof(msg->u.log.data), format,
                   arguments );
        va_end( arguments );

        if( _sched_log_num < SCHED_LOGS_MAX )
            ++_sched_log_num;
    }
}
// ****************************************************************************

// ****************************************************************************
// Debug - Scheduler Log Start
// ===========================
void sm_debug_sched_log_start( char* domain )
{
    if( 0 == _sched_log_set )
    {
        _sched_log_set = 1;
    } else {
        _sched_log_set = 0;
    }

    _sched_log_num = 0;
    memset( &(_sched_logs[_sched_log_set][0]), 0, sizeof(SmDebugSchedLogSetT) );
}
// ****************************************************************************

// ****************************************************************************
// Debug - Scheduler Log Done
// ==========================
void sm_debug_sched_log_done( char* domain )
{
    if( -1 < _client_fd )
    {
        bool changes = false;
        SmDebugThreadMsgT* msg_lhs;
        SmDebugThreadMsgT* msg_rhs;

        int sched_log_i;
        for( sched_log_i=0; SCHED_LOGS_MAX > sched_log_i; ++sched_log_i )
        {
            msg_lhs =  &(_sched_logs[0][sched_log_i]);
            msg_rhs =  &(_sched_logs[1][sched_log_i]);

            if( 0 != memcmp( &(msg_lhs->u.log.data[0]),
                             &(msg_rhs->u.log.data[0]),
                             SM_DEBUG_THREAD_LOG_MAX_CHARS ) )
            {
                changes = true;
                break;
            }
        }

        if( changes )
        {
            SmDebugThreadMsgT* msg;

            for( sched_log_i=0; _sched_log_num > sched_log_i; ++sched_log_i )
            {
                if( sched_log_i >= SCHED_LOGS_MAX )
                    break;

                msg =  &(_sched_logs[_sched_log_set][sched_log_i]);

                send( _client_fd, msg, sizeof(SmDebugThreadMsgLogT), 0 );
            }
        } else {
            sm_debug_log( SM_DEBUG_SCHED_LOG, "%s: no scheduling changes "
                          "required.", domain );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Debug - Get Thread Information
// ==============================
const char* sm_debug_get_thread_info( void )
{
    SmDebugThreadInfoT* info;

    info = (SmDebugThreadInfoT*) pthread_getspecific( _thread_key );

    if( NULL != info )
        return( info->thread_identifier );

    return( "unknown" );
}
// ****************************************************************************

// ****************************************************************************
// Debug - Set Thread Information
// ==============================
void sm_debug_set_thread_info( void )
{
    SmDebugThreadInfoT* info;

    int thread_i;
    for( thread_i=0; SM_THREADS_MAX > thread_i; ++thread_i )
    {
        info = &(_thread_info[thread_i]);
        if( !(info->inuse) )
        {
            info->log_seqnum = 0;
            pthread_getname_np( pthread_self(), info->thread_name, 
                                sizeof(info->thread_name) );
            info->thread_id = (int) syscall(SYS_gettid);
            snprintf( info->thread_identifier, sizeof(info->thread_identifier),
                      "%s[%i]", info->thread_name, info->thread_id );
            info->inuse = true;

            pthread_setspecific( _thread_key, info );
            break;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Debug - Initialize
// ==================
SmErrorT sm_debug_initialize( void )
{
    int flags;
    int sockets[2];
    int buffer_len = 1048576;
    int result;
    SmErrorT error;

    memset( _thread_info, 0, sizeof(_thread_info) );

    pthread_key_create( &_thread_key, NULL );

    sm_debug_set_thread_info();

    result = socketpair( AF_UNIX, SOCK_DGRAM, 0, sockets );
    if( 0 > result )
    {
        printf( "Failed to create debug communication sockets, error=%s.\n",
                strerror( errno ) );
        return( SM_FAILED );
    }

    int socket_i;
    for( socket_i=0; socket_i < 2; ++socket_i )
    {
        flags = fcntl( sockets[socket_i] , F_GETFL, 0 );
        if( 0 > flags )
        {
            printf( "Failed to get debug communication socket (%i) flags, "
                    "error=%s.\n", socket_i, strerror( errno ) );
            return( SM_FAILED );
        }

        result = fcntl( sockets[socket_i], F_SETFL, flags | O_NONBLOCK );
        if( 0 > result )
        {
            printf( "Failed to set debug communication socket (%i) to "
                    "non-blocking, error=%s.\n", socket_i, 
                      strerror( errno ) );
            return( SM_FAILED );
        }

        result = setsockopt( sockets[socket_i], SOL_SOCKET, SO_SNDBUF,
                             &buffer_len, sizeof(buffer_len) );
        if( 0 > result )
        {
            printf( "Failed to set debug communication socket (%i) "
                    "send buffer length (%i), error=%s.\n", socket_i,
                    buffer_len, strerror( errno ) );
            return( SM_FAILED );
        }

        result = setsockopt( sockets[socket_i], SOL_SOCKET, SO_RCVBUF,
                             &buffer_len, sizeof(buffer_len) );
        if( 0 > result )
        {
            printf( "Failed to set debug communication socket (%i) "
                    "receive buffer length (%i), error=%s.\n", socket_i,
                    buffer_len, strerror( errno ) );
            return( SM_FAILED );
        }
    }

    _server_fd = sockets[0];
    _client_fd = sockets[1];

    error = sm_debug_thread_start( _server_fd );
    if( SM_OKAY != error )
    {
        printf( "Failed to start debug thread, error=%s.\n",
                sm_error_str( error ) );
        return( error );
    }

    _sched_log_set = 0;
    _sched_log_num = 0;
    memset( _sched_logs, 0, sizeof(_sched_logs) );

    _initialized = true;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Debug - Finalize
// ================
SmErrorT sm_debug_finalize( void )
{
    SmErrorT error;
    
    error = sm_debug_thread_stop();
    if( SM_OKAY != error )
    {
        printf( "Failed to stop debug thread, error=%s.\n",
                sm_error_str( error ) );
    }

    memset( _thread_info, 0, sizeof(_thread_info) );
    
    if( -1 < _server_fd )
    {
        close( _server_fd );
        _server_fd = -1;
    }

    if( -1 < _client_fd )
    {
        close( _client_fd );
        _client_fd = -1;
    }

    _sched_log_set = 0;
    _sched_log_num = 0;
    memset( _sched_logs, 0, sizeof(_sched_logs) );

    _initialized = false;

    return( SM_OKAY );
}
// ****************************************************************************
