
//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_heartbeat.h"

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
// Database Service Heartbeat - Convert
// ====================================
SmErrorT sm_db_service_heartbeat_convert( const char* col_name, 
    const char* col_data, void* data )
{
    SmDbServiceHeartbeatT* record = (SmDbServiceHeartbeatT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_ID, col_name ) )
    {
        record->id = atoll(col_data);
    }
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_PROVISIONED,
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
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_NAME, col_name ) )
    {
        snprintf( record->name, sizeof(record->name), "%s", 
                  col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_TYPE, col_name ) )
    {
        record->type = sm_service_heartbeat_type_value( col_data );
    }
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_SRC_ADDRESS,
                          col_name ) )
    {
        snprintf( record->src_address, sizeof(record->src_address), "%s", 
                  col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_SRC_PORT,
                          col_name ) )
    {
        record->src_port = col_data ? atoi(col_data) : 0;
    }

    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_DST_ADDRESS,
                          col_name ) )
    {
        snprintf( record->dst_address, sizeof(record->dst_address), "%s", 
                  col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_DST_PORT,
                          col_name ) )
    {
        record->dst_port = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MESSAGE, col_name ) )
    {
        snprintf( record->message, sizeof(record->message), "%s", 
                  col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_INTERVAL_IN_MS,
                          col_name ) )
    {
        record->interval_in_ms = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_WARN,
                          col_name ) )
    {
        record->missed_warn = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_DEGRADE,
                          col_name ) )
    {
        record->missed_degrade = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_FAIL,
                          col_name ) )
    {
        record->missed_fail = col_data ? atoi(col_data) : 0;
    } 
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_STATE,
                          col_name ) )
    {
        record->state = sm_service_heartbeat_state_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED,
                          col_name ) )
    {
        record->missed = col_data ? atoi(col_data) : 0;
    } 
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_TIMER_ID,
                          col_name ) )
    {
        record->heartbeat_timer_id = col_data ? atoi(col_data) : 0;
    } 
    else if( 0 == strcmp( SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_SOCKET,
                          col_name ) )
    {
        record->heartbeat_socket = col_data ? atoi(col_data) : 0;
    } 
    else 
    {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Read Callback
// ==========================================
static int sm_db_service_heartbeat_read_callback( void* user_data, int num_cols,
    char **col_data, char **col_name )
{
    SmDbServiceHeartbeatT* record = (SmDbServiceHeartbeatT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_heartbeat_convert( col_name[col_i], 
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
// Database Service Heartbeat - Read
// =================================
SmErrorT sm_db_service_heartbeat_read( SmDbHandleT* sm_db_handle, char name[],
    SmDbServiceHeartbeatT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceHeartbeatT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s';",
              SM_SERVICE_HEARTBEAT_TABLE_NAME,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_NAME,
              name );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_heartbeat_read_callback,
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
// Database Service Heartbeat - Read By Identifier
// ===============================================
SmErrorT sm_db_service_heartbeat_read_by_id( SmDbHandleT* sm_db_handle,
    int64_t id, SmDbServiceHeartbeatT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceHeartbeatT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%" PRIi64 "';",
              SM_SERVICE_HEARTBEAT_TABLE_NAME,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_ID, id );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_heartbeat_read_callback,
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
// Database Service Heartbeat - Query
// ==================================
SmErrorT sm_db_service_heartbeat_query( SmDbHandleT* sm_db_handle,
    const char* db_query, SmDbServiceHeartbeatT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceHeartbeatT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s;",
              SM_SERVICE_HEARTBEAT_TABLE_NAME, db_query );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_service_heartbeat_read_callback, record, &error );
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
// Database Service Heartbeat - Insert
// ===================================
SmErrorT sm_db_service_heartbeat_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceHeartbeatT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "INSERT INTO %s ( %s, "
                  "%s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s', '%i', '%s', '%i', '%s', "
                  "'%i', '%i', '%i', '%i', '%s', '%i', %i', '%i' );",
                  SM_SERVICE_HEARTBEAT_TABLE_NAME, 
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_PROVISIONED, 
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_NAME, 
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_TYPE,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_SRC_ADDRESS,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_SRC_PORT,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_DST_ADDRESS,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_DST_PORT,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MESSAGE,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_INTERVAL_IN_MS,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_WARN,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_DEGRADE,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_FAIL,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_STATE,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_TIMER_ID,
                  SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_SOCKET,
                  record->provisioned ? "yes" : "no",
                  record->name,
                  sm_service_heartbeat_type_str( record->type ),
                  record->src_address,
                  record->src_port,
                  record->dst_address,
                  record->dst_port,
                  record->message,
                  record->interval_in_ms,
                  record->missed_warn,
                  record->missed_degrade,
                  record->missed_fail,
                  sm_service_heartbeat_state_str( record->state ),
                  record->missed,
                  record->heartbeat_timer_id,
                  record->heartbeat_socket
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
// Database Service Heartbeat - Update
// ===================================
SmErrorT sm_db_service_heartbeat_update( SmDbHandleT* sm_db_handle, 
    SmDbServiceHeartbeatT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_HEARTBEAT_TABLE_NAME);

    if( SM_SERVICE_HEARTBEAT_TYPE_NIL != record->type )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_TYPE,
                         sm_service_heartbeat_type_str( record->type ) );
    }

    if( '\0' != record->src_address[0] )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_SRC_ADDRESS,
                         record->src_address );
    }

    if( 0 <= record->src_port )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_SRC_PORT,
                         record->src_port );
    }

    if( '\0' != record->dst_address[0] )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_DST_ADDRESS,
                         record->dst_address );
    }

    if( 0 <= record->dst_port )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_DST_PORT,
                         record->dst_port );
    }
    
    if( '\0' != record->message[0] )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MESSAGE,
                         record->message );
    }

    if( 0 <= record->interval_in_ms )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_INTERVAL_IN_MS,
                         record->interval_in_ms );
    }

    if( 0 <= record->missed_warn )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_WARN,
                         record->missed_warn );
    }

    if( 0 <= record->missed_degrade )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_DEGRADE,
                         record->missed_degrade );
    }

    if( 0 <= record->missed_fail )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_FAIL,
                         record->missed_fail );
    }

    if( SM_SERVICE_HEARTBEAT_STATE_NIL != record->state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_STATE,
                         sm_service_heartbeat_state_str(record->state) );
    }

    if( 0 <= record->missed )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED,
                         record->missed );
    }

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                     SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_TIMER_ID,
                     record->heartbeat_timer_id );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                     SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_SOCKET,
                     record->heartbeat_socket );

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s';",
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_NAME, record->name );

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
// Database Service Heartbeat - Delete
// ===================================
SmErrorT sm_db_service_heartbeat_delete( SmDbHandleT* sm_db_handle, char name[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s WHERE %s = '%s';",
              SM_SERVICE_HEARTBEAT_TABLE_NAME,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_NAME,
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
// Database Service Heartbeat - Create Table
// =========================================
SmErrorT sm_db_service_heartbeat_create_table( SmDbHandleT* sm_db_handle )
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
                  "%s CHAR(%i), "
                  "%s INT, "
                  "%s CHAR(%i), "
                  "%s INT, "
                  "%s INT, "
                  "%s INT, "
                  "%s INT, "
                  "%s CHAR(%i), "
                  "%s INT, "
                  "%s INT, "
                  "%s INT );",
              SM_SERVICE_HEARTBEAT_TABLE_NAME,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_ID,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_PROVISIONED,
              SM_SERVICE_HEARTBEAT_PROVISIONED_MAX_CHAR,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_NAME,
              SM_SERVICE_HEARTBEAT_NAME_MAX_CHAR,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_TYPE,
              SM_SERVICE_HEARTBEAT_TYPE_MAX_CHAR,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_SRC_ADDRESS,
              SM_SERVICE_HEARTBEAT_ADDRESS_MAX_CHAR,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_SRC_PORT,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_DST_ADDRESS,
              SM_SERVICE_HEARTBEAT_ADDRESS_MAX_CHAR,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_DST_PORT,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MESSAGE,
              SM_SERVICE_HEARTBEAT_MESSAGE_MAX_CHAR,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_INTERVAL_IN_MS,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_WARN,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_DEGRADE,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_FAIL,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_STATE,
              SM_SERVICE_HEARTBEAT_STATE_MAX_CHAR,              
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_TIMER_ID,
              SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_SOCKET );

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
// Database Service Heartbeat - Delete Table
// =========================================
SmErrorT sm_db_service_heartbeat_delete_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_HEARTBEAT_TABLE_NAME );

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
// Database Service Heartbeat - Cleanup Table
// ==========================================
SmErrorT sm_db_service_heartbeat_cleanup_table( SmDbHandleT* sm_db_handle )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_HEARTBEAT_TABLE_NAME );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                     SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED, 0 );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                     SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_TIMER_ID,
                     SM_TIMER_ID_INVALID );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                     SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_SOCKET, -1 );

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
// Database Service Heartbeat - Initialize
// =======================================
SmErrorT sm_db_service_heartbeat_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Finalize
// =====================================
SmErrorT sm_db_service_heartbeat_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
