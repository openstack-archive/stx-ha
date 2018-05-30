//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_process_death.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/eventfd.h>
#include <sys/prctl.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_selobj.h"

#define __SM_PROCESS_DEATH_KERNEL_NOTIFICATION_SUPPORTED__
#ifdef __SM_PROCESS_DEATH_KERNEL_NOTIFICATION_SUPPORTED__

#define SM_KERNEL_NOTIFY_EXITED     0x02
#define SM_KERNEL_NOTIFY_KILLED     0x04
#define SM_KERNEL_NOTIFY_DUMPED     0x08
#define SM_KERNEL_NOTIFY_TRAPPED    0x10
#define SM_KERNEL_NOTIFY_STOPPED    0x20
#define SM_KERNEL_NOTIFY_CONTINUED  0x40

// Note: SM_KERNEL_NOTIFY_TRAPPED is for debug purposes.
#define SM_KERNEL_NOTIFY_FLAGS \
    ( SM_KERNEL_NOTIFY_EXITED | SM_KERNEL_NOTIFY_KILLED )

// Set/get notification for task state changes.
#define PR_DO_NOTIFY_TASK_STATE 17

#define SM_KERNEL_NOTIFY_SIGNAL (SIGRTMIN+1)

// This is the data structure for requestion process death
// (and other state change) information.  Sig of -1 means
// query, sig of 0 means deregistration, positive sig means
// that you want to set it.  sig and events are value-result
// and will be updated with the previous values on every
// successful call.
struct task_state_notify_info 
{
    pid_t pid;
	int sig;
	unsigned int events;
};
#endif // __SM_PROCESS_DEATH_KERNEL_NOTIFICATION_SUPPORTED__

#define SM_PROCESS_DEATH_MAX                              1024
#define SM_PROCESS_DEATH_INFO_VALID                 0xFDFDFDFD
#define SM_PROCESS_DEATH_MAX_DISPATCH                        8

typedef struct
{
    uint32_t valid;
    pid_t pid;
    int exit_code;
} SmProcessDeathInfoT;

typedef struct
{
    uint32_t valid;
    pid_t pid;
    bool child_process;
    int64_t user_data;
    SmProcessDeathCallbackT death_callback;
} SmProcessCallbackInfoT;

static int _process_death_fd = -1;
static SmProcessCallbackInfoT _callbacks[SM_PROCESS_DEATH_MAX];
static SmProcessDeathInfoT _process_deaths[SM_PROCESS_DEATH_MAX];
static uint64_t _process_death_count = 0;

// ****************************************************************************
// Process Death - Already Registered
// ==================================
bool sm_process_death_already_registered( pid_t pid,
    SmProcessDeathCallbackT death_callback )
{
    SmProcessCallbackInfoT* callback;

    unsigned int callbacks_i;
    for( callbacks_i=0; SM_PROCESS_DEATH_MAX > callbacks_i; ++callbacks_i )
    {
        callback = &(_callbacks[callbacks_i]);

        if( SM_PROCESS_DEATH_INFO_VALID == callback->valid )
        {
            if(( pid == callback->pid )&&
               ( death_callback == callback->death_callback ))
            {
                return( true );
            }
        }
    }

    return( false );
}
// ****************************************************************************

// ****************************************************************************
// Process Death - Register
// ========================
SmErrorT sm_process_death_register( pid_t pid, bool child_process,
    SmProcessDeathCallbackT death_callback, int64_t user_data )
{
    SmProcessCallbackInfoT* callback = NULL;

#ifdef __SM_PROCESS_DEATH_KERNEL_NOTIFICATION_SUPPORTED__
    if( !child_process )
    {
        struct task_state_notify_info info;

        info.pid    = pid ;
        info.sig    = SM_KERNEL_NOTIFY_SIGNAL;
        info.events = SM_KERNEL_NOTIFY_FLAGS;

        if( -1 > prctl( PR_DO_NOTIFY_TASK_STATE, &info ) )
        {
            DPRINTFE( "Failed to register for kernel notifications for "
                      "pid (%i), error=%s.", (int) pid, strerror(errno) );
        }

        DPRINTFD( "Register for kernel notifications for pid (%i).",
                  (int) pid );
    }
#endif // __SM_PROCESS_DEATH_KERNEL_NOTIFICATION_SUPPORTED__

    unsigned int callbacks_i;
    for( callbacks_i=0; SM_PROCESS_DEATH_MAX > callbacks_i; ++callbacks_i )
    {
        callback = &(_callbacks[callbacks_i]);

        if( SM_PROCESS_DEATH_INFO_VALID == callback->valid )
        {
            if( pid == callback->pid )
            {
                callback->child_process = child_process;
                callback->death_callback = death_callback;
                callback->user_data = user_data;
                break;
            }
        } else {
            callback->valid = SM_PROCESS_DEATH_INFO_VALID;
            callback->pid = pid;
            callback->child_process = child_process;
            callback->death_callback = death_callback;
            callback->user_data = user_data;
            break;
        }
    }

    if( SM_PROCESS_DEATH_MAX <= callbacks_i )
    {
        DPRINTFE( "Failed to register process death callback for pid (%i).",
                  (int) pid );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Process Death - Deregister
// ==========================
SmErrorT sm_process_death_deregister( pid_t pid )
{
    SmProcessCallbackInfoT* callback = NULL;

    unsigned int callbacks_i;
    for( callbacks_i=0; SM_PROCESS_DEATH_MAX > callbacks_i; ++callbacks_i )
    {
        callback = &(_callbacks[callbacks_i]);

        if( SM_PROCESS_DEATH_INFO_VALID != callback->valid )
            continue;

        if( pid != callback->pid )
            continue;

#ifdef __SM_PROCESS_DEATH_KERNEL_NOTIFICATION_SUPPORTED__
        if( !callback->child_process )
        {
            struct task_state_notify_info info;

            info.pid    = pid ;
            info.sig    = 0 ;
            info.events = SM_KERNEL_NOTIFY_FLAGS;
        
            if( -1 > prctl( PR_DO_NOTIFY_TASK_STATE, &info ) )
            {
                DPRINTFE( "Failed to register for kernel notifications for "
                          "pid (%i), error=%s.", (int) pid, strerror(errno) );
            }
        }
#endif // __SM_PROCESS_DEATH_KERNEL_NOTIFICATION_SUPPORTED__

        callback->valid = 0;
        callback->pid = 0;
        callback->death_callback = NULL;
        callback->user_data = 0;
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Process Death - Save
// ====================
SmErrorT sm_process_death_save( pid_t pid, int exit_code )
{
    uint64_t process_death_count = ++_process_death_count;
    SmProcessDeathInfoT* info = NULL;

    if( 0 > write( _process_death_fd, &process_death_count,
                   sizeof(process_death_count) ) )
    {
        DPRINTFE( "Failed to signal process death, error=%s",
                  strerror( errno ) );
    }

    DPRINTFD( "Process (%i) died.", (int) pid );

    unsigned int death_i;
    for( death_i=0; SM_PROCESS_DEATH_MAX > death_i; ++death_i )
    {
        info = &(_process_deaths[death_i]);

        if( SM_PROCESS_DEATH_INFO_VALID == info->valid )
        {
            if( pid == info->pid )
            {
                info->exit_code = exit_code;
                break;
            }
        } else {
            info->valid = SM_PROCESS_DEATH_INFO_VALID;
            info->pid = pid;
            info->exit_code = exit_code;
            break;
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Process Death - Dispatch
// ========================
static void sm_process_death_dispatch( int selobj, int64_t user_data )
{
    static unsigned int _last_entry = 0;

    uint64_t process_death_count;
    SmProcessDeathInfoT* info = NULL;
    SmProcessCallbackInfoT* callback = NULL;
    unsigned int num_process_death_dispatched = 0;

    read( _process_death_fd, &process_death_count,
          sizeof(process_death_count) );
   
    unsigned int death_i;
    for( death_i=_last_entry; SM_PROCESS_DEATH_MAX > death_i; ++death_i )
    {
        info = &(_process_deaths[death_i]);

        if( SM_PROCESS_DEATH_INFO_VALID != info->valid )
            continue;

        if( 0 == info->pid )
            continue;
        
        DPRINTFD( "Process (%i) exited with %i.", (int) info->pid,
                  info->exit_code );

        unsigned int callbacks_i;
        for( callbacks_i=0; SM_PROCESS_DEATH_MAX > callbacks_i; ++callbacks_i )
        {
            callback = &(_callbacks[callbacks_i]);

            if( SM_PROCESS_DEATH_INFO_VALID == callback->valid )
            {
                if( info->pid == callback->pid )
                {
                    if( NULL != callback->death_callback )
                    {
                        callback->death_callback( info->pid, info->exit_code,
                                                  callback->user_data );
                        callback->valid = 0;
                    }
                }
            }
        }

        info->valid = 0;

        if( SM_PROCESS_DEATH_MAX_DISPATCH <= ++num_process_death_dispatched )
        {
            DPRINTFD( "Maximum process death dispatches (%i) reached.",
                      SM_PROCESS_DEATH_MAX_DISPATCH );
        }
    }

    if( SM_PROCESS_DEATH_MAX <= death_i )
    {
        _last_entry = 0;
    } else {
        _last_entry = death_i;
    }

    // Check for outstanding process deaths to handle.
    for( death_i=0; SM_PROCESS_DEATH_MAX > death_i; ++death_i )
    {
        info = &(_process_deaths[death_i]);

        if( SM_PROCESS_DEATH_INFO_VALID != info->valid )
            continue;

        if( 0 == info->pid )
            continue;
        
        if( 0 > write( _process_death_fd, &process_death_count,
                       sizeof(process_death_count) ) )
        {
            DPRINTFE( "Failed to signal process death, error=%s", 
                      strerror( errno ) );
        }
        break;
    }
}
// ****************************************************************************

#ifdef __SM_PROCESS_DEATH_KERNEL_NOTIFICATION_SUPPORTED__
// ****************************************************************************
// Process Death - Signal Handler
// ==============================
static void sm_process_death_signal_handler( int sig_num, siginfo_t* info,
    void* context )
{
    if( NULL != info )
    {
        DPRINTFD( "Kernel process (%i) death notification, exit=%i.",
                  (int) info->si_pid, info->si_status );

        sm_process_death_save( info->si_pid, SM_PROCESS_FAILED );
    }
}
#endif // __SM_PROCESS_DEATH_KERNEL_NOTIFICATION_SUPPORTED__

// ****************************************************************************
// Process Death - Initialize
// ==========================
SmErrorT sm_process_death_initialize( void )
{
    SmErrorT error;

    _process_death_fd = eventfd( 0, EFD_CLOEXEC | EFD_NONBLOCK  );
    if( 0 > _process_death_fd )
    {
        DPRINTFE( "Failed to open process death file descriptor,error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    error = sm_selobj_register( _process_death_fd, 
                                sm_process_death_dispatch, 0 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register selection object, error=%s.",
                  sm_error_str( error ) );
        close( _process_death_fd );
        _process_death_fd = -1;
        return( error );
    }

#ifdef __SM_PROCESS_DEATH_KERNEL_NOTIFICATION_SUPPORTED__
    struct sigaction sa;
    
    memset( &sa, 0, sizeof(sa) );
    sa.sa_sigaction = sm_process_death_signal_handler;
    sa.sa_flags = (SA_NOCLDSTOP | SA_NOCLDWAIT | SA_SIGINFO);
    sigaction( SM_KERNEL_NOTIFY_SIGNAL, &sa, NULL );

    DPRINTFI( "Kernel notify signal (%i) registered.",
              SM_KERNEL_NOTIFY_SIGNAL );
#endif // __SM_PROCESS_DEATH_KERNEL_NOTIFICATION_SUPPORTED__

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Process Death - Finalize
// ========================
SmErrorT sm_process_death_finalize( void )
{
    SmErrorT error;

    if( 0 <= _process_death_fd )
    {
        error = sm_selobj_deregister( _process_death_fd );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object, error=%s.",
                      sm_error_str( error ) );
        }

        close( _process_death_fd );
    }

    return( SM_OKAY );
}
// ****************************************************************************
