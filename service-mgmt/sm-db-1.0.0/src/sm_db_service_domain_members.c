//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_domain_members.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db.h"

// ****************************************************************************
// Database Service Domain Members - Convert
// =========================================
SmErrorT sm_db_service_domain_members_convert( const char* col_name,
    const char* col_data, void* data )
{
    SmDbServiceDomainMemberT* record = (SmDbServiceDomainMemberT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_ID,
                      col_name ) )
    {
        record->id = atoll(col_data);
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_PROVISIONED,
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
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_NAME, col_name ) )
    {
        snprintf( record->name, sizeof(record->name),
                  "%s", col_data ? col_data : "" );

    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_NAME,
                          col_name ) )
    {
        snprintf( record->service_group_name, sizeof(record->service_group_name),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_REDUNDANCY_MODEL,
                          col_name ) )
    {
        record->redundancy_model 
            = sm_service_domain_member_redundancy_model_value( col_data );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_N_ACTIVE,
                          col_name ) )
    {
        record->n_active = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_M_STANDBY,
                          col_name ) )
    {
        record->m_standby = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_AGGREGATE,
                          col_name ) )
    {
        snprintf( record->service_group_aggregate,
                  sizeof(record->service_group_aggregate),
                  "%s", col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_ACTIVE_ONLY_IF_ACTIVE,
                          col_name ) )
    {
        snprintf( record->active_only_if_active,
                  sizeof(record->active_only_if_active),
                 "%s", col_data ? col_data : "" );

    } else {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Read Callback
// ===============================================
static int sm_db_service_domain_members_read_callback( void* user_data,
    int num_cols, char **col_data, char **col_name )
{
    SmDbServiceDomainMemberT* record = (SmDbServiceDomainMemberT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_domain_members_convert( col_name[col_i],
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
// Database Service Domain Members - Read
// ======================================
SmErrorT sm_db_service_domain_members_read( SmDbHandleT* sm_db_handle,
    char name[], char service_group_name[], SmDbServiceDomainMemberT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainMemberT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s' "
              "and %s = '%s';",
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_NAME,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_NAME, name,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_NAME,
              service_group_name );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_domain_members_read_callback,
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
// Database Service Domain Members - Query
// =======================================
SmErrorT sm_db_service_domain_members_query( SmDbHandleT* sm_db_handle, 
    const char* db_query, SmDbServiceDomainMemberT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainMemberT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s;",
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_NAME, db_query );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_service_domain_members_read_callback,
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
// Database Service Domain Members - Count Callback
// ================================================
static int sm_db_service_domain_members_count_callback( void* user_data,
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
// Database Service Domain Members - Count
// =======================================
SmErrorT sm_db_service_domain_members_count( SmDbHandleT* sm_db_handle, 
    const char* db_distinct_columns, const char* db_query, int* count )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    *count = 0;

    if( NULL == db_distinct_columns )
    {
        snprintf( sql, sizeof(sql), "SELECT COUNT(*) AS record_count FROM %s "
                  "WHERE %s;", SM_SERVICE_DOMAIN_MEMBERS_TABLE_NAME,
                  db_query );
    } else {
        snprintf( sql, sizeof(sql), "SELECT COUNT(DISTINCT %s) AS record_count "
                  "FROM %s WHERE %s;", db_distinct_columns,
                  SM_SERVICE_DOMAIN_MEMBERS_TABLE_NAME, db_query );
    }

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_service_domain_members_count_callback,
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
// Database Service Domain Members - Insert
// ========================================
SmErrorT sm_db_service_domain_members_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceDomainMemberT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "INSERT INTO %s ( "
              "%s, %s, %s, %s, %s, %s, %s, %s ) "
              "VALUES ('%s', '%s', '%s', '%s', '%i', '%i', '%s', '%s');",
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_NAME,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_PROVISIONED,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_NAME,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_NAME,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_REDUNDANCY_MODEL,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_N_ACTIVE,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_M_STANDBY,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_AGGREGATE,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_ACTIVE_ONLY_IF_ACTIVE,
              record->provisioned ? "yes" : "no",
              record->name, record->service_group_name,
              sm_service_domain_member_redundancy_model_str(record->redundancy_model),
              record->n_active, record->m_standby,
              record->service_group_aggregate, record->active_only_if_active );

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
// Database Service Domain Members - Update
// ========================================
SmErrorT sm_db_service_domain_members_update( SmDbHandleT* sm_db_handle, 
    SmDbServiceDomainMemberT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_DOMAIN_MEMBERS_TABLE_NAME );

    if( SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_NIL != record->redundancy_model )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_REDUNDANCY_MODEL,
                         sm_service_domain_member_redundancy_model_str(
                            record->redundancy_model) );
    }

    if( 0 < record->n_active )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_N_ACTIVE,
                         record->n_active );
    }

    if( 0 < record->m_standby )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_M_STANDBY,
                         record->m_standby );
    }

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_AGGREGATE,
                     record->service_group_aggregate );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_ACTIVE_ONLY_IF_ACTIVE,
                     record->active_only_if_active );

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s' and %s = '%s';",
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_NAME, record->name,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_NAME,
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
// Database Service Domain Members - Delete
// ========================================
SmErrorT sm_db_service_domain_members_delete( SmDbHandleT* sm_db_handle,
    char name[], char service_group_name[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s "
              "WHERE %s = '%s' and %s = '%s';",
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_NAME, 
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_NAME, name,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_NAME,
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
// Database Service Domain Members - Create Table
// ==============================================
SmErrorT sm_db_service_domain_members_create_table( SmDbHandleT* sm_db_handle )
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
                  "%s INT, "
                  "%s CHAR(%i), "
                  "%s CHAR(%i) );",
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_NAME,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_ID,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_PROVISIONED,
              SM_SERVICE_DOMAIN_MEMBER_PROVISIONED_MAX_CHAR,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_NAME,
              SM_SERVICE_DOMAIN_NAME_MAX_CHAR,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_NAME,
              SM_SERVICE_GROUP_NAME_MAX_CHAR,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_REDUNDANCY_MODEL,
              SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_MAX_CHAR,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_N_ACTIVE,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_M_STANDBY,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_AGGREGATE,
              SM_SERVICE_GROUP_AGGREGATE_NAME_MAX_CHAR,
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_ACTIVE_ONLY_IF_ACTIVE,
              SM_SERVICE_GROUP_NAME_MAX_CHAR );

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
// Database Service Domain Members - Delete Table
// ==============================================
SmErrorT sm_db_service_domain_members_delete_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_NAME );

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
// Database Service Domain Members - Initialize
// ============================================
SmErrorT sm_db_service_domain_members_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Finalize
// ==========================================
SmErrorT sm_db_service_domain_members_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
