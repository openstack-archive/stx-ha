//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_trap_thread.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/prctl.h>

#include "sm_time.h"
#include "sm_utils.h"
#include "sm_selobj.h"
#include "sm_timer.h"
#include "sm_debug.h"

#define SM_TRAP_THREAD_NAME                                          "sm_trap"
#define SM_TRAP_THREAD_TICK_INTERVAL_IN_MS                                1000
#define SM_TRAP_THREAD_LOG_FILE                         "/var/log/sm-trap.log"

static sig_atomic_t _stay_on = 1;
static pid_t _trap_thread_pid = -1;
static int _trap_fd = -1;
static FILE* _trap_log = NULL;
static char _si_num_str[80] = "";
static char _si_code_str[80] = "";
static char _msg_buffer[8192] __attribute__((aligned));
static char _rx_buffer[sizeof(_msg_buffer)*2] __attribute__((aligned));

// ****************************************************************************
// Trap Thread - Signal Handler
// ============================
static void sm_trap_thread_signal_handler( int signum )
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

        case SIGHUP:
            DPRINTFD( "Parent process is shutting down or has died, "
                      "exiting." );
            _stay_on = 0;
        break;

        default:
            DPRINTFD( "Signal (%i) ignored.", signum );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Setup Signal Handler
// ==================================
static void sm_trap_thread_setup_signal_handler( void )
{
    struct sigaction sa;

    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = sm_trap_thread_signal_handler;

    sigaction( SIGINT,  &sa, NULL );
    sigaction( SIGTERM, &sa, NULL );
    sigaction( SIGQUIT, &sa, NULL );
    sigaction( SIGCONT, &sa, NULL );
    sigaction( SIGHUP, &sa, NULL );
}
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Si-Num String
// ===========================
static const char* sm_trap_thread_si_num_str( int signum )
{
    _si_num_str[0] = '\0';

    switch( signum )
    {
        case SIGSEGV:
            snprintf( _si_num_str, sizeof(_si_num_str), 
                      "invalid-memory-reference" );
        break;
        case SIGILL:
            snprintf( _si_num_str, sizeof(_si_num_str),
                      "illegal-instruction" );
        break;
        case SIGFPE:
            snprintf( _si_num_str, sizeof(_si_num_str),
                      "floating-point-exception" );
        break;
        case SIGBUS:
            snprintf( _si_num_str, sizeof(_si_num_str),
                      "bus-error" );
        break;
        case SIGABRT:
            snprintf( _si_num_str, sizeof(_si_num_str),
                      "abort" );
        break;
    }

    return( _si_num_str );
}
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Si-Code String
// ============================
static const char* sm_trap_thread_si_code_str( int signum, int si_code )
{
    _si_code_str[0] = '\0';

    if( SIGILL == signum )
    {
        switch( si_code )
        {
            case ILL_ILLOPC:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "illegal-opcode" );
            break;
            case ILL_ILLOPN:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "illegal-operand" );
            break;
            case ILL_ILLADR:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "illegal-addressing-mode" );
            break;
            case ILL_ILLTRP:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "illegal-trap" );
            break;
            case ILL_PRVOPC:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "privileged-opcode" );
            break;
            case ILL_PRVREG:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "privileged-register" );
            break;
            case ILL_COPROC:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "coprocessor-error" );
            break;
            case ILL_BADSTK:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "internal-stack-error" );
            break;
        }

    } else if( SIGFPE == signum ) {
        switch( si_code )
        {
            case FPE_INTDIV:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "integer-divide-by-zero" );
            break;
            case FPE_INTOVF:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "integer-overflow" );
            break;
            case FPE_FLTDIV:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "floating-point-divide-by-zero" );
            break;
            case FPE_FLTOVF:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "floating-point-overflow" );
            break;
            case FPE_FLTUND:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "floating-point-underflow" );
            break;
            case FPE_FLTRES:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "floating-point-inexact-result" );
            break;
            case FPE_FLTINV:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "floating-point-invalid-operation" );
            break;
            case FPE_FLTSUB:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "subscript-out-of-range" );
            break;
        }

    } else if( SIGSEGV == signum ) {
        switch( si_code )
        {
            case SEGV_MAPERR:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "address-not-mapped-to-object" );
            break;
            case SEGV_ACCERR:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "invalid-permissions-for-mapped-object" );
            break;
        }

    } else if( SIGBUS == signum ) {
        switch( si_code )
        {
            case BUS_ADRALN:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "invalid-address-alignment" );
            break;
            case BUS_ADRERR:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "nonexistent-physical-address" );
            break;
            case BUS_OBJERR:
                snprintf( _si_code_str, sizeof(_si_code_str),
                          "object-specific-hardware-error" );
            break;
        }
    }

    if( '\0' == _si_code_str[0] )
    {
        switch( si_code )
        {
            case SI_USER:
                snprintf( _si_code_str, sizeof(_si_code_str), "si-user" );
            break;
            case SI_KERNEL:
                snprintf( _si_code_str, sizeof(_si_code_str), "si-kernel" );
            break;
            case SI_QUEUE:
                snprintf( _si_code_str, sizeof(_si_code_str), "si-queue" );
            break;
            case SI_TIMER:
                snprintf( _si_code_str, sizeof(_si_code_str), "si-timer" );
            break;
            case SI_MESGQ:
                snprintf( _si_code_str, sizeof(_si_code_str), "si-msg_queue" );
            break;
            case SI_ASYNCIO:
                snprintf( _si_code_str, sizeof(_si_code_str), "si-async_io" );
            break;
            case SI_SIGIO:
                snprintf( _si_code_str, sizeof(_si_code_str), "si-sigio" );
            break;
            case SI_TKILL:
                snprintf( _si_code_str, sizeof(_si_code_str), "si-tkill" );
            break;
        }
    }

    return( _si_code_str );
}
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Log
// =================
static void sm_trap_thread_log( SmTrapThreadMsgT* msg )
{
    char time_str[80];
    char date_str[32];
    struct tm t_real;
    struct timespec ts_real;
    ucontext_t* ucontext = &(msg->user_context);

    if( NULL == localtime_r( &(msg->ts_real.tv_sec), &t_real ) )
    {
        snprintf( time_str, sizeof(time_str), "YYYY:MM:DD HH:MM:SS.xxx" );
    } else {
        strftime( date_str, sizeof(date_str), "%FT%T", &t_real );
        snprintf( time_str, sizeof(time_str), "%s.%03ld", date_str,
                  msg->ts_real.tv_nsec/1000000 );
    }

    fprintf( _trap_log, "\n*** trap detected at %s ***\n", time_str );

    clock_gettime( CLOCK_REALTIME, &(ts_real) );

    if( NULL == localtime_r( &(ts_real.tv_sec), &t_real ) )
    {
        snprintf( time_str, sizeof(time_str), "YYYY:MM:DD HH:MM:SS.xxx" );
    } else {
        strftime( date_str, sizeof(date_str), "%FT%T", &t_real );
        snprintf( time_str, sizeof(time_str), "%s.%03ld", date_str,
                  ts_real.tv_nsec/1000000 );
    }

    fprintf( _trap_log, "written-at:    %s\n", time_str );

    fprintf( _trap_log, "process-name:  %s (%i)\n", msg->process_name,
             msg->process_id );

    if( '\0' == msg->thread_name[0] )
    {
        fprintf( _trap_log, "thread-name:   %s (%i)\n", msg->process_name,
                 msg->thread_id );
    } else {
        fprintf( _trap_log, "thread-name:   %s (%i)\n", msg->thread_name,
                 msg->thread_id );
    }

    fprintf( _trap_log, "signal-number: %s (%i)\n", 
             sm_trap_thread_si_num_str(msg->si_num), msg->si_num );

    fprintf( _trap_log, "signal-code:   %s (%i)\n", 
             sm_trap_thread_si_code_str( msg->si_num, msg->si_code),
             msg->si_code );

    if( 0 != msg->si_errno )
    {
        fprintf( _trap_log, "signal-errno:  %s (%i)\n",
                 strerror(msg->si_errno), msg->si_errno );
    }

    fprintf( _trap_log, "fault-address: %p\n", msg->si_address );

    fprintf( _trap_log, "registers: \n" );
#ifdef __x86_64__
    greg_t* gregs = &(ucontext->uc_mcontext.gregs[0]);

    fprintf( _trap_log, "RIP = %016llX (instruction-pointer)\n",
             (long long) gregs[REG_RIP] );

    fprintf( _trap_log, "RDI = %016llX  RSP = %016llX  "
             "RSI = %016llX  RDX = %016llX\n",
             (long long) gregs[REG_RDI], (long long) gregs[REG_RSP],
             (long long) gregs[REG_RSI], (long long) gregs[REG_RDX] );

    fprintf( _trap_log, "RCX = %016llX  RBP = %016llX  "
             "RAX = %016llX  RBX = %016llX\n",
             (long long) gregs[REG_RCX], (long long) gregs[REG_RBP],
             (long long) gregs[REG_RAX], (long long) gregs[REG_RBX] );

    fprintf( _trap_log, "R8  = %016llX  R9  = %016llX  "
             "R10 = %016llX  R11 = %016llX\n",
             (long long) gregs[REG_R8], (long long) gregs[REG_R9],
             (long long) gregs[REG_R10], (long long) gregs[REG_R11] );

    fprintf( _trap_log, "R12 = %016llX  R13 = %016llX  "
             "R14 = %016llX  R15 = %016llX\n",
             (long long) gregs[REG_R12], (long long) gregs[REG_R13],
             (long long) gregs[REG_R14], (long long) gregs[REG_R15] );

    fprintf( _trap_log, "ERR = %016llX  TRAPNO  = %016llX\n",
             (long long) gregs[REG_ERR], (long long) gregs[REG_TRAPNO] );

    fprintf( _trap_log, "CR2 = %016llX  OLDMASK = %016llX\n",
             (long long) gregs[REG_CR2], (long long) gregs[REG_OLDMASK] );

#elif __i386__
    greg_t* gregs = &(ucontext->uc_mcontext.gregs[0]);

    fprintf( _trap_log, "EIP = %08lX (instruction-pointer)\n",
             (long) gregs[REG_EIP] );

    fprintf( _trap_log, "EDI = %08lX  ESP = %08lX  ESI = %08lX  EBP = %08lX\n",
             (long) gregs[REG_EDI], (long) gregs[REG_ESP],
             (long) gregs[REG_ESI], (long) gregs[REG_EBP] );

    fprintf( _trap_log, "EDX = %08lX  EAX = %08lX  ECX = %08lX  EBX = %08lX\n",
             (long) gregs[REG_EDX], (long) gregs[REG_EAX],
             (long) gregs[REG_ECX], (long) gregs[REG_EBX] );

    fprintf( _trap_log, "ERR = %08lX  TRAPNO = %08lX\n",
             (long) gregs[REG_ERR], (long) gregs[REG_TRAPNO] );

#elif __PPC__
    fprintf( _trap_log, "NIP = %08lX (instruction-pointer)\n",
             (long) ucontext->uc_mcontext.regs->nip );
#endif

    fprintf( _trap_log, "traceback: \n" );
    fprintf( _trap_log, "%s\n", msg->address_trace_symbols );

    fflush( _trap_log );
}
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Get Message
// =========================
static bool sm_trap_thread_get_message( char** byte_pointer, int* bytes_left )
{
    bool msg_boundary_found = false;
    char* msg_start_pointer = NULL;
    char* msg_end_pointer = NULL;
    int msg_start_offset = 0;
    int msg_size = 0;
    int bytes_move = 0;

    msg_boundary_found = false;
    msg_start_pointer = _rx_buffer;
    while( msg_start_pointer != *byte_pointer )
    {
        if( SM_TRAP_THREAD_MSG_START_MARKER == *(uint32_t*) msg_start_pointer )
        {
            DPRINTFD( "Message start delimiter found." );
            msg_start_pointer += sizeof(uint32_t);
            msg_boundary_found = true;
            break;
        }
        ++msg_start_pointer;
        ++msg_start_offset;
    }
    
    if( !msg_boundary_found )
    {
        DPRINTFD( "Message start point not yet received." );
        return( false );
    }

    msg_boundary_found = false;
    msg_end_pointer = msg_start_pointer;
    while( msg_end_pointer != *byte_pointer )
    {
        if( SM_TRAP_THREAD_MSG_END_MARKER == *(uint32_t*) msg_end_pointer )
        {
            DPRINTFD( "Message end delimiter found." );
            msg_end_pointer += sizeof(uint32_t);
            msg_boundary_found = true;
            break;
        }
        ++msg_end_pointer;
        ++msg_size;
    }
   
    if( !msg_boundary_found )
    {
        DPRINTFD( "Message end point not yet received." );
        return( false );
    }

    if( msg_size > (int) sizeof(_msg_buffer) )
    {
        DPRINTFD( "Message received is too large, msg_size=%i, max=%i.",
                  msg_size, (int) sizeof(_msg_buffer) );
        *byte_pointer = _rx_buffer;
        *bytes_left = sizeof(_rx_buffer);
        memset( *byte_pointer, 0, sizeof(_rx_buffer) );
        return( false );
    }

    DPRINTFD( "Message received, msg_size=%i.", msg_size );

    memset( _msg_buffer, 0, sizeof(_msg_buffer) );
    memcpy( _msg_buffer, msg_start_pointer, msg_size );

    // Move the remaining bytes to the head of the receive buffer.
    if( msg_end_pointer != *byte_pointer )
    {
        char* p;
        for( p = msg_end_pointer; p != *byte_pointer; ++p )
        {
            ++bytes_move;
        }
        
        if( bytes_move > 0 )
        {
            memmove( _rx_buffer, msg_end_pointer, bytes_move );
            memset( msg_end_pointer, 0, bytes_move );
        }
    }

    *byte_pointer = &(_rx_buffer[bytes_move]);
    *bytes_left = sizeof(_rx_buffer) - bytes_move;
    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Dispatch
// ======================
static void sm_trap_thread_dispatch( int selobj, int64_t user_data )
{
    static char* byte_pointer = _rx_buffer;
    static int bytes_left = sizeof(_rx_buffer);

    int bytes_read = 0;

    int retry_count;
    for( retry_count = 5; retry_count != 0; --retry_count )
    {
        bytes_read = read( selobj, byte_pointer, bytes_left );
        if( 0 < bytes_read )
        {
            DPRINTFD( "Bytes read %i.", bytes_read );

        } else if( 0 == bytes_read ) {
            // For connection oriented sockets, this indicates that the peer
            // has performed an orderly shutdown.
            DPRINTFI( "Trap connection has been broken, exiting." );
            _stay_on = 0;
            break;

        } else if(( 0 > bytes_read )&&( EAGAIN == errno )) {
            DPRINTFD( "No more data to be read." );
            break;

        } else if(( 0 > bytes_read )&&( EINTR != errno )) {
            DPRINTFE( "Failed to read message, errno=%s.", strerror( errno ) );
            return;
        }

        byte_pointer += bytes_read;
        bytes_left   -= bytes_read;

        if( 0 > bytes_left )
        {
            byte_pointer = _rx_buffer;
            bytes_left = sizeof(_rx_buffer);
            memset( byte_pointer, 0, sizeof(_rx_buffer) );
            return;
        }
    }

    while( sm_trap_thread_get_message( &byte_pointer, &bytes_left ) )
    {
        sm_trap_thread_log( (SmTrapThreadMsgT*) _msg_buffer );
    }
}
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Initialize
// ========================
static SmErrorT sm_trap_thread_initialize( int trap_fd )
{
    SmErrorT error;

    error = sm_selobj_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize selection object module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_timer_initialize( SM_TRAP_THREAD_TICK_INTERVAL_IN_MS );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize timer module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_selobj_register( trap_fd, sm_trap_thread_dispatch, 0 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register selection object, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    _trap_fd = trap_fd;

    _trap_log = fopen( SM_TRAP_THREAD_LOG_FILE, "a" );
    if( NULL == _trap_log )
    {
        DPRINTFE( "Failed to open trap log file (%s).",
                  SM_TRAP_THREAD_LOG_FILE );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Finalize
// ======================
static SmErrorT sm_trap_thread_finalize( void )
{
    SmErrorT error;

    if( -1 != _trap_fd )
    {
        error = sm_selobj_deregister( _trap_fd );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object, error=%s.",
                      sm_error_str( error ) );
        }

        close( _trap_fd );
        _trap_fd = -1;
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

    if( NULL != _trap_log )
    {
        fflush( _trap_log );
        fclose( _trap_log );
        _trap_log = NULL;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Main
// ==================
static SmErrorT sm_trap_thread_main( int trap_fd )
{
    int result;
    SmErrorT error;

    sm_trap_thread_setup_signal_handler();
    
    DPRINTFI( "Starting" );

    result = setpriority( PRIO_PROCESS, getpid(), -2 );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set priority of trap thread, error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    error = sm_trap_thread_initialize( trap_fd );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize trap thread, error=%s.", 
                  sm_error_str(error) );
        return( error );
    }

    DPRINTFI( "Started." );

    while( _stay_on )
    {
        error = sm_selobj_dispatch( SM_TRAP_THREAD_TICK_INTERVAL_IN_MS );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Selection object dispatch failed, error=%s.",
                      sm_error_str(error) );
            break;
        }
    }

    DPRINTFI( "Shutting down." );

    error = sm_trap_thread_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize trap thread, error=%s.",
                  sm_error_str(error) );
    }

    DPRINTFI( "Shutdown complete." );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Start
// ===================
SmErrorT sm_trap_thread_start( int trap_fd )
{
    pid_t pid;

    _trap_thread_pid = -1;

    pid = fork();
    if( 0 > pid )
    {
        printf( "Failed to fork process for trap thread, error=%s.\n",
                strerror( errno ) );
        return( SM_FAILED );

    } else if( 0 == pid ) {
        // Child process.
        int result;
        struct rlimit file_limits;
        SmErrorT error;

        result = prctl( PR_SET_NAME, SM_TRAP_THREAD_NAME );
        if( 0 > result )
        {
            printf( "Failed to set trap thread process name, "
                    "error=%s.\n", strerror(errno) );
            exit( EXIT_FAILURE );
        }

        result = prctl( PR_SET_PDEATHSIG, SIGHUP );
        if( 0 > result )
        {
            printf( "Failed to set trap thread parent death signal, "
                    "error=%s.\n", strerror(errno) );
            exit( EXIT_FAILURE );
        }

        if( 0 > getrlimit( RLIMIT_NOFILE, &file_limits ) )
        {
            printf( "Failed to get file limits, error=%s.",
                    strerror( errno ) );
            exit( EXIT_FAILURE );
        }

        int fd_i;
        for( fd_i=0; fd_i < (int) file_limits.rlim_cur; ++fd_i )
        {
            if( fd_i != trap_fd )
            {
                close( fd_i );
            }
        }
        
        if( 0 > open( "/dev/null", O_RDONLY ) )
        {
            printf( "Failed to open stdin to /dev/null, error=%s.\n",
                    strerror( errno ) );
        }

        if( 0 > open( "/dev/null", O_WRONLY ) )
        {
            printf( "Failed to open stdout to /dev/null, error=%s.\n",
                    strerror( errno ) );
        }

        if( 0 > open( "/dev/null", O_WRONLY ) )
        {
            printf( "Failed to open stderr to /dev/null, error=%s.\n",
                    strerror( errno ) );
        }

        error = sm_debug_initialize();
        if( SM_OKAY != error )
        {
            printf( "Debug initialization failed, error=%s.\n", 
                    sm_error_str( error ) );
            exit( EXIT_FAILURE );
        }

        if( !sm_utils_set_pid_file( SM_TRAP_PROCESS_PID_FILENAME ) )
        {
            printf( "Failed to write pid file for %s, error=%s.\n",
                    SM_TRAP_THREAD_NAME, strerror(errno) );
            exit( EXIT_FAILURE );
        }

        error = sm_trap_thread_main( trap_fd );
        if( SM_OKAY != error )
        {
            printf( "Process failure, error=%s.\n", sm_error_str( error ) );
            exit( EXIT_FAILURE );
        }

        error = sm_debug_finalize();
        if( SM_OKAY != error )
        {
            printf( "Debug finalization failed, error=%s.\n",
                    sm_error_str( error ) );
        }

        exit( EXIT_SUCCESS );

    } else {
        // Parent process.
        printf( "Trap thread (%i) created.\n", (int) pid );
        _trap_thread_pid = pid;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Trap Thread - Stop
// ==================
SmErrorT sm_trap_thread_stop( void )
{
    if( -1 != _trap_thread_pid )
    {
        pid_t pid;
        long ms_expired;
        SmTimeT time_prev;

        while( true )
        {
            pid = waitpid( _trap_thread_pid, NULL, WNOHANG | WUNTRACED );
            if( -1 == pid )
            {
                if( ECHILD != errno )
                {
                    printf( "Failed to wait for trap thread exit, error=%s.\n",
                            strerror(errno) );
                    kill( _trap_thread_pid, SIGKILL );
                    _trap_thread_pid = -1;
                }
                break;

            } else if( pid == _trap_thread_pid ) {
                break;
            }

            printf( "Sending trap thread (%i) shutdown signal.\n",
                    (int) _trap_thread_pid );
            kill( _trap_thread_pid, SIGHUP );

            ms_expired = sm_time_get_elapsed_ms( &time_prev );
            if( 5000 <= ms_expired )
            {
                printf( "Failed to stop trap thread, using SIGKILL.\n" );
                kill( _trap_thread_pid, SIGKILL );
                _trap_thread_pid = -1;
                break;
            }

            usleep( 250000 ); // 250 milliseconds.
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************
