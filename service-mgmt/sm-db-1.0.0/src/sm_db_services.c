//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_services.h"

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
// Database Services - Convert
// ===========================
SmErrorT sm_db_services_convert( const char* col_name, const char* col_data,
    void* data )
{
    SmDbServiceT* record = (SmDbServiceT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_SERVICES_TABLE_COLUMN_ID,col_name ) )
    {
        record->id = atoll(col_data);
    }
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_PROVISIONED,
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
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_NAME, col_name ) )
    {
        snprintf( record->name, sizeof(record->name),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_DESIRED_STATE, col_name ) )
    {
        record->desired_state = sm_service_state_value( col_data ); 
    }
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_STATE, col_name ) )
    {
        record->state = sm_service_state_value( col_data ); 
    }
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_STATUS, col_name ) )
    {
        record->status = sm_service_status_value( col_data ); 
    }
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_CONDITION, col_name ) )
    {
        record->condition = sm_service_condition_value( col_data ); 
    }
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_MAX_FAILURES, 
                          col_name ) )
    {
        record->max_failures = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_FAIL_COUNTDOWN, 
                          col_name ) )
    {
        record->fail_countdown = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_FAIL_COUNTDOWN_INTERVAL,
                          col_name ) )
    {
        record->fail_countdown_interval_in_ms = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_MAX_ACTION_FAILURES, 
                          col_name ) )
    {
        record->max_action_failures = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_MAX_TRANSITION_FAILURES, 
                          col_name ) )
    {
        record->max_transition_failures = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICES_TABLE_COLUMN_PID_FILE, col_name ) )
    {
        snprintf( record->pid_file, sizeof(record->pid_file),
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
// Database Services - Read Callback
// =================================
static int sm_db_services_read_callback( void* user_data, int num_cols,
    char **col_data, char **col_name )
{
    SmDbServiceT* record = (SmDbServiceT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_services_convert( col_name[col_i], col_data[col_i],
                                        record );
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
// Database Services - Read
// ========================
SmErrorT sm_db_services_read( SmDbHandleT* sm_db_handle, char name[],
    SmDbServiceT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s';",
              SM_SERVICES_TABLE_NAME, SM_SERVICES_TABLE_COLUMN_NAME, name );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_services_read_callback, record, &error );
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
// Database Services - Read By Identifier
// ======================================
SmErrorT sm_db_services_read_by_id( SmDbHandleT* sm_db_handle, int64_t id,
    SmDbServiceT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%" PRIi64 "';",
              SM_SERVICES_TABLE_NAME, SM_SERVICES_TABLE_COLUMN_ID, id );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_services_read_callback,
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
// Database Services - Query
// =========================
SmErrorT sm_db_services_query( SmDbHandleT* sm_db_handle, 
    const char* db_query, SmDbServiceT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s;",
              SM_SERVICES_TABLE_NAME, db_query );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_services_read_callback, record, &error );
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
// Database Services - Insert
// ==========================
SmErrorT sm_db_services_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "INSERT INTO %s ( "
                  "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', %i, %i, %i, "
                  " %i, %i, %s );",
                  SM_SERVICES_TABLE_NAME,
                  SM_SERVICES_TABLE_COLUMN_PROVISIONED,
                  SM_SERVICES_TABLE_COLUMN_NAME,
                  SM_SERVICES_TABLE_COLUMN_DESIRED_STATE,
                  SM_SERVICES_TABLE_COLUMN_STATE,
                  SM_SERVICES_TABLE_COLUMN_STATUS,
                  SM_SERVICES_TABLE_COLUMN_CONDITION,
                  SM_SERVICES_TABLE_COLUMN_MAX_FAILURES,
                  SM_SERVICES_TABLE_COLUMN_FAIL_COUNTDOWN,
                  SM_SERVICES_TABLE_COLUMN_FAIL_COUNTDOWN_INTERVAL,
                  SM_SERVICES_TABLE_COLUMN_MAX_ACTION_FAILURES,
                  SM_SERVICES_TABLE_COLUMN_MAX_TRANSITION_FAILURES,
                  SM_SERVICES_TABLE_COLUMN_PID_FILE,
                  record->provisioned ? "yes" : "no",
                  record->name,
                  sm_service_state_str( record->desired_state ),
                  sm_service_state_str( record->state ),
                  sm_service_status_str( record->status ),
                  sm_service_condition_str( record->condition ),
                  record->max_failures,
                  record->fail_countdown,
                  record->fail_countdown_interval_in_ms,
                  record->max_action_failures,
                  record->max_transition_failures,
                  record->pid_file );

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
// Database Services - Update
// ==========================
SmErrorT sm_db_services_update( SmDbHandleT* sm_db_handle,
    SmDbServiceT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICES_TABLE_NAME );

    if( SM_SERVICE_STATE_NIL != record->desired_state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICES_TABLE_COLUMN_DESIRED_STATE,
                         sm_service_state_str( record->desired_state ) );
    }

    if( SM_SERVICE_STATE_NIL != record->state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICES_TABLE_COLUMN_STATE,
                         sm_service_state_str( record->state ) );
    }

    if( SM_SERVICE_STATUS_NIL != record->status )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICES_TABLE_COLUMN_STATUS,
                         sm_service_status_str( record->status ) );
    }

    if( SM_SERVICE_CONDITION_NIL != record->condition )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICES_TABLE_COLUMN_CONDITION,
                         sm_service_condition_str( record->condition ) );
    }

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICES_TABLE_COLUMN_PROVISIONED,
                     record->provisioned ? "yes": "no" );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                     SM_SERVICES_TABLE_COLUMN_MAX_FAILURES,
                     record->max_failures );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                     SM_SERVICES_TABLE_COLUMN_FAIL_COUNTDOWN,
                     record->fail_countdown );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                     SM_SERVICES_TABLE_COLUMN_FAIL_COUNTDOWN_INTERVAL,
                     record->fail_countdown_interval_in_ms );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                     SM_SERVICES_TABLE_COLUMN_MAX_ACTION_FAILURES,
                     record->max_action_failures );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                     SM_SERVICES_TABLE_COLUMN_MAX_TRANSITION_FAILURES,
                     record->max_transition_failures );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICES_TABLE_COLUMN_PID_FILE,
                     record->pid_file );

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s';",
              SM_SERVICES_TABLE_COLUMN_NAME, record->name );

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
// Database Services - Delete
// ==========================
SmErrorT sm_db_services_delete( SmDbHandleT* sm_db_handle, char name[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s "
              "WHERE %s = '%s';", SM_SERVICES_TABLE_NAME,
              SM_SERVICES_TABLE_COLUMN_NAME, name );

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
// Database Services - Create Table
// ================================
SmErrorT sm_db_services_create_table( SmDbHandleT* sm_db_handle )
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
                  "%s INT, "
                  "%s INT, "
                  "%s INT, "
                  "%s INT, "
                  "%s INT, "
                  "%s CHAR(%i) );",
              SM_SERVICES_TABLE_NAME,
              SM_SERVICES_TABLE_COLUMN_ID,
              SM_SERVICES_TABLE_COLUMN_PROVISIONED,
              SM_SERVICE_PROVISIONED_MAX_CHAR,
              SM_SERVICES_TABLE_COLUMN_NAME,
              SM_SERVICE_NAME_MAX_CHAR,
              SM_SERVICES_TABLE_COLUMN_DESIRED_STATE,
              SM_SERVICE_STATE_MAX_CHAR,
              SM_SERVICES_TABLE_COLUMN_STATE,
              SM_SERVICE_STATE_MAX_CHAR,
              SM_SERVICES_TABLE_COLUMN_STATUS,
              SM_SERVICE_STATUS_MAX_CHAR,
              SM_SERVICES_TABLE_COLUMN_CONDITION,
              SM_SERVICE_CONDITION_MAX_CHAR,
              SM_SERVICES_TABLE_COLUMN_MAX_FAILURES,
              SM_SERVICES_TABLE_COLUMN_FAIL_COUNTDOWN,
              SM_SERVICES_TABLE_COLUMN_FAIL_COUNTDOWN_INTERVAL,
              SM_SERVICES_TABLE_COLUMN_MAX_ACTION_FAILURES,
              SM_SERVICES_TABLE_COLUMN_MAX_TRANSITION_FAILURES,
              SM_SERVICES_TABLE_COLUMN_PID_FILE,
              SM_SERVICE_PID_FILE_MAX_CHAR );

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
// Database Services - Delete Table
// ================================
SmErrorT sm_db_services_delete_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICES_TABLE_NAME );

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
// Database Services - Cleanup Table
// =================================
SmErrorT sm_db_services_cleanup_table( SmDbHandleT* sm_db_handle )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICES_TABLE_NAME );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICES_TABLE_COLUMN_STATE,
                     sm_service_state_str( SM_SERVICE_STATE_INITIAL ) );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICES_TABLE_COLUMN_STATUS,
                     sm_service_status_str( SM_SERVICE_STATUS_NONE ) );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICES_TABLE_COLUMN_CONDITION,
                     sm_service_condition_str( SM_SERVICE_CONDITION_NONE ) );

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
// Database Services - Initialize
// ==============================
SmErrorT sm_db_services_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Services - Finalize
// ============================
SmErrorT sm_db_services_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
