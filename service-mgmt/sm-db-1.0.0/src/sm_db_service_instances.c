//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_instances.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db.h"

// ****************************************************************************
// Database Service Instances - Convert
// ====================================
SmErrorT sm_db_service_instances_convert( const char* col_name, 
    const char* col_data, void* data )
{
    SmDbServiceInstanceT* record = (SmDbServiceInstanceT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_SERVICE_INSTANCES_TABLE_COLUMN_ID, col_name ) )
    {
        record->id = atoll(col_data);
    }
    else if( 0 == strcmp( SM_SERVICE_INSTANCES_TABLE_COLUMN_SERVICE_NAME,
                          col_name ) )
    {
        snprintf( record->service_name, sizeof(record->service_name),
                  "%s", col_data ? col_data : "" );
    } 
    else if( 0 == strcmp( SM_SERVICE_INSTANCES_TABLE_COLUMN_INSTANCE_NAME,
                          col_name ) )
    {
        snprintf( record->instance_name, sizeof(record->instance_name),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_INSTANCES_TABLE_COLUMN_INSTANCE_PARAMETERS,
                          col_name ) )
    {
        snprintf( record->instance_params, sizeof(record->instance_params),
                  "%s", col_data ? col_data : "" );
    }
    else 
    {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Instances - Read Callback
// ==========================================
static int sm_db_service_instances_read_callback( void* user_data,
    int num_cols, char **col_data, char **col_name )
{
    SmDbServiceInstanceT* record = (SmDbServiceInstanceT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_instances_convert( col_name[col_i], 
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
// Database Service Instances - Read
// =================================
SmErrorT sm_db_service_instances_read( SmDbHandleT* sm_db_handle, 
    char service_name[], SmDbServiceInstanceT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceInstanceT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s';",
              SM_SERVICE_INSTANCES_TABLE_NAME,
              SM_SERVICE_INSTANCES_TABLE_COLUMN_SERVICE_NAME,
              service_name );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_instances_read_callback,
                       record, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to read, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Read finished." );

    if( 0 != strcmp( service_name, record->service_name ) )
    {
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Instances - Insert
// ===================================
SmErrorT sm_db_service_instances_insert( SmDbHandleT* sm_db_handle, 
    SmDbServiceInstanceT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "INSERT INTO %s ( %s, %s, %s ) "
                  "VALUES ('%s', '%s', '%s' );",
                  SM_SERVICE_INSTANCES_TABLE_NAME,
                  SM_SERVICE_INSTANCES_TABLE_COLUMN_SERVICE_NAME,
                  SM_SERVICE_INSTANCES_TABLE_COLUMN_INSTANCE_NAME,
                  SM_SERVICE_INSTANCES_TABLE_COLUMN_INSTANCE_PARAMETERS,
                  record->service_name, record->instance_name, 
                  record->instance_params );

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
// Database Service Instances - Update
// ===================================
SmErrorT sm_db_service_instances_update( SmDbHandleT* sm_db_handle, 
    SmDbServiceInstanceT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_INSTANCES_TABLE_NAME );

    if( 0 < strlen( record->instance_name ) )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_INSTANCES_TABLE_COLUMN_INSTANCE_NAME,
                         record->instance_name );
    }

    if( 0 < strlen( record->instance_params ) )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_INSTANCES_TABLE_COLUMN_INSTANCE_PARAMETERS,
                         record->instance_params );
    }

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s';",
              SM_SERVICE_INSTANCES_TABLE_COLUMN_SERVICE_NAME,
              record->service_name );

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
// Database Service Instances - Delete
// ===================================
SmErrorT sm_db_service_instances_delete( SmDbHandleT* sm_db_handle, 
    char service_name[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s WHERE %s = '%s';",
              SM_SERVICE_INSTANCES_TABLE_NAME,
              SM_SERVICE_INSTANCES_TABLE_COLUMN_SERVICE_NAME,
              service_name );

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
// Database Service Instances - Create Table
// =========================================
SmErrorT sm_db_service_instances_create_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "CREATE TABLE IF NOT EXISTS "
              "%s ( "
                  "%s INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i) );",
              SM_SERVICE_INSTANCES_TABLE_NAME,
              SM_SERVICE_INSTANCES_TABLE_COLUMN_ID,              
              SM_SERVICE_INSTANCES_TABLE_COLUMN_SERVICE_NAME,              
              SM_SERVICE_NAME_MAX_CHAR,
              SM_SERVICE_INSTANCES_TABLE_COLUMN_INSTANCE_NAME,
              SM_SERVICE_INSTANCE_NAME_MAX_CHAR,
              SM_SERVICE_INSTANCES_TABLE_COLUMN_INSTANCE_PARAMETERS,
              SM_SERVICE_INSTANCE_PARAMS_MAX_CHAR );

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
// Database Service Instances - Delete Table
// =========================================
SmErrorT sm_db_service_instances_delete_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_INSTANCES_TABLE_NAME );

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
// Database Service Instances - Initialize
// =======================================
SmErrorT sm_db_service_instances_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Instances - Finalize
// =====================================
SmErrorT sm_db_service_instances_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
