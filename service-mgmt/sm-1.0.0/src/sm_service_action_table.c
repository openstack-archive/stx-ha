//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_action_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_time.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_services.h"
#include "sm_db_service_actions.h"

static SmListT* _service_actions = NULL;
static SmDbHandleT* _sm_db_handle = NULL;

static int _interval_extension_in_secs = 0;
static int _timeout_extension_in_secs = 0;

// ****************************************************************************
// Service Action Table - Set Interval Extension
// =============================================
void sm_service_action_table_set_interval_extension( 
    int interval_extension_in_secs )
{
    _interval_extension_in_secs = interval_extension_in_secs;
}
// ****************************************************************************

// ****************************************************************************
// Service Action Table - Set Timeout Extension
// ============================================
void sm_service_action_table_set_timeout_extension(
    int timeout_extension_in_secs )
{
    _timeout_extension_in_secs = timeout_extension_in_secs;
}
// ****************************************************************************

// ****************************************************************************
// Service Action Table - Read
// ===========================
SmServiceActionDataT* sm_service_action_table_read( char service_name[],
    SmServiceActionT action )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceActionDataT* service_action;

    SM_LIST_FOREACH( _service_actions, entry, entry_data )
    {
        service_action = (SmServiceActionDataT*) entry_data;

        if(( action == service_action->action )&&
           ( 0 == strcmp( service_name, service_action->service_name ) ))
        {
            return( service_action );
        }
    }

    return( NULL );    
}
// ****************************************************************************

// ****************************************************************************
// Service Action Table - Add
// ==========================
static SmErrorT sm_service_action_table_add( void* user_data[], void* record )
{
    SmDbServiceT db_service;
    SmServiceActionDataT* service_action;
    SmDbServiceActionT* db_service_action = (SmDbServiceActionT*) record;
    SmErrorT error;

    error = sm_db_services_read( _sm_db_handle,
                                 db_service_action->service_name,
                                 &db_service );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  db_service_action->service_name, sm_error_str( error ) );
        return( SM_FAILED );
    }

    service_action = sm_service_action_table_read(
                                        db_service_action->service_name,
                                        db_service_action->action );
    if( NULL == service_action )
    {
        service_action
            = (SmServiceActionDataT*) malloc( sizeof(SmServiceActionDataT) );
        if( NULL == service_action )
        {
            DPRINTFE( "Failed to allocate service action table entry." );
            return( SM_FAILED );
        }

        memset( service_action, 0, sizeof(SmServiceActionDataT) );

        snprintf( service_action->service_name, 
                  sizeof(service_action->service_name), "%s",
                  db_service_action->service_name );
        service_action->action = db_service_action->action;
        snprintf( service_action->plugin_type, 
                  sizeof(service_action->plugin_type), "%s",
                  db_service_action->plugin_type );
        snprintf( service_action->plugin_class, 
                  sizeof(service_action->plugin_class), "%s",
                  db_service_action->plugin_class );
        snprintf( service_action->plugin_name, 
                  sizeof(service_action->plugin_name), "%s",
                  db_service_action->plugin_name );
        snprintf( service_action->plugin_command, 
                  sizeof(service_action->plugin_command), "%s",
                  db_service_action->plugin_command );
        snprintf( service_action->plugin_params, 
                  sizeof(service_action->plugin_params), "%s",
                  db_service_action->plugin_params );
        service_action->max_failure_retries 
            = db_service_action->max_failure_retries;
        service_action->max_timeout_retries
            = db_service_action->max_timeout_retries;
        service_action->max_total_retries
            = db_service_action->max_total_retries;
        service_action->timeout_in_secs
            = db_service_action->timeout_in_secs + _timeout_extension_in_secs;
        service_action->interval_in_secs
            = db_service_action->interval_in_secs + _interval_extension_in_secs;
        memset( &(service_action->hash), 0, sizeof(service_action->hash) );
        sm_time_get( &(service_action->last_hash_validate) );

        SM_LIST_PREPEND( _service_actions, 
                         (SmListEntryDataPtrT) service_action );
    } else { 
        snprintf( service_action->plugin_type, 
                  sizeof(service_action->plugin_type), "%s",
                  db_service_action->plugin_type );
        snprintf( service_action->plugin_class, 
                  sizeof(service_action->plugin_class), "%s",
                  db_service_action->plugin_class );
        snprintf( service_action->plugin_name, 
                  sizeof(service_action->plugin_name), "%s",
                  db_service_action->plugin_name );
        snprintf( service_action->plugin_command, 
                  sizeof(service_action->plugin_command), "%s",
                  db_service_action->plugin_command );
        snprintf( service_action->plugin_params, 
                  sizeof(service_action->plugin_params), "%s",
                  db_service_action->plugin_params );
        service_action->max_failure_retries 
            = db_service_action->max_failure_retries;
        service_action->max_timeout_retries
            = db_service_action->max_timeout_retries;
        service_action->max_total_retries
            = db_service_action->max_total_retries;
        service_action->timeout_in_secs
            = db_service_action->timeout_in_secs + _timeout_extension_in_secs;
        service_action->interval_in_secs
            = db_service_action->interval_in_secs + _interval_extension_in_secs;
        memset( &(service_action->hash), 0, sizeof(service_action->hash) );
        sm_time_get( &(service_action->last_hash_validate) );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action Table - Load
// ===========================
SmErrorT sm_service_action_table_load( void )
{
    SmDbServiceActionT service_action;
    SmErrorT error;

    error = sm_db_foreach( SM_DATABASE_NAME, SM_SERVICE_ACTIONS_TABLE_NAME,
                           NULL, &service_action, sm_db_service_actions_convert,
                           sm_service_action_table_add, NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over service actions in database, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action Table - Initialize
// =================================
SmErrorT sm_service_action_table_initialize( void )
{
    SmErrorT error;

    _service_actions = NULL;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_action_table_load();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to load service actions from database, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action Table - Finalize
// ===============================
SmErrorT sm_service_action_table_finalize( void )
{
    SmErrorT error;

    SM_LIST_CLEANUP_ALL( _service_actions );

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
