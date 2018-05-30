//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_action_result_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_service_action_results.h"

static SmListT* _service_action_results = NULL;
static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Action Result Table - Read
// ==================================
SmServiceActionResultDataT* sm_service_action_result_table_read(
    char plugin_type[], char plugin_name[], char plugin_command[],
    char plugin_exit_code[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceActionResultDataT* service_action_result;

    SM_LIST_FOREACH( _service_action_results, entry, entry_data )
    {
        service_action_result = (SmServiceActionResultDataT*) entry_data;

        if(( 0 == strcmp( plugin_type, service_action_result->plugin_type ) )&&
           ( 0 == strcmp( plugin_name, service_action_result->plugin_name ) )&&
           ( 0 == strcmp( plugin_command,
                          service_action_result->plugin_command ) )&&
           ( 0 == strcmp( plugin_exit_code,
                          service_action_result->plugin_exit_code ) ))
        {
            return( service_action_result );
        }
    }

    return( NULL );    
}
// ****************************************************************************

// ****************************************************************************
// Service Action Result Table - Add
// =================================
static SmErrorT sm_service_action_result_table_add( void* user_data[],
    void* record )
{
    SmServiceActionResultDataT* service_action_result;
    SmDbServiceActionResultT* db_service_action_result;
    
    db_service_action_result = (SmDbServiceActionResultT*) record;

    service_action_result = sm_service_action_result_table_read( 
                                db_service_action_result->plugin_type,
                                db_service_action_result->plugin_name,
                                db_service_action_result->plugin_command,
                                db_service_action_result->plugin_exit_code );
    if( NULL == service_action_result )
    {
        service_action_result = (SmServiceActionResultDataT*)
                                malloc( sizeof(SmServiceActionResultDataT) );
        if( NULL == service_action_result )
        {
            DPRINTFE( "Failed to allocate service action result table entry." );
            return( SM_FAILED );
        }

        memset( service_action_result, 0, sizeof(SmServiceActionResultDataT) );

        snprintf( service_action_result->plugin_type, 
                  sizeof(service_action_result->plugin_type), "%s",
                  db_service_action_result->plugin_type );
        snprintf( service_action_result->plugin_name, 
                  sizeof(service_action_result->plugin_name), "%s",
                  db_service_action_result->plugin_name );
        snprintf( service_action_result->plugin_command, 
                  sizeof(service_action_result->plugin_command), "%s",
                  db_service_action_result->plugin_command );
        snprintf( service_action_result->plugin_exit_code, 
                  sizeof(service_action_result->plugin_exit_code), "%s",
                  db_service_action_result->plugin_exit_code );
        service_action_result->action_result 
            = db_service_action_result->action_result;
        service_action_result->service_state 
            = db_service_action_result->service_state;
        service_action_result->service_status 
            = db_service_action_result->service_status;
        service_action_result->service_condition
            = db_service_action_result->service_condition;

        SM_LIST_PREPEND( _service_action_results, 
                         (SmListEntryDataPtrT) service_action_result );

    } else { 
        service_action_result->action_result 
            = db_service_action_result->action_result;
        service_action_result->service_state 
            = db_service_action_result->service_state;
        service_action_result->service_status 
            = db_service_action_result->service_status;
        service_action_result->service_condition
            = db_service_action_result->service_condition;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action Result Table - Load
// ==================================
SmErrorT sm_service_action_result_table_load( void )
{
    SmDbServiceActionResultT service_action_result;
    SmErrorT error;

    error = sm_db_foreach( SM_DATABASE_NAME, SM_SERVICE_ACTION_RESULTS_TABLE_NAME,
                           NULL, &service_action_result,
                           sm_db_service_action_results_convert,
                           sm_service_action_result_table_add, NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over service action results in database, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action Result Table - Initialize
// ========================================
SmErrorT sm_service_action_result_table_initialize( void )
{
    SmErrorT error;

    _service_action_results = NULL;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_action_result_table_load();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to load service action results from database, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action Result Table - Finalize
// ======================================
SmErrorT sm_service_action_result_table_finalize( void )
{
    SmErrorT error;

    SM_LIST_CLEANUP_ALL( _service_action_results );

    if( NULL != _sm_db_handle )
    {
        error = sm_db_disconnect( _sm_db_handle );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to disconnect from database (%s), error=%s.",
                      SM_DATABASE_NAME, sm_error_str( error ) );
        }

        _sm_db_handle = NULL;
    }

    return( SM_OKAY );
}
// ****************************************************************************
