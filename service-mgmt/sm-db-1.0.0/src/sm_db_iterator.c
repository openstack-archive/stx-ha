//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_iterator.h"

#include <stdio.h>
#include <string.h>
#include <sqlite3.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_debug.h"
#include "sm_db.h"

#define SM_DB_ITERATOR_VALID  0xFDFDFDFD

// ****************************************************************************
// Database Iterator - First
// =========================
SmErrorT sm_db_iterator_first( SmDbIteratorT* it )
{
    if( SM_DB_ITERATOR_VALID == it->valid )
    {    
        SmErrorT error;

        error = sm_db_statement_result_reset( it->sm_db_statement );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to reset statement results, error=%s.",
                      sm_error_str( error ) );
            return( SM_FAILED );
        }

        error = sm_db_statement_result_step( it->sm_db_statement,
                                             &(it->done) );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to set through statement results, error=%s.",
                      sm_error_str( error ) );
            return( SM_FAILED );
        }

        return( SM_OKAY );

    } else {
        DPRINTFE( "Database iterator is not valid." );
        return( SM_FAILED );
    }
}
// ****************************************************************************

// ****************************************************************************
// Database Iterator - Get
// =======================
SmErrorT sm_db_iterator_get( SmDbIteratorT* it, void* record,
    SmDbIteratorRecordConverterT converter )
{
    const char* col_name = NULL;
    const char* col_data = NULL;
    int num_cols = sqlite3_column_count( (sqlite3_stmt*) it->sm_db_statement );
    SmErrorT error;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        col_name = sqlite3_column_name( (sqlite3_stmt*) it->sm_db_statement,
                                        col_i );
        col_data = (const char*) sqlite3_column_text( 
                                    (sqlite3_stmt*) it->sm_db_statement, col_i );

        DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

        error = converter( col_name, col_data, record );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to convert database data to record, "
                      "error=%s.", sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Iterator - Next
// ========================
SmErrorT sm_db_iterator_next( SmDbIteratorT* it )
{
    if( SM_DB_ITERATOR_VALID == it->valid )
    {    
        SmErrorT error;

        error = sm_db_statement_result_step( it->sm_db_statement,
                                             &(it->done) );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to set through statement results, error=%s.",
                      sm_error_str( error ) );
            return( SM_FAILED );
        }

        return( SM_OKAY );

    } else {
        DPRINTFE( "Database iterator is not valid." );
        return( SM_FAILED );
    }
}
// ****************************************************************************

// ****************************************************************************
// Database Iterator - Initialize
// ==============================
SmErrorT sm_db_iterator_initialize( const char* db_name, 
    const char* db_table, const char* db_query, SmDbIteratorT* it )
{
    char sql[SM_SQL_STATEMENT_MAX_CHAR];
    SmErrorT error;

    memset( it, 0, sizeof(SmDbIteratorT) );

    error = sm_db_connect( db_name, &(it->sm_db_handle) );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.", db_name,
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    if( NULL == db_query )
    {
        snprintf( sql, sizeof(sql), "SELECT * FROM %s", db_table );
    } else {
        snprintf( sql, sizeof(sql), "SELECT * FROM %s WHERE %s", db_table,
                  db_query );
    }

    error = sm_db_statement_initialize( it->sm_db_handle, sql,
                                        &(it->sm_db_statement) );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize statement (%s), error=%s.", sql,
                  sm_error_str( error ) );
        sm_db_disconnect( it->sm_db_handle );
        return( SM_FAILED );
    }

    it->valid = SM_DB_ITERATOR_VALID;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Iterator - Finalize
// ============================
SmErrorT sm_db_iterator_finalize( SmDbIteratorT* it )
{
    if( SM_DB_ITERATOR_VALID == it->valid )
    {
        SmErrorT error;

        if( NULL != it->sm_db_statement )
        {
            error = sm_db_statement_finalize( it->sm_db_statement );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to finalize statement, error=%s.",
                          sm_error_str( error ) );
            }
        }

        if( NULL != it->sm_db_handle )
        {
            error = sm_db_disconnect( it->sm_db_handle );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to disconnect from database, error=%s.",
                          sm_error_str( error ) );
            }
        }

        memset( it, 0, sizeof(SmDbIteratorT) );        
    }

    return( SM_OKAY );
}
// ****************************************************************************
