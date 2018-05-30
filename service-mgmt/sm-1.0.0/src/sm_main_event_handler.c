//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_main_event_handler.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_timer.h"
#include "sm_utils.h"
#include "sm_api.h"
#include "sm_notify_api.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_service_groups.h"
#include "sm_service_group_table.h"
#include "sm_node_api.h"
#include "sm_service_domain_interface_api.h"
#include "sm_service_group_api.h"
#include "sm_service_api.h"
#include "sm_failover.h"
#include "sm_node_swact_monitor.h"

#define SM_NODE_AUDIT_TIMER_IN_MS           1000
#define SM_INTERFACE_AUDIT_TIMER_IN_MS      1000

static SmDbHandleT* _sm_db_handle = NULL;
static SmApiCallbacksT _api_callbacks = {0}; 
static SmNotifyApiCallbacksT _notify_api_callbacks = {0};
static SmTimerIdT _node_audit_timer_id = SM_TIMER_ID_INVALID;
static SmTimerIdT _interface_audit_timer_id = SM_TIMER_ID_INVALID;

// ****************************************************************************
// Main Event Handler - Audit Node
// ===============================
static bool sm_main_event_handler_audit_node( SmTimerIdT timer_id,
    int64_t user_data )
{
    bool complete;
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.", sm_error_str( error ) );
        return( true );
    }

    error = sm_node_api_config_complete( hostname, &complete );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to determine if configuration is complete, "
                  "error=%s.", sm_error_str( error ) );
        return( true );
    }

    if( complete )
    {
        error = sm_node_api_add_node( hostname );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to add node to database, error=%s.",
                      sm_error_str( error ) );
            return( true );
        }

        error = sm_node_api_audit();
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to audit nodes, error=%s.",
                      sm_error_str( error ) );
            return( true );
        }
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - Audit Interfaces
// =====================================
static bool sm_main_event_handler_audit_interfaces( SmTimerIdT timer_id,
    int64_t user_data )
{
    SmErrorT error;

    error = sm_service_domain_interface_api_audit();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to audit interfaces, error=%s.",
                  sm_error_str( error ) );
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - Release Service Group
// ==========================================
static SmErrorT sm_main_event_handler_release_service_group(
    void* user_data[], void* record )
{
    SmDbServiceGroupT* service_group = (SmDbServiceGroupT*) record;
    SmErrorT error;

    error = sm_service_group_api_go_active( service_group->name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to perform go-active action on member (%s), "
                  "error=%s.", service_group->name, sm_error_str(error) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - Release Service Groups
// ===========================================
static SmErrorT sm_main_event_handler_release_service_groups( void )
{
    char db_query[SM_DB_QUERY_STATEMENT_MAX_CHAR]; 
    SmDbServiceGroupT service_group;
    SmErrorT error;

    snprintf( db_query, sizeof(db_query), "%s = 'yes'",
              SM_SERVICE_GROUPS_TABLE_COLUMN_AUTO_RECOVER );

    error = sm_db_foreach( SM_DATABASE_NAME, SM_SERVICE_GROUPS_TABLE_NAME,
                           db_query, &service_group,
                           &sm_db_service_groups_convert,
                           sm_main_event_handler_release_service_group,
                           NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over service groups, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - API Node Set Callback
// ==========================================
static void sm_main_event_handler_api_node_set_callback( char node_name[],
    SmNodeSetActionT action, SmNodeAdminStateT admin_state,
    SmNodeOperationalStateT oper_state, SmNodeAvailStatusT avail_status,
    int seqno )
{
    SmErrorT error;

    DPRINTFI( "Set node (%s) requested, action=%i, admin_state=%s, "
              "oper_state=%s, avail_status=%s, seqno=%i.", node_name, action,
              sm_node_admin_state_str(admin_state),
              sm_node_oper_state_str(oper_state),
              sm_node_avail_status_str(avail_status), seqno );

    error = sm_node_api_update_node( node_name, admin_state, oper_state,
                                     avail_status );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update node (%s), seqno=%i, error=%s.",
                  node_name, seqno, sm_error_str(error) );
        return;
    }

    if( SM_NODE_SET_ACTION_SWACT == action )
    {
        SmNodeSwactMonitor::SwactStart(SM_NODE_STATE_STANDBY);
        error = sm_node_api_swact( node_name, false );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to swact node (%s), seqno=%i, error=%s.",
                      node_name, seqno, sm_error_str(error) );
            return;
        }
    } else if( SM_NODE_SET_ACTION_SWACT_FORCE == action ) {
        SmNodeSwactMonitor::SwactStart(SM_NODE_STATE_STANDBY);
        error = sm_node_api_swact( node_name, true );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to swact force node (%s), seqno=%i, error=%s.",
                      node_name, seqno, sm_error_str(error) );
            return;
        }
    }

    error = sm_api_send_node_set_ack( node_name, action, admin_state, 
                                      oper_state, avail_status, seqno );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send node (%s) set ack, seqno=%i, error=%s.",
                  node_name, seqno, sm_error_str(error) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - API Service Restart Callback
// =================================================
static void sm_main_event_handler_api_service_restart_callback(
    char service_name[], int seqno, int flag )
{
    DPRINTFI( "Service (%s) restart requested, seqno=%i, flag=%i.",
              service_name, seqno, flag );

    sm_service_api_restart( service_name, flag );
}
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - Notify API Service Event Callback
// ======================================================
static void sm_main_event_handler_notify_api_service_event_callback(
    char service_name[], SmNotifyServiceEventT event )
{
    DPRINTFI( "Received notification event %i for service %s.", event,
              service_name );

    sm_service_api_audit( service_name );
}
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - Reload Data
// ================================
void sm_main_event_handler_reload_data( void )
{
}
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - Service Group Degraded Count
// =================================================
static void sm_main_event_handler_service_group_degraded_count(
    void* user_data[], SmServiceGroupT* service_group )
{
    int* degrade_count = (int*) user_data[0];

    if( SM_SERVICE_GROUP_STATUS_DEGRADED == service_group->status )
    {
        *degrade_count = *degrade_count + 1;
    }
}
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - Service Group State Callback
// =================================================
static void sm_main_event_handler_service_group_state_callback(
    char service_group_name[], SmServiceGroupStateT state,
    SmServiceGroupStatusT status, SmServiceGroupConditionT condition,
    int64_t health, const char reason_text[] )
{
    int degrade_count = 0;
    void* user_data[] = { &degrade_count };

    sm_service_group_table_foreach( user_data,
                        sm_main_event_handler_service_group_degraded_count );

    if( 0 < degrade_count )
    {
        sm_failover_degrade(SM_FAILOVER_DEGRADE_SOURCE_SERVICE_GROUP);
    } else {
        sm_failover_degrade_clear(SM_FAILOVER_DEGRADE_SOURCE_SERVICE_GROUP);
    }
}
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - Initialize
// ===============================
SmErrorT sm_main_event_handler_initialize( void )
{
    SmErrorT error;
   
    memset( &_api_callbacks, 0, sizeof(_api_callbacks) );
    memset( &_notify_api_callbacks, 0, sizeof(_notify_api_callbacks) );

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_timer_register( "node audit",
                               SM_NODE_AUDIT_TIMER_IN_MS,
                               sm_main_event_handler_audit_node,
                               0, &_node_audit_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create node audit timer, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_timer_register( "interface audit",
                               SM_INTERFACE_AUDIT_TIMER_IN_MS,
                               sm_main_event_handler_audit_interfaces,
                               0, &_interface_audit_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create interface audit timer, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_api_abort_actions();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to cleanup service actions, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_node_api_audit();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to audit nodes, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_main_event_handler_release_service_groups();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to release service groups, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_api_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize api, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    _api_callbacks.node_set 
        = sm_main_event_handler_api_node_set_callback;
    _api_callbacks.service_restart 
        = sm_main_event_handler_api_service_restart_callback;

    error = sm_api_register_callbacks( &_api_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register api callbacks, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_notify_api_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize notify api, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    _notify_api_callbacks.service_event 
        = sm_main_event_handler_notify_api_service_event_callback;

    error = sm_notify_api_register_callbacks( &_notify_api_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register notify api callbacks, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_api_register_callback(
                    sm_main_event_handler_service_group_state_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for service group state callbacks, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }
    
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - Finalize
// =============================
SmErrorT sm_main_event_handler_finalize( void )
{
    SmErrorT error;

    error = sm_service_group_api_deregister_callback(
                    sm_main_event_handler_service_group_state_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister for service group state callbacks, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_notify_api_deregister_callbacks( &_notify_api_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister notify api callbacks, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_notify_api_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize notify api, error=%s.",
                  sm_error_str( error ) );
    }
    
    error = sm_api_deregister_callbacks( &_api_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister api callbacks, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_api_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize api, error=%s.", sm_error_str( error ) );
    }

    if( SM_TIMER_ID_INVALID != _node_audit_timer_id )
    {
        error = sm_timer_deregister( _node_audit_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel node audit timer, error=%s.",
                      sm_error_str( error ) );
        }

        _node_audit_timer_id = SM_TIMER_ID_INVALID;
    }

    if( SM_TIMER_ID_INVALID != _interface_audit_timer_id )
    {
        error = sm_timer_deregister( _interface_audit_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel interface audit timer, error=%s.",
                      sm_error_str( error ) );
        }

        _interface_audit_timer_id = SM_TIMER_ID_INVALID;
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
