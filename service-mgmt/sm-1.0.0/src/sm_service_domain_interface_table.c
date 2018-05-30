//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_interface_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_service_domain_interfaces.h"

static SmListT* _service_domain_interfaces = NULL;
static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Domain Interface Table - Read
// =====================================
SmServiceDomainInterfaceT* sm_service_domain_interface_table_read( 
    char service_domain_name[], char service_domain_interface_name[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainInterfaceT* interface;

    SM_LIST_FOREACH( _service_domain_interfaces, entry, entry_data )
    {
        interface = (SmServiceDomainInterfaceT*) entry_data;

        if(( 0 == strcmp( service_domain_name, interface->service_domain ) )&&
           ( 0 == strcmp( service_domain_interface_name,
                          interface->service_domain_interface ) ))
        {
            return( interface );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - Read By Identifier
// ===================================================
SmServiceDomainInterfaceT* sm_service_domain_interface_table_read_by_id( 
    int64_t id )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainInterfaceT* interface;

    SM_LIST_FOREACH( _service_domain_interfaces, entry, entry_data )
    {
        interface = (SmServiceDomainInterfaceT*) entry_data;

        if( id == interface->id )
        {
            return( interface );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - For Each
// =========================================
void sm_service_domain_interface_table_foreach( void* user_data[],
    SmServiceDomainInterfaceTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;

    SM_LIST_FOREACH( _service_domain_interfaces, entry, entry_data )
    {
        callback( user_data, (SmServiceDomainInterfaceT*) entry_data );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - For Each Service Domain
// ========================================================
void sm_service_domain_interface_table_foreach_service_domain( 
    char service_domain_name[], void* user_data[],
    SmServiceDomainInterfaceTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainInterfaceT* interface;

    SM_LIST_FOREACH( _service_domain_interfaces, entry, entry_data )
    {
        interface = (SmServiceDomainInterfaceT*) entry_data;

        if( 0 == strcmp( service_domain_name, interface->service_domain ) )
        {
            callback( user_data, interface );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - Add
// ====================================
static SmErrorT sm_service_domain_interface_table_add( void* user_data[],
    void* record )
{
    SmServiceDomainInterfaceT* interface;
    SmDbServiceDomainInterfaceT* db_interface;

    db_interface = (SmDbServiceDomainInterfaceT*) record;

    interface = sm_service_domain_interface_table_read( 
                                    db_interface->service_domain,
                                    db_interface->service_domain_interface );
    if( NULL == interface )
    {
        interface = (SmServiceDomainInterfaceT*) 
                    malloc( sizeof(SmServiceDomainInterfaceT) );
        if( NULL == interface )
        {
            DPRINTFE( "Failed to allocate service domain interface table "
                      "entry." );
            return( SM_FAILED );
        }

        memset( interface, 0, sizeof(SmServiceDomainInterfaceT) );

        interface->id = db_interface->id;
        snprintf( interface->service_domain, 
                  sizeof(interface->service_domain), "%s", 
                  db_interface->service_domain );
        snprintf( interface->service_domain_interface, 
                  sizeof(interface->service_domain_interface), "%s", 
                  db_interface->service_domain_interface );
        interface->path_type = db_interface->path_type;
        interface->auth_type = db_interface->auth_type;
        snprintf( interface->auth_key, sizeof(interface->auth_key), "%s", 
                  db_interface->auth_key );
        snprintf( interface->interface_name,
                  sizeof(interface->interface_name), "%s", 
                  db_interface->interface_name );
        interface->interface_state = db_interface->interface_state;
        interface->network_type = db_interface->network_type;
        interface->network_multicast = db_interface->network_multicast;
        interface->network_address = db_interface->network_address;
        interface->network_port = db_interface->network_port;
        interface->network_heartbeat_port = db_interface->network_heartbeat_port;
        interface->network_peer_address = db_interface->network_peer_address;
        interface->network_peer_port = db_interface->network_peer_port;
        interface->network_peer_heartbeat_port
            = db_interface->network_peer_heartbeat_port;
        interface->unicast_socket = -1;
        interface->multicast_socket = -1;
        interface->interface_connect_type = db_interface->interface_connect_type;
        interface->interface_type = sm_get_interface_type(interface->service_domain_interface);

        SM_LIST_PREPEND( _service_domain_interfaces, 
                         (SmListEntryDataPtrT) interface );

    } else { 
        interface->id = db_interface->id;
        interface->path_type = db_interface->path_type;
        interface->auth_type = db_interface->auth_type;
        snprintf( interface->auth_key, sizeof(interface->auth_key), "%s", 
                  db_interface->auth_key );
        snprintf( interface->interface_name,
                  sizeof(interface->interface_name), "%s", 
                  db_interface->interface_name );
        interface->interface_state = db_interface->interface_state;
        interface->network_type = db_interface->network_type;
        interface->network_multicast = db_interface->network_multicast;
        interface->network_address = db_interface->network_address;
        interface->network_port = db_interface->network_port;
        interface->network_heartbeat_port = db_interface->network_heartbeat_port;
        interface->network_peer_address = db_interface->network_peer_address;
        interface->network_peer_port = db_interface->network_peer_port;
        interface->network_peer_heartbeat_port 
            = db_interface->network_peer_heartbeat_port;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - Load
// =====================================
SmErrorT sm_service_domain_interface_table_load( void )
{
    char db_query[SM_DB_QUERY_STATEMENT_MAX_CHAR]; 
    SmDbServiceDomainInterfaceT interface;
    SmErrorT error;

    snprintf( db_query, sizeof(db_query), "%s = 'yes'",
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_PROVISIONED );

    error = sm_db_foreach( SM_DATABASE_NAME, 
                           SM_SERVICE_DOMAIN_INTERFACES_TABLE_NAME,
                           db_query, &interface, 
                           sm_db_service_domain_interfaces_convert,
                           sm_service_domain_interface_table_add, NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over service domain interfaces in "
                  "database, error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - Persist
// ========================================
SmErrorT sm_service_domain_interface_table_persist(
    SmServiceDomainInterfaceT* interface )
{
    SmDbServiceDomainInterfaceT db_interface;
    SmErrorT error;

    memset( &db_interface, 0, sizeof(db_interface) );

    db_interface.id = interface->id;
    snprintf( db_interface.service_domain, 
              sizeof(db_interface.service_domain), "%s", 
              interface->service_domain );
    snprintf( db_interface.service_domain_interface, 
              sizeof(db_interface.service_domain_interface), "%s", 
              interface->service_domain_interface );
    db_interface.path_type = interface->path_type;
    db_interface.auth_type = interface->auth_type;
    snprintf( db_interface.auth_key, sizeof(db_interface.auth_key), "%s", 
              interface->auth_key );
    snprintf( db_interface.interface_name,
              sizeof(db_interface.interface_name), "%s", 
              interface->interface_name );
    db_interface.interface_state = interface->interface_state;
    db_interface.network_type = interface->network_type;
    db_interface.network_multicast = interface->network_multicast;
    db_interface.network_address = interface->network_address;
    db_interface.network_port = interface->network_port;
    db_interface.network_heartbeat_port = interface->network_heartbeat_port;
    db_interface.network_peer_address = interface->network_peer_address;
    db_interface.network_peer_port = interface->network_peer_port;
    db_interface.network_peer_heartbeat_port
        = interface->network_peer_heartbeat_port;

    error = sm_db_service_domain_interfaces_update( _sm_db_handle,
                                                    &db_interface );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update database, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - Initialize
// ===========================================
SmErrorT sm_service_domain_interface_table_initialize( void )
{
    SmErrorT error;

    _service_domain_interfaces = NULL;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_interface_table_load();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to load service domain interfaces from database, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - Finalize
// =========================================
SmErrorT sm_service_domain_interface_table_finalize( void )
{
    SmErrorT error;

    SM_LIST_CLEANUP_ALL( _service_domain_interfaces );

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
