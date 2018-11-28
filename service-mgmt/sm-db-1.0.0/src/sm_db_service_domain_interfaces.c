//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_domain_interfaces.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db.h"

// ****************************************************************************
// Database Service Domain Interfaces - Convert
// ============================================
SmErrorT sm_db_service_domain_interfaces_convert( const char* col_name,
    const char* col_data, void* data )
{
    SmDbServiceDomainInterfaceT* record = (SmDbServiceDomainInterfaceT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_ID, col_name ) )
    {
        record->id = atoll(col_data);
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_PROVISIONED,
                          col_name ) )
    {
        record->provisioned = false;

        if( col_data )
        {
            if(( 0 == strcmp( "yes", col_data ))||
               ( 0 == strcmp( "Yes", col_data ))||
               ( 0 == strcmp( "YES", col_data )))
            {
                record->provisioned = true;
            }
        }
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN, 
                          col_name ) )
    {
        snprintf( record->service_domain, sizeof(record->service_domain),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN_INTERFACE, 
                          col_name ) )
    {
        snprintf( record->service_domain_interface,
                  sizeof(record->service_domain_interface),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_PATH_TYPE,
                          col_name ) )
    {
        record->path_type = sm_path_type_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_AUTH_TYPE, col_name ) )
    {
        record->auth_type = sm_auth_type_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_AUTH_KEY, 
                          col_name ) )
    {
        snprintf( record->auth_key, sizeof(record->auth_key),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_NAME,
                          col_name ) )
    {
        snprintf( record->interface_name, sizeof(record->interface_name), "%s", 
                  col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_STATE,
                          col_name ) )
    {
        record->interface_state = sm_interface_state_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_TYPE, 
                          col_name ) )
    {
        record->network_type = sm_network_type_value( col_data );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_MULTICAST, 
                          col_name ) )
    {
        // Assumes that network type has already been set in the record.
        record->network_multicast.type = record->network_type;
        if( !sm_network_address_value( col_data, &(record->network_multicast) ) )
        {
            // MULTICAST NOT CONFIGURED
            record->network_multicast.type = SM_NETWORK_TYPE_NIL;
            DPRINTFI( "Multicast not configured, look like (%s).", col_data );
        }
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_ADDRESS, 
                          col_name ) )
    {
        // Assumes that network type has already been set in the record.
        record->network_address.type = record->network_type;
        sm_network_address_value( col_data, &(record->network_address) );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PORT,
                          col_name ) )
    {
        record->network_port = col_data ? atoi(col_data) : -1;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_HEARTBEAT_PORT,
                          col_name ) )
    {
        record->network_heartbeat_port = col_data ? atoi(col_data) : -1;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_ADDRESS,
                          col_name ) )
    {
        // Assumes that network type has already been set in the record.
        record->network_peer_address.type = record->network_type;
        sm_network_address_value( col_data, &(record->network_peer_address) );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_PORT,
                          col_name ) )
    {
        record->network_peer_port = col_data ? atoi(col_data) : -1;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_HEARTBEAT_PORT,
                          col_name ) )
    {
        record->network_peer_heartbeat_port = col_data ? atoi(col_data) : -1;
    }else if ( 0 == strcmp( SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_CONNECT_TYPE, col_name ) )
    {
        if ( 0 == strcmp( SM_INTERFACE_CONNECT_TYPE_DC, col_data ) )
        {
            record->interface_connect_type = SM_SERVICE_DOMAIN_INTERFACE_CONNECT_TYPE_DC;
            DPRINTFD( "connect type %s", SM_INTERFACE_CONNECT_TYPE_DC );
        }
        else if ( 0 == strcmp( SM_INTERFACE_CONNECT_TYPE_TOR, col_data ) )
        {
            record->interface_connect_type = SM_SERVICE_DOMAIN_INTERFACE_CONNECT_TYPE_TOR;
            DPRINTFD( "connect type %s", SM_INTERFACE_CONNECT_TYPE_TOR );
        }
        else
        {
            DPRINTFE( "Unknown interface connect type (%s).", col_data );
        }
    }
    else 
    {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Read Callback
// ==================================================
static int sm_db_service_domain_interfaces_read_callback( void* user_data,
    int num_cols, char **col_data, char **col_name )
{
    SmDbServiceDomainInterfaceT* record = (SmDbServiceDomainInterfaceT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_domain_interfaces_convert( col_name[col_i],
                                                    col_data[col_i], record );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Convert failed for column (%s), error=%s.", 
                      col_name[col_i], sm_error_str( error ) );
            return( SQLITE_ERROR );
        }
    }

    return( SQLITE_OK );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Read
// =========================================
SmErrorT sm_db_service_domain_interfaces_read( SmDbHandleT* sm_db_handle,
    char service_domain[], char service_domain_interface[],
    SmDbServiceDomainInterfaceT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainInterfaceT) );
    record->network_type = SM_NETWORK_TYPE_UNKNOWN;
    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s' and "
              "%s = '%s';",
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_NAME,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN,
              service_domain,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN_INTERFACE,
              service_domain_interface );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_domain_interfaces_read_callback,
                       record, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to read, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Read finished." );

    if(( 0 != strcmp( service_domain, record->service_domain ) )&&
       ( 0 != strcmp( service_domain_interface,
                      record->service_domain_interface ) ))
    {
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Query
// ==========================================
SmErrorT sm_db_service_domain_interfaces_query( SmDbHandleT* sm_db_handle, 
    const char* db_query, SmDbServiceDomainInterfaceT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainInterfaceT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s;",
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_NAME, db_query );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_service_domain_interfaces_read_callback, record,
                       &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to read, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Read finished." );

    if( 0 >= strlen( record->service_domain_interface ) )
    {
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Insert
// ===========================================
SmErrorT sm_db_service_domain_interfaces_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceDomainInterfaceT* record )
{
    char network_multicast[SM_NETWORK_ADDRESS_MAX_CHAR];
    char network_address[SM_NETWORK_ADDRESS_MAX_CHAR];
    char network_peer_address[SM_NETWORK_ADDRESS_MAX_CHAR];
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    sm_network_address_str( &(record->network_multicast), network_multicast );
    sm_network_address_str( &(record->network_address), network_address );
    sm_network_address_str( &(record->network_peer_address), network_peer_address );

    snprintf( sql, sizeof(sql), "INSERT INTO %s "
                  "( %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', "
                  "'%s', '%s', '%i', '%i', '%s', '%i', '%i');",
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_NAME, 
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_PROVISIONED,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN_INTERFACE,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_PATH_TYPE,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_AUTH_TYPE,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_AUTH_KEY,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_NAME, 
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_STATE,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_TYPE,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_MULTICAST,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_ADDRESS,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PORT,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_HEARTBEAT_PORT,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_ADDRESS,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_PORT,
                  SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_HEARTBEAT_PORT,
                  record->provisioned ? "yes" : "no",                  
                  record->service_domain,
                  record->service_domain_interface,
                  sm_path_type_str( record->path_type ),
                  sm_auth_type_str( record->auth_type ),
                  record->auth_key,
                  record->interface_name,
                  sm_interface_state_str( record->interface_state ),
                  sm_network_type_str( record->network_type ),
                  network_multicast, network_address, record->network_port,
                  record->network_heartbeat_port, network_peer_address,
                  record->network_peer_port, record->network_peer_heartbeat_port
            );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, NULL, NULL, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to insert, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Insert completed." );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database  Service Domain Interfaces - Update
// ============================================
SmErrorT sm_db_service_domain_interfaces_update( SmDbHandleT* sm_db_handle,
    SmDbServiceDomainInterfaceT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_DOMAIN_INTERFACES_TABLE_NAME);

    if( SM_PATH_TYPE_NIL != record->path_type )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_PATH_TYPE,
                         sm_path_type_str(record->path_type) );
    }

    if( SM_AUTH_TYPE_NIL != record->auth_type )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_AUTH_TYPE,
                         sm_auth_type_str(record->auth_type) );
    }

    if( '\0' != record->auth_key[0] )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_AUTH_KEY,
                         record->auth_key );
    }

    if( '\0' != record->interface_name[0] )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_NAME,
                         record->interface_name );
    }

    if( SM_INTERFACE_STATE_NIL != record->interface_state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_STATE,
                         sm_interface_state_str(record->interface_state) );
    }

    if( SM_NETWORK_TYPE_NIL != record->network_type )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_TYPE,
                         sm_network_type_str(record->network_type) );
    }

    if( SM_NETWORK_TYPE_UNKNOWN != record->network_multicast.type )
    {
        char network_multicast[SM_NETWORK_ADDRESS_MAX_CHAR];

        sm_network_address_str( &(record->network_multicast),
                                network_multicast );

        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_MULTICAST,
                         network_multicast );
    }

    if( SM_NETWORK_TYPE_UNKNOWN != record->network_address.type )
    {
        char network_address[SM_NETWORK_ADDRESS_MAX_CHAR];

        sm_network_address_str( &(record->network_address), network_address );

        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_ADDRESS,
                         network_address );
    }

    if( -1 != record->network_port )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PORT,
                         record->network_port );
    }

    if( -1 != record->network_heartbeat_port )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_HEARTBEAT_PORT,
                         record->network_heartbeat_port );
    }

    if( SM_NETWORK_TYPE_UNKNOWN != record->network_peer_address.type )
    {
        char network_peer_address[SM_NETWORK_ADDRESS_MAX_CHAR];

        sm_network_address_str( &(record->network_peer_address),
                                network_peer_address );

        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_ADDRESS,
                         network_peer_address );
    }

    if( -1 != record->network_peer_port )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_PORT,
                         record->network_peer_port );
    }

    if( -1 != record->network_peer_heartbeat_port )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_HEARTBEAT_PORT,
                         record->network_peer_heartbeat_port );
    }

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s' and %s = '%s';",
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN,
              record->service_domain,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN_INTERFACE,
              record->service_domain_interface );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, NULL, NULL, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to update, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Update completed." );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Delete
// ===========================================
SmErrorT sm_db_service_domain_interfaces_delete( SmDbHandleT* sm_db_handle,
    char service_domain[], char service_domain_interface[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s WHERE %s = '%s' and %s = '%s';",
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_NAME,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN,
              service_domain,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN_INTERFACE,
              service_domain_interface );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, NULL, NULL, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to delete table, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Delete finished." );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Create Table
// =================================================
SmErrorT sm_db_service_domain_interfaces_create_table(
    SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "CREATE TABLE IF NOT EXISTS "
              "%s ( "
                  "%s INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s INT, "
                  "%s INT, "
                  "%s CHAR(%i), "
                  "%s INT, "
                  "%s INT );",
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_NAME,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_ID,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_PROVISIONED,
              SM_SERVICE_DOMAIN_INTERFACE_PROVISIONED_MAX_CHAR,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN,
              SM_SERVICE_DOMAIN_NAME_MAX_CHAR,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN_INTERFACE,
              SM_SERVICE_DOMAIN_INTERFACE_NAME_MAX_CHAR,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_PATH_TYPE,
              SM_PATH_TYPE_MAX_CHAR,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_AUTH_TYPE,
              SM_AUTHENTICATION_TYPE_MAX_CHAR,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_AUTH_KEY,
              SM_AUTHENTICATION_KEY_MAX_CHAR,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_NAME,
              SM_INTERFACE_NAME_MAX_CHAR, 
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_STATE,
              SM_INTERFACE_STATE_MAX_CHAR,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_TYPE,
              SM_NETWORK_TYPE_MAX_CHAR,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_MULTICAST,
              SM_NETWORK_ADDRESS_MAX_CHAR,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_ADDRESS,
              SM_NETWORK_ADDRESS_MAX_CHAR,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PORT,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_HEARTBEAT_PORT,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_ADDRESS,
              SM_NETWORK_ADDRESS_MAX_CHAR,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_PORT,
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_HEARTBEAT_PORT );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, NULL, NULL, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to create table, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Table created." );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Delete Table
// =================================================
SmErrorT sm_db_service_domain_interfaces_delete_table(
    SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_DOMAIN_INTERFACES_TABLE_NAME );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, NULL, NULL, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to delete table, rc=%i, error=%s.", rc, error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Table deleted." );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Cleanup Table
// ==================================================
SmErrorT sm_db_service_domain_interfaces_cleanup_table(
    SmDbHandleT* sm_db_handle )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_DOMAIN_INTERFACES_TABLE_NAME );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_STATE,
                     sm_interface_state_str( SM_INTERFACE_STATE_UNKNOWN ) );

    snprintf( sql+len-2, sizeof(sql)-len, ";" );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, NULL, NULL, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to update, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Cleanup completed." );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Initialize
// ===============================================
SmErrorT sm_db_service_domain_interfaces_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Finalize
// =============================================
SmErrorT sm_db_service_domain_interfaces_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
