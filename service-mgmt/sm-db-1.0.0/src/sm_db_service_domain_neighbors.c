//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_domain_neighbors.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sqlite3.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db.h"

// ****************************************************************************
// Database Service Domain Neighbors - Convert
// ===========================================
SmErrorT sm_db_service_domain_neighbors_convert( const char* col_name,
    const char* col_data, void* data )
{
    SmDbServiceDomainNeighborT* record = (SmDbServiceDomainNeighborT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_ID,
                      col_name ) )
    {
        record->id = atoll(col_data);
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_NAME,
                          col_name ) )
    {
        snprintf( record->name, sizeof(record->name), "%s", 
                  col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_SERVICE_DOMAIN, 
                          col_name ) )
    {
        snprintf( record->service_domain, sizeof(record->service_domain),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_ORCHESTRATION, 
                          col_name ) )
    {
        snprintf( record->orchestration, sizeof(record->orchestration),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_DESIGNATION, 
                          col_name ) )
    {
        snprintf( record->designation, sizeof(record->designation),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_GENERATION,
                          col_name ) )
    {
        record->generation = col_data ? atoi(col_data) : 0;
    }    
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_PRIORITY,
                          col_name ) )
    {
        record->priority = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_HELLO_INTERVAL,
                          col_name ) )
    {
        record->hello_interval = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_DEAD_INTERVAL,
                          col_name ) )
    {
        record->dead_interval = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_WAIT_INTERVAL,
                          col_name ) )
    {
        record->wait_interval = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_EXCHANGE_INTERVAL,
                          col_name ) )
    {
        record->exchange_interval = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_STATE,
                          col_name ) )
    {
        record->state = sm_service_domain_neighbor_state_value( col_data );
    }
    else 
    {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Read Callback
// =================================================
static int sm_db_service_domain_neighbors_read_callback( void* user_data,
    int num_cols, char **col_data, char **col_name )
{
    SmDbServiceDomainNeighborT* record = (SmDbServiceDomainNeighborT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_domain_neighbors_convert( col_name[col_i],
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
// Database Service Domain Neighbors - Read
// ========================================
SmErrorT sm_db_service_domain_neighbors_read( SmDbHandleT* sm_db_handle,
    char name[], char service_domain[], SmDbServiceDomainNeighborT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainNeighborT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s' and "
              "%s = '%s';",
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_NAME,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_NAME, name,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_SERVICE_DOMAIN,
              service_domain );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_domain_neighbors_read_callback,
                       record, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to read, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Read finished." );

    if( 0 != strcmp( name, record->name ) )
    {
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Read By Identifier
// ======================================================
SmErrorT sm_db_service_domain_neighbors_read_by_id( SmDbHandleT* sm_db_handle,
    int64_t id, SmDbServiceDomainNeighborT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainNeighborT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%" PRIi64 "';",
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_NAME,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_ID, id );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_domain_neighbors_read_callback,
                       record, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to read, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Read finished." );

    if( id != record->id )
    {
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Query
// =========================================
SmErrorT sm_db_service_domain_neighbors_query( SmDbHandleT* sm_db_handle, 
    const char* db_query, SmDbServiceDomainNeighborT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainNeighborT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s;",
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_NAME, db_query );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_service_domain_neighbors_read_callback, record,
                       &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to read, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Read finished." );

    if( 0 >= strlen( record->name ) )
    {
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Insert
// ==========================================
SmErrorT sm_db_service_domain_neighbors_insert( SmDbHandleT* sm_db_handle, 
    SmDbServiceDomainNeighborT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "INSERT INTO %s "
                  "( %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s', '%i', '%i', '%i', '%i', "
                  "'%i', '%i', '%s');",
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_NAME, 
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_NAME, 
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_SERVICE_DOMAIN,
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_ORCHESTRATION,
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_DESIGNATION,
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_GENERATION,
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_PRIORITY,
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_HELLO_INTERVAL,
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_DEAD_INTERVAL,
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_WAIT_INTERVAL,
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_EXCHANGE_INTERVAL,
                  SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_STATE,
                  record->name,
                  record->service_domain,
                  record->orchestration,
                  record->designation,
                  record->generation,
                  record->priority,
                  record->hello_interval, record->dead_interval,
                  record->wait_interval, record->exchange_interval,
                  sm_service_domain_neighbor_state_str( record->state )
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
// Database Service Domain Neighbors - Update
// ==========================================
SmErrorT sm_db_service_domain_neighbors_update( SmDbHandleT* sm_db_handle,
    SmDbServiceDomainNeighborT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_NAME);

    if( '\0' != record->orchestration[0] )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_ORCHESTRATION,
                         record->orchestration );
    }

    if( '\0' != record->designation[0] )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_DESIGNATION,
                         record->designation );
    }

    if( -1 < record->generation )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_GENERATION,
                         record->generation );
    }
    
    if( 0 < record->priority )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_PRIORITY,
                         record->priority );
    }

    if( 0 < record->hello_interval )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_HELLO_INTERVAL,
                         record->hello_interval );
    }

    if( 0 < record->dead_interval )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_DEAD_INTERVAL,
                         record->dead_interval );
    }

    if( 0 < record->wait_interval )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_WAIT_INTERVAL,
                         record->wait_interval );
    }

    if( 0 < record->exchange_interval )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_EXCHANGE_INTERVAL,
                         record->exchange_interval );
    }

    if( SM_SERVICE_DOMAIN_NEIGHBOR_STATE_NIL != record->state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_STATE,
                         sm_service_domain_neighbor_state_str(record->state) );
    }

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s' and %s = '%s';",
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_NAME, record->name, 
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_SERVICE_DOMAIN,
              record->service_domain );

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
// Database Service Domain Neighbors - Delete
// ==========================================
SmErrorT sm_db_service_domain_neighbors_delete( SmDbHandleT* sm_db_handle,
    char name[], char service_domain[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s WHERE %s = '%s' "
              "and %s = '%s';",
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_NAME,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_NAME, name, 
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_SERVICE_DOMAIN, 
              service_domain );

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
// Database Service Domain Neighbors - Create Table
// ================================================
SmErrorT sm_db_service_domain_neighbors_create_table(
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
                  "%s INT, "                  
                  "%s INT, "
                  "%s INT, "
                  "%s INT, "
                  "%s INT, "
                  "%s INT, "
                  "%s CHAR(%i));",
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_NAME,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_ID,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_NAME,
              SM_NODE_NAME_MAX_CHAR,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_SERVICE_DOMAIN,
              SM_SERVICE_DOMAIN_NAME_MAX_CHAR,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_ORCHESTRATION,
              SM_ORCHESTRATION_MAX_CHAR,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_DESIGNATION,
              SM_DESIGNATION_MAX_CHAR,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_GENERATION,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_PRIORITY,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_HELLO_INTERVAL,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_DEAD_INTERVAL,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_WAIT_INTERVAL,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_EXCHANGE_INTERVAL,
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_STATE,
              SM_SERVICE_DOMAIN_NEIGHBOR_STATE_MAX_CHAR );

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
// Database Service Domain Neighbors - Delete Table
// ================================================
SmErrorT sm_db_service_domain_neighbors_delete_table(
    SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_NAME );

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
// Database Service Domain Neighbors - Cleanup Table
// =================================================
SmErrorT sm_db_service_domain_neighbors_cleanup_table( 
    SmDbHandleT* sm_db_handle )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_NAME );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_STATE,
                     sm_service_domain_neighbor_state_str(
                                SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN ) );

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
// Database Service Domain Neighbors - Initialize
// ==============================================
SmErrorT sm_db_service_domain_neighbors_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Finalize
// ============================================
SmErrorT sm_db_service_domain_neighbors_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
