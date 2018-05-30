//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_heartbeat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_timer.h"
#include "sm_selobj.h"
#include "sm_db.h"
#include "sm_db_service_heartbeat.h"
#include "sm_service_heartbeat_api.h"

static SmDbHandleT* _sm_db_handle = NULL;
static SmServiceHeartbeatCallbacksT _hb_callbacks;

// ****************************************************************************
// Service Heartbeat - Create Unix Socket
// ======================================
static SmErrorT sm_service_heartbeat_create_unix_socket( 
    SmDbServiceHeartbeatT* service_heartbeat, int* socket_fd )
{
    int sock;
    socklen_t len;
    struct sockaddr_un src_addr;
    int result;

    memset( &src_addr, 0, sizeof(src_addr) );

    src_addr.sun_family = AF_UNIX;
    snprintf( src_addr.sun_path, sizeof(src_addr.sun_path), "%s",
              service_heartbeat->src_address );

    sock = socket( AF_LOCAL, SOCK_DGRAM, 0 );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to create socket for service (%s), error=%s.",
                  service_heartbeat->name, strerror( errno ) );
        return( SM_FAILED );
    }

    unlink( src_addr.sun_path );
    
    len = strlen( src_addr.sun_path) + sizeof(src_addr.sun_family);

    result = bind( sock, (struct sockaddr *) &src_addr, len );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind address (%s) to socket for service (%s), "
                  "error=%s.", src_addr.sun_path, service_heartbeat->name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    *socket_fd = sock;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat - Create UDP Socket
// ======================================
static SmErrorT sm_service_heartbeat_create_udp_socket( 
    SmDbServiceHeartbeatT* service_heartbeat, int* socket_fd )
{
    int sock;
    struct sockaddr_in src_addr;
    int result;

    memset( &src_addr, 0, sizeof(src_addr) );

    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(service_heartbeat->src_port);
    src_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to create socket for service (%s), error=%s.",
                  service_heartbeat->name, strerror( errno ) );
        return( SM_FAILED );
    }

    result = bind( sock, (struct sockaddr *) &src_addr, sizeof(src_addr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind address to socket for service (%s), "
                  "error=%s.", service_heartbeat->name, strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    *socket_fd = sock;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat - Create Socket
// =================================
static SmErrorT sm_service_heartbeat_create_socket( 
    SmDbServiceHeartbeatT* service_heartbeat, int* socket_fd )
{
    if( SM_SERVICE_HEARTBEAT_TYPE_UNIX == service_heartbeat->type )
    {
        return( sm_service_heartbeat_create_unix_socket( service_heartbeat,
                                                         socket_fd ) );
    } 
    else if( SM_SERVICE_HEARTBEAT_TYPE_UDP == service_heartbeat->type )
    {
        return( sm_service_heartbeat_create_udp_socket( service_heartbeat,
                                                        socket_fd ) );
    } else {
        DPRINTFE( "Unknown heartbeat type (%s).", 
                  sm_service_heartbeat_type_str(service_heartbeat->type) );
        return( SM_FAILED );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat - Timer
// =========================
static bool sm_service_heartbeat_timer( SmTimerIdT timer_id, int64_t user_data )
{
    int64_t id = user_data;
    SmDbServiceHeartbeatT service_heartbeat;
    SmErrorT error;

    error = sm_db_service_heartbeat_read_by_id( _sm_db_handle, id,
                                                &service_heartbeat );
    if( SM_OKAY != error )
    {
        if( SM_NOT_FOUND == error )
        {
            DPRINTFD( "Not service heartbeat required for service." );
            return( false );
        } else {
            DPRINTFE( "Failed to loop over service heartbeat, error=%s.",
                      sm_error_str( error ) );
            return( true );
        }
    }

    if( timer_id != service_heartbeat.heartbeat_timer_id )
    {
        DPRINTFI( "Timer mismatch for service (%s).", service_heartbeat.name );
        return( false );
    }

    if( SM_SERVICE_HEARTBEAT_STATE_STARTED !=  service_heartbeat.state )
    {
        DPRINTFI( "Service (%s) heartbeat in the stopped state.",
                  service_heartbeat.name );
        return( false );
    }

    service_heartbeat.missed++;

    if(( service_heartbeat.missed > service_heartbeat.missed_fail )&&
       ( 0 != service_heartbeat.missed_fail ))
    {
        error = sm_service_heartbeat_api_fail_heartbeat( service_heartbeat.name );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send failure message for service (%s), "
                      "error=%s.", service_heartbeat.name,
                      sm_error_str( error ) );
        }

    } else if(( service_heartbeat.missed > service_heartbeat.missed_degrade )&&
              ( 0 != service_heartbeat.missed_degrade ))
    {
        error = sm_service_heartbeat_api_degrade_heartbeat( service_heartbeat.name );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send degrade message for service (%s), "
                      "error=%s.", service_heartbeat.name,
                      sm_error_str( error ) );
        }

    } else if(( service_heartbeat.missed > service_heartbeat.missed_warn )&&
              ( 0 != service_heartbeat.missed_warn ))
    {
        error = sm_service_heartbeat_api_warn_heartbeat( service_heartbeat.name );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send warning message for service (%s), "
                      "error=%s.", service_heartbeat.name,
                      sm_error_str( error ) );
        }
    } else {
        error = sm_service_heartbeat_api_okay_heartbeat( service_heartbeat.name );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send okay message for service (%s), "
                      "error=%s.", service_heartbeat.name,
                      sm_error_str( error ) );
        }
    }

    error = sm_db_service_heartbeat_update( _sm_db_handle, &service_heartbeat );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update service heartbeat information for "
                  "service (%s), error=%s.", service_heartbeat.name,
                  sm_error_str( error ) );
        abort();
    }

    if( SM_SERVICE_HEARTBEAT_TYPE_UNIX == service_heartbeat.type )
    {
        struct sockaddr_un dst_addr;

        memset( &dst_addr, 0, sizeof(dst_addr) );
        
        dst_addr.sun_family = AF_UNIX;
        snprintf( dst_addr.sun_path, sizeof(dst_addr.sun_path), "%s",
                  service_heartbeat.dst_address );

        if( 0 > sendto( service_heartbeat.heartbeat_socket, 
                        service_heartbeat.message, 
                        strlen(service_heartbeat.message)+1, 0,
                        (struct sockaddr*) &dst_addr, sizeof(dst_addr) ) )
        {
            DPRINTFE( "Failed to send service heartbeat for service (%s), "
                      " error=%s", service_heartbeat.name, strerror( errno ) );
        }

    } else if( SM_SERVICE_HEARTBEAT_TYPE_UDP == service_heartbeat.type ) {
        struct sockaddr_in dst_addr;

        memset( &dst_addr, 0, sizeof(dst_addr) );

        dst_addr.sin_family = AF_INET;
        dst_addr.sin_port = htons(service_heartbeat.dst_port);
        dst_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if( 0 > sendto( service_heartbeat.heartbeat_socket, 
                        service_heartbeat.message, 
                        strlen(service_heartbeat.message)+1, 0,
                        (struct sockaddr*) &dst_addr, sizeof(dst_addr) ) )
        {
            DPRINTFE( "Failed to send service heartbeat for service (%s), "
                      " error=%s", service_heartbeat.name, strerror( errno ) );
        }

    } else {
        DPRINTFE( "Unknown heartbeat type (%s).", 
                  sm_service_heartbeat_type_str(service_heartbeat.type) );
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat - Dispatch
// ============================
static void sm_service_heartbeat_dispatch( int selobj, int64_t user_data )
{
    int bytes_read;
    char msg[SM_SERVICE_HEARTBEAT_MESSAGE_MAX_CHAR] = {0};
    int64_t id = user_data;
    SmDbServiceHeartbeatT service_heartbeat;
    SmErrorT error;

    error = sm_db_service_heartbeat_read_by_id( _sm_db_handle, id,
                                                &service_heartbeat );
    if( SM_OKAY != error )
    {
        if( SM_NOT_FOUND == error )
        {
            DPRINTFD( "Not service heartbeat required." );

            error = sm_selobj_deregister( selobj );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to deregister selection object, error=%s.",
                          sm_error_str( error ) );
            }

            close( selobj );

        } else {
            DPRINTFE( "Failed to loop over service heartbeat, error=%s.",
                      sm_error_str( error ) );
        }
        return;
    }

    if( selobj != service_heartbeat.heartbeat_socket )
    {
        DPRINTFI( "Selection object mismatch for service (%s).",
                  service_heartbeat.name );

        error = sm_selobj_deregister( selobj );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object, error=%s.",
                      sm_error_str( error ) );
            return;
        }

        close( selobj );
        return;
    }

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

        DPRINTFD( "Interrupted while receiving message, retry=%d, errno=%s.",
                  retry_count, strerror( errno ) );
    }

    service_heartbeat.missed = 0;

    error = sm_db_service_heartbeat_update( _sm_db_handle, &service_heartbeat );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update service heartbeat information for "
                  "service (%s), error=%s.", service_heartbeat.name,
                  sm_error_str( error ) );
        abort();
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat - Start Callback
// ==================================
static void sm_service_heartbeat_start_callback( char service_name[] )
{
    char timer_name[80] = "";
    int socket_fd;
    SmTimerIdT timer_id;
    SmDbServiceHeartbeatT service_heartbeat;
    SmErrorT error;

    DPRINTFD( "Service (%s) heartbeat start callback.", service_name );

    error = sm_db_service_heartbeat_read( _sm_db_handle, service_name,
                                          &service_heartbeat );
    if( SM_OKAY != error )
    {
        if( SM_NOT_FOUND == error )
        {
            DPRINTFD( "Not service heartbeat required for service (%s).",
                      service_name );
        } else {
            DPRINTFE( "Failed to loop over service (%s) dependencies, "
                      "error=%s.", service_name, sm_error_str( error ) );
        }
        return;
    }

    if( SM_SERVICE_HEARTBEAT_STATE_STARTED == service_heartbeat.state )
    {
        DPRINTFD( "Service (%s) heartbeat already started.",
                  service_heartbeat.name );
        return;
    }

    error = sm_service_heartbeat_create_socket( &service_heartbeat, 
                                                &socket_fd );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create heartbeat socket for service (%s), "
                  "error=%s.", service_name, sm_error_str( error ) );
        return;
    }

    error = sm_selobj_register( socket_fd, sm_service_heartbeat_dispatch,
                                service_heartbeat.id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register selection object, error=%s.",
                  sm_error_str( error ) );
        close( socket_fd );
        return;
    }

    snprintf( timer_name, sizeof(timer_name), "%s heartbeat",
              service_name );

    error = sm_timer_register( timer_name, service_heartbeat.interval_in_ms,
                               sm_service_heartbeat_timer,
                               service_heartbeat.id, &timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service heartbeat timer for service (%s), "
                  "error=%s.", service_heartbeat.name, sm_error_str( error ) );
        close( socket_fd );
        return;
    }

    service_heartbeat.state = SM_SERVICE_HEARTBEAT_STATE_STARTED;
    service_heartbeat.heartbeat_timer_id = timer_id;
    service_heartbeat.heartbeat_socket = socket_fd;
    service_heartbeat.missed = 0;

    error = sm_db_service_heartbeat_update( _sm_db_handle, &service_heartbeat );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update service heartbeat information for "
                  "service (%s), error=%s.", service_heartbeat.name,
                  sm_error_str( error ) );
        abort();
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat - Stop Callback
// =================================
static void sm_service_heartbeat_stop_callback( char service_name[] )
{
    SmDbServiceHeartbeatT service_heartbeat;
    SmErrorT error;

    DPRINTFD( "Service (%s) heartbeat stop callback.", service_name );

    error = sm_db_service_heartbeat_read( _sm_db_handle, service_name,
                                          &service_heartbeat );
    if( SM_OKAY != error )
    {
        if( SM_NOT_FOUND == error )
        {
            DPRINTFD( "Not service heartbeat required for service (%s).",
                      service_name );
        } else {
            DPRINTFE( "Failed to loop over service (%s) dependencies, "
                      "error=%s.", service_name, sm_error_str( error ) );
        }
        return;
    }

    if( -1 <  service_heartbeat.heartbeat_socket )
    {
        error = sm_selobj_deregister( service_heartbeat.heartbeat_socket );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object, error=%s.",
                      sm_error_str( error ) );
        }

        close( service_heartbeat.heartbeat_socket );
    }

    if( SM_TIMER_ID_INVALID != service_heartbeat.heartbeat_timer_id )
    {
        error = sm_timer_deregister( service_heartbeat.heartbeat_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to delete service heartbeat timer for "
                      "service (%s), error=%s.", service_heartbeat.name,
                      sm_error_str( error ) );
        }
    }

    service_heartbeat.state = SM_SERVICE_HEARTBEAT_STATE_STOPPED;
    service_heartbeat.heartbeat_timer_id = SM_TIMER_ID_INVALID;
    service_heartbeat.heartbeat_socket = -1;
    service_heartbeat.missed = 0;

    error = sm_db_service_heartbeat_update( _sm_db_handle, &service_heartbeat );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update service heartbeat information for "
                  "service (%s), error=%s.", service_heartbeat.name,
                  sm_error_str( error ) );
        abort();
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat - Initialize
// ==============================
SmErrorT sm_service_heartbeat_initialize( void )
{
    SmErrorT error;

    error = sm_db_connect( SM_HEARTBEAT_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    memset( &_hb_callbacks, 0, sizeof(_hb_callbacks) );

    _hb_callbacks.start_callback = sm_service_heartbeat_start_callback;
    _hb_callbacks.stop_callback  = sm_service_heartbeat_stop_callback;
    
    error = sm_service_heartbeat_api_register_callbacks( &_hb_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register service heartbeat callbacks, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }  

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Heartbeat - Finalize
// ============================
SmErrorT sm_service_heartbeat_finalize( void )
{
    SmErrorT error;

    if( NULL != _sm_db_handle )
    {
        error = sm_db_disconnect( _sm_db_handle );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to disconnect from database (%s), error=%s.",
                      SM_DATABASE_NAME, sm_error_str( error ) );
        }

        _sm_db_handle = NULL;
    }

    error = sm_service_heartbeat_api_deregister_callbacks( &_hb_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister service heartbeat callbacks, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    memset( &_hb_callbacks, 0, sizeof(_hb_callbacks) );

    return( SM_OKAY );
}
// ****************************************************************************
