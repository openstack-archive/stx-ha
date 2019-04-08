//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_interface_api.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_hw.h"
#include "sm_node_utils.h"
#include "sm_node_fsm.h"
#include "sm_service_domain_interface_table.h"
#include "sm_service_domain_interface_fsm.h"
#include "sm_log.h"

static void sm_service_domain_interface_api_send_event( void* user_data[],
    SmServiceDomainInterfaceT* interface );

// ****************************************************************************
// Service Domain Interface API - Get Hardware Interface
// =====================================================
static void sm_service_domain_interface_api_get_hw_interface( 
    SmServiceDomainInterfaceT* interface )
{
    char net_addr_str[SM_NETWORK_ADDRESS_MAX_CHAR];
    SmErrorT error;

    sm_network_address_str( &(interface->network_address), net_addr_str );
    
    if( 0 == strcmp( SM_SERVICE_DOMAIN_MGMT_INTERFACE,
                     interface->service_domain_interface ) )
    {
        error = sm_node_utils_get_mgmt_interface( interface->interface_name );
        if( SM_OKAY == error )
        {
            DPRINTFI( "Network address (%s) maps to %s interface from config, "
                      "type=%s.", net_addr_str, interface->interface_name,
                      interface->service_domain_interface );
            goto PERSIST;
        }
        else if( SM_NOT_FOUND != error )
        {
            DPRINTFE( "Failed to look up management interface, error=%s.",
                      sm_error_str(error) );
            return;
        }

    } else if( 0 == strcmp( SM_SERVICE_DOMAIN_OAM_INTERFACE,
                            interface->service_domain_interface ) )
    {
        error = sm_node_utils_get_oam_interface( interface->interface_name );
        if( SM_OKAY == error )
        {
            DPRINTFI( "Network address (%s) maps to %s interface from config, "
                      "type=%s.", net_addr_str, interface->interface_name,
                      interface->service_domain_interface );
            goto PERSIST;
        }
        else if( SM_NOT_FOUND != error )
        {
            DPRINTFE( "Failed to look up oam interface, error=%s.",
                      sm_error_str(error) );
            return;
        }

    } else if( 0 == strcmp( SM_SERVICE_DOMAIN_CLUSTER_HOST_INTERFACE,
                            interface->service_domain_interface ) )
    {
        error = sm_node_utils_get_cluster_host_interface( interface->interface_name );
        if( SM_OKAY == error )
        {
            DPRINTFI( "Network address (%s) maps to %s interface from config, "
                      "type=%s.", net_addr_str, interface->interface_name,
                      interface->service_domain_interface );
            goto PERSIST;
        }
        else if( SM_NOT_FOUND != error )
        {
            DPRINTFE( "Failed to look up cluster-host interface, error=%s.",
                      sm_error_str(error) );
            return;
        }
    }  

    error = sm_hw_get_if_by_network_address( &(interface->network_address),
                                             interface->interface_name );
    if( SM_OKAY == error )
    {
        DPRINTFI( "Network address (%s) maps to %s interface from kernel, "
                  "type=%s.", net_addr_str, interface->interface_name,
                  interface->service_domain_interface );
        goto PERSIST;

    } else {
        if( SM_NOT_FOUND == error )
        {
            DPRINTFE( "Failed to get interface by network address (%s), "
                      "error=%s.", net_addr_str, sm_error_str( error ) );
        } else {
            DPRINTFE( "Failed to get interface by network address (%s), "
                      "error=%s.", net_addr_str, sm_error_str( error ) );
        }
        return;
    }

PERSIST:
    error = sm_service_domain_interface_table_persist( interface );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update service domain (%s) interface (%s), "
                  "error=%s.", interface->service_domain,
                  interface->service_domain_interface,
                  sm_error_str( error ) );
        return;
    }

    if( 0 != strcmp( SM_SERVICE_DOMAIN_OAM_INTERFACE,
                     interface->service_domain_interface ) )
    {
        bool is_aio_sx = false;
        error = sm_node_utils_is_aio_simplex(&is_aio_sx);
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to check for AIO simplex system, error=%s.",
                      sm_error_str(error) );
        }

        if ( is_aio_sx )
        {
            char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR];
            SmServiceDomainInterfaceEventT event;
            void* user_data[] = { &event, reason_text };

            event = SM_SERVICE_DOMAIN_INTERFACE_EVENT_NOT_IN_USE;

            snprintf( reason_text, sizeof(reason_text), "%s interface is not in use",
                        interface->service_domain_interface );

            sm_service_domain_interface_api_send_event( user_data, interface );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface API - Send Event
// =========================================
static void sm_service_domain_interface_api_send_event( void* user_data[],
    SmServiceDomainInterfaceT* interface )
{
    const char* reason_text;
    SmServiceDomainInterfaceEventT* event;
    SmErrorT error;

    event = (SmServiceDomainInterfaceEventT*) user_data[0];
    reason_text = (char*) user_data[1];

    if( '\0' == interface->interface_name[0] )
    {
        if(( SM_NETWORK_TYPE_NIL == interface->network_address.type )||
           ( SM_NETWORK_TYPE_UNKNOWN == interface->network_address.type ))
        {
            DPRINTFD( "Network address not set for interface yet." );
            return;
        }

        sm_service_domain_interface_api_get_hw_interface( interface );

    } else {
        error = sm_service_domain_interface_fsm_event_handler(
                                        interface->service_domain,
                                        interface->service_domain_interface,
                                        *event, NULL, reason_text );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Event (%s) not handled for service domain (%s) "
                      "interface (%s), error=%s.",
                      sm_service_domain_interface_event_str( *event ),
                      interface->service_domain,
                      interface->service_domain_interface,
                      sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface API - Node Enabled
// ===========================================
SmErrorT sm_service_domain_interface_api_node_enabled( void )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceDomainInterfaceEventT event;
    void* user_data[] = { &event, reason_text };

    event = SM_SERVICE_DOMAIN_INTERFACE_EVENT_NODE_ENABLED;

    snprintf( reason_text, sizeof(reason_text), "node enabled" );

    sm_service_domain_interface_table_foreach( user_data,
                                sm_service_domain_interface_api_send_event );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface API - Node Disabled
// ============================================
SmErrorT sm_service_domain_interface_api_node_disabled( void )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceDomainInterfaceEventT event;
    void* user_data[] = { &event, reason_text };

    event = SM_SERVICE_DOMAIN_INTERFACE_EVENT_NODE_DISABLED;

    snprintf( reason_text, sizeof(reason_text), "node disabled" );

    sm_service_domain_interface_table_foreach( user_data,
                                sm_service_domain_interface_api_send_event );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface API - Audit
// ====================================
SmErrorT sm_service_domain_interface_api_audit( void )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceDomainInterfaceEventT event;
    void* user_data[] = { &event, reason_text };

    event = SM_SERVICE_DOMAIN_INTERFACE_EVENT_AUDIT;

    snprintf( reason_text, sizeof(reason_text), "audit requested" );

    sm_service_domain_interface_table_foreach( user_data,
                                sm_service_domain_interface_api_send_event );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface API - Node State Callback
// ==================================================
static void sm_service_domain_interface_api_node_state_callback(
    char node_name[], SmNodeReadyStateT ready_state )
{
    SmErrorT error;

    char hostname[SM_NODE_NAME_MAX_CHAR];
    error = sm_node_utils_get_hostname( hostname );
    if (SM_OKAY != error)
    {
        DPRINTFE("Failed to find host name, %s", sm_error_str(error));
        return;
    }

    if( 0 != strcmp(node_name, hostname) )
    {
        DPRINTFD("Node ready state changed for %s, not local host, nothing to do", node_name);
        return;
    }
    if( SM_NODE_READY_STATE_ENABLED == ready_state )
    {
        error = sm_service_domain_interface_api_node_enabled( );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Notifying interfaces that node (%s) is "
                      "enabled, error=%s.", node_name,
                      sm_error_str( error ) );
            return;
        }

    } else if( SM_NODE_READY_STATE_DISABLED == ready_state ) {

        error = sm_service_domain_interface_api_node_disabled( );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Notifying interfaces that node (%s) is "
                      "disabled failed, error=%s.", node_name,
                      sm_error_str( error ) );
            return;
        }

    } else {
        DPRINTFD( "Ignoring ready state (%s) from node (%s).",
                  sm_node_ready_state_str( ready_state ), node_name );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface API - set i/f not in use in aio-sx
// =========================================
static void _sm_service_domain_interface_set_niu(void* unused_param[],
    SmServiceDomainInterfaceT* interface )
{
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR];
    SmServiceDomainInterfaceEventT event;
    void* user_data[] = { &event, reason_text };

    if( 0 == strcmp( SM_SERVICE_DOMAIN_OAM_INTERFACE,
                            interface->service_domain_interface ) )
    {
        // don't change oam i/f
        return;
    }

    // aio sx i/f other than OAM are not in use
    event = SM_SERVICE_DOMAIN_INTERFACE_EVENT_NOT_IN_USE;

    snprintf( reason_text, sizeof(reason_text), "interface %s not in use",
        interface->service_domain_interface);

    sm_service_domain_interface_api_send_event( user_data, interface );
}

// ****************************************************************************

// ****************************************************************************
// Service Domain Interface API - Initialize 
// =========================================
SmErrorT sm_service_domain_interface_api_initialize( void )
{
    SmErrorT error;

    error = sm_service_domain_interface_table_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain interface table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_interface_fsm_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain interface fsm, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_node_fsm_register_callback(
                sm_service_domain_interface_api_node_state_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for node state changes, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    bool is_aio_sx = false;
    error = sm_node_utils_is_aio_simplex(&is_aio_sx);
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to check for AIO simplex system, error=%s.",
                  sm_error_str(error) );
     return( error );
    }

    if ( is_aio_sx )
    {
        sm_service_domain_interface_table_foreach( NULL,
                                    _sm_service_domain_interface_set_niu );
    }
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface API - Finalize 
// =======================================
SmErrorT sm_service_domain_interface_api_finalize( void )
{
    SmErrorT error;

    error = sm_node_fsm_deregister_callback(
                sm_service_domain_interface_api_node_state_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister for node state changes, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_domain_interface_fsm_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain interface fsm, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_interface_table_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain interface table, "
                  "error=%s.", sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
