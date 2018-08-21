//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_service_domains.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_timer.h"
#include "sm_debug.h"
#include "sm_db.h"

// ****************************************************************************
// Database Service Domains - Convert
// ==================================
SmErrorT sm_db_service_domains_convert( const char* col_name,
    const char* col_data, void* data )
{
    SmDbServiceDomainT* record = (SmDbServiceDomainT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_ID,col_name ) )
    {
        record->id = atoll(col_data);
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_PROVISIONED,
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
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_NAME, col_name ) )
    {
        snprintf( record->name, sizeof(record->name), "%s", 
                  col_data ? col_data : "" );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_ORCHESTRATION,
                          col_name ) )
    {
        record->orchestration = sm_orchestration_type_value( col_data );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_DESIGNATION,
                          col_name ) )
    {
        record->designation = sm_designation_type_value( col_data );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_PREEMPT, 
                          col_name ) )
    {
        record->preempt = false;

        if( col_data )
        {
            if(( 0 == strcmp( "yes", col_data ))||
               ( 0 == strcmp( "Yes", col_data ))||
               ( 0 == strcmp( "YES", col_data )))
            {
                record->preempt = true;
            }
        }
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_GENERATION, 
                          col_name ) )
    {
        record->generation = col_data ? atoi(col_data) : 0;
    }    
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_PRIORITY, 
                          col_name ) )
    {
        record->priority = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_HELLO_INTERVAL, 
                          col_name ) )
    {
        record->hello_interval = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_DEAD_INTERVAL, 
                          col_name ) )
    {
        record->dead_interval = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_WAIT_INTERVAL, 
                          col_name ) )
    {
        record->wait_interval = col_data ? atoi(col_data) : 0;
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_EXCHANGE_INTERVAL,
                          col_name ) )
    {
        record->exchange_interval = col_data ? atoi(col_data) : 0;
    }    
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_STATE, 
                          col_name ) )
    {
        record->state = sm_service_domain_state_value( col_data );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_SPLIT_BRAIN_RECOVERY,
                          col_name ) )
    {
        record->split_brain_recovery
            = sm_service_domain_split_brain_recovery_value( col_data );
    }
    else if( 0 == strcmp( SM_SERVICE_DOMAINS_TABLE_COLUMN_LEADER,
                          col_name ) )
    {
        snprintf( record->leader, sizeof(record->leader), "%s",
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
// Database Service Domains - Read Callback
// ========================================
static int sm_db_service_domains_read_callback( void* user_data, int num_cols,
    char **col_data, char **col_name )
{
    SmDbServiceDomainT* record = (SmDbServiceDomainT*) user_data;
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        error = sm_db_service_domains_convert( col_name[col_i], col_data[col_i],
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
// Database Service Domains - Read
// ===============================
SmErrorT sm_db_service_domains_read( SmDbHandleT* sm_db_handle, char name[],
    SmDbServiceDomainT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%s';",
              SM_SERVICE_DOMAINS_TABLE_NAME,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_NAME, name );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_domains_read_callback,
                       record, &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to read, sql=%s, rc=%i, error=%s.", sql, rc, error );
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
// Database Service Domains - Read By Identifier
// =============================================
SmErrorT sm_db_service_domains_read_by_id( SmDbHandleT* sm_db_handle,
    int64_t id, SmDbServiceDomainT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s = '%" PRIi64 "';",
              SM_SERVICE_DOMAINS_TABLE_NAME,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_ID, id );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql,
                       sm_db_service_domains_read_callback,
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
// Database Service Domains - Query
// ================================
SmErrorT sm_db_service_domains_query( SmDbHandleT* sm_db_handle, 
    const char* db_query, SmDbServiceDomainT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    memset( record, 0, sizeof(SmDbServiceDomainT) );

    snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s;",
              SM_SERVICE_DOMAINS_TABLE_NAME, db_query );

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, sql, 
                       sm_db_service_domains_read_callback, record, &error );
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
// Database Service Domains - Insert
// =================================
SmErrorT sm_db_service_domains_insert( SmDbHandleT* sm_db_handle, 
    SmDbServiceDomainT* record )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "INSERT INTO %s "
                  "( %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s, %s ) "
                  "VALUES ('%s', '%s', '%s', '%s', '%s', '%i', '%i', '%i', "
                  "'%i', '%i', '%i', '%s', '%s', '%s');",
                  SM_SERVICE_DOMAINS_TABLE_NAME, 
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_PROVISIONED,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_NAME,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_ORCHESTRATION,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_DESIGNATION,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_PREEMPT,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_GENERATION,                  
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_PRIORITY,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_HELLO_INTERVAL,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_DEAD_INTERVAL,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_WAIT_INTERVAL,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_EXCHANGE_INTERVAL,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_STATE,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_SPLIT_BRAIN_RECOVERY,
                  SM_SERVICE_DOMAINS_TABLE_COLUMN_LEADER,
                  record->provisioned ? "yes" : "no",
                  record->name,
                  sm_orchestration_type_str(record->orchestration),
                  sm_designation_type_str(record->designation),
                  record->preempt ? "yes" : "no",
                  record->generation,
                  record->priority,
                  record->hello_interval,
                  record->dead_interval,
                  record->wait_interval,
                  record->exchange_interval,
                  sm_service_domain_state_str( record->state ),
                  sm_service_domain_split_brain_recovery_str( 
                                        record->split_brain_recovery ),
                  record->leader
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
// Database Service Domains - Update
// =================================
SmErrorT sm_db_service_domains_update( SmDbHandleT* sm_db_handle,
    SmDbServiceDomainT* record )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_DOMAINS_TABLE_NAME);

    if( SM_ORCHESTRATION_TYPE_NIL != record->orchestration )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAINS_TABLE_COLUMN_ORCHESTRATION,
                         sm_orchestration_type_str(record->orchestration) );
    }

    if( SM_DESIGNATION_TYPE_NIL != record->designation )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAINS_TABLE_COLUMN_DESIGNATION,
                         sm_designation_type_str(record->designation) );
    }

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAINS_TABLE_COLUMN_PREEMPT,
                     record->preempt ? "yes" : "no" );

    if( -1 < record->generation )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAINS_TABLE_COLUMN_GENERATION,
                         record->generation );
    }
    
    if( -1 < record->priority )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAINS_TABLE_COLUMN_PRIORITY,
                         record->priority );
    }

    if( 0 < record->hello_interval )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAINS_TABLE_COLUMN_HELLO_INTERVAL,
                         record->hello_interval );
    }

    if( 0 < record->dead_interval )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAINS_TABLE_COLUMN_DEAD_INTERVAL,
                         record->dead_interval );
    }

    if( 0 < record->wait_interval )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAINS_TABLE_COLUMN_WAIT_INTERVAL,
                         record->wait_interval );
    }

    if( 0 < record->exchange_interval )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%i', ",
                         SM_SERVICE_DOMAINS_TABLE_COLUMN_EXCHANGE_INTERVAL,
                         record->exchange_interval );
    }

    if( SM_SERVICE_DOMAIN_STATE_NIL != record->state )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAINS_TABLE_COLUMN_STATE,
                         sm_service_domain_state_str(record->state) );
    }

    if( SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_NIL != record->split_brain_recovery )
    {
        len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                         SM_SERVICE_DOMAINS_TABLE_COLUMN_SPLIT_BRAIN_RECOVERY,
                         sm_service_domain_split_brain_recovery_str(
                                                record->split_brain_recovery) );
    }

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAINS_TABLE_COLUMN_LEADER,
                     record->leader );

    snprintf( sql+len-2, sizeof(sql)-len, " WHERE %s = '%s';",
              SM_SERVICE_DOMAINS_TABLE_COLUMN_NAME, 
              record->name );

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
// Database Service Domains - Delete
// =================================
SmErrorT sm_db_service_domains_delete( SmDbHandleT* sm_db_handle, char name[] )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DELETE FROM %s WHERE %s = '%s';",
              SM_SERVICE_DOMAINS_TABLE_NAME,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_NAME,
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
// Database Service Domains - Create Table
// =======================================
SmErrorT sm_db_service_domains_create_table( SmDbHandleT* sm_db_handle )
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
                  "%s INT, "                  
                  "%s INT, "
                  "%s INT, "
                  "%s INT, "
                  "%s INT, "
                  "%s INT, "
                  "%s CHAR(%i), "
                  "%s CHAR(%i), "
                  "%s CHAR(%i));",
              SM_SERVICE_DOMAINS_TABLE_NAME,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_ID,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_PROVISIONED,
              SM_SERVICE_DOMAIN_PROVISIONED_MAX_CHAR,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_NAME,
              SM_SERVICE_DOMAIN_NAME_MAX_CHAR,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_ORCHESTRATION,
              SM_ORCHESTRATION_MAX_CHAR,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_DESIGNATION,
              SM_DESIGNATION_MAX_CHAR,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_PREEMPT,
              SM_SERVICE_DOMAIN_PREEMPT_MAX_CHAR,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_GENERATION,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_PRIORITY,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_HELLO_INTERVAL,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_DEAD_INTERVAL,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_WAIT_INTERVAL,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_EXCHANGE_INTERVAL,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_STATE,
              SM_SERVICE_DOMAIN_STATE_MAX_CHAR,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_SPLIT_BRAIN_RECOVERY,
              SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_MAX_CHAR,
              SM_SERVICE_DOMAINS_TABLE_COLUMN_LEADER,
              SM_NODE_NAME_MAX_CHAR );

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
// Database Service Domains - Delete Table
// =======================================
SmErrorT sm_db_service_domains_delete_table( SmDbHandleT* sm_db_handle )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    snprintf( sql, sizeof(sql), "DROP TABLE IF EXISTS %s;",
              SM_SERVICE_DOMAINS_TABLE_NAME );

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
// Database Service Domains - Cleanup Table
// ========================================
SmErrorT sm_db_service_domains_cleanup_table( SmDbHandleT* sm_db_handle )
{
    int len;
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    char* error = NULL;
    int rc;

    len = snprintf( sql, sizeof(sql), "UPDATE %s SET  ",
                    SM_SERVICE_DOMAINS_TABLE_NAME );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAINS_TABLE_COLUMN_DESIGNATION,
                     sm_designation_type_str(SM_DESIGNATION_TYPE_UNKNOWN) );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAINS_TABLE_COLUMN_STATE,
                     sm_service_domain_state_str(SM_SERVICE_DOMAIN_STATE_INITIAL) );

    len += snprintf( sql+len, sizeof(sql)-len, "%s = '%s', ",
                     SM_SERVICE_DOMAINS_TABLE_COLUMN_LEADER, "" );

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
// Database Service Domains - Initialize
// =====================================
SmErrorT sm_db_service_domains_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Finalize
// ===================================
SmErrorT sm_db_service_domains_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
