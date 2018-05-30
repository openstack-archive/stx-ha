//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_service_groups.h"

static SmListT* _service_groups = NULL;
static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Group Table - Read
// ==========================
SmServiceGroupT* sm_service_group_table_read( char service_group_name[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceGroupT* service_group;

    SM_LIST_FOREACH( _service_groups, entry, entry_data )
    {
        service_group = (SmServiceGroupT*) entry_data;

        if( 0 == strcmp( service_group_name, service_group->name ) )
        {
            return( service_group );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Read By Identifier
// ========================================
SmServiceGroupT* sm_service_group_table_read_by_id( int64_t service_group_id )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceGroupT* service_group;

    SM_LIST_FOREACH( _service_groups, entry, entry_data )
    {
        service_group = (SmServiceGroupT*) entry_data;

        if( service_group_id == service_group->id )
        {
            return( service_group );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Read By Notification Pid
// ==============================================
SmServiceGroupT* sm_service_group_table_read_by_notification_pid( int pid )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceGroupT* service_group;

    SM_LIST_FOREACH( _service_groups, entry, entry_data )
    {
        service_group = (SmServiceGroupT*) entry_data;

        if( pid == service_group->notification_pid )
        {
            return( service_group );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Table - For Each
// ==============================
void sm_service_group_table_foreach( void* user_data[],
    SmServiceGroupTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;

    SM_LIST_FOREACH( _service_groups, entry, entry_data )
    {
        callback( user_data, (SmServiceGroupT*) entry_data );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Add
// =========================
static SmErrorT sm_service_group_table_add( void* user_data[], void* record )
{
    SmServiceGroupT* service_group;
    SmDbServiceGroupT* db_service_group = (SmDbServiceGroupT*) record;

    service_group = sm_service_group_table_read( db_service_group->name );
    if( NULL == service_group )
    {
        service_group = (SmServiceGroupT*) malloc( sizeof(SmServiceGroupT) );
        if( NULL == service_group )
        {
            DPRINTFE( "Failed to allocate service group table entry." );
            return( SM_FAILED );
        }

        memset( service_group, 0, sizeof(SmServiceGroupT) );

        service_group->id = db_service_group->id;
        snprintf( service_group->name, sizeof(service_group->name), "%s",
                  db_service_group->name );
        service_group->auto_recover = db_service_group->auto_recover;
        service_group->core = db_service_group->core;
        service_group->desired_state = db_service_group->desired_state;
        service_group->state = db_service_group->state;
        service_group->status = db_service_group->status;
        service_group->condition = db_service_group->condition;
        service_group->failure_debounce_in_ms
            = db_service_group->failure_debounce_in_ms;
        service_group->fatal_error_reboot 
            = db_service_group->fatal_error_reboot;
        service_group->reason_text[0] = '\0';
        service_group->notification = SM_SERVICE_GROUP_NOTIFICATION_NIL;
        service_group->notification_str = NULL;
        service_group->notification_pid = -1;
        service_group->notification_timer_id = SM_TIMER_ID_INVALID;
        service_group->notification_complete = false;
        service_group->notification_failed = false;
        service_group->notification_timeout = false;

        SM_LIST_PREPEND( _service_groups, (SmListEntryDataPtrT) service_group );

    } else {
        service_group->id = db_service_group->id;
        service_group->auto_recover = db_service_group->auto_recover;
        service_group->core = db_service_group->core;
        service_group->failure_debounce_in_ms
            = db_service_group->failure_debounce_in_ms;
        service_group->fatal_error_reboot
            = db_service_group->fatal_error_reboot;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Load
// ==========================
SmErrorT sm_service_group_table_load( void )
{
    char db_query[SM_DB_QUERY_STATEMENT_MAX_CHAR]; 
    SmDbServiceGroupT service_group;
    SmErrorT error;

    snprintf( db_query, sizeof(db_query), "%s = 'yes'",
              SM_SERVICE_GROUPS_TABLE_COLUMN_PROVISIONED );
    
    error = sm_db_foreach( SM_DATABASE_NAME, SM_SERVICE_GROUPS_TABLE_NAME,
                           db_query, &service_group,
                           sm_db_service_groups_convert,
                           sm_service_group_table_add, NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over service groups in database, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Persist
// =============================
SmErrorT sm_service_group_table_persist( SmServiceGroupT* service_group )
{
    SmDbServiceGroupT db_service_group;
    SmErrorT error;

    memset( &db_service_group, 0, sizeof(db_service_group) );

    db_service_group.id = service_group->id;
    snprintf( db_service_group.name, sizeof(db_service_group.name), "%s",
              service_group->name );
    db_service_group.auto_recover = service_group->auto_recover;
    db_service_group.core = service_group->core;
    db_service_group.desired_state = service_group->desired_state;
    db_service_group.state = service_group->state;
    db_service_group.status = service_group->status;
    db_service_group.condition = service_group->condition;
    db_service_group.failure_debounce_in_ms
        = service_group->failure_debounce_in_ms;
    db_service_group.fatal_error_reboot = service_group->fatal_error_reboot;

    error = sm_db_service_groups_update( _sm_db_handle, &db_service_group );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update database, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Initialize
// ================================
SmErrorT sm_service_group_table_initialize( void )
{
    SmErrorT error;

    _service_groups = NULL;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_table_load();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to load service groups from database, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Finalize
// ==============================
SmErrorT sm_service_group_table_finalize( void )
{
    SmErrorT error;

    SM_LIST_CLEANUP_ALL( _service_groups );

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
