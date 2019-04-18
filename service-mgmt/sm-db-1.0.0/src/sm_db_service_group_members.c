//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_group_members.h"

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
// Database Service Group Members - Convert
// ========================================
SmErrorT sm_db_service_group_members_convert( const char* col_name,
    const char* col_data, void* data )
{
    SmDbServiceGroupMemberT* record = (SmDbServiceGroupMemberT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_ID,
                      col_name ) )
    {
        record->id = atoll(col_data);
    }
    else if( 0 == strcmp( SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_PROVISIONED,
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
    else if( 0 == strcmp( SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_NAME,
                          col_name ) )
    {
        snprintf( record->name, sizeof(record->name),
                  "%s", col_data ? col_data : "" );

    } else if( 0 == strcmp( SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_NAME,
                            col_name ) )
    {
        snprintf( record->service_name, sizeof(record->service_name),
                  "%s", col_data ? col_data : "" );

    } else if( 0 == strcmp( SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_FAILURE_IMPACT,
                            col_name ) )
    {
        record->service_failure_impact = sm_service_severity_value( col_data );

    } else {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Read Callback
// ==============================================
static int sm_db_service_group_members_read_callback( void* user_data,
    int num_cols, char **col_data, char **col_name )
{
    SmDbServiceGroupMemberT* record = (SmDbServiceGroupMemberT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_group_members_convert( col_name[col_i],
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
// Database Service Group Members - Read
// =====================================
SmErrorT sm_db_service_group_members_read( SmDbHandleT* sm_db_handle,
    char name[], char service_name[], SmDbServiceGroupMemberT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceGroupMemberT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s' "
              "and %s = '%s';",
              SM_SERVICE_GROUP_MEMBERS_TABLE_NAME,
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_NAME, name,
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_NAME,
              service_name );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_group_members_read_callback,
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
// Database Service Group Members - Query
// ======================================
SmErrorT sm_db_service_group_members_query( SmDbHandleT* sm_db_handle, 
    const char* db_query, SmDbServiceGroupMemberT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceGroupMemberT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s;",
              SM_SERVICE_GROUP_MEMBERS_TABLE_NAME, db_query );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_service_group_members_read_callback,
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
// Database Service Group Members - Insert
// =======================================
SmErrorT sm_db_service_group_members_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceGroupMemberT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "INSERT INTO %s ( "
                  "%s, %s, %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s');",
                  SM_SERVICE_GROUP_MEMBERS_TABLE_NAME,
                  SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_PROVISIONED,
                  SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_NAME,
                  SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_NAME,
                  SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_FAILURE_IMPACT,
                  record->provisioned ? "yes" : "no",
                  record->name, record->service_name,
                  sm_service_severity_str( record->service_failure_impact ) );

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
// Database Service Group Members - Update
// =======================================
SmErrorT sm_db_service_group_members_update( SmDbHandleT* sm_db_handle, 
    SmDbServiceGroupMemberT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_GROUP_MEMBERS_TABLE_NAME );

    if( SM_SERVICE_SEVERITY_NIL != record->service_failure_impact )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_FAILURE_IMPACT,
                         sm_service_severity_str( record->service_failure_impact ) );
    }

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_PROVISIONED,
                     record->provisioned ? "yes": "no" );

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s' and %s = '%s';",
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_NAME, record->name,
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_NAME,
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
// Database Service Group Members - Delete
// =======================================
SmErrorT sm_db_service_group_members_delete( SmDbHandleT* sm_db_handle,
    char name[], char service_name[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s "
              "WHERE %s = '%s' and %s = '%s';",
              SM_SERVICE_GROUP_MEMBERS_TABLE_NAME, 
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_NAME, name,
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_NAME,
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
// Database Service Group Members - Create Table
// =============================================
SmErrorT sm_db_service_group_members_create_table( SmDbHandleT* sm_db_handle )
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
                  "%s CHAR(%i));",
              SM_SERVICE_GROUP_MEMBERS_TABLE_NAME,
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_ID,
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_PROVISIONED,
              SM_SERVICE_GROUP_MEMBER_PROVISIONED_MAX_CHAR,
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_NAME,
              SM_SERVICE_GROUP_NAME_MAX_CHAR,
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_NAME,
              SM_SERVICE_NAME_MAX_CHAR,
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_FAILURE_IMPACT,
              SM_SERVICE_SEVERITY_MAX_CHAR );

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
// Database Service Group Members - Delete Table
// =============================================
SmErrorT sm_db_service_group_members_delete_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_GROUP_MEMBERS_TABLE_NAME );

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
// Database Service Group Members - Cleanup Table
// ==============================================
SmErrorT sm_db_service_group_members_cleanup_table( 
    SmDbHandleT* sm_db_handle )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_GROUP_MEMBERS_TABLE_NAME );

    // Nothing at the momement.

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
// Database Service Group Members - Initialize
// ===========================================
SmErrorT sm_db_service_group_members_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Finalize
// =========================================
SmErrorT sm_db_service_group_members_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
