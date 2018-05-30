//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_trap.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <execinfo.h>
#include <ucontext.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_trap_thread.h"

typedef struct
{
    bool inuse;
    int thread_id;
    char thread_name[SM_THREAD_NAME_MAX_CHAR];
} SmTrapThreadInfoT;

static int _trap_fd = -1;
static int _process_id;
static char _process_name[SM_PROCESS_NAME_MAX_CHAR];
static SmTrapThreadInfoT _threads[SM_THREADS_MAX];
static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_spinlock_t _thread_spinlock;

// ***************************************************************************
// Trap - Find Thread Info
// ========================
static SmTrapThreadInfoT* sm_trap_find_thread_info( int thread_id )
{
    unsigned int thread_i;
    for( thread_i=0; SM_THREADS_MAX > thread_i; ++thread_i )
    {
        if( !(_threads[thread_i].inuse) )
            continue;

        if( thread_id == _threads[thread_i].thread_id )
            return( &(_threads[thread_i]) );
    }

    return( NULL );
}
// ***************************************************************************

// ***************************************************************************
// Trap - Send Message
// ===================
static void sm_trap_send_msg( void* msg, int msg_size )
{
    int result;

    int retry_i;
    for( retry_i = 5; retry_i != 0; --retry_i )
    {
        result = write( _trap_fd, msg, msg_size );
        if(( 0 > result )&&( EINTR == errno ))
        {
            continue;
        }
        break;
    }
}
// ***************************************************************************

// ***************************************************************************
// Trap - Signal Handler
// =====================
static void sm_trap_signal_handler( int signum, siginfo_t* siginfo,
    void* context )
{
    ucontext_t* ucontext = (ucontext_t*) context;
    uint32_t msg_marker;
    SmTrapThreadMsgT msg;
    SmTrapThreadInfoT* info;
    int result;

    result = pthread_spin_trylock( &_thread_spinlock );
    if( EBUSY == result )
    {
        // Yield to lower priority threads that first called this
        // signal handler.
        sigset_t signal_mask;
        sigemptyset( &signal_mask );
        pselect( 0, NULL, NULL, NULL, NULL, &signal_mask );
    }

    memset( &msg, 0, sizeof(msg) );

    clock_gettime( CLOCK_REALTIME, &(msg.ts_real) );

    msg.process_id = _process_id;
    snprintf( msg.process_name, sizeof(msg.process_name), "%s",
              _process_name );

    msg.thread_id = (int) syscall( SYS_gettid );

    info = sm_trap_find_thread_info( msg.thread_id );
    if( NULL != info )
    {
        snprintf( msg.thread_name, sizeof(msg.thread_name), "%s",
                  info->thread_name );
    }

    msg.si_address = siginfo->si_addr;
    msg.si_num = siginfo->si_signo;
    msg.si_code = siginfo->si_code;
    msg.si_errno = siginfo->si_errno;

    memcpy( &(msg.user_context), ucontext, sizeof(ucontext_t) );

    msg.address_trace_count = backtrace( &(msg.address_trace[0]),
                                         SM_TRAP_MAX_ADDRESS_TRACE );

    // Send message start marker.
    msg_marker = SM_TRAP_THREAD_MSG_START_MARKER;
    sm_trap_send_msg( &msg_marker, sizeof(msg_marker) );

    // Send the process information across.
    sm_trap_send_msg( &msg, sizeof(msg) );

    // Send the function names across.
    backtrace_symbols_fd( msg.address_trace, msg.address_trace_count,
                          _trap_fd );

    // Send message end marker.
    msg_marker = SM_TRAP_THREAD_MSG_END_MARKER;
    sm_trap_send_msg( &msg_marker, sizeof(msg_marker) );

    close( _trap_fd );

    // Core dump produced if enabled.
    _exit( EXIT_SUCCESS );
}
// ****************************************************************************

// ***************************************************************************
// Trap - Set Thread Information
// =============================
void sm_trap_set_thread_info( void )
{
    int thread_id;
    SmTrapThreadInfoT* info;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return;
    }
    
    thread_id = (int) syscall( SYS_gettid );

    info = sm_trap_find_thread_info( thread_id );
    if( NULL == info )
    {
        int thread_i;
        for( thread_i=0; SM_THREADS_MAX > thread_i; ++thread_i )
        {
            info = &(_threads[thread_i]);

            if( !(info->inuse) )
            {
                info->thread_id = thread_id;
                pthread_getname_np( pthread_self(), info->thread_name,
                                    sizeof(info->thread_name) );
                info->inuse = true;
                break;
            }
        }
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return;
    }
}
// ***************************************************************************

// ***************************************************************************
// Trap - Initialize
// =================
SmErrorT sm_trap_initialize( const char process_name[] )
{
    int fd_i;
    int flags;
    int result;
    int trap_fds[2] = {-1, -1};
    struct sigaction sa;
    void* dummy_trace[1];
    int dummy_trace_count;
    SmErrorT error = SM_FAILED;

    _process_id = (int) getpid();
    snprintf( _process_name, sizeof(_process_name), "%s", process_name );

    // Force load the backtrace function symbols, need to avoid malloc
    // in the trap exception handler.
    dummy_trace_count = backtrace( dummy_trace, 1 );
    backtrace_symbols_fd( dummy_trace, dummy_trace_count, -1 );

    result = pipe( trap_fds );
    if( 0 > result )
    {
        printf( "Failed to open a pipe to trap thread, error=%s.\n",
                strerror( errno ) );
        error = SM_FAILED;
        goto ERROR;
    }

    for( fd_i=0; 2 > fd_i; ++fd_i )
    {
        // Set to non-blocking.
        flags = fcntl( trap_fds[fd_i], F_GETFL, 0 );
        if( 0 > flags )
        {
            printf( "Failed to get flags, error=%s.\n", strerror( errno ) );
            error = SM_FAILED;
            goto ERROR;
        }

        result = fcntl( trap_fds[fd_i], F_SETFL, flags | O_NONBLOCK );
        if( 0 > result )
        {
            printf( "Failed to set flags, error=%s.\n", strerror( errno ) );
            error = SM_FAILED;
            goto ERROR;
        }

        // Close on exec.
        flags = fcntl( trap_fds[fd_i], F_GETFD, 0 );
        if( 0 > flags )
        {
            printf( "Failed to get flags, error=%s.\n", strerror( errno ) );
            error = SM_FAILED;
            goto ERROR;
        }

        result = fcntl( trap_fds[fd_i], F_SETFD, flags | FD_CLOEXEC );
        if( 0 > result )
        {
            printf( "Failed to set flags, error=%s.\n", strerror( errno ) );
            error = SM_FAILED;
            goto ERROR;
        }            
    }
    
    error = sm_trap_thread_start( trap_fds[0] );
    if( SM_OKAY != error )
    {
        printf( "Failed to start trap thread, error=%s.\n",
                sm_error_str( error ) );
        goto ERROR;
    }

    result = pthread_spin_init( &_thread_spinlock, 0 );
    if( 0 != result )
    {
        printf( "Failed initialize thread spin lock, error=%s.\n",
                strerror( errno ) );
        error = SM_FAILED;
        goto ERROR;
    }

    close( trap_fds[0] ); // close the read end
    _trap_fd = trap_fds[1]; // save write end

    memset( &sa, 0, sizeof(sa) );
    sigfillset( &sa.sa_mask );
    sa.sa_sigaction = sm_trap_signal_handler;
    sa.sa_flags = SA_SIGINFO;

    sigaction( SIGSEGV, &sa, NULL );
    sigaction( SIGILL,  &sa, NULL );
    sigaction( SIGFPE,  &sa, NULL );
    sigaction( SIGBUS,  &sa, NULL );
    sigaction( SIGABRT,  &sa, NULL );

    return( SM_OKAY );

ERROR:
    for( fd_i=0; 2 > fd_i; ++fd_i )
    {
        if( -1 != trap_fds[fd_i] )
        {
            close( trap_fds[fd_i] );
            trap_fds[fd_i] = -1;
        }
    }
    return( error );
}
// ***************************************************************************

// ***************************************************************************
// Trap - Finalize
// ===============
SmErrorT sm_trap_finalize( void )
{
    SmErrorT error;

    error = sm_trap_thread_stop();
    if( SM_OKAY != error )
    {
        printf( "Failed to stop trap thread, error=%s.\n",
                sm_error_str( error ) );
    }

    if( -1 != _trap_fd )
    {
        close( _trap_fd );
        _trap_fd = -1;
    }

    return( SM_OKAY );
}
// ***************************************************************************
