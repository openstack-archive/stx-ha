//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_foreach.h"

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db.h"
#include "sm_db_iterator.h"

// ****************************************************************************
// Database For-Each
// =================
SmErrorT sm_db_foreach( const char* db_name, const char* db_table,
    const char* db_query, void* record_storage, 
    SmDbForEachRecordConverterT record_converter,
    SmDbForEachRecordCallbackT record_callback, void* user_data[] )
{
    SmDbIteratorT it;
    SmErrorT error, error2;

    error = sm_db_iterator_initialize( db_name, db_table, db_query, &it );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize iterator, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_iterator_first( &it );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to go to first result of iterator, error=%s.",
                  sm_error_str( error ) );
        goto ERROR;
    }

    while( !(it.done) )
    {
        error = sm_db_iterator_get( &it, record_storage, record_converter );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to go to get result from iterator, error=%s.",
                      sm_error_str( error ) );
            goto ERROR;
        }

        error = record_callback( user_data, record_storage );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Callback failed , error=%s.", sm_error_str( error ) );
            goto ERROR;
        }

        error = sm_db_iterator_next( &it );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to go to next result of iterator, error=%s.",
                      sm_error_str( error ) );
            goto ERROR;
        }
    }

    error = SM_OKAY;

ERROR:
    error2 = sm_db_iterator_finalize( &it );
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to finalize iterator, error=%s.",
                  sm_error_str( error2 ) );
        return( error );
    }

    return( error );
}
// ****************************************************************************
