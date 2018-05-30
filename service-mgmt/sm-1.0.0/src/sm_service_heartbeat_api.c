//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_heartbeat_api.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_selobj.h"
#include "sm_timer.h"

typedef enum
{
    SM_SERVICE_HEARTBEAT_MSG_TYPE_UNKNOWN,
    SM_SERVICE_HEARTBEAT_MSG_TYPE_START,
    SM_SERVICE_HEARTBEAT_MSG_TYPE_STOP,
    SM_SERVICE_HEARTBEAT_MSG_TYPE_OKAY,
    SM_SERVICE_HEARTBEAT_MSG_TYPE_WARN,
    SM_SERVICE_HEARTBEAT_MSG_TYPE_DEGRADE,
    SM_SERVICE_HEARTBEAT_MSG_TYPE_FAIL,
    SM_SERVICE_HEARTBEAT_MSG_TYPE_MAX,
} SmServiceHeartbeatApiMsgTypeT;

typedef struct
{
    char service_name[SM_SERVICE_NAME_MAX_CHAR];
} SmServiceHeartbeatApiMsgStartT;

typedef struct
{
    char service_name[SM_SERVICE_NAME_MAX_CHAR];
} SmServiceHeartbeatApiMsgStopT;

typedef struct
{
    char service_name[SM_SERVICE_NAME_MAX_CHAR];
} SmServiceHeartbeatApiMsgOkayT;

typedef struct
{
    char service_name[SM_SERVICE_NAME_MAX_CHAR];
} SmServiceHeartbeatApiMsgWarnT;

typedef struct
{
    char service_name[SM_SERVICE_NAME_MAX_CHAR];
} SmServiceHeartbeatApiMsgDegradeT;

typedef struct
{
    char service_name[SM_SERVICE_NAME_MAX_CHAR];
} SmServiceHeartbeatApiMsgFailT;

typedef struct
{
    SmServiceHeartbeatApiMsgTypeT msg_type;
    union
    {
        SmServiceHeartbeatApiMsgStartT start;
        SmServiceHeartbeatApiMsgStopT stop;
        SmServiceHeartbeatApiMsgOkayT okay;
        SmServiceHeartbeatApiMsgWarnT warn;
        SmServiceHeartbeatApiMsgDegradeT degrade;
        SmServiceHeartbeatApiMsgFailT fail;
    } u;
} SmServiceHeartbeatApiMsgT;

static int _sm_heartbeat_api_server_fd = -1;
static int _sm_heartbeat_api_client_fd = -1;
static SmListT* _callbacks = NULL;
static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

// ****************************************************************************
// Service Heartbeat API - Register Callbacks
// ==========================================
SmErrorT sm_service_heartbeat_api_register_callbacks( 
    SmServiceHeartbeatCallbacksT* callbacks )
{
    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    SM_LIST_PREPEND( _callbacks, (SmListEntryDataPtrT) callbacks );

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Deregister Callbacks
// ============================================
SmErrorT sm_service_heartbeat_api_deregister_callbacks(
    SmServiceHeartbeatCallbacksT* callbacks )
{
    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    SM_LIST_REMOVE( _callbacks, (SmListEntryDataPtrT) callbacks );

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
    }
    
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Start Heartbeat
// =======================================
SmErrorT sm_service_heartbeat_api_start_heartbeat( char service_name[] )
{
    SmServiceHeartbeatApiMsgT msg;
    SmServiceHeartbeatApiMsgStartT* start_msg = &(msg.u.start);

    memset( &msg, 0, sizeof(msg) );
    msg.msg_type = SM_SERVICE_HEARTBEAT_MSG_TYPE_START;
    snprintf( start_msg->service_name, sizeof(start_msg->service_name),
              "%s", service_name );

    if( 0 > send( _sm_heartbeat_api_server_fd, &msg, sizeof(msg), 0 ) )
    {
        DPRINTFE( "Failed to signal service heartbeat for service (%s) to "
                  "start, error=%s", service_name, strerror( errno ) );
        return( SM_FAILED );
    }

    DPRINTFD( "Heartbeat start called for service (%s).", service_name );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Stop Heartbeat
// ======================================
SmErrorT sm_service_heartbeat_api_stop_heartbeat( char service_name[] )
{
    SmServiceHeartbeatApiMsgT msg;
    SmServiceHeartbeatApiMsgStopT* stop_msg = &(msg.u.stop);

    memset( &msg, 0, sizeof(msg) );
    msg.msg_type = SM_SERVICE_HEARTBEAT_MSG_TYPE_STOP;
    snprintf( stop_msg->service_name, sizeof(stop_msg->service_name),
              "%s", service_name );

    if( 0 > send( _sm_heartbeat_api_server_fd, &msg, sizeof(msg), 0 ) )
    {
        DPRINTFE( "Failed to signal service heartbeat for service (%s) to "
                  "stop, error=%s", service_name, strerror( errno ) );
        return( SM_FAILED );
    }

    DPRINTFD( "Heartbeat stop called for service (%s).", service_name );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Okay Heartbeat
// ======================================
SmErrorT sm_service_heartbeat_api_okay_heartbeat( char service_name[] )
{
    SmServiceHeartbeatApiMsgT msg;
    SmServiceHeartbeatApiMsgOkayT* okay_msg = &(msg.u.okay);

    memset( &msg, 0, sizeof(msg) );
    msg.msg_type = SM_SERVICE_HEARTBEAT_MSG_TYPE_OKAY;
    snprintf( okay_msg->service_name, sizeof(okay_msg->service_name),
              "%s", service_name );

    if( 0 > send( _sm_heartbeat_api_client_fd, &msg, sizeof(msg), 0 ) )
    {
        DPRINTFE( "Failed to send service heartbeat okay for service (%s), "
                  "error=%s", service_name, strerror( errno ) );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Warn Heartbeat
// ======================================
SmErrorT sm_service_heartbeat_api_warn_heartbeat( char service_name[] )
{
    SmServiceHeartbeatApiMsgT msg;
    SmServiceHeartbeatApiMsgWarnT* warn_msg = &(msg.u.warn);

    memset( &msg, 0, sizeof(msg) );
    msg.msg_type = SM_SERVICE_HEARTBEAT_MSG_TYPE_WARN;
    snprintf( warn_msg->service_name, sizeof(warn_msg->service_name),
              "%s", service_name );

    if( 0 > send( _sm_heartbeat_api_client_fd, &msg, sizeof(msg), 0 ) )
    {
        DPRINTFE( "Failed to send service heartbeat warning for service (%s), "
                  "error=%s", service_name, strerror( errno ) );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Degrade Heartbeat
// =========================================
SmErrorT sm_service_heartbeat_api_degrade_heartbeat( char service_name[] )
{
    SmServiceHeartbeatApiMsgT msg;
    SmServiceHeartbeatApiMsgDegradeT* degrade_msg = &(msg.u.degrade);

    memset( &msg, 0, sizeof(msg) );
    msg.msg_type = SM_SERVICE_HEARTBEAT_MSG_TYPE_DEGRADE;
    snprintf( degrade_msg->service_name, sizeof(degrade_msg->service_name),
              "%s", service_name );

    if( 0 > send( _sm_heartbeat_api_client_fd, &msg, sizeof(msg), 0 ) )
    {
        DPRINTFE( "Failed to send service heartbeat degrade for service (%s), "
                  "error=%s", service_name, strerror( errno ) );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Fail Heartbeat
// ======================================
SmErrorT sm_service_heartbeat_api_fail_heartbeat( char service_name[] )
{
    SmServiceHeartbeatApiMsgT msg;
    SmServiceHeartbeatApiMsgFailT* fail_msg = &(msg.u.fail);

    memset( &msg, 0, sizeof(msg) );
    msg.msg_type = SM_SERVICE_HEARTBEAT_MSG_TYPE_FAIL;
    snprintf( fail_msg->service_name, sizeof(fail_msg->service_name),
              "%s", service_name );

    if( 0 > send( _sm_heartbeat_api_client_fd, &msg, sizeof(msg), 0 ) )
    {
        DPRINTFE( "Failed to send service heartbeat failure for service (%s), "
                  "error=%s", service_name, strerror( errno ) );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Dispatch
// ================================
static void sm_service_heartbeat_api_dispatch( int selobj, int64_t user_data )
{
    int bytes_read;
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceHeartbeatCallbacksT* callbacks;

    SmServiceHeartbeatApiMsgT msg;

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
            DPRINTFE( "Failed to receive message, errno=%s.",
                      strerror( errno ) );
            return;
        }

        DPRINTFE( "Interrupted while receiving message, retry=%d, errno=%s.",
                  retry_count, strerror( errno ) );
    }

    if( bytes_read != sizeof(msg) )
    {
        DPRINTFE( "Message truncated, bytes_read=%i, expected=%i",
                  bytes_read, (int) sizeof(msg) );
        return;
    }

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return;
    }

    SM_LIST_FOREACH( _callbacks, entry, entry_data )
    {
        callbacks = (SmServiceHeartbeatCallbacksT*) entry_data;

        switch( msg.msg_type )
        {
            case SM_SERVICE_HEARTBEAT_MSG_TYPE_START:
                if( NULL != callbacks->start_callback )
                {
                    callbacks->start_callback( msg.u.start.service_name );
                }
            break;

            case SM_SERVICE_HEARTBEAT_MSG_TYPE_STOP:
                if( NULL != callbacks->stop_callback )
                {
                    callbacks->stop_callback( msg.u.stop.service_name );
                }
            break;

            case SM_SERVICE_HEARTBEAT_MSG_TYPE_OKAY:
                if( NULL != callbacks->okay_callback )
                {
                    callbacks->okay_callback( msg.u.okay.service_name );
                }
            break;

            case SM_SERVICE_HEARTBEAT_MSG_TYPE_WARN:
                if( NULL != callbacks->warn_callback )
                {
                    callbacks->warn_callback( msg.u.warn.service_name );
                }
            break;

            case SM_SERVICE_HEARTBEAT_MSG_TYPE_DEGRADE:
                if( NULL != callbacks->degrade_callback )
                {
                    callbacks->degrade_callback( msg.u.degrade.service_name );
                }
            break;

            case SM_SERVICE_HEARTBEAT_MSG_TYPE_FAIL:
                if( NULL != callbacks->fail_callback )
                {
                    callbacks->fail_callback( msg.u.fail.service_name );
                }
            break;

            default:
                DPRINTFE( "Unknown message type (%i) received.", 
                          msg.msg_type );
            break;
        }
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Initialize
// ==================================
SmErrorT sm_service_heartbeat_api_initialize( bool main_process )
{
    SmErrorT error;

    if( main_process )
    {
        int flags;
        int sockets[2];
        int result;

        result = socketpair( AF_UNIX, SOCK_DGRAM, 0, sockets );
        if( 0 > result )
        {
            DPRINTFE( "Failed to create communication service heartbeat "
                      "sockets, error=%s.", strerror( errno ) );
            return( SM_FAILED );
        }

        int socket_i;
        for( socket_i=0; socket_i < 2; ++socket_i )
        {
            flags = fcntl( sockets[socket_i], F_GETFL, 0 );
            if( 0 > flags )
            {
                DPRINTFE( "Failed to get service heartbeat socket (%i) flags, "
                          "error=%s.", socket_i, strerror( errno ) );
                return( SM_FAILED );
            }

            result = fcntl( sockets[socket_i], F_SETFL, flags | O_NONBLOCK );
            if( 0 > result )
            {
                DPRINTFE( "Failed to set service heartbeat socket (%i) to "
                          "non-blocking, error=%s.", socket_i, 
                          strerror( errno ) );
                return( SM_FAILED );
            }
        }

        _sm_heartbeat_api_server_fd = sockets[0];
        _sm_heartbeat_api_client_fd = sockets[1];

        error = sm_selobj_register( _sm_heartbeat_api_client_fd, 
                                    sm_service_heartbeat_api_dispatch, 0 );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to register selection object, error=%s.",
                      sm_error_str( error ) );
            return( error );
        }
    } else {
        error = sm_selobj_register( _sm_heartbeat_api_server_fd, 
                                    sm_service_heartbeat_api_dispatch, 0 );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to register selection object, error=%s.",
                      sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat API - Finialize
// =================================
SmErrorT sm_service_heartbeat_api_finalize( bool main_process )
{
    SmErrorT error;

    if( main_process )
    {
        if( -1 < _sm_heartbeat_api_client_fd )
        {
            error = sm_selobj_deregister( _sm_heartbeat_api_client_fd );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to deregister selection object, error=%s.",
                          sm_error_str( error ) );
            }
        }
    } else {
        if( -1 < _sm_heartbeat_api_server_fd )
        {
            error = sm_selobj_deregister( _sm_heartbeat_api_server_fd );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to deregister selection object, error=%s.",
                          sm_error_str( error ) );
            }
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************
