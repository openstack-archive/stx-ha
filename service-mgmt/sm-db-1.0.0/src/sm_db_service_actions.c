//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_actions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db.h"

// ****************************************************************************
// Database Service Actions - Convert
// ==================================
SmErrorT sm_db_service_actions_convert( const char* col_name, 
    const char* col_data, void* data )
{
    SmDbServiceActionT* record = (SmDbServiceActionT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_SERVICE_NAME, col_name ) )
    {
        snprintf( record->service_name, sizeof(record->service_name),
                  "%s", col_data ? col_data : "" );
    } 
    else if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_ACTION,
                          col_name ) ) 
    {
        record->action = sm_service_action_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_TYPE,
                          col_name ) ) 
    {
        int max_plugin_type_str_len = sizeof(record->plugin_type)-1;
        int char_i = 0;

        if( NULL != col_data )
        {
            for( ; '\0' != col_data[char_i]; ++char_i )
            {
                if( max_plugin_type_str_len < char_i )
                {
                    break;
                }

                record->plugin_type[char_i] = tolower( col_data[char_i] );
            }
        }

        record->plugin_type[char_i] = '\0';
    } 
    else if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_CLASS,
                          col_name ) ) 
    {
        snprintf( record->plugin_class, sizeof(record->plugin_class),
                  "%s", col_data ? col_data : "" );
    } 
    else if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_NAME,
                          col_name ) ) 
    {
        snprintf( record->plugin_name, sizeof(record->plugin_name),
                  "%s", col_data ? col_data : "" );
    } 
    else if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_COMMAND,
                          col_name ) )
    {
        snprintf( record->plugin_command, sizeof(record->plugin_command),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_PARAMETERS,
                          col_name ) )
    {
        snprintf( record->plugin_params, sizeof(record->plugin_params),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_FAILURE_RETRIES,
                          col_name ) ) 
    {
        record->max_failure_retries = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_TIMEOUT_RETRIES,
                          col_name ) ) 
    {
        record->max_timeout_retries = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_TOTAL_RETRIES,
                          col_name ) ) 
    {
        record->max_total_retries = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_TIMEOUT_IN_SECS,
                            col_name ) ) 
    {
        record->timeout_in_secs = col_data ? atoi(col_data) : 0;
    } 
    else if( 0 == strcmp( SM_SERVICE_ACTIONS_TABLE_COLUMN_INTERVAL_IN_SECS,
                          col_name ) ) 
    {
        record->interval_in_secs = col_data ? atoi(col_data) : 0;
    } 
    else 
    {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Actions - Read Callback
// ========================================
static int sm_db_service_actions_read_callback( void* user_data, int num_cols,
    char **col_data, char **col_name )
{
    SmDbServiceActionT* record = (SmDbServiceActionT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_actions_convert( col_name[col_i], 
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
// Database Service Actions - Read
// ===============================
SmErrorT sm_db_service_actions_read( SmDbHandleT* sm_db_handle, 
    char service_name[], SmServiceActionT action, SmDbServiceActionT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceActionT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s "
              "WHERE %s = '%s' and %s = '%s';",
              SM_SERVICE_ACTIONS_TABLE_NAME,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_SERVICE_NAME,
              service_name,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_ACTION,
              sm_service_action_str( action ) );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_actions_read_callback,
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
// Database Service Actions - Insert
// =================================
SmErrorT sm_db_service_actions_insert( SmDbHandleT* sm_db_handle, 
    SmDbServiceActionT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "INSERT INTO %s ( "
                  "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s , %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', "
                  "'%s', %i, %i, %i, %i, %i);",
                  SM_SERVICE_ACTIONS_TABLE_NAME,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_SERVICE_NAME,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_ACTION,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_TYPE,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_CLASS,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_NAME,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_COMMAND,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_PARAMETERS,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_FAILURE_RETRIES,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_TIMEOUT_RETRIES,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_TOTAL_RETRIES,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_TIMEOUT_IN_SECS,
                  SM_SERVICE_ACTIONS_TABLE_COLUMN_INTERVAL_IN_SECS,
                  record->service_name, 
                  sm_service_action_str( record->action ),
                  record->plugin_type, record->plugin_class,
                  record->plugin_name, record->plugin_command, 
                  record->plugin_params, record->max_failure_retries,
                  record->max_timeout_retries, record->max_total_retries,
                  record->timeout_in_secs, record->interval_in_secs );

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
// Database Service Actions - Update
// =================================
SmErrorT sm_db_service_actions_update( SmDbHandleT* sm_db_handle, 
    SmDbServiceActionT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_ACTIONS_TABLE_NAME );

    if( 0 < strlen( record->plugin_type ) )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_TYPE,
                         record->plugin_type );
    }

    if( 0 < strlen( record->plugin_class ) )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_CLASS,
                         record->plugin_class );
    }

    if( 0 < strlen( record->plugin_name ) )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_NAME,
                         record->plugin_name );
    }

    if( 0 < strlen( record->plugin_command ) )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_COMMAND,
                         record->plugin_command );
    }

    if( 0 < strlen( record->plugin_params ) )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_PARAMETERS,
                         record->plugin_params );
    }

    if( 0 <= record->max_failure_retries )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_FAILURE_RETRIES,
                         record->max_failure_retries );
    }

    if( 0 <= record->max_timeout_retries )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_TIMEOUT_RETRIES,
                         record->max_timeout_retries );
    }

    if( 0 <= record->max_total_retries )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_TOTAL_RETRIES,
                         record->max_total_retries );
    }

    if( 0 <= record->timeout_in_secs )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_ACTIONS_TABLE_COLUMN_TIMEOUT_IN_SECS,
                         record->timeout_in_secs );
    }

    if( 0 <= record->interval_in_secs )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_ACTIONS_TABLE_COLUMN_INTERVAL_IN_SECS,
                         record->interval_in_secs );
    }

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s' and %s = '%s';",
              SM_SERVICE_ACTIONS_TABLE_COLUMN_SERVICE_NAME,
              record->service_name,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_ACTION,
              sm_service_action_str( record->action ) );

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
// Database Service Actions - Delete
// =================================
SmErrorT sm_db_service_actions_delete( SmDbHandleT* sm_db_handle, 
    char service_name[], SmServiceActionT action )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s WHERE %s = '%s' and "
              "%s = '%s';", SM_SERVICE_ACTIONS_TABLE_NAME,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_SERVICE_NAME, service_name,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_ACTION, 
              sm_service_action_str( action ) );

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
// Database Service Actions - Create Table
// =======================================
SmErrorT sm_db_service_actions_create_table( SmDbHandleT* sm_db_handle )
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
                  "%s INT,      "
                  "%s INT,      "
                  "%s INT,      "
                  "%s INT,      "
                  "%s INT,      "
                  "PRIMARY KEY (%s, %s));",
              SM_SERVICE_ACTIONS_TABLE_NAME,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_SERVICE_NAME,              
              SM_SERVICE_NAME_MAX_CHAR,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_ACTION,
              SM_SERVICE_ACTION_NAME_MAX_CHAR,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_TYPE,
              SM_SERVICE_ACTION_PLUGIN_TYPE_MAX_CHAR, 
              SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_CLASS,
              SM_SERVICE_ACTION_PLUGIN_CLASS_MAX_CHAR,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_NAME,
              SM_SERVICE_ACTION_PLUGIN_NAME_MAX_CHAR,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_COMMAND,
              SM_SERVICE_ACTION_PLUGIN_COMMAND_MAX_CHAR,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_PARAMETERS,
              SM_SERVICE_ACTION_PLUGIN_PARAMS_MAX_CHAR,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_FAILURE_RETRIES,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_TIMEOUT_RETRIES,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_TOTAL_RETRIES,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_TIMEOUT_IN_SECS,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_INTERVAL_IN_SECS,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_SERVICE_NAME,
              SM_SERVICE_ACTIONS_TABLE_COLUMN_ACTION );

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
// Database Service Actions - Delete Table
// =======================================
SmErrorT sm_db_service_actions_delete_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_ACTIONS_TABLE_NAME );

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
// Database Service Actions - Initialize
// =====================================
SmErrorT sm_db_service_actions_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Actions - Finalize
// ===================================
SmErrorT sm_db_service_actions_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
