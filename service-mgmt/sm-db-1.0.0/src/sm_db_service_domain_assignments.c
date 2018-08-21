//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_domain_assignments.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_utils.h"
#include "sm_db.h"

// ****************************************************************************
// Database Service Domain Assignments - Convert
// =============================================
SmErrorT sm_db_service_domain_assignments_convert( const char* col_name,
    const char* col_data, void* data )
{
    SmDbServiceDomainAssignmentT* record = (SmDbServiceDomainAssignmentT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_ID,
                      col_name ) )
    {
        record->id = atoll(col_data);
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_UUID,
                          col_name ) )
    {
        snprintf( record->uuid, sizeof(record->uuid), "%s",
                  col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NAME,
                          col_name ) )
    {
        snprintf( record->name, sizeof(record->name),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NODE_NAME,
                          col_name ) )
    {
        snprintf( record->node_name, sizeof(record->node_name),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_SERVICE_GROUP_NAME,
                          col_name ) )
    {
        snprintf( record->service_group_name, sizeof(record->service_group_name),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_DESIRED_STATE,
                          col_name ) )
    {
        record->desired_state = sm_service_group_state_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATE,
                          col_name ) )
    {
        record->state = sm_service_group_state_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATUS,
                          col_name ) )
    {
        record->status = sm_service_group_status_value( col_data );
    } 
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_CONDITION,
                          col_name ) )
    {
        record->condition = sm_service_group_condition_value( col_data );

    } else {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Read Callback
// ===================================================
static int sm_db_service_domain_assignments_read_callback( void* user_data,
    int num_cols, char **col_data, char **col_name )
{
    SmDbServiceDomainAssignmentT* record; 
    SmErrorT error;

    record = (SmDbServiceDomainAssignmentT*) user_data;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_domain_assignments_convert( col_name[col_i],
                                                          col_data[col_i],
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
// Database Service Domain Assignments - Read
// ==========================================
SmErrorT sm_db_service_domain_assignments_read( SmDbHandleT* sm_db_handle,
    char name[], char node_name[], char service_group_name[], 
    SmDbServiceDomainAssignmentT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainAssignmentT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s' "
              "and %s = '%s' and %s = '%s';",
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NAME, name,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NODE_NAME, node_name,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_SERVICE_GROUP_NAME,
              service_group_name );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_domain_assignments_read_callback,
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
// Database Service Domain Assignments - Read By Identifier
// ========================================================
SmErrorT  sm_db_service_domain_assignments_read_by_id(
    SmDbHandleT* sm_db_handle, int64_t id, 
    SmDbServiceDomainAssignmentT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainAssignmentT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%" PRIi64 "';",
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_ID, id );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_domain_assignments_read_callback,
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
// Database Service Domain Assignments - Query
// ===========================================
SmErrorT sm_db_service_domain_assignments_query( SmDbHandleT* sm_db_handle, 
    const char* db_query, SmDbServiceDomainAssignmentT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainAssignmentT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s;",
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME, db_query );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_service_domain_assignments_read_callback,
                       record, &error );
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
// Database Service Domain Assignments - Count Callback
// ====================================================
static int sm_db_service_domain_assignments_count_callback( void* user_data,
    int num_cols, char **col_data, char **col_name )
{
    int* count = (int*) user_data;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        if( 0 == strcmp( col_name[col_i], "record_count" ) )
        {
            *count = atoi( col_data[col_i] );
        }
    }

    return( SQLITE_OK );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Count
// ===========================================
SmErrorT sm_db_service_domain_assignments_count( SmDbHandleT* sm_db_handle, 
    const char* db_distinct_columns, const char* db_query, int* count )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    *count = 0;

    if( NULL == db_distinct_columns )
    {
        snprintf( sql, sizeof(sql), "SELECT COUNT(*) AS record_count FROM %s "
                  "WHERE %s;", SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME,
                  db_query );
    } else {
        snprintf( sql, sizeof(sql), "SELECT COUNT(DISTINCT %s) AS record_count "
                  "FROM %s WHERE %s;", db_distinct_columns,
                  SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME, db_query );
    }

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_service_domain_assignments_count_callback,
                       count, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to count, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Sum Callback
// ==================================================
static int sm_db_service_domain_assignments_sum_callback( void* user_data,
    int num_cols, char **col_data, char **col_name )
{
    int* sum = (int*) user_data;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        if( 0 == strcmp( col_name[col_i], "record_sum" ) )
        {
            *sum = atoi( col_data[col_i] );
        }
    }

    return( SQLITE_OK );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Sum
// =========================================
SmErrorT sm_db_service_domain_assignments_sum( SmDbHandleT* sm_db_handle, 
    const char* db_sum_column, const char* db_query, int* sum )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    *sum = 0;

    snprintf( sql, sizeof(sql), "SELECT SUM(%s) AS record_sum "
                  "FROM %s WHERE %s;", db_sum_column,
                  SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME, db_query );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_service_domain_assignments_sum_callback,
                       sum, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to sum, sql=%s, rc=%i, error=%s.", sql, rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Insert
// ============================================
SmErrorT sm_db_service_domain_assignments_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceDomainAssignmentT* record )
{
    SmUuidT uuid;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    sm_uuid_create( uuid );

    snprintf( sql, sizeof(sql), "INSERT INTO %s ( "
                  "%s, %s, %s, %s, %s, %s, %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s');",
                  SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME,
                  SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_UUID,
                  SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NAME,
                  SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NODE_NAME,
                  SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_SERVICE_GROUP_NAME,
                  SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_DESIRED_STATE,
                  SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATE,
                  SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATUS,
                  SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_CONDITION,
                  uuid,
                  record->name, record->node_name, record->service_group_name,
                  sm_service_group_state_str( record->desired_state ),
                  sm_service_group_state_str( record->state ),
                  sm_service_group_status_str( record->status ),
                  sm_service_group_condition_str( record->condition ) );

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
// Database Service Domain Assignments - Update
// ============================================
SmErrorT sm_db_service_domain_assignments_update( SmDbHandleT* sm_db_handle, 
    SmDbServiceDomainAssignmentT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME);

    if( SM_SERVICE_GROUP_STATE_NIL != record->desired_state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_DESIRED_STATE,
                         sm_service_group_state_str(record->desired_state) );
    }

    if( SM_SERVICE_GROUP_STATE_NIL != record->state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATE,
                         sm_service_group_state_str(record->state) );
    }

    if( SM_SERVICE_GROUP_STATUS_NIL != record->status )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATUS,
                         sm_service_group_status_str(record->status) );
    }

    if( SM_SERVICE_GROUP_CONDITION_NIL != record->condition )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_CONDITION,
                         sm_service_group_condition_str(record->condition) );
    }

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s' and %s = '%s' "
              "and %s = '%s';",
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NAME, record->name,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NODE_NAME,
              record->node_name,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_SERVICE_GROUP_NAME,
              record->service_group_name );

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
// Database Service Domain Assignments - Delete
// ============================================
SmErrorT sm_db_service_domain_assignments_delete( SmDbHandleT* sm_db_handle,
    char name[], char node_name[], char service_group_name[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s "
              "WHERE %s = '%s' and %s = '%s' and %s = '%s';",
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME, 
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NAME, name,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NODE_NAME, node_name,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_SERVICE_GROUP_NAME,
              service_group_name );

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
// Database Service Domain Assignments - Create Table
// ==================================================
SmErrorT sm_db_service_domain_assignments_create_table(
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
                  "%s CHAR(%i) );",
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_ID,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_UUID,
              SM_UUID_MAX_CHAR,              
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NAME,
              SM_SERVICE_DOMAIN_NAME_MAX_CHAR,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NODE_NAME,
              SM_NODE_NAME_MAX_CHAR,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_SERVICE_GROUP_NAME,
              SM_SERVICE_GROUP_NAME_MAX_CHAR,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_DESIRED_STATE,
              SM_SERVICE_GROUP_STATE_MAX_CHAR,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATE,
              SM_SERVICE_GROUP_STATE_MAX_CHAR,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATUS,
              SM_SERVICE_GROUP_STATUS_MAX_CHAR,
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_CONDITION,
              SM_SERVICE_GROUP_CONDITION_MAX_CHAR );

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
// Database Service Domain Assignments - Delete Table
// ==================================================
SmErrorT sm_db_service_domain_assignments_delete_table(
    SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME );

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
// Database Service Domain Assignments - Cleanup Table
// ===================================================
SmErrorT sm_db_service_domain_assignments_cleanup_table(
    SmDbHandleT* sm_db_handle )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    if( sm_utils_boot_complete() )
    {
        DPRINTFD( "Boot is complete, leave service domain assignments "
                  "alone." );
        return( SM_OKAY );
    }

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME);

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_DESIRED_STATE,
                     sm_service_group_state_str(SM_SERVICE_GROUP_STATE_DISABLED) );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATE,
                     sm_service_group_state_str(SM_SERVICE_GROUP_STATE_DISABLED) );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATUS,
                     sm_service_group_status_str(SM_SERVICE_GROUP_STATUS_NONE) );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_CONDITION,
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
// Database Service Domain Assignments - Initialize
// ================================================
SmErrorT sm_db_service_domain_assignments_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Finalize
// ==============================================
SmErrorT sm_db_service_domain_assignments_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
