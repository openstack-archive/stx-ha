//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_action_results.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db.h"

// ****************************************************************************
// Database Service Action Results - Convert
// =========================================
SmErrorT sm_db_service_action_results_convert( const char* col_name, 
    const char* col_data, void* data )
{
    SmDbServiceActionResultT* record;
    
    record = (SmDbServiceActionResultT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 == strcmp( SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_TYPE,
                     col_name ) )
    {
        snprintf( record->plugin_type, sizeof(record->plugin_type),
                  "%s", col_data ? col_data : "" );
    } 
    else if( 0 == strcmp( SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_NAME,
                          col_name ) ) 
    {
        snprintf( record->plugin_name, sizeof(record->plugin_name),
                  "%s", col_data ? col_data : "" );

    } 
    else if( 0 == strcmp( SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_COMMAND,
                          col_name ) )
    {
        snprintf( record->plugin_command, sizeof(record->plugin_command),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_EXIT_CODE,
                          col_name ) ) 
    {
        snprintf( record->plugin_exit_code, sizeof(record->plugin_exit_code),
                  "%s", col_data ? col_data : "" );
    } 
    else if( 0 == strcmp( SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_ACTION_RESULT,
                          col_name ) ) 
    {
        record->action_result = sm_service_action_result_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_STATE,
                          col_name ) )
    {
        record->service_state = sm_service_state_value( col_data );
    }
    else if( 0 == strcmp( SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_STATUS,
                          col_name ) )
    {
        record->service_status = sm_service_status_value( col_data );
    }
    else if( 0 == strcmp( SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_CONDITION,
                          col_name ) )
    {
        record->service_condition = sm_service_condition_value( col_data );
    }
    else 
    {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Action Results - Read Callback
// ===============================================
static int sm_db_service_action_results_read_callback( void* user_data,
    int num_cols, char **col_data, char **col_name )
{
    SmDbServiceActionResultT* record;
    SmErrorT error;
    
    record = (SmDbServiceActionResultT*) user_data;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_action_results_convert( col_name[col_i],
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
// Database Service Action Results - Read
// ======================================
SmErrorT sm_db_service_action_results_read( SmDbHandleT* sm_db_handle,
    char plugin_type[],  char plugin_name[], char plugin_command[],
    char plugin_exit_code[], SmDbServiceActionResultT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceActionResultT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s "
              "WHERE %s = '%s' and %s = '%s' and %s = '%s' and %s = '%s';",
              SM_SERVICE_ACTION_RESULTS_TABLE_NAME,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_TYPE,
              plugin_type,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_NAME,
              plugin_name,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_COMMAND,
              plugin_command,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_EXIT_CODE,
              plugin_exit_code );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_action_results_read_callback,
                       record, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to read, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    DPRINTFD( "Read finished." );

    if( 0 != strcmp( plugin_type, record->plugin_type ) )
    {
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Action Results - Insert
// ========================================
SmErrorT sm_db_service_action_results_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceActionResultT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;
    
    snprintf( sql, sizeof(sql), "INSERT INTO %s ( "
                  "%s, %s, %s, %s, %s, %s %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s' );",
                  SM_SERVICE_ACTION_RESULTS_TABLE_NAME,
                  SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_TYPE,
                  SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_NAME,
                  SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_COMMAND,
                  SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_EXIT_CODE,
                  SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_ACTION_RESULT,
                  SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_STATE,
                  SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_STATUS,
                  SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_CONDITION,
                  record->plugin_type, record->plugin_name,
                  record->plugin_command, record->plugin_exit_code, 
                  sm_service_action_result_str( record->action_result ),
                  sm_service_state_str( record->service_state ),
                  sm_service_status_str( record->service_status ),
                  sm_service_condition_str( record->service_condition ) );

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
// Database Service Action Results - Update
// ========================================
SmErrorT sm_db_service_action_results_update( SmDbHandleT* sm_db_handle,
    SmDbServiceActionResultT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_ACTION_RESULTS_TABLE_NAME );

    if( SM_SERVICE_ACTION_RESULT_NIL != record->action_result )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_ACTION_RESULT,
                         sm_service_action_result_str( record->action_result ) );
    }

    if( SM_SERVICE_STATE_NIL != record->service_state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_STATE,
                         sm_service_state_str( record->service_state ) );
    }

    if( SM_SERVICE_STATUS_NIL != record->service_status )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_STATUS,
                         sm_service_status_str( record->service_status ) );
    }

    if( SM_SERVICE_CONDITION_NIL != record->service_condition )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_CONDITION,
                         sm_service_condition_str( record->service_condition ) );
    }
    
    snprintf( sql+len-2, sizeof(sql)-len,
              " WHERE %s = '%s' %s = '%s' and %s = '%s' and %s = '%s';",
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_TYPE,
              record->plugin_type,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_NAME,
              record->plugin_name,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_COMMAND,
              record->plugin_command,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_EXIT_CODE,
              record->plugin_exit_code );

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
// Database Service Action Results - Delete
// ========================================
SmErrorT sm_db_service_action_results_delete( SmDbHandleT* sm_db_handle,
    char plugin_type[], char plugin_name[], char plugin_command[], 
    char plugin_exit_code[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s "
              "WHERE %s = '%s' and %s = '%s' and %s = '%s' and %s = '%s';",
              SM_SERVICE_ACTION_RESULTS_TABLE_NAME,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_TYPE,
              plugin_type,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_NAME,
              plugin_name,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_COMMAND,
              plugin_command,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_EXIT_CODE,
              plugin_exit_code );

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
// Database Service Action Results - Create Table
// ==============================================
SmErrorT sm_db_service_action_results_create_table( SmDbHandleT* sm_db_handle )
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
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "PRIMARY KEY (%s, %s, %s, %s));",
              SM_SERVICE_ACTION_RESULTS_TABLE_NAME,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_TYPE,
              SM_SERVICE_ACTION_PLUGIN_TYPE_MAX_CHAR,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_NAME,
              SM_SERVICE_ACTION_PLUGIN_NAME_MAX_CHAR,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_COMMAND,
              SM_SERVICE_ACTION_PLUGIN_COMMAND_MAX_CHAR,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_EXIT_CODE,
              SM_SERVICE_ACTION_PLUGIN_EXIT_CODE_MAX_CHAR,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_ACTION_RESULT,
              SM_SERVICE_ACTION_RESULT_MAX_CHAR,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_STATE,
              SM_SERVICE_STATE_MAX_CHAR,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_STATUS,
              SM_SERVICE_STATUS_MAX_CHAR,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_CONDITION,
              SM_SERVICE_CONDITION_MAX_CHAR,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_TYPE,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_NAME,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_COMMAND,
              SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_EXIT_CODE );

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
// Database Service Action Results - Delete Table
// ==============================================
SmErrorT sm_db_service_action_results_delete_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_ACTION_RESULTS_TABLE_NAME );

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
// Database Service Action Results - Initialize
// ============================================
SmErrorT sm_db_service_action_results_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Action Results - Finalize
// ==========================================
SmErrorT sm_db_service_action_results_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
