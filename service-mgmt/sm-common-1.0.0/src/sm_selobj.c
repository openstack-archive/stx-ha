//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_selobj.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"

#define SM_SEL_OBJ_ENTRY_VALID  0xFDFDFDFD

typedef struct 
{
    uint32_t valid;
    int selobj;
    SmSelObjCallbackT callback;
    int64_t user_data;
} SmSelObjSelectEntryT;

typedef struct
{
    bool inuse;
    pthread_t thread_id;
    int last_selobj;
    fd_set selobjs_set;
    SmSelObjSelectEntryT selobjs[SM_THREAD_SELECT_OBJS_MAX];
} SmSelObjThreadInfoT;

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
static SmSelObjThreadInfoT _threads[SM_THREADS_MAX];

// ****************************************************************************
// Selection Object - Find Thread Info
// ===================================
static SmSelObjThreadInfoT* sm_selobj_find_thread_info( void )
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
// Selection Object - Find Selection Object
// ========================================
static SmSelObjSelectEntryT* sm_selobj_find( SmSelObjThreadInfoT* thread_info,
    int selobj )
{
    SmSelObjSelectEntryT* entry;

    unsigned int entry_i;
    for( entry_i=0; SM_THREAD_SELECT_OBJS_MAX > entry_i; ++entry_i )
    {
        entry = (SmSelObjSelectEntryT*) &(thread_info->selobjs[entry_i]);

        if( SM_SEL_OBJ_ENTRY_VALID == entry->valid )
        {
            if( selobj == entry->selobj )
            {
                return( entry );
            }
        }
    }
    
    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Selection Object - Register
// ===========================
SmErrorT sm_selobj_register( int selobj, SmSelObjCallbackT callback,
    int64_t user_data )
{
    SmSelObjThreadInfoT* thread_info;
    SmSelObjSelectEntryT* entry;

    thread_info = sm_selobj_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( SM_FAILED );
    }

    entry = sm_selobj_find( thread_info, selobj );
    if( NULL == entry )
    {
        unsigned int entry_i;
        for( entry_i=0; SM_THREAD_SELECT_OBJS_MAX > entry_i; ++entry_i )
        {
            entry = (SmSelObjSelectEntryT*) &(thread_info->selobjs[entry_i]);
            if( SM_SEL_OBJ_ENTRY_VALID != entry->valid )
            {
                entry->valid = SM_SEL_OBJ_ENTRY_VALID;
                entry->selobj = selobj;
                entry->callback = callback;
                entry->user_data = user_data;
                FD_SET( selobj, &(thread_info->selobjs_set) );
                if( selobj > thread_info->last_selobj )
                {
                    thread_info->last_selobj = selobj;
                }
                break;
            }
        }
    } else {
        entry->callback = callback;
        entry->user_data = user_data;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Selection Object - Deregister
// =============================
SmErrorT sm_selobj_deregister( int selobj )
{
    SmSelObjThreadInfoT* thread_info;
    SmSelObjSelectEntryT* entry;

    thread_info = sm_selobj_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( SM_FAILED );
    }

    entry = sm_selobj_find( thread_info, selobj );
    if( NULL != entry )
    {
        memset( entry, 0, sizeof(SmSelObjSelectEntryT) );
        FD_CLR( selobj, &(thread_info->selobjs_set) );
        thread_info->last_selobj = 0;

        unsigned int entry_i;
        for( entry_i=0; SM_THREAD_SELECT_OBJS_MAX > entry_i; ++entry_i )
        {
            entry = (SmSelObjSelectEntryT*) &(thread_info->selobjs[entry_i]);

            if( SM_SEL_OBJ_ENTRY_VALID == entry->valid )
            {
                if( entry->selobj > thread_info->last_selobj )
                {
                    thread_info->last_selobj = entry->selobj;
                }
            }
        }    
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Selection Object - Dispatch
// ===========================
SmErrorT sm_selobj_dispatch( unsigned int timeout_in_ms )
{
    SmSelObjThreadInfoT* thread_info;
    SmSelObjSelectEntryT* entry;
    int num_fds;
    fd_set fds;
    struct timeval tv;
    int result;

    thread_info = sm_selobj_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( SM_FAILED );
    }

    num_fds = thread_info->last_selobj;
    fds = thread_info->selobjs_set;
    
    tv.tv_sec = timeout_in_ms / 1000;
    tv.tv_usec = (timeout_in_ms % 1000) * 1000;
        
    result = select( num_fds+1, &fds, NULL, NULL, &tv ); 
    if( 0 > result )
    {
        if( errno == EINTR )
        {
            DPRINTFD( "Interrupted by a signal." );
            return( SM_OKAY );
        } else {
            DPRINTFE( "Select failed, error=%s.", strerror( errno ) );
            return( SM_FAILED );
        }
    } else if( 0 == result ) {
        DPRINTFD( "Nothing selected." );
        return( SM_OKAY );
    }

    unsigned int entry_i;
    for( entry_i=0; SM_THREAD_SELECT_OBJS_MAX > entry_i; ++entry_i )
    {
        entry = (SmSelObjSelectEntryT*) &(thread_info->selobjs[entry_i]);

        if( SM_SEL_OBJ_ENTRY_VALID != entry->valid )
            continue;

        if( FD_ISSET( entry->selobj, &fds ) )
        {
            if( NULL != entry->callback )
            {
                entry->callback( entry->selobj, entry->user_data );
            }
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Selection Object - Initialize
// =============================
SmErrorT sm_selobj_initialize( void )
{
    SmSelObjThreadInfoT* thread_info = NULL;

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
    
    memset( thread_info, 0, sizeof(SmSelObjThreadInfoT) );

    thread_info->inuse = true;
    thread_info->thread_id = pthread_self();
    FD_ZERO( &(thread_info->selobjs_set) );

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
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
// Selection Object - Finalize
// ===========================
SmErrorT sm_selobj_finalize( void )
{
    SmSelObjThreadInfoT* thread_info;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    thread_info = sm_selobj_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( SM_OKAY );
    }

    memset( thread_info, 0, sizeof(SmSelObjThreadInfoT) );
    FD_ZERO( &(thread_info->selobjs_set) );

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
    }

    return( SM_OKAY );
}
// ****************************************************************************
