//
// Copyright (c) 2015 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_notify_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <glib.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_timer.h"
#include "sm_selobj.h"

#define SM_NOTIFY_API_MSG_ADDRESS                   "/tmp/.sm_notify_api"

#define SM_NOTIFY_API_MSG_VERSION                   "1"
#define SM_NOTIFY_API_MSG_REVISION                  "1"
#define SM_NOTIFY_API_MSG_TYPE_SERVICE_EVENT        "SERVICE_EVENT"

#define SM_NOTIFY_API_MSG_SERVICE_EVENT_SYNC_START  "sync-start"
#define SM_NOTIFY_API_MSG_SERVICE_EVENT_SYNC_END    "sync-end"

#define SM_NOTIFY_API_MSG_VERSION_FIELD             0
#define SM_NOTIFY_API_MSG_REVISION_FIELD            1
#define SM_NOTIFY_API_MSG_TYPE_FIELD                2
#define SM_NOTIFY_API_MSG_ORIGIN_FIELD              3
#define SM_NOTIFY_API_MSG_SERVICE_NAME_FIELD        4
#define SM_NOTIFY_API_MSG_SERVICE_EVENT_FIELD       5

#define SM_NOTIFY_API_MAX_MSG_SIZE                  256

static int _sm_notify_api_socket = -1;
static char _rx_buffer[SM_NOTIFY_API_MAX_MSG_SIZE] __attribute__((aligned));
static SmNotifyApiCallbacksT _callbacks = {0};

// ****************************************************************************
// Notify API - Register Callbacks
// ===============================
SmErrorT sm_notify_api_register_callbacks( SmNotifyApiCallbacksT* callbacks )
{
    memcpy( &_callbacks, callbacks, sizeof(SmNotifyApiCallbacksT) );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Notify API - Deregister Callbacks
// =================================
SmErrorT sm_notify_api_deregister_callbacks( SmNotifyApiCallbacksT* callbacks )
{
    memset( &_callbacks, 0, sizeof(SmNotifyApiCallbacksT) );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Notify API - Service Event Value
// ================================
static SmNotifyServiceEventT sm_notify_api_service_event_value(
    const char* event_str )
{
    if( 0 == strcmp( SM_NOTIFY_API_MSG_SERVICE_EVENT_SYNC_START,
                     event_str ) )
    {
        return( SM_NOTIFY_SERVICE_EVENT_SYNC_START );

    } else if( 0 == strcmp( SM_NOTIFY_API_MSG_SERVICE_EVENT_SYNC_END,
                            event_str ) )
    {
        return( SM_NOTIFY_SERVICE_EVENT_SYNC_END );

    } else {
        return( SM_NOTIFY_SERVICE_EVENT_UNKNOWN );
    }
}
// ****************************************************************************

// ****************************************************************************
// Notify API - Dispatch
// =====================
static void sm_notify_api_dispatch( int selobj, int64_t user_data )
{
    char* service_name;
    gchar** params;
    int bytes_read;
    SmNotifyServiceEventT service_event;

    int retry_count;
    for( retry_count = 5; retry_count != 0; --retry_count )
    {
        memset( _rx_buffer, 0, sizeof(_rx_buffer) );

        bytes_read = recv( selobj, &_rx_buffer, sizeof(_rx_buffer), 0 );
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

        DPRINTFD( "Interrupted while receiving message, retry=%d, errno=%s.",
                  retry_count, strerror( errno ) );
    }

    params = g_strsplit( _rx_buffer, ",", -1 );
    if( params[SM_NOTIFY_API_MSG_VERSION_FIELD] == NULL )
    {
        DPRINTFE( "Missing version field in received message." );
        goto ERROR;
    }

    if( 0 != strcmp( SM_NOTIFY_API_MSG_VERSION,
                     params[SM_NOTIFY_API_MSG_VERSION_FIELD] ) )
    {
        DPRINTFE( "Unsupported version (%s) received, expected=%s.",
                  params[SM_NOTIFY_API_MSG_VERSION_FIELD], 
                  SM_NOTIFY_API_MSG_VERSION );
        goto ERROR;
    }

    if( params[SM_NOTIFY_API_MSG_REVISION_FIELD] == NULL )
    {
        DPRINTFE( "Missing revision field in received message." );
        goto ERROR;
    }

    if( params[SM_NOTIFY_API_MSG_TYPE_FIELD] == NULL )
    {
        DPRINTFE( "Missing message-type field in received message." );
        goto ERROR;
    }
    
    if( 0 == strcmp( SM_NOTIFY_API_MSG_TYPE_SERVICE_EVENT,
                     params[SM_NOTIFY_API_MSG_TYPE_FIELD] ) )
    {
        if( params[SM_NOTIFY_API_MSG_ORIGIN_FIELD] == NULL )
        {
            DPRINTFE( "Missing origin field in received message." );
            goto ERROR;
        }

        if( params[SM_NOTIFY_API_MSG_SERVICE_NAME_FIELD] == NULL )
        {
            DPRINTFE( "Missing service-name field in received message." );
            goto ERROR;
        }
        
        service_name = (char*) params[SM_NOTIFY_API_MSG_SERVICE_NAME_FIELD];
        
        if( params[SM_NOTIFY_API_MSG_SERVICE_EVENT_FIELD] == NULL )
        {
            DPRINTFE( "Missing service-event field in received message." );
            goto ERROR;
        }

        service_event = sm_notify_api_service_event_value( (char*)
                            params[SM_NOTIFY_API_MSG_SERVICE_EVENT_FIELD]);

        if( NULL != _callbacks.service_event )
        {
            _callbacks.service_event( service_name, service_event );
        }

    } else {
        DPRINTFE( "Unknown/unsupported message-type (%s) received.",
                  params[SM_NOTIFY_API_MSG_TYPE_FIELD] );
        goto ERROR;
    }

    g_strfreev( params );
    return;

ERROR:
    g_strfreev( params );
    return;
}
// ***************************************************************************

// ***************************************************************************
// Notify API - Initialize
// =======================
SmErrorT sm_notify_api_initialize( void )
{
    int sock;
    socklen_t len;
    struct sockaddr_un src_addr;
    int result;
    SmErrorT error;

    memset( &src_addr, 0, sizeof(src_addr) );

    src_addr.sun_family = AF_UNIX;
    snprintf( src_addr.sun_path, sizeof(src_addr.sun_path),
              SM_NOTIFY_API_MSG_ADDRESS );

    sock = socket( AF_LOCAL, SOCK_DGRAM, 0 );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to create socket for sm-notify-api, error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    unlink( src_addr.sun_path );
    
    len = strlen(src_addr.sun_path) + sizeof(src_addr.sun_family);

    result = bind( sock, (struct sockaddr *) &src_addr, len );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind address (%s) to socket for sm-notify-api, "
                  "error=%s.", src_addr.sun_path, strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    _sm_notify_api_socket = sock;

    error = sm_selobj_register( _sm_notify_api_socket, 
                                sm_notify_api_dispatch, 0 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register selection object, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }
    
    return( SM_OKAY );
}
// ***************************************************************************

// ***************************************************************************
// Notify API - Finalize
// =====================
SmErrorT sm_notify_api_finalize( void )
{
    SmErrorT error;

    memset( &_callbacks, 0, sizeof(_callbacks) );

    if( -1 < _sm_notify_api_socket )
    {
        error = sm_selobj_deregister( _sm_notify_api_socket );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object, error=%s.",
                      sm_error_str( error ) );
        }
    }

    return( SM_OKAY );
}
// ***************************************************************************
