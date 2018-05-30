//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_groups.h"

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
// Database Service Groups - Convert
// =================================
SmErrorT sm_db_service_groups_convert( const char* col_name, 
    const char* col_data, void* data )
{
    SmDbServiceGroupT* record = (SmDbServiceGroupT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_SERVICE_GROUPS_TABLE_COLUMN_ID,col_name ) )
    {
        record->id = atoll(col_data);
    }
    else if( 0 == strcmp( SM_SERVICE_GROUPS_TABLE_COLUMN_PROVISIONED,
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
    else if( 0 == strcmp( SM_SERVICE_GROUPS_TABLE_COLUMN_NAME, col_name ) )
    {
        snprintf( record->name, sizeof(record->name), "%s", 
                  col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_GROUPS_TABLE_COLUMN_AUTO_RECOVER,
                          col_name ) )
    {
        record->auto_recover = false;

        if( col_data )
        {
            if(( 0 == strcmp( "yes", col_data ))||
               ( 0 == strcmp( "Yes", col_data ))||
               ( 0 == strcmp( "YES", col_data )))
            {
                record->auto_recover = true;
            }
        }
    }
    else if( 0 == strcmp( SM_SERVICE_GROUPS_TABLE_COLUMN_CORE,
                          col_name ) )
    {
        record->core = false;

        if( col_data )
        {
            if(( 0 == strcmp( "yes", col_data ))||
               ( 0 == strcmp( "Yes", col_data ))||
               ( 0 == strcmp( "YES", col_data )))
            {
                record->core = true;
            }
        }
    }
    else if( 0 == strcmp( SM_SERVICE_GROUPS_TABLE_COLUMN_DESIRED_STATE,
                          col_name ) )
    {
        record->desired_state = sm_service_group_state_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_GROUPS_TABLE_COLUMN_STATE, col_name ) )
    {
        record->state = sm_service_group_state_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_GROUPS_TABLE_COLUMN_STATUS, col_name ) )
    {
        record->status = sm_service_group_status_value( col_data );
    }
    else if( 0 == strcmp( SM_SERVICE_GROUPS_TABLE_COLUMN_CONDITION, col_name ) )
    {
        record->condition = sm_service_group_condition_value( col_data );
    }
    else if( 0 == strcmp( SM_SERVICE_GROUPS_TABLE_COLUMN_FAILURE_DEBOUNCE,
                          col_name ) )
    {
        record->failure_debounce_in_ms = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_GROUPS_TABLE_COLUMN_FATAL_ERROR_REBOOT,
                          col_name ) )
    {
        record->fatal_error_reboot = false;

        if( col_data )
        {
            if(( 0 == strcmp( "yes", col_data ))||
               ( 0 == strcmp( "Yes", col_data ))||
               ( 0 == strcmp( "YES", col_data )))
            {
                record->fatal_error_reboot = true;
            }
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
// Database Service Groups - Read Callback
// =======================================
static int sm_db_service_groups_read_callback( void* user_data, int num_cols,
    char **col_data, char **col_name )
{
    SmDbServiceGroupT* record = (SmDbServiceGroupT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_groups_convert( col_name[col_i], col_data[col_i],
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
// Database Service Groups - Read
// ==============================
SmErrorT sm_db_service_groups_read( SmDbHandleT* sm_db_handle, char name[],
    SmDbServiceGroupT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceGroupT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s';",
              SM_SERVICE_GROUPS_TABLE_NAME,
              SM_SERVICE_GROUPS_TABLE_COLUMN_NAME,
              name );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_groups_read_callback,
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
// Database Service Groups - Insert
// ================================
SmErrorT sm_db_service_groups_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceGroupT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "INSERT INTO %s ( %s, %s, %s, %s, %s, %s, %s, "
                  "%s, %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', "
                  "'%i', '%s');",
                  SM_SERVICE_GROUPS_TABLE_NAME, 
                  SM_SERVICE_GROUPS_TABLE_COLUMN_PROVISIONED,
                  SM_SERVICE_GROUPS_TABLE_COLUMN_NAME,
                  SM_SERVICE_GROUPS_TABLE_COLUMN_AUTO_RECOVER,
                  SM_SERVICE_GROUPS_TABLE_COLUMN_CORE,
                  SM_SERVICE_GROUPS_TABLE_COLUMN_DESIRED_STATE,
                  SM_SERVICE_GROUPS_TABLE_COLUMN_STATE,
                  SM_SERVICE_GROUPS_TABLE_COLUMN_STATUS,
                  SM_SERVICE_GROUPS_TABLE_COLUMN_CONDITION,
                  SM_SERVICE_GROUPS_TABLE_COLUMN_FAILURE_DEBOUNCE,
                  SM_SERVICE_GROUPS_TABLE_COLUMN_FATAL_ERROR_REBOOT,
                  record->provisioned ? "yes" : "no",
                  record->name,
                  record->auto_recover ? "yes" : "no",
                  record->core ? "yes" : "no",
                  sm_service_group_state_str( record->desired_state ),
                  sm_service_group_state_str( record->state ),
                  sm_service_group_status_str( record->status ),
                  sm_service_group_condition_str( record->condition ),
                  record->failure_debounce_in_ms,
                  record->fatal_error_reboot ? "yes" : "no"
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
// Database Service Groups - Update
// ================================
SmErrorT sm_db_service_groups_update( SmDbHandleT* sm_db_handle, 
    SmDbServiceGroupT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_GROUPS_TABLE_NAME);

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_GROUPS_TABLE_COLUMN_AUTO_RECOVER,
                     record->auto_recover ? "yes" : "no" );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_GROUPS_TABLE_COLUMN_CORE,
                     record->core ? "yes" : "no" );

    if( SM_SERVICE_GROUP_STATE_NIL != record->desired_state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_GROUPS_TABLE_COLUMN_DESIRED_STATE,
                         sm_service_group_state_str(record->desired_state) );
    }

    if( SM_SERVICE_GROUP_STATE_NIL != record->state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_GROUPS_TABLE_COLUMN_STATE,
                         sm_service_group_state_str(record->state) );
    }

    if( SM_SERVICE_GROUP_STATUS_NIL != record->status )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_GROUPS_TABLE_COLUMN_STATUS,
                         sm_service_group_status_str(record->status) );
    }

    if( SM_SERVICE_GROUP_CONDITION_NIL != record->condition )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_GROUPS_TABLE_COLUMN_CONDITION,
                         sm_service_group_condition_str(record->condition) );
    }

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                     SM_SERVICE_GROUPS_TABLE_COLUMN_FAILURE_DEBOUNCE,
                     record->failure_debounce_in_ms );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_GROUPS_TABLE_COLUMN_FATAL_ERROR_REBOOT,
                     record->fatal_error_reboot ? "yes" : "no" );

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s';",
              SM_SERVICE_GROUPS_TABLE_COLUMN_NAME, record->name );

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
// Database Service Groups - Delete
// ================================
SmErrorT sm_db_service_groups_delete( SmDbHandleT* sm_db_handle, char name[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s WHERE %s = '%s';",
              SM_SERVICE_GROUPS_TABLE_NAME,
              SM_SERVICE_GROUPS_TABLE_COLUMN_NAME,
              name );

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
// Database Service Groups - Create Table
// ======================================
SmErrorT sm_db_service_groups_create_table( SmDbHandleT* sm_db_handle )
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
                  "%s INT, "
                  "%s CHAR(%i) );",
              SM_SERVICE_GROUPS_TABLE_NAME,
              SM_SERVICE_GROUPS_TABLE_COLUMN_ID,
              SM_SERVICE_GROUPS_TABLE_COLUMN_PROVISIONED,
              SM_SERVICE_GROUP_PROVISIONED_MAX_CHAR,
              SM_SERVICE_GROUPS_TABLE_COLUMN_NAME,
              SM_SERVICE_GROUP_NAME_MAX_CHAR, 
              SM_SERVICE_GROUPS_TABLE_COLUMN_AUTO_RECOVER,
              SM_SERVICE_GROUP_AUTO_RECOVER_MAX_CHAR,
              SM_SERVICE_GROUPS_TABLE_COLUMN_CORE,
              SM_SERVICE_GROUP_CORE_MAX_CHAR,
              SM_SERVICE_GROUPS_TABLE_COLUMN_DESIRED_STATE,
              SM_SERVICE_GROUP_STATE_MAX_CHAR,
              SM_SERVICE_GROUPS_TABLE_COLUMN_STATE,
              SM_SERVICE_GROUP_STATE_MAX_CHAR,
              SM_SERVICE_GROUPS_TABLE_COLUMN_STATUS,
              SM_SERVICE_GROUP_STATUS_MAX_CHAR,
              SM_SERVICE_GROUPS_TABLE_COLUMN_CONDITION,
              SM_SERVICE_GROUP_CONDITION_MAX_CHAR,
              SM_SERVICE_GROUPS_TABLE_COLUMN_FAILURE_DEBOUNCE,
              SM_SERVICE_GROUPS_TABLE_COLUMN_FATAL_ERROR_REBOOT,
              SM_SERVICE_GROUP_FATAL_ERROR_REBOOT_MAX_CHAR );

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
// Database Service Groups - Delete Table
// ======================================
SmErrorT sm_db_service_groups_delete_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_GROUPS_TABLE_NAME );

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
// Database Service Groups - Cleanup Table
// =======================================
SmErrorT sm_db_service_groups_cleanup_table( SmDbHandleT* sm_db_handle )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_GROUPS_TABLE_NAME);

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_GROUPS_TABLE_COLUMN_STATE,
                     sm_service_group_state_str(SM_SERVICE_GROUP_STATE_INITIAL) );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_GROUPS_TABLE_COLUMN_STATUS,
                     sm_service_group_status_str(SM_SERVICE_GROUP_STATUS_NONE) );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_GROUPS_TABLE_COLUMN_CONDITION,
                     sm_service_group_condition_str(SM_SERVICE_GROUP_CONDITION_NONE) );

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
// Database Service Groups - Initialize
// ====================================
SmErrorT sm_db_service_groups_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Groups - Finalize
// ==================================
SmErrorT sm_db_service_groups_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
