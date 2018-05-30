//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_dependency.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db.h"

// ****************************************************************************
// Database Service Dependency - Convert
// =====================================
SmErrorT sm_db_service_dependency_convert( const char* col_name,
    const char* col_data, void* data )
{
    SmDbServiceDependencyT* record = (SmDbServiceDependencyT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 == strcmp( SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENCY_TYPE,
                     col_name ) )
    {
        record->type = sm_service_dependency_type_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_DEPENDENCY_TABLE_COLUMN_SERVICE_NAME,
                          col_name ) )
    {
        snprintf( record->service_name, sizeof(record->service_name),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DEPENDENCY_TABLE_COLUMN_STATE,
                          col_name ) )
    {
        if( col_data )
        {
            record->state = sm_service_state_value( col_data ); 
        } else {
            record->state = SM_SERVICE_STATE_NIL;
        }
    }
    else if( 0 == strcmp( SM_SERVICE_DEPENDENCY_TABLE_COLUMN_ACTION,
                          col_name ) )
    {
        if( col_data )
        {
            record->action = sm_service_action_value( col_data );
        } else {
            record->action = SM_SERVICE_ACTION_NIL;
        }
    }
    else if( 0 == strcmp( SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT,
                          col_name ) )
    {
        snprintf( record->dependent, sizeof(record->dependent),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT_STATE,
                          col_name ) )
    {
        record->dependent_state = sm_service_state_value( col_data ); 
    }
    else
    {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Dependency - Read Callback
// ===========================================
static int sm_db_service_dependency_read_callback( void* user_data,
    int num_cols, char **col_data, char **col_name )
{
    SmDbServiceDependencyT* record = (SmDbServiceDependencyT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_dependency_convert( col_name[col_i],
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
// Database Service Dependency - Read
// ==================================
SmErrorT sm_db_service_dependency_read( SmDbHandleT* sm_db_handle,
     SmServiceDependencyTypeT type, char service_name[], SmServiceStateT state,
     SmServiceActionT action, char dependent[], SmDbServiceDependencyT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDependencyT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s' "
              "and %s = '%s' and %s = '%s' and %s = '%s' and %s = '%s';",
              SM_SERVICE_DEPENDENCY_TABLE_NAME,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENCY_TYPE,
              sm_service_dependency_type_str( type ),
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_SERVICE_NAME,
              service_name,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_STATE,
              sm_service_state_str( state ),
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_ACTION,
              sm_service_action_str( action ),
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT,
              dependent );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_dependency_read_callback,
                       record, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to read, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Read finished." );

    if( type != record->type )
    {
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Dependency - Insert
// ====================================
SmErrorT sm_db_service_dependency_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceDependencyT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "INSERT INTO %s ( "
                  "%s, %s, %s, %s, %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s', '%s', %s );",
                  SM_SERVICE_DEPENDENCY_TABLE_NAME,
                  SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENCY_TYPE,
                  SM_SERVICE_DEPENDENCY_TABLE_COLUMN_SERVICE_NAME,
                  SM_SERVICE_DEPENDENCY_TABLE_COLUMN_STATE,
                  SM_SERVICE_DEPENDENCY_TABLE_COLUMN_ACTION,
                  SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT,
                  SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT_STATE,
                  sm_service_dependency_type_str( record->type ), 
                  record->service_name,
                  sm_service_state_str( record->state ),
                  sm_service_action_str( record->action ),
                  record->dependent,
                  sm_service_state_str( record->dependent_state ) );

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
// Database Service Dependency - Update
// ====================================
SmErrorT sm_db_service_dependency_update( SmDbHandleT* sm_db_handle,
    SmDbServiceDependencyT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_DEPENDENCY_TABLE_NAME );

    if( SM_SERVICE_STATE_NIL != record->dependent_state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT_STATE,
                         sm_service_state_str( record->dependent_state ) );
    }

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s' and %s = '%s' "
              "and %s = '%s' and %s = '%s' and %s = '%s';",
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENCY_TYPE,
              sm_service_dependency_type_str( record->type ),
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_SERVICE_NAME,
              record->service_name,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_STATE,
              sm_service_state_str( record->state ),
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_ACTION,
              sm_service_action_str( record->action ),
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT,
              record->dependent );

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
// Database Service Dependency - Delete
// ====================================
SmErrorT sm_db_service_dependency_delete( SmDbHandleT* sm_db_handle,
    SmServiceDependencyTypeT type, char service_name[], SmServiceStateT state,
    SmServiceActionT action, char dependent[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s "
              "WHERE %s = '%s' and %s = '%s' and %s = '%s' "
              "and %s = '%s' and %s = '%s';",
              SM_SERVICE_DEPENDENCY_TABLE_NAME,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENCY_TYPE,
              sm_service_dependency_type_str( type ),
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_SERVICE_NAME,
              service_name,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_STATE,
              sm_service_state_str( state ),
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_ACTION,
              sm_service_action_str( action ),
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT,
              dependent );

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
// Database Service Dependency - Create Table
// ==========================================
SmErrorT sm_db_service_dependency_create_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "CREATE TABLE IF NOT EXISTS "
              "%s ( "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "PRIMARY KEY (%s, %s, %s, %s, %s));",
              SM_SERVICE_DEPENDENCY_TABLE_NAME,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENCY_TYPE,
              SM_SERVICE_DEPENDENCY_MAX_CHAR,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_SERVICE_NAME,
              SM_SERVICE_NAME_MAX_CHAR,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_STATE,
              SM_SERVICE_STATE_MAX_CHAR,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_ACTION,
              SM_SERVICE_ACTION_NAME_MAX_CHAR,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT,
              SM_SERVICE_NAME_MAX_CHAR,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT_STATE,
              SM_SERVICE_STATE_MAX_CHAR,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENCY_TYPE,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_SERVICE_NAME,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_STATE,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_ACTION,
              SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT );

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
// Database Service Dependency - Delete Table
// ==========================================
SmErrorT sm_db_service_dependency_delete_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_DEPENDENCY_TABLE_NAME );

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
// Database Service Dependency - Initialize
// ========================================
SmErrorT sm_db_service_dependency_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Dependency - Finalize
// ======================================
SmErrorT sm_db_service_dependency_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
