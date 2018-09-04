//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_node_api.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/inotify.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_selobj.h"
#include "sm_time.h"
#include "sm_timer.h"
#include "sm_uuid.h"
#include "sm_msg.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_nodes.h"
#include "sm_db_node_history.h"
#include "sm_node_utils.h"
#include "sm_node_fsm.h"
#include "sm_service_domain_scheduler.h"
#include "sm_service_domain_interface_table.h"
#include "sm_log.h"
#include "sm_alarm.h"
#include "sm_log.h"
#include "sm_troubleshoot.h"
#include "sm_node_swact_monitor.h"

#define SM_REBOOT_DELAY_IN_MS           30000
#define SM_REBOOT_TIMEOUT_IN_MINS       8
#define SM_REBOOT_TIMEOUT_IN_MS         (SM_REBOOT_TIMEOUT_IN_MINS*60*1000)

static SmDbHandleT* _sm_db_handle = NULL;
static SmMsgCallbacksT _msg_callbacks = {0};

static SmTimerIdT _reboot_delay_timer_id = SM_TIMER_ID_INVALID;
static SmTimerIdT _reboot_timer_id = SM_TIMER_ID_INVALID;

// ****************************************************************************
// Node API - Get Host Name
// ========================
SmErrorT sm_node_api_get_hostname( char node_name[] )
{
    return( sm_node_utils_get_hostname( node_name ) );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Get Peer Name
// ========================
SmErrorT sm_node_api_get_peername(char peer_name[SM_NODE_NAME_MAX_CHAR])
{
    char node_name[SM_NODE_NAME_MAX_CHAR];
    SmErrorT error = sm_node_api_get_hostname(node_name);
    if(SM_OKAY != error)
    {
        return error;
    }

    char format[] = "name <> '%s'";
    char query[SM_NODE_NAME_MAX_CHAR + sizeof(format)];
    unsigned int cnt = snprintf(query, sizeof(query), format, node_name);
    if(cnt < sizeof(query) - 1)
    {
        SmDbNodeT node;
        error = sm_db_nodes_query(_sm_db_handle, query, &node);
        if(SM_OKAY != error)
        {
            return error;
        }
        strncpy(peer_name, node.name, SM_NODE_NAME_MAX_CHAR);
        return SM_OKAY;
    }
    return SM_FAILED;
}
// ****************************************************************************

// ****************************************************************************
// Node API - Configuration Complete
// =================================
SmErrorT sm_node_api_config_complete( char node_name[], bool* complete )
{
    return( sm_node_utils_config_complete( complete ) );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Interface Send Node Hello
// ====================================
static void sm_node_api_interface_send_node_hello( void* user_data[],
    SmServiceDomainInterfaceT* interface )
{
    SmDbNodeT* node = (SmDbNodeT*) user_data[0];
    long uptime = *(long*) user_data[1];
    SmErrorT error;

    if(( SM_INTERFACE_STATE_ENABLED == interface->interface_state )&&
       ( SM_PATH_TYPE_STATUS_ONLY != interface->path_type ))
    {
        error = sm_msg_send_node_hello( node->name, node->admin_state, 
                                        node->oper_state, node->avail_status,
                                        node->ready_state, node->state_uuid,
                                        uptime, interface );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send node (%s) hello, error=%s.",
                      node->name, sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Node API - Send Node Hello
// ==========================
static SmErrorT sm_node_api_send_node_hello( SmDbNodeT* node )
{
    long uptime;
    void* user_data[] = {node, &uptime};
    SmErrorT error;

    error = sm_node_utils_get_uptime( &uptime );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to to get local node uptime, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    sm_msg_increment_seq_num();

    sm_service_domain_interface_table_foreach( user_data,
                                    sm_node_api_interface_send_node_hello );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Interface Send Node Update
// =====================================
static void sm_node_api_interface_send_node_update( void* user_data[],
    SmServiceDomainInterfaceT* interface )
{
    SmUuidPtrT old_state_uuid = (SmUuidPtrT) user_data[0];
    SmDbNodeT* node = (SmDbNodeT*) user_data[1];
    long uptime = *(long*) user_data[2];
    bool force = *(bool*) user_data[3];
    SmErrorT error;

    if(( SM_INTERFACE_STATE_ENABLED == interface->interface_state )&&
       ( SM_PATH_TYPE_STATUS_ONLY != interface->path_type ))
    {
        error = sm_msg_send_node_update( node->name, node->admin_state,
                                         node->oper_state, node->avail_status,
                                         node->ready_state, old_state_uuid,
                                         node->state_uuid, uptime, force,
                                         interface );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send node (%s) update, error=%s.",
                      node->name, sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Node API - Send Node Update
// ===========================
static SmErrorT sm_node_api_send_node_update( SmUuidT old_state_uuid, 
    SmDbNodeT* node, bool force )
{
    long uptime;
    void* user_data[] = {old_state_uuid, node, &uptime, &force};
    SmErrorT error;

    error = sm_node_utils_get_uptime( &uptime );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to to get local node uptime, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    sm_msg_increment_seq_num();

    sm_service_domain_interface_table_foreach( user_data,
                                    sm_node_api_interface_send_node_update );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Interface Send Node Swact
// ====================================
static void sm_node_api_interface_send_node_swact( void* user_data[],
    SmServiceDomainInterfaceT* interface )
{
    SmUuidPtrT request_uuid = (SmUuidPtrT) user_data[0];
    SmDbNodeT* node = (SmDbNodeT*) user_data[1];
    bool force = *(bool*) user_data[2];
    SmErrorT error;

    if(( SM_INTERFACE_STATE_ENABLED == interface->interface_state )&&
       ( SM_PATH_TYPE_STATUS_ONLY != interface->path_type ))
    {
        error = sm_msg_send_node_swact( node->name, force, request_uuid,
                                        interface );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send node (%s) swact, error=%s.",
                      node->name, sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Node API - Send Node Swact 
// ==========================
static SmErrorT sm_node_api_send_node_swact( SmDbNodeT* node, bool force,
    SmUuidT request_uuid )
{
    void* user_data[] = {request_uuid, node, &force};

    sm_msg_increment_seq_num();
    
    sm_service_domain_interface_table_foreach( user_data,
                                    sm_node_api_interface_send_node_swact );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Node Hello Callback
// ==============================
static void sm_node_api_node_hello_callback(
    SmNetworkAddressT* network_address, int network_port, int version, 
    int revision, char node_name[], SmNodeAdminStateT admin_state,
    SmNodeOperationalStateT oper_state, SmNodeAvailStatusT avail_status,
    SmNodeReadyStateT ready_state, SmUuidT state_uuid, long uptime )
{
    long local_uptime;
    char db_query[SM_DB_QUERY_STATEMENT_MAX_CHAR]; 
    SmUuidT new_state_uuid;
    SmDbNodeT node;
    SmDbNodeHistoryT node_history;
    SmErrorT error;

    sm_uuid_create( new_state_uuid );

    error = sm_node_utils_get_uptime( &local_uptime );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to to get local node uptime, error=%s.",
                  sm_error_str( error ) );
        return;
    }

    error = sm_db_nodes_read( _sm_db_handle, node_name, &node );
    if( SM_OKAY == error )
    {
        if( 0 == strcmp( state_uuid, node.state_uuid ) )
        {
            if(( admin_state == node.admin_state )&&
               ( oper_state  == node.oper_state  ) &&
               ( ready_state == node.ready_state ))
            {
                DPRINTFD( "Node (%s) info up to date.", node_name );
                return;
            }

            if( admin_state != node.admin_state )
            {
                sm_log_node_state_change( node_name,
                    sm_node_state_str( node.admin_state, node.ready_state ),
                    sm_node_state_str( admin_state, node.ready_state ),
                    "customer action" );
            }

            node.admin_state = admin_state;
            node.oper_state = oper_state;
            node.avail_status = avail_status;
            node.ready_state = ready_state;

            error = sm_db_nodes_update( _sm_db_handle, &node );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to update node (%s) info, error=%s.",
                          node_name, sm_error_str( error ) );
                return;
            }

            return;

        } else {
            DPRINTFI( "Node (%s) received=%s, have=%s.", node_name,
                      state_uuid, node.state_uuid );
        }

        snprintf( db_query, sizeof(db_query), "%s = '%s' and %s = '%s'",
                  SM_NODES_TABLE_COLUMN_NAME, node_name,
                  SM_NODES_TABLE_COLUMN_STATE_UUID, state_uuid );

        error = sm_db_node_history_query( _sm_db_handle, db_query,
                                          &node_history );
        if( SM_OKAY == error )
        {
            DPRINTFI( "Send update for node (%s) uuid=%s.", node_name,
                      state_uuid );

            error = sm_node_api_send_node_update( state_uuid, &node, false );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to send node (%s) update, error=%s.",
                          node_name, sm_error_str( error ) );
                return;
            }         
        } else if( SM_NOT_FOUND ) {
            if( uptime <= local_uptime )
            {
                DPRINTFI( "Send update for node (%s) uuid=%s, uptime=%ld, "
                          "local_uptime=%ld.", node_name, state_uuid, uptime,
                          local_uptime );

                error = sm_node_api_send_node_update( state_uuid, &node,
                                                      false );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to send node (%s) update, error=%s.",
                              node_name, sm_error_str( error ) );
                    return;
                }         
            } else {
                DPRINTFI( "Update node (%s) uuid=%s, was=%s.", node_name,
                          state_uuid, node.state_uuid );

                snprintf( node_history.name, sizeof(node_history.name),
                          "%s", node.name );
                node_history.admin_state = node.admin_state;
                node_history.oper_state = node.oper_state;
                node_history.avail_status = node.avail_status;
                node_history.ready_state = node.ready_state;
                snprintf( node_history.state_uuid, 
                          sizeof(node_history.state_uuid),
                          "%s", node.state_uuid );

                error = sm_db_node_history_insert( _sm_db_handle,
                                                   &node_history );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to save node (%s) info, error=%s.",
                              node_name, sm_error_str( error ) );
                    return;
                }

                if( admin_state != node.admin_state )
                {
                    sm_log_node_state_change( node_name,
                        sm_node_state_str( node.admin_state, node.ready_state ),
                        sm_node_state_str( admin_state, node.ready_state ),
                        "customer action" );
                }

                node.admin_state = admin_state;
                node.oper_state = oper_state;
                node.avail_status = avail_status;
                node.ready_state = ready_state;
                snprintf( node.state_uuid, sizeof(node.state_uuid), "%s",
                          state_uuid );

                error = sm_db_nodes_update( _sm_db_handle, &node );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to update node (%s) info, error=%s.",
                              node_name, sm_error_str( error ) );
                    return;
                }
            }
        } else {
            DPRINTFE( "Failed to read node history (%s), error=%s.",
                      node_name, sm_error_str( error ) );
            return;
        }

    } else if( SM_NOT_FOUND == error ) {
        DPRINTFI( "Inserting node (%s), uuid=%s.", node_name, 
                  new_state_uuid );

        snprintf( node.name, SM_NODE_NAME_MAX_CHAR, "%s", node_name );
        node.admin_state = admin_state;
        node.oper_state = oper_state;
        node.avail_status = avail_status;
        node.ready_state = ready_state;
        snprintf( node.state_uuid, sizeof(node.state_uuid), "%s",
                  new_state_uuid );

        error = sm_db_nodes_insert( _sm_db_handle, &node );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to insert node (%s) info, error=%s.",
                      node_name, sm_error_str( error ) );
            return;
        }
        
        sm_log_node_state_change( node_name,
            sm_node_state_str( SM_NODE_ADMIN_STATE_UNKNOWN,
                               SM_NODE_READY_STATE_UNKNOWN ),
            sm_node_state_str( node.admin_state, node.ready_state ),
            "customer action" );

    } else {
        DPRINTFE( "Failed to read node (%s), error=%s.", node_name,
                  sm_error_str( error ) );
        return;
    } 
}
// ****************************************************************************

// ****************************************************************************
// Node API - Node Update Callback
// ===============================
static void sm_node_api_node_update_callback(
    SmNetworkAddressT* network_address, int network_port, int version, 
    int revision, char node_name[], SmNodeAdminStateT admin_state,
    SmNodeOperationalStateT oper_state, SmNodeAvailStatusT avail_status,
    SmNodeReadyStateT ready_state, SmUuidT old_state_uuid, SmUuidT state_uuid,
    long uptime, bool force )
{
    bool update = false;
    char db_query[SM_DB_QUERY_STATEMENT_MAX_CHAR]; 
    SmDbNodeT node;
    SmDbNodeHistoryT node_history;
    SmErrorT error;

    if( force )
    {
        error = sm_db_nodes_query( _sm_db_handle, NULL, &node );
        if( SM_OKAY == error )
        {
            update = true;
            DPRINTFI("Updating node (%s) info.", node_name );

        } else if( SM_NOT_FOUND != error ) {
            DPRINTFE( "Failed to read node (%s), error=%s.", node_name,
                      sm_error_str( error ) );
            return;
        }
    } else {
        snprintf( db_query, sizeof(db_query), "%s = '%s' and %s = '%s'",
                  SM_NODES_TABLE_COLUMN_NAME, node_name,
                  SM_NODES_TABLE_COLUMN_STATE_UUID, old_state_uuid );

        error = sm_db_nodes_query( _sm_db_handle, db_query, &node );
        if( SM_OKAY == error )
        {
            update = true;
            DPRINTFI("Updating node (%s) info.", node_name );

        } else if( SM_NOT_FOUND != error ) {
            DPRINTFE( "Failed to read node (%s), error=%s.", node_name,
                      sm_error_str( error ) );
            return;
        }
    }

    if( update )
    {
        if( admin_state != node.admin_state )
        {
            sm_log_node_state_change( node_name,
                    sm_node_state_str( node.admin_state, node.ready_state ),
                    sm_node_state_str( admin_state, node.ready_state ),
                    "customer action" );
        }

        snprintf( node_history.name, sizeof(node_history.name),
                  "%s", node.name );
        node_history.admin_state = node.admin_state;
        node_history.oper_state = node.oper_state;
        node_history.avail_status = node.avail_status;
        node_history.ready_state = node.ready_state;
        snprintf( node_history.state_uuid, sizeof(node_history.state_uuid),
                  "%s", node.state_uuid );

        error = sm_db_node_history_insert( _sm_db_handle, &node_history );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to save node (%s) info, error=%s.",
                      node_name, sm_error_str( error ) );
            return;
        }

        node.admin_state = admin_state;
        node.oper_state = oper_state;
        node.avail_status = avail_status;
        node.ready_state = ready_state;
        snprintf( node.state_uuid, sizeof(node.state_uuid), "%s",
                  state_uuid );

        error = sm_db_nodes_update( _sm_db_handle, &node );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to update node (%s) info, error=%s.",
                      node_name, sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Node API - Add Node
// ===================
SmErrorT sm_node_api_add_node( char node_name[] )
{
    SmDbNodeT node;
    SmUuidT state_uuid;
    SmErrorT error;

    sm_uuid_create( state_uuid );
    
    error = sm_db_nodes_read( _sm_db_handle, node_name, &node );
    if( SM_OKAY == error )
    {
        DPRINTFD( "Already added node (%s).", node_name );

    } else if( SM_NOT_FOUND == error ) {

        snprintf( node.name, SM_NODE_NAME_MAX_CHAR, "%s", node_name );
        node.admin_state = SM_NODE_ADMIN_STATE_UNLOCKED;
        node.oper_state = SM_NODE_OPERATIONAL_STATE_ENABLED;
        node.avail_status = SM_NODE_AVAIL_STATUS_AVAILABLE;
        node.ready_state = SM_NODE_READY_STATE_UNKNOWN;
        snprintf( node.state_uuid, sizeof(node.state_uuid), "%s",
                  state_uuid );

        error = sm_db_nodes_insert( _sm_db_handle, &node );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to insert node (%s) info, error=%s.",
                      node_name, sm_error_str( error ) );
            return( error );
        }

        DPRINTFI( "Added node (%s).", node.name );
        
        sm_log_node_state_change( node_name,
            sm_node_state_str( SM_NODE_ADMIN_STATE_UNKNOWN,
                               SM_NODE_READY_STATE_UNKNOWN ),
            sm_node_state_str( node.admin_state, node.ready_state ),
            "customer action" );

    } else {
        DPRINTFE( "Failed to read node (%s) information, error=%s.",
                  node_name, sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Update Node
// ======================
SmErrorT sm_node_api_update_node( char node_name[],
    SmNodeAdminStateT admin_state, SmNodeOperationalStateT oper_state,
    SmNodeAvailStatusT avail_status )
{
    bool send_update = false;
    SmUuidT old_state_uuid;
    SmUuidT new_state_uuid;
    SmDbNodeT node;
    SmDbNodeHistoryT node_history;
    SmErrorT error;

    sm_uuid_create( new_state_uuid );

    error = sm_db_nodes_read( _sm_db_handle, node_name, &node );
    if( SM_OKAY == error )
    {
        if( admin_state != node.admin_state )
        {
            sm_log_node_state_change( node_name,
                    sm_node_state_str( node.admin_state, node.ready_state ),
                    sm_node_state_str( admin_state, node.ready_state ),
                    "customer action" );
        }

        if( oper_state != node.oper_state )
        {
            sm_log_node_state_change( node_name,
                    sm_node_oper_state_str( node.oper_state ),
                    sm_node_oper_state_str( oper_state ),
                    "oper state changed" );
        }

        if(( admin_state != node.admin_state )||
           ( oper_state != node.oper_state ))
        {
            memcpy( old_state_uuid, node.state_uuid, sizeof(old_state_uuid) );

            snprintf( node_history.name, sizeof(node_history.name),
                      "%s", node.name );

            node_history.admin_state = node.admin_state;
            node_history.oper_state = node.oper_state;
            node_history.avail_status = node.avail_status;
            node_history.ready_state = node.ready_state;
            snprintf( node_history.state_uuid, sizeof(node_history.state_uuid),
                      "%s", node.state_uuid );

            error = sm_db_node_history_insert( _sm_db_handle, &node_history );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to save node (%s) info, error=%s.",
                          node_name, sm_error_str( error ) );
                return( error );
            }

            node.admin_state = admin_state;
            node.oper_state = oper_state;
            node.avail_status = avail_status;
            snprintf( node.state_uuid, sizeof(node.state_uuid), "%s",
                      new_state_uuid );

            error = sm_db_nodes_update( _sm_db_handle, &node );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to update node (%s) info, error=%s.",
                          node_name, sm_error_str( error ) );
                return( error );
            }

            send_update = true;
        }
    } else if( SM_NOT_FOUND == error ) {

        memset( old_state_uuid, 0, sizeof(old_state_uuid) );

        snprintf( node.name, SM_NODE_NAME_MAX_CHAR, "%s", node_name );

        node.admin_state = admin_state;
        node.oper_state = oper_state;
        node.avail_status = avail_status;
        node.ready_state = SM_NODE_READY_STATE_DISABLED;
        snprintf( node.state_uuid, sizeof(node.state_uuid), "%s",
                  new_state_uuid );

        error = sm_db_nodes_insert( _sm_db_handle, &node );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to insert node (%s) info, error=%s.",
                      node_name, sm_error_str( error ) );
            return( error );
        }

        sm_log_node_state_change( node_name,
            sm_node_state_str( SM_NODE_ADMIN_STATE_UNKNOWN,
                               SM_NODE_READY_STATE_UNKNOWN ),
            sm_node_state_str( node.admin_state, node.ready_state ),
            "customer action" );

        send_update = true;

    } else {
        DPRINTFE( "Failed to read node (%s) information, error=%s.",
                  node_name, sm_error_str( error ) );
        return( error );
    }

    if( send_update )
    {
        error = sm_node_api_send_node_update( old_state_uuid, &node, true );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send node (%s) node update, error=%s.",
                      node_name, sm_error_str(error) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Fail Node
// ======================
SmErrorT sm_node_api_fail_node( char node_name[] )
{
    SmDbNodeT node;
    SmErrorT error;
    error = sm_db_nodes_read( _sm_db_handle, node_name, &node );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to read node (%s) information, error=%s.",
                  node_name, sm_error_str( error ) );
        return( error );
    }

    if( node.oper_state == SM_NODE_OPERATIONAL_STATE_DISABLED &&
        node.avail_status == SM_NODE_AVAIL_STATUS_FAILED )
    {
        DPRINTFD("Already in failure mode %s", node_name);
    }

    DPRINTFE("Node %s is entering to failure mode.", node_name);

    error = sm_node_api_update_node(
        node_name,
        node.admin_state,
        SM_NODE_OPERATIONAL_STATE_DISABLED,
        SM_NODE_AVAIL_STATUS_FAILED);

    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to set node (%s) failed, error=%s.",
                  node_name, sm_error_str( error ) );
    }
    return( error );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Delete Node
// ======================
SmErrorT sm_node_api_delete_node( char node_name[] )
{
    SmErrorT error;

    error = sm_db_nodes_delete( _sm_db_handle, node_name );

    if(( SM_OKAY != error )&&( SM_NOT_FOUND != error ))
    {
        DPRINTFE( "Failed to delete node, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Swact
// ================
SmErrorT sm_node_api_swact( char node_name[], bool force )
{
    SmUuidT request_uuid;
    SmDbNodeT node;
    SmErrorT error;

    sm_uuid_create( request_uuid );
    
    error = sm_db_nodes_read( _sm_db_handle, node_name, &node );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to read node (%s) information, error=%s.",
                  node_name, sm_error_str( error ) );
        return( error );
    }

    error = sm_node_api_send_node_swact( &node, force, request_uuid );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send node (%s) swact, error=%s.",
                  node_name, sm_error_str( error ) );
        return( error );
    }
    
    error = sm_service_domain_scheduler_swact_node( node_name, force,
                                                    request_uuid );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to set node scheduling state, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Reboot Timeout
// =========================
static bool sm_node_api_reboot_timeout( SmTimerIdT timer_id, int64_t user_data )
{
    int sysrq_handler_fd;
    int sysrq_tigger_fd;
    char hostname[SM_NODE_NAME_MAX_CHAR] = "";
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
    }

    // Enable sysrq handling.
    sysrq_handler_fd = open( "/proc/sys/kernel/sysrq", O_RDWR | O_CLOEXEC );
    if( 0 > sysrq_handler_fd )
    {
        DPRINTFE( "Failed to open sysrq handler file, error=%s.",
                  strerror(errno) );
        return( true );
    }

    write( sysrq_handler_fd, "1", 1 );
    close( sysrq_handler_fd );

    // Trigger sysrq command.
    sysrq_tigger_fd = open( "/proc/sysrq-trigger", O_RDWR | O_CLOEXEC );
    if( 0 > sysrq_tigger_fd )
    {
        DPRINTFE( "Failed to open sysrq trigger file, error=%s.",
                  strerror(errno) );
        return( true );
    }

    snprintf( reason_text, sizeof(reason_text), "timed out after %i minute%s "
              "waiting for a controlled reboot, escalating to a forced reboot",
              SM_REBOOT_TIMEOUT_IN_MINS,
              (1 == SM_REBOOT_TIMEOUT_IN_MINS) ? "" : "s" );

    sm_log_node_reboot( hostname, reason_text, true );

    DPRINTFI( "******************************************************"
              "************************************" );
    DPRINTFI( "** Issuing an immediate reboot of the system, without "
              "unmounting or syncing filesystems **" );
    DPRINTFI( "******************************************************"
              "************************************" );

    sleep(5); // wait 5 seconds before a forced reboot.
    write( sysrq_tigger_fd, "b", 1 ); 
    close( sysrq_tigger_fd );

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Reboot Delay
// =======================
static bool sm_node_api_reboot_delay( SmTimerIdT timer_id, int64_t user_data )
{
    pid_t pid;
    char hostname[SM_NODE_NAME_MAX_CHAR] = "";
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
    }

    snprintf( reason_text, sizeof(reason_text), "reboot delay expired, "
              "issuing reboot" );

    sm_log_node_reboot( hostname, reason_text, false );

    DPRINTFI( "***********************************************" );
    DPRINTFI( "** Issuing a controlled reboot of the system **" );
    DPRINTFI( "***********************************************" );

    pid = fork();
    if( 0 > pid )
    {
        DPRINTFE( "Failed to fork process for reboot, error=%s.",
                  strerror( errno ) );
        return( true );

    } else if( 0 == pid ) {
        // Child process.
        struct rlimit file_limits;
        char reboot_cmd[] = "reboot";
        char* reboot_argv[] = {reboot_cmd, NULL};
        char* reboot_env[] = {NULL};

        setpgid( 0, 0 );

        if( 0 == getrlimit( RLIMIT_NOFILE, &file_limits ) )
        {
            unsigned int fd_i;
            for( fd_i=0; fd_i < file_limits.rlim_cur; ++fd_i )
            {
                close( fd_i );
            }

            open( "/dev/null", O_RDONLY ); // stdin
            open( "/dev/null", O_WRONLY ); // stdout
            open( "/dev/null", O_WRONLY ); // stderr
        }

        execve( "/sbin/reboot", reboot_argv, reboot_env );
        
        // Shouldn't get this far, else there was an error.
        exit(-1);

    } else {
        // Parent process.
        SmErrorT error;

        DPRINTFI( "Child process (%i) created for reboot.", (int) pid );

        error = sm_timer_register( "reboot force", SM_REBOOT_TIMEOUT_IN_MS,
                                   sm_node_api_reboot_timeout, 0,
                                   &_reboot_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to create reboot timer, error=%s.",
                      sm_error_str( error ) );
            return( true );
        }
    }
    return( false );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Reboot
// =================
SmErrorT sm_node_api_reboot( char reason_text[] )
{
    if( SM_TIMER_ID_INVALID == _reboot_delay_timer_id )
    {
        char hostname[SM_NODE_NAME_MAX_CHAR] = "";
        SmErrorT error;

        error = sm_node_api_get_hostname( hostname );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to get hostname, error=%s.",
                      sm_error_str( error ) );
        }

        DPRINTFI( "Reboot of %s requested, reason=%s.", hostname,
                  reason_text );
                  
        sm_log_node_reboot( hostname, reason_text, false );

        sm_troubleshoot_dump_data( reason_text );

        // Give some time to allow the dump data to finish before
        // actually attempting a reboot.
        error = sm_timer_register( "reboot delay", SM_REBOOT_DELAY_IN_MS,
                                   sm_node_api_reboot_delay, 0,
                                   &_reboot_delay_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to create reboot delay timer, error=%s.",
                      sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Send Event
// =====================
static SmErrorT sm_node_api_send_event( void* user_data[], void* record )
{
    SmNodeEventT* event = (SmNodeEventT*) user_data[0];
    const char* reason_text = (char*) user_data[1];
    SmDbNodeT* node = (SmDbNodeT*) record;
    SmErrorT error;

    error = sm_node_api_send_node_hello( node );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send hello for node (%s), error=%s.",
                  node->name, sm_error_str( error ) );
    }

    error = sm_node_fsm_event_handler( node->name, *event, NULL, reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Event (%s) not handled for node (%s), error=%s.",
                  sm_node_event_str( *event ), node->name,
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Audit
// ================
SmErrorT sm_node_api_audit( void )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];
    char db_query[SM_DB_QUERY_STATEMENT_MAX_CHAR]; 
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "audit requested";
    SmNodeEventT event = SM_NODE_EVENT_AUDIT;
    SmDbNodeT node;
    SmErrorT error;
    void* user_data[] = { &event, reason_text };

    error = sm_node_utils_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.", sm_error_str( error ) );
        return( error );
    }

    snprintf( db_query, sizeof(db_query), "%s = '%s'",
              SM_NODES_TABLE_COLUMN_NAME, hostname );

    error = sm_db_foreach( SM_DATABASE_NAME, SM_NODES_TABLE_NAME, db_query,
                           &node, sm_db_nodes_convert, sm_node_api_send_event,
                           user_data );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over nodes, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************
void sm_node_api_node_swact_callback(
        SmNetworkAddressT* network_address, int network_port, int version,
        int revision, char node_name[], bool force, SmUuidT request_uuid )
{
    SmNodeSwactMonitor::SwactStart(SM_NODE_STATE_ACTIVE);
}

// ****************************************************************************
// Node API - Initialize 
// =====================
SmErrorT sm_node_api_initialize( void )
{
    SmErrorT error;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_node_fsm_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize node fsm, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    memset( &_msg_callbacks, 0, sizeof(_msg_callbacks) );

    _msg_callbacks.node_hello = sm_node_api_node_hello_callback;
    _msg_callbacks.node_update = sm_node_api_node_update_callback;
    _msg_callbacks.node_swact = sm_node_api_node_swact_callback;

    error = sm_msg_register_callbacks( &_msg_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register messaging callbacks, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node API - Finalize 
// ===================
SmErrorT sm_node_api_finalize( void )
{
    SmErrorT error;

    error = sm_msg_deregister_callbacks( &_msg_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister messaging callbacks, error=%s.",
                  sm_error_str( error ) );
    }

    memset( &_msg_callbacks, 0, sizeof(_msg_callbacks) );

    error = sm_node_fsm_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize node fsm, error=%s.",
                  sm_error_str( error ) );
    }

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

    return( SM_OKAY );
}
// ****************************************************************************
