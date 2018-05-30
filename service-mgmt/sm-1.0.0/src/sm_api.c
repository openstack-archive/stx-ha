//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_api.h"

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
#include "sm_list.h"
#include "sm_timer.h"
#include "sm_selobj.h"

#define SM_API_MSG_SERVER_ADDRESS                   "/tmp/.sm_server_api"
#define SM_API_MSG_CLIENT_ADDRESS                   "/tmp/.sm_client_api"

#define SM_API_MSG_VERSION                          "1"
#define SM_API_MSG_REVISION                         "1"
#define SM_API_MSG_TYPE_SET_NODE                    "SET_NODE"
#define SM_API_MSG_TYPE_SET_NODE_ACK                "SET_NODE_ACK"
#define SM_API_MSG_TYPE_RESTART_SERVICE             "RESTART_SERVICE"
#define SM_API_MSG_SKIP_DEP_CHECK                   "skip-dep"

#define SM_API_MSG_NODE_ACTION_UNKNOWN              "unknown"
#define SM_API_MSG_NODE_ACTION_LOCK                 "lock"
#define SM_API_MSG_NODE_ACTION_UNLOCK               "unlock"
#define SM_API_MSG_NODE_ACTION_SWACT                "swact"
#define SM_API_MSG_NODE_ACTION_SWACT_FORCE          "swact-force"
#define SM_API_MSG_NODE_ACTION_EVENT                "event"

#define SM_API_MSG_NODE_ADMIN_STATE_UNKNOWN         "unknown"
#define SM_API_MSG_NODE_ADMIN_STATE_LOCKED          "locked"
#define SM_API_MSG_NODE_ADMIN_STATE_UNLOCKED        "unlocked"

#define SM_API_MSG_NODE_OPER_STATE_UNKNOWN          "unknown"
#define SM_API_MSG_NODE_OPER_STATE_ENABLED          "enabled"
#define SM_API_MSG_NODE_OPER_STATE_DISABLED         "disabled"

#define SM_API_MSG_NODE_AVAIL_STATUS_UNKNOWN        "unknown"
#define SM_API_MSG_NODE_AVAIL_STATUS_NONE           "none"
#define SM_API_MSG_NODE_AVAIL_STATUS_AVAILABLE      "available"
#define SM_API_MSG_NODE_AVAIL_STATUS_DEGRADED       "degraded"
#define SM_API_MSG_NODE_AVAIL_STATUS_FAILED         "failed"

#define SM_API_MSG_VERSION_FIELD                    0
#define SM_API_MSG_REVISION_FIELD                   1
#define SM_API_MSG_SEQNO_FIELD                      2
#define SM_API_MSG_TYPE_FIELD                       3
#define SM_API_MSG_ORIGIN_FIELD                     4
#define SM_API_MSG_NODE_NAME_FIELD                  5
#define SM_API_MSG_NODE_ACTION_FIELD                6
#define SM_API_MSG_NODE_ADMIN_FIELD                 7
#define SM_API_MSG_NODE_OPER_FIELD                  8
#define SM_API_MSG_NODE_AVAIL_FIELD                 9

#define SM_API_MSG_SERVICE_NAME_FIELD               5
#define SM_API_MSG_PARAM                            6

#define SM_API_MAX_MSG_SIZE                         2048

static int _sm_api_socket = -1;
static struct sockaddr_un _sm_api_client_address = {0};
static socklen_t _sm_api_client_address_len = 0;
static char _tx_buffer[SM_API_MAX_MSG_SIZE] __attribute__((aligned));
static char _rx_buffer[SM_API_MAX_MSG_SIZE] __attribute__((aligned));
static SmApiCallbacksT _callbacks = {0};

// ****************************************************************************
// API - Register Callbacks
// ========================
SmErrorT sm_api_register_callbacks( SmApiCallbacksT* callbacks )
{
    memcpy( &_callbacks, callbacks, sizeof(SmApiCallbacksT) );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// API - Deregister Callbacks
// ==========================
SmErrorT sm_api_deregister_callbacks( SmApiCallbacksT* callbacks )
{
    memset( &_callbacks, 0, sizeof(SmApiCallbacksT) );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// API - Node Set Action String
// ============================
static const char* sm_api_node_action_string( SmNodeSetActionT action )
{
    switch( action )
    {
        case SM_NODE_SET_ACTION_LOCK:
            return( SM_API_MSG_NODE_ACTION_LOCK );
        break;
        case SM_NODE_SET_ACTION_UNLOCK:
            return( SM_API_MSG_NODE_ACTION_UNLOCK );
        break;
        case SM_NODE_SET_ACTION_SWACT:
            return( SM_API_MSG_NODE_ACTION_SWACT );
        break;
        case SM_NODE_SET_ACTION_SWACT_FORCE:
            return( SM_API_MSG_NODE_ACTION_SWACT_FORCE );
        break;
        case SM_NODE_SET_ACTION_EVENT:
            return( SM_API_MSG_NODE_ACTION_EVENT );
        break;
        default:
            return( SM_API_MSG_NODE_ACTION_UNKNOWN );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// API - Node Set Action Value
// ===========================
static SmNodeSetActionT sm_api_node_action_value( const char* action_str )
{
    if( 0 == strcmp( SM_API_MSG_NODE_ACTION_LOCK, action_str ) )
    {
        return( SM_NODE_SET_ACTION_LOCK );

    } else if( 0 == strcmp( SM_API_MSG_NODE_ACTION_UNLOCK, action_str ) ) {
        return( SM_NODE_SET_ACTION_UNLOCK );

    } else if( 0 == strcmp( SM_API_MSG_NODE_ACTION_SWACT, action_str ) ) {
        return( SM_NODE_SET_ACTION_SWACT );

    } else if( 0 == strcmp( SM_API_MSG_NODE_ACTION_SWACT_FORCE, action_str ) ) {
        return( SM_NODE_SET_ACTION_SWACT_FORCE );

    } else if( 0 == strcmp( SM_API_MSG_NODE_ACTION_EVENT, action_str ) ) {
        return( SM_NODE_SET_ACTION_EVENT );

    } else {
        return( SM_NODE_SET_ACTION_UNKNOWN );
    }
}
// ****************************************************************************

// ****************************************************************************
// API - Node Administrative State String
// ======================================
static const char* sm_api_node_admin_state_string(
    SmNodeAdminStateT admin_state )
{
    switch( admin_state )
    {
        case SM_NODE_ADMIN_STATE_LOCKED:
            return( SM_API_MSG_NODE_ADMIN_STATE_LOCKED );
        break;
        case SM_NODE_ADMIN_STATE_UNLOCKED:
            return( SM_API_MSG_NODE_ADMIN_STATE_UNLOCKED );
        break;
        default:
            return( SM_API_MSG_NODE_ADMIN_STATE_UNKNOWN );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// API - Node Administrative State Value
// =====================================
static SmNodeAdminStateT sm_api_node_admin_state_value(
    const char* state_str )
{
    if( 0 == strcmp( SM_API_MSG_NODE_ADMIN_STATE_LOCKED, state_str ) )
    {
        return( SM_NODE_ADMIN_STATE_LOCKED );

    } else if( 0 == strcmp( SM_API_MSG_NODE_ADMIN_STATE_UNLOCKED,
                            state_str ) )
    {
        return( SM_NODE_ADMIN_STATE_UNLOCKED );

    } else {
        return( SM_NODE_ADMIN_STATE_UNKNOWN );
    }
}
// ****************************************************************************

// ****************************************************************************
// API - Node Operational State String
// ===================================
static const char* sm_api_node_oper_state_string(
    SmNodeOperationalStateT oper_state )
{
    switch( oper_state )
    {
        case SM_NODE_OPERATIONAL_STATE_ENABLED:
            return( SM_API_MSG_NODE_OPER_STATE_ENABLED );
        break;
        case SM_NODE_OPERATIONAL_STATE_DISABLED:
            return( SM_API_MSG_NODE_OPER_STATE_DISABLED );
        break;
        default:
            return( SM_API_MSG_NODE_OPER_STATE_UNKNOWN );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// API - Node Operational State Value
// ==================================
static SmNodeOperationalStateT sm_api_node_oper_state_value(
    const char* state_str )
{
    if( 0 == strcmp( SM_API_MSG_NODE_OPER_STATE_ENABLED, state_str ) )
    {
        return( SM_NODE_OPERATIONAL_STATE_ENABLED );

    } else if( 0 == strcmp( SM_API_MSG_NODE_OPER_STATE_DISABLED,
                            state_str ) )
    {
        return( SM_NODE_OPERATIONAL_STATE_DISABLED );

    } else {
        return( SM_NODE_OPERATIONAL_STATE_UNKNOWN );
    }
}
// ****************************************************************************

// ****************************************************************************
// API - Node Availability Status String
// =====================================
static const char* sm_api_node_avail_status_string(
    SmNodeAvailStatusT avail_status )
{
    switch( avail_status )
    {
        case SM_NODE_AVAIL_STATUS_NONE:
            return( SM_API_MSG_NODE_AVAIL_STATUS_NONE );
        break;
        case SM_NODE_AVAIL_STATUS_AVAILABLE:
            return( SM_API_MSG_NODE_AVAIL_STATUS_AVAILABLE );
        break;
        case SM_NODE_AVAIL_STATUS_DEGRADED:
            return( SM_API_MSG_NODE_AVAIL_STATUS_DEGRADED );
        break;
        case SM_NODE_AVAIL_STATUS_FAILED:
            return( SM_API_MSG_NODE_AVAIL_STATUS_FAILED );
        break;
        default:
            return( SM_API_MSG_NODE_AVAIL_STATUS_UNKNOWN );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// API - Node Availability Status Value
// ====================================
static SmNodeAvailStatusT sm_api_node_avail_status_value(
    const char* state_str )
{
    if( 0 == strcmp( SM_API_MSG_NODE_AVAIL_STATUS_NONE, state_str ) )
    {
        return( SM_NODE_AVAIL_STATUS_NONE );

    } else if( 0 == strcmp( SM_API_MSG_NODE_AVAIL_STATUS_AVAILABLE,
                            state_str ) )
    {
        return( SM_NODE_AVAIL_STATUS_AVAILABLE );

    } else if( 0 == strcmp( SM_API_MSG_NODE_AVAIL_STATUS_DEGRADED,
                            state_str ) )
    {
        return( SM_NODE_AVAIL_STATUS_DEGRADED );

    } else if( 0 == strcmp( SM_API_MSG_NODE_AVAIL_STATUS_FAILED,
                            state_str ) )
    {
        return( SM_NODE_AVAIL_STATUS_FAILED );

    } else {
        return( SM_NODE_AVAIL_STATUS_UNKNOWN );
    }
}
// ****************************************************************************

// ****************************************************************************
// API - Send Node Set
// ===================
SmErrorT sm_api_send_node_set( char node_name[], SmNodeSetActionT action,
    SmNodeAdminStateT admin_state, SmNodeOperationalStateT oper_state,
    SmNodeAvailStatusT avail_status, int seqno )
{
    const char* action_str = sm_api_node_action_string(action);
    const char* admin_state_str = sm_api_node_admin_state_string(admin_state);
    const char* oper_state_str = sm_api_node_oper_state_string(oper_state);
    const char* avail_status_str = sm_api_node_avail_status_string(avail_status);
    int bytes_written;

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    bytes_written = snprintf( _tx_buffer, sizeof(_tx_buffer),
                              "%s,%s,%i,%s,%s,%s,%s,%s,%s,%s", 
                              SM_API_MSG_VERSION, SM_API_MSG_REVISION,
                              seqno, SM_API_MSG_TYPE_SET_NODE, "sm",
                              node_name, action_str, admin_state_str,
                              oper_state_str, avail_status_str );
    if( 0 < bytes_written )
    {
        if( 0 > sendto( _sm_api_socket, &_tx_buffer, bytes_written, 0,
                        (struct sockaddr*) &_sm_api_client_address,
                        _sm_api_client_address_len ) )
        {
            DPRINTFE( "Failed to send node (%s) set, error=%s", 
                      node_name, strerror( errno ) );
            return( SM_FAILED );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// API - Send Node Set Ack
// =======================
SmErrorT sm_api_send_node_set_ack( char node_name[], SmNodeSetActionT action,
    SmNodeAdminStateT admin_state, SmNodeOperationalStateT oper_state,
    SmNodeAvailStatusT avail_status, int seqno )
{
    const char* action_str = sm_api_node_action_string(action);
    const char* admin_state_str = sm_api_node_admin_state_string(admin_state);
    const char* oper_state_str = sm_api_node_oper_state_string(oper_state);
    const char* avail_status_str = sm_api_node_avail_status_string(avail_status);
    int bytes_written;

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    bytes_written = snprintf( _tx_buffer, sizeof(_tx_buffer),
                              "%s,%s,%i,%s,%s,%s,%s,%s,%s,%s", 
                              SM_API_MSG_VERSION, SM_API_MSG_REVISION,
                              seqno, SM_API_MSG_TYPE_SET_NODE_ACK, "sm",
                              node_name, action_str, admin_state_str,
                              oper_state_str, avail_status_str );
    if( 0 < bytes_written )
    {
        if( 0 > sendto( _sm_api_socket, &_tx_buffer, bytes_written, 0,
                        (struct sockaddr*) &_sm_api_client_address,
                        _sm_api_client_address_len ) )
        {
            DPRINTFE( "Failed to send node (%s) set ack, error=%s",
                      node_name, strerror( errno ) );
            return( SM_FAILED );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// API - Dispatch
// ==============
static void sm_api_dispatch( int selobj, int64_t user_data )
{
    int seqno;
    char* node_name;
    char* service_name;
    gchar** params;
    int bytes_read;
    SmNodeSetActionT action;
    SmNodeAdminStateT admin_state;
    SmNodeOperationalStateT oper_state;
    SmNodeAvailStatusT avail_status;
    int action_flag = 0;

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
    if( params[SM_API_MSG_VERSION_FIELD] == NULL )
    {
        DPRINTFE( "Missing version field in received message." );
        goto ERROR;
    }

    if( 0 != strcmp( SM_API_MSG_VERSION, params[SM_API_MSG_VERSION_FIELD] ) )
    {
        DPRINTFE( "Unsupported version (%s) received, expected=%s.",
                  params[SM_API_MSG_VERSION_FIELD], SM_API_MSG_VERSION );
        goto ERROR;
    }

    if( params[SM_API_MSG_REVISION_FIELD] == NULL )
    {
        DPRINTFE( "Missing revision field in received message." );
        goto ERROR;
    }

    if( params[SM_API_MSG_SEQNO_FIELD] == NULL )
    {
        DPRINTFE( "Missing seqno field in received message." );
        goto ERROR;
    }

    seqno = atoi( params[SM_API_MSG_SEQNO_FIELD] );

    if( params[SM_API_MSG_TYPE_FIELD] == NULL )
    {
        DPRINTFE( "Missing message-type field in received message." );
        goto ERROR;
    }
    
    if( 0 == strcmp( SM_API_MSG_TYPE_SET_NODE, params[SM_API_MSG_TYPE_FIELD] ) )
    {
        if( params[SM_API_MSG_ORIGIN_FIELD] == NULL )
        {
            DPRINTFE( "Missing origin field in received message." );
            goto ERROR;
        }

        if( params[SM_API_MSG_NODE_NAME_FIELD] == NULL )
        {
            DPRINTFE( "Missing node-name field in received message." );
            goto ERROR;
        }
        
        node_name  = (char*) params[SM_API_MSG_NODE_NAME_FIELD];
        
        if( params[SM_API_MSG_NODE_ACTION_FIELD] == NULL )
        {
            DPRINTFE( "Missing node-action field in received message." );
            goto ERROR;
        }

        action = sm_api_node_action_value(
                            (char*) params[SM_API_MSG_NODE_ACTION_FIELD]);

        if( params[SM_API_MSG_NODE_ADMIN_FIELD] == NULL )
        {
            DPRINTFE( "Missing node-admin-state field in received message." );
            goto ERROR;
        }

        admin_state = sm_api_node_admin_state_value(
                            (char*) params[SM_API_MSG_NODE_ADMIN_FIELD]);
        
        if( params[SM_API_MSG_NODE_OPER_FIELD] == NULL )
        {
            DPRINTFE( "Missing node-oper-state field in received message." );
            goto ERROR;
        }

        oper_state = sm_api_node_oper_state_value(
                            (char*) params[SM_API_MSG_NODE_OPER_FIELD]);

        if( params[SM_API_MSG_NODE_AVAIL_FIELD] == NULL )
        {
            DPRINTFE( "Missing node-avail-status field in received message." );
            goto ERROR;
        }

        avail_status = sm_api_node_avail_status_value(
                            (char*) params[SM_API_MSG_NODE_AVAIL_FIELD]);
        
        if( NULL != _callbacks.node_set )
        {
            _callbacks.node_set( node_name, action, admin_state, oper_state,
                                 avail_status, seqno );
        }
    }
    else if( 0 == strcmp( SM_API_MSG_TYPE_RESTART_SERVICE,
                          params[SM_API_MSG_TYPE_FIELD] ) )
    {
        if( params[SM_API_MSG_ORIGIN_FIELD] == NULL )
        {
            DPRINTFE( "Missing origin field in received message." );
            goto ERROR;
        }

        if( params[SM_API_MSG_SERVICE_NAME_FIELD] == NULL )
        {
            DPRINTFE( "Missing service-name field in received message." );
            goto ERROR;
        }
        
        service_name  = (char*) params[SM_API_MSG_SERVICE_NAME_FIELD];
        if ( params[SM_API_MSG_PARAM] != NULL &&
            0 == strcmp( SM_API_MSG_SKIP_DEP_CHECK, params[SM_API_MSG_PARAM]) )
        {
            DPRINTFI( "Confirm restart-safe request is received." );
            action_flag = SM_SVC_RESTART_NO_DEP;
        }else
        {
            action_flag = 0;
        }
        
        if( NULL != _callbacks.service_restart )
        {
            _callbacks.service_restart( service_name, seqno, action_flag);
        }
    } else {
        DPRINTFE( "Unknown/unsupported message-type (%s) received.",
                  params[SM_API_MSG_TYPE_FIELD] );
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
// API - Initialize
// ================
SmErrorT sm_api_initialize( void )
{
    int sock;
    socklen_t len;
    struct sockaddr_un src_addr;
    struct sockaddr_un client_addr;
    int result;
    SmErrorT error;

    memset( &src_addr, 0, sizeof(src_addr) );

    src_addr.sun_family = AF_UNIX;
    snprintf( src_addr.sun_path, sizeof(src_addr.sun_path),
              SM_API_MSG_SERVER_ADDRESS );

    sock = socket( AF_LOCAL, SOCK_DGRAM, 0 );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to create socket for sm-api, error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    unlink( src_addr.sun_path );
    
    len = strlen(src_addr.sun_path) + sizeof(src_addr.sun_family);

    result = bind( sock, (struct sockaddr *) &src_addr, len );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind address (%s) to socket for sm-api, "
                  "error=%s.", src_addr.sun_path, strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    memset( &client_addr, 0, sizeof(client_addr) );

    client_addr.sun_family = AF_UNIX;
    snprintf( client_addr.sun_path, sizeof(client_addr.sun_path),
              SM_API_MSG_CLIENT_ADDRESS );
    len = strlen(client_addr.sun_path) + sizeof(client_addr.sun_family);

    _sm_api_socket = sock;
    _sm_api_client_address = client_addr;
    _sm_api_client_address_len = len;
    
    error = sm_selobj_register( _sm_api_socket, sm_api_dispatch, 0 );
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
// API - Finalize
// ==============
SmErrorT sm_api_finalize( void )
{
    SmErrorT error;

    memset( &_callbacks, 0, sizeof(_callbacks) );

    if( -1 < _sm_api_socket )
    {
        error = sm_selobj_deregister( _sm_api_socket );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object, error=%s.",
                      sm_error_str( error ) );
        }
    }

    return( SM_OKAY );
}
// ***************************************************************************
