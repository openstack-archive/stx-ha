//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_timer.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/timerfd.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_time.h"
#include "sm_debug.h"
#include "sm_selobj.h"

#define SM_TIMER_ENTRY_INUSE                        0xFDFDFDFD
#define SM_MAX_TIMERS                                      512
#define SM_MAX_TIMERS_PER_TICK                               8
#define SM_MIN_TICK_IN_MS                                   25
#define SM_MAX_TICK_INTERVALS                                2
#define SM_SCHEDULING_ON_TIME_DEBOUNCE_IN_MS              1000

typedef struct
{
    uint32_t inuse;
    char timer_name[40];
    long timer_run_time_in_ms;
} SmTimerHistoryEntryT;

typedef struct
{
    uint32_t inuse;
    uint64_t timer_instance;
    char timer_name[40];
    SmTimerIdT timer_id;
    unsigned int ms_interval;
    SmTimeT arm_timestamp;
    SmTimerCallbackT callback;
    int64_t user_data;
    SmTimeT last_fired;
    uint32_t total_fired;
} SmTimerEntryT;

typedef struct
{
    bool inuse;
    pthread_t thread_id;
    char thread_name[SM_THREAD_NAME_MAX_CHAR];
    int tick_timer_fd;
    bool scheduling_on_time;
    SmTimeT sched_timestamp;
    SmTimeT delay_timestamp;
    uint64_t timer_instance;
    unsigned int tick_interval_in_ms;
    unsigned int last_timer_dispatched;
    SmTimerEntryT timers[SM_MAX_TIMERS];
} SmTimerThreadInfoT;

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
static SmTimerThreadInfoT _threads[SM_THREADS_MAX];

// ****************************************************************************
// Timer - Find Thread Info
// ========================
static SmTimerThreadInfoT* sm_timer_find_thread_info( void )
{
    pthread_t thread_id = pthread_self();

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
// ****************************************************************************

// ****************************************************************************
// Timer - Register
// ================
SmErrorT sm_timer_register( const char name[], unsigned int ms,
    SmTimerCallbackT callback, int64_t user_data, SmTimerIdT* timer_id )
{
    SmTimerEntryT* timer_entry;
    SmTimerThreadInfoT* thread_info;

    *timer_id = SM_TIMER_ID_INVALID;

    thread_info = sm_timer_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( SM_FAILED );
    }

    unsigned int timer_i;
    for( timer_i=0; SM_MAX_TIMERS > timer_i; ++timer_i )
    {
        timer_entry = &(thread_info->timers[timer_i]);

        if( SM_TIMER_ENTRY_INUSE == timer_entry->inuse )
            continue;

        memset( timer_entry, 0, sizeof(SmTimerEntryT) );

        timer_entry->inuse = SM_TIMER_ENTRY_INUSE;
        timer_entry->timer_instance = ++thread_info->timer_instance;
        snprintf( timer_entry->timer_name, sizeof(timer_entry->timer_name),
                  "%s", name );
        timer_entry->timer_id = timer_i;
        timer_entry->ms_interval = ms;
        sm_time_get( &timer_entry->arm_timestamp );
        timer_entry->callback = callback;
        timer_entry->user_data = user_data;
        timer_entry->last_fired.tv_sec = timer_entry->last_fired.tv_nsec = 0;
        timer_entry->total_fired = 0;
        break;
    }

    if( SM_MAX_TIMERS <= timer_i )
    {
        DPRINTFE( "No space available to create timer (%s), exiting...",
                  name );
        abort();
    }

    *timer_id = timer_i;

    DPRINTFD( "Created timer (name=%s, id=%i).", timer_entry->timer_name,
              timer_entry->timer_id );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Timer - Deregister
// ==================
SmErrorT sm_timer_deregister( SmTimerIdT timer_id )
{
    SmTimerEntryT* timer_entry = NULL;
    SmTimerThreadInfoT* thread_info;

    if(( SM_TIMER_ID_INVALID == timer_id )||
       ( SM_MAX_TIMERS <= timer_id ))
    {
        return( SM_OKAY );
    }

    thread_info = sm_timer_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( SM_FAILED );
    }

    timer_entry = &(thread_info->timers[timer_id]);
    timer_entry->inuse = 0;
    timer_entry->timer_instance = 0;
    timer_entry->user_data = 0;

    DPRINTFD( "Cancelled timer (name=%s, id=%i).", timer_entry->timer_name,
              timer_entry->timer_id );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Timer - Schedule
// ================
static SmErrorT sm_timer_schedule( SmTimerThreadInfoT* thread_info )
{
    SmTimerEntryT* timer_entry;
    struct itimerspec tick_time;
    unsigned int interval_in_ms = thread_info->tick_interval_in_ms;
    long ms_expired, ms_remaining;
    int timerfd_rc;

    unsigned int timer_i;
    for( timer_i=0; SM_MAX_TIMERS > timer_i; ++timer_i )
    {
        timer_entry = &(thread_info->timers[timer_i]);
    
        if( SM_TIMER_ENTRY_INUSE == timer_entry->inuse )
        {
            ms_expired = sm_time_get_elapsed_ms( &timer_entry->arm_timestamp );

            if( ms_expired < timer_entry->ms_interval )
            {
                ms_remaining = timer_entry->ms_interval - ms_expired;
                if( ms_remaining < interval_in_ms )
                {
                    interval_in_ms = ms_remaining;
                }
            } else {
                interval_in_ms = SM_MIN_TICK_IN_MS;
                break;
            }
        }
    }

    if( SM_MIN_TICK_IN_MS > interval_in_ms )
    {
        interval_in_ms = SM_MIN_TICK_IN_MS;
    }

    DPRINTFD( "Scheduling tick timer in %d ms.", interval_in_ms );

    tick_time.it_value.tv_sec = interval_in_ms / 1000;;
    tick_time.it_value.tv_nsec = (interval_in_ms % 1000) * 1000000;
    tick_time.it_interval.tv_sec = tick_time.it_value.tv_sec;
    tick_time.it_interval.tv_nsec =  tick_time.it_value.tv_nsec;

    timerfd_rc = timerfd_settime( thread_info->tick_timer_fd, 0, &tick_time,
                                  NULL );
    if( 0 > timerfd_rc )
    {
        DPRINTFE( "Failed to arm timer, error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Timer - Dispatch
// ================
static void sm_timer_dispatch( int selobj, int64_t user_data )
{
    int retries;
    long ms_expired;
    SmTimeT time_prev, timer_prev;
    unsigned int total_timers_fired = 0;
    uint64_t num_expires = 0;
    ssize_t s;
    SmTimerEntryT* timer_entry;
    SmTimerThreadInfoT* thread_info;
    SmTimerHistoryEntryT history[SM_MAX_TIMERS_PER_TICK];
    SmErrorT error;

    memset( history, 0, sizeof(history) );

    for( retries=0; 5 > retries; ++retries )
    {
        errno = 0;
        s = read( selobj, &num_expires, sizeof(uint64_t) );
        if(( -1 == s )&&( EINTR == errno ))
            continue;
        break;
    }

    if( sizeof(uint64_t) != s )
    {
        DPRINTFE( "Failed to read the number of timer expires." );
        return;
    }

    thread_info = sm_timer_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return;
    }
    
    ms_expired = sm_time_get_elapsed_ms( &thread_info->sched_timestamp );
    if( ms_expired >= (thread_info->tick_interval_in_ms*SM_MAX_TICK_INTERVALS) )
    {
        if( thread_info->scheduling_on_time )
        {
            DPRINTFI( "Not scheduling on time, elapsed=%li ms.", ms_expired );
            thread_info->scheduling_on_time = false;
        }

        sm_time_get( &thread_info->delay_timestamp );

    } else {
        if( !thread_info->scheduling_on_time )
        {
            ms_expired = sm_time_get_elapsed_ms( &thread_info->delay_timestamp );
            if( SM_SCHEDULING_ON_TIME_DEBOUNCE_IN_MS < ms_expired )
            {
                DPRINTFI( "Now scheduling on time." );
                thread_info->scheduling_on_time = true;
            }
        }
    }

    sm_time_get( &time_prev );

    unsigned int timer_i;
    for( timer_i=thread_info->last_timer_dispatched;
         SM_MAX_TIMERS > timer_i; ++timer_i )
    {
        timer_entry = &(thread_info->timers[timer_i]);

        if( SM_TIMER_ENTRY_INUSE == timer_entry->inuse )
        {
            ms_expired = sm_time_get_elapsed_ms( &timer_entry->arm_timestamp );
 
            if( ms_expired >= timer_entry->ms_interval )
            {
                bool rearm;
                uint64_t timer_instance = timer_entry->timer_instance;
                SmTimerHistoryEntryT* history_entry;
                
                history_entry = &(history[total_timers_fired]);

                history_entry->inuse = SM_TIMER_ENTRY_INUSE;
                snprintf( history_entry->timer_name,
                          sizeof(history_entry->timer_name),
                          "%s", timer_entry->timer_name );

                DPRINTFD( "Timer %s fire, ms_interval=%d, ms_expired=%li.",
                          timer_entry->timer_name, timer_entry->ms_interval,
                          ms_expired );
                sm_time_get( &timer_prev );
                rearm = timer_entry->callback( timer_entry->timer_id,
                                               timer_entry->user_data );
                clock_gettime( CLOCK_REALTIME, &timer_entry->last_fired );
                timer_entry->total_fired ++;
                ms_expired = sm_time_get_elapsed_ms( &timer_prev );
                history_entry->timer_run_time_in_ms = ms_expired;
                DPRINTFD( "Timer %s took %li ms.", history_entry->timer_name,
                          history_entry->timer_run_time_in_ms );

                if( timer_instance == timer_entry->timer_instance )
                {
                    if( rearm )
                    {
                        sm_time_get( &timer_entry->arm_timestamp );
                        DPRINTFD( "Timer (%i) rearmed.", timer_entry->timer_id );
                    } else {
                        timer_entry->inuse = 0;
                        DPRINTFD( "Timer (%i) removed.", timer_entry->timer_id );
                    }
                } else {
                    DPRINTFD( "Timer (%i) instance changed since callback, "
                              "rearm=%d.", timer_entry->timer_id, (int) rearm );
                }

                if( SM_MAX_TIMERS_PER_TICK <= ++total_timers_fired )
                {
                    DPRINTFD( "Maximum timers per tick (%d) reached.",
                              SM_MAX_TIMERS_PER_TICK );
                    break;
                }
            }
        }
    }

    if( SM_MAX_TIMERS <= timer_i )
    {
        thread_info->last_timer_dispatched = 0;
    } else {
        thread_info->last_timer_dispatched = timer_i;
    }

    ms_expired = sm_time_get_elapsed_ms( &time_prev );
    if( ms_expired >= (thread_info->tick_interval_in_ms*SM_MAX_TICK_INTERVALS) )
    {
        unsigned int timer_i;
        for( timer_i=0; SM_MAX_TIMERS_PER_TICK > timer_i; ++timer_i )
        {
            SmTimerHistoryEntryT* history_entry = &(history[timer_i]);

            if( SM_TIMER_ENTRY_INUSE == history_entry->inuse )
            {
                DPRINTFI( "Timer %s took %li ms.", history_entry->timer_name,
                          history_entry->timer_run_time_in_ms );
            }
        }

        thread_info->scheduling_on_time = false;
        sm_time_get( &thread_info->delay_timestamp );

        DPRINTFI( "Not scheduling on time, timer callbacks are taking too "
                  "long to execute, elapsed_time=%li ms.", ms_expired );
    } else {
        unsigned int timer_i;
        for( timer_i=0; SM_MAX_TIMERS_PER_TICK > timer_i; ++timer_i )
        {
            SmTimerHistoryEntryT* history_entry = &(history[timer_i]);

            if( SM_TIMER_ENTRY_INUSE == history_entry->inuse )
            {
                DPRINTFD( "Timer %s took %li ms.", history_entry->timer_name,
                          history_entry->timer_run_time_in_ms );
            }
        }

        DPRINTFD( "Timer callbacks took %li ms.", ms_expired );
    }

    error = sm_timer_schedule( thread_info );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to schedule timers, error=%s.",
                  sm_error_str( error ) );
    }

    sm_time_get( &thread_info->sched_timestamp );
}
// ****************************************************************************

// ****************************************************************************
// Timer - Scheduling On Time
// ==========================
bool sm_timer_scheduling_on_time( void )
{
    SmTimerThreadInfoT* thread_info;

    thread_info = sm_timer_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( true );
    }

    return( thread_info->scheduling_on_time );
}
// ****************************************************************************

// ****************************************************************************
// Timer - Scheduling On Time In Period
// ====================================
bool sm_timer_scheduling_on_time_in_period( unsigned int period_in_ms )
{
    long ms_expired;
    SmTimerThreadInfoT* thread_info;

    thread_info = sm_timer_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( true );
    }

    ms_expired = sm_time_get_elapsed_ms( &thread_info->delay_timestamp );
    return( period_in_ms < ms_expired );
}
// ****************************************************************************

// ****************************************************************************
// Timer - Dump Data
// =================
void sm_timer_dump_data( FILE* log )
{
    SmTimerEntryT* timer_entry;
    SmTimerThreadInfoT* thread_info;
    char buffer[24];

    fprintf( log, "--------------------------------------------------------------------\n" );
    int thread_i;
    for( thread_i=0; SM_THREADS_MAX > thread_i; ++thread_i )
    {
        thread_info = &(_threads[thread_i]);

        if( !(thread_info->inuse) )
            continue;

        fprintf( log, "TIMER DATA for %s\n", thread_info->thread_name );
        fprintf( log, "  scheduling_on_time......%s\n", thread_info->scheduling_on_time ? "yes" : "no" );
        fprintf( log, "  tick_interval_in_ms.....%u\n", thread_info->tick_interval_in_ms );

        unsigned int timer_i;
        for( timer_i=0; SM_MAX_TIMERS > timer_i; ++timer_i )
        {
            timer_entry = &(thread_info->timers[timer_i]);

            if( SM_TIMER_ENTRY_INUSE == timer_entry->inuse )
            {
                fprintf( log, "  timer (name=%s, id=%i)\n", timer_entry->timer_name,
                         timer_entry->timer_id );
                fprintf( log, "    instance..........%" PRIu64 "\n", timer_entry->timer_instance );
                fprintf( log, "    ms_interval.......%i\n", timer_entry->ms_interval );
                fprintf( log, "    user_data.........%" PRIi64 "\n", timer_entry->user_data );
                sm_time_format_monotonic_time(&timer_entry->arm_timestamp, buffer, sizeof(buffer));
                fprintf( log, "    timer created at .%s\n", buffer );
                sm_time_format_realtime(&timer_entry->last_fired, buffer, sizeof(buffer));
                fprintf( log, "    last fired at ....%s\n", buffer );
                fprintf( log, "    total fired ......%d\n", timer_entry->total_fired );
            }
        }
        fprintf( log, "\n" );
    }
    fprintf( log, "--------------------------------------------------------------------\n" );
}
// ****************************************************************************

// ****************************************************************************
// Timer - Initialize
// ==================
SmErrorT sm_timer_initialize( unsigned int tick_interval_in_ms )
{
    SmTimerThreadInfoT* thread_info = NULL;
    SmErrorT error;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    unsigned int thread_i;
    for( thread_i=0; SM_THREADS_MAX > thread_i; ++thread_i )
    {
        if( !(_threads[thread_i].inuse) )
        {
            thread_info = &(_threads[thread_i]);
            break;
        }
    }

    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to allocate thread information." );
        goto ERROR;
    }

    memset( thread_info, 0, sizeof(SmTimerThreadInfoT) );

    thread_info->thread_id = pthread_self();
    pthread_getname_np( pthread_self(), thread_info->thread_name,
                        sizeof(thread_info->thread_name) );

    memset( thread_info->timers, 0 , sizeof(thread_info->timers) );

    thread_info->tick_timer_fd = timerfd_create( CLOCK_MONOTONIC,
                                                 TFD_NONBLOCK | TFD_CLOEXEC );

    if( 0 > thread_info->tick_timer_fd )
    {
        DPRINTFE( "Failed to create tick timer, error=%s.", 
                  strerror( errno ) );
        goto ERROR;
    }

    error = sm_selobj_register( thread_info->tick_timer_fd,
                                sm_timer_dispatch, 0 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register selection object, error=%s.",
                  sm_error_str( error ) );
        close( thread_info->tick_timer_fd );
        thread_info->tick_timer_fd = -1;
        goto ERROR;
    }

    thread_info->scheduling_on_time = true;
    sm_time_get( &thread_info->sched_timestamp );
    thread_info->timer_instance = 0;
    thread_info->tick_interval_in_ms = tick_interval_in_ms;
    thread_info->last_timer_dispatched = 0;
    thread_info->inuse = true;

    error = sm_timer_schedule( thread_info );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to schedule timers, error=%s.",
                  sm_error_str( error ) );
        close( thread_info->tick_timer_fd );
        thread_info->tick_timer_fd = -1;
        goto ERROR;
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_OKAY );

ERROR:
    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_FAILED );
}
// ****************************************************************************

// ****************************************************************************
// Timer - Finalize
// ================
SmErrorT sm_timer_finalize( void )
{
    SmTimerThreadInfoT* thread_info;
    SmErrorT error;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    thread_info = sm_timer_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        goto DONE;
    }

    memset( thread_info->timers, 0 , sizeof(thread_info->timers) );

    if( 0 <= thread_info->tick_timer_fd )
    {
        error = sm_selobj_deregister( thread_info->tick_timer_fd );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object, error=%s.",
                      sm_error_str( error ) );
        }

        close( thread_info->tick_timer_fd );
    }

DONE:
    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
    }

    return( SM_OKAY );
}
// ****************************************************************************
