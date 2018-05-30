//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_nodes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_uuid.h"
#include "sm_db.h"
#include "sm_node_utils.h"

// ****************************************************************************
// Database Nodes - Convert
// ========================
SmErrorT sm_db_nodes_convert( const char* col_name, const char* col_data,
    void* data )
{
    SmDbNodeT* record = (SmDbNodeT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_NODES_TABLE_COLUMN_ID, col_name ) )
    {
        record->id = atoll(col_data);
    }
    else if( 0 == strcmp( SM_NODES_TABLE_COLUMN_NAME, col_name ) )
    {
        snprintf( record->name, sizeof(record->name), "%s", 
                  col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_NODES_TABLE_COLUMN_ADMIN_STATE, col_name ) )
    {
        record->admin_state = sm_node_admin_state_value( col_data );
    } 
    else if( 0 == strcmp( SM_NODES_TABLE_COLUMN_OPER_STATE, col_name ) )
    {
        record->oper_state = sm_node_oper_state_value( col_data );
    }
    else if( 0 == strcmp( SM_NODES_TABLE_COLUMN_AVAIL_STATUS, col_name ) )
    {
        record->avail_status = sm_node_avail_status_value( col_data );
    } 
    else if( 0 == strcmp( SM_NODES_TABLE_COLUMN_READY_STATE, col_name ) )
    {
        record->ready_state = sm_node_ready_state_value( col_data );
    }
    else if( 0 == strcmp( SM_NODES_TABLE_COLUMN_STATE_UUID, col_name ) )
    {
        snprintf( record->state_uuid, sizeof(record->state_uuid), "%s",
                  col_data ? col_data : "" );
    }
    else 
    {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Read Callback
// ==============================
static int sm_db_nodes_read_callback( void* user_data, int num_cols,
    char **col_data, char **col_name )
{
    SmDbNodeT* record = (SmDbNodeT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_nodes_convert( col_name[col_i], col_data[col_i],
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
// Database Nodes - Read
// =====================
SmErrorT sm_db_nodes_read( SmDbHandleT* sm_db_handle, char name[],
    SmDbNodeT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbNodeT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s';",
              SM_NODES_TABLE_NAME,
              SM_NODES_TABLE_COLUMN_NAME,
              name );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_nodes_read_callback, record, &error );
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
// Database Nodes - Query
// ======================
SmErrorT sm_db_nodes_query( SmDbHandleT* sm_db_handle, const char* db_query,
    SmDbNodeT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbNodeT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s;",
              SM_NODES_TABLE_NAME, db_query );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_nodes_read_callback, record, &error );
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
// Database Nodes - Insert
// =======================
SmErrorT sm_db_nodes_insert( SmDbHandleT* sm_db_handle, SmDbNodeT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "INSERT INTO %s ( %s, %s, %s, %s, %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s', '%s', '%s');",
                  SM_NODES_TABLE_NAME,
                  SM_NODES_TABLE_COLUMN_NAME,
                  SM_NODES_TABLE_COLUMN_ADMIN_STATE,
                  SM_NODES_TABLE_COLUMN_OPER_STATE,
                  SM_NODES_TABLE_COLUMN_AVAIL_STATUS,
                  SM_NODES_TABLE_COLUMN_READY_STATE,
                  SM_NODES_TABLE_COLUMN_STATE_UUID,
                  record->name,
                  sm_node_admin_state_str( record->admin_state ),
                  sm_node_oper_state_str( record->oper_state ),
                  sm_node_avail_status_str( record->avail_status ),
                  sm_node_ready_state_str( record->ready_state ),
                  record->state_uuid
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
// Database Nodes - Update
// =======================
SmErrorT sm_db_nodes_update( SmDbHandleT* sm_db_handle, SmDbNodeT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_NODES_TABLE_NAME);

    if( SM_NODE_ADMIN_STATE_NIL != record->admin_state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_NODES_TABLE_COLUMN_ADMIN_STATE,
                         sm_node_admin_state_str(record->admin_state) );
    }

    if( SM_NODE_OPERATIONAL_STATE_NIL != record->oper_state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_NODES_TABLE_COLUMN_OPER_STATE,
                         sm_node_oper_state_str(record->oper_state) );
    }

    if( SM_NODE_AVAIL_STATUS_NIL != record->avail_status )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_NODES_TABLE_COLUMN_AVAIL_STATUS,
                         sm_node_avail_status_str(record->avail_status) );
    }

    if( SM_NODE_READY_STATE_NIL != record->ready_state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_NODES_TABLE_COLUMN_READY_STATE,
                         sm_node_ready_state_str(record->ready_state) );
    }

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_NODES_TABLE_COLUMN_STATE_UUID,
                     record->state_uuid );

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s';",
              SM_NODES_TABLE_COLUMN_NAME, record->name );

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
// Database Nodes - Delete
// =======================
SmErrorT sm_db_nodes_delete( SmDbHandleT* sm_db_handle, char name[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s WHERE %s = '%s';",
              SM_NODES_TABLE_NAME,
              SM_NODES_TABLE_COLUMN_NAME,
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
// Database Nodes - Create Table
// =============================
SmErrorT sm_db_nodes_create_table( SmDbHandleT* sm_db_handle )
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
                  "%s CHAR(%i));",
              SM_NODES_TABLE_NAME,
              SM_NODES_TABLE_COLUMN_ID,
              SM_NODES_TABLE_COLUMN_NAME,
              SM_NODE_NAME_MAX_CHAR, 
              SM_NODES_TABLE_COLUMN_ADMIN_STATE,
              SM_NODE_ADMIN_STATE_MAX_CHAR,
              SM_NODES_TABLE_COLUMN_OPER_STATE,
              SM_NODE_OPERATIONAL_STATE_MAX_CHAR,
              SM_NODES_TABLE_COLUMN_AVAIL_STATUS,
              SM_NODE_AVAIL_STATUS_MAX_CHAR,
              SM_NODES_TABLE_COLUMN_READY_STATE,
              SM_NODE_READY_STATE_MAX_CHAR,
              SM_NODES_TABLE_COLUMN_STATE_UUID,
              SM_UUID_MAX_CHAR );

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
// Database Nodes - Delete Table
// =============================
SmErrorT sm_db_nodes_delete_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_NODES_TABLE_NAME );

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
// Database Nodes - Cleanup Table
// ==============================
SmErrorT sm_db_nodes_cleanup_table( SmDbHandleT* sm_db_handle )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    char hostname[SM_NODE_NAME_MAX_CHAR];
    int rc;
    SmErrorT sm_error;

    sm_error = sm_node_utils_get_hostname( hostname );
    if( SM_OKAY != sm_error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( sm_error ) );
        return( sm_error );
    }

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_NODES_TABLE_NAME);

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_NODES_TABLE_COLUMN_READY_STATE,
                     sm_node_ready_state_str(SM_NODE_READY_STATE_DISABLED) );

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s';",
              SM_NODES_TABLE_COLUMN_NAME, hostname );

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
// Database Nodes - Initialize
// ===========================
SmErrorT sm_db_nodes_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Finalize
// =========================
SmErrorT sm_db_nodes_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
