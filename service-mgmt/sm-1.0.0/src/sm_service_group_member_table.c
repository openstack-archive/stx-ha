//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_member_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_service_group_members.h"

static SmListT* _service_group_members = NULL;
static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Group Member Table - Read
// =================================
SmServiceGroupMemberT* sm_service_group_member_table_read(
    char service_group_name[], char service_name[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceGroupMemberT* service_group_member;

    SM_LIST_FOREACH( _service_group_members, entry, entry_data )
    {
        service_group_member = (SmServiceGroupMemberT*) entry_data;

        if(( 0 == strcmp( service_group_name, service_group_member->name ) )&&
           ( 0 == strcmp( service_name, service_group_member->service_name ) )) 
        {
            return( service_group_member );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - Read By Identifier
// ===============================================
SmServiceGroupMemberT* sm_service_group_member_table_read_by_id( 
    int64_t service_group_member_id )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceGroupMemberT* service_group_member;

    SM_LIST_FOREACH( _service_group_members, entry, entry_data )
    {
        service_group_member = (SmServiceGroupMemberT*) entry_data;

        if( service_group_member_id == service_group_member->id )
        {
            return( service_group_member );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - Read By Service
// ============================================
SmServiceGroupMemberT* sm_service_group_member_table_read_by_service( 
    char service_name[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceGroupMemberT* service_group_member;

    SM_LIST_FOREACH( _service_group_members, entry, entry_data )
    {
        service_group_member = (SmServiceGroupMemberT*) entry_data;

        if( 0 == strcmp( service_name, service_group_member->service_name ) )
        {
            return( service_group_member );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - For Each
// =====================================
void sm_service_group_member_table_foreach( void* user_data[],
    SmServiceGroupMemberTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;

    SM_LIST_FOREACH( _service_group_members, entry, entry_data )
    {
        callback( user_data, (SmServiceGroupMemberT*) entry_data );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - For Each Member
// ============================================
void sm_service_group_member_table_foreach_member( char service_group_name[],
    void* user_data[], SmServiceGroupMemberTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceGroupMemberT* service_group_member;

    SM_LIST_FOREACH( _service_group_members, entry, entry_data )
    {
        service_group_member = (SmServiceGroupMemberT*) entry_data;

        if( 0 == strcmp( service_group_name, service_group_member->name ) )
        {
            callback( user_data, service_group_member );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - Add
// ================================
static SmErrorT sm_service_group_member_table_add( void* user_data[],
    void* record )
{
    SmServiceGroupMemberT* service_group_member;
    SmDbServiceGroupMemberT* db_service_group_member;
    
    db_service_group_member = (SmDbServiceGroupMemberT*) record;

    service_group_member = sm_service_group_member_table_read( 
                                    db_service_group_member->name,
                                    db_service_group_member->service_name );
    if( NULL == service_group_member )
    {
        service_group_member 
            = (SmServiceGroupMemberT*) malloc( sizeof(SmServiceGroupMemberT) );
        if( NULL == service_group_member )
        {
            DPRINTFE( "Failed to allocate service group member table entry." );
            return( SM_FAILED );
        }

        memset( service_group_member, 0, sizeof(SmServiceGroupMemberT) );

        service_group_member->id = db_service_group_member->id;
        snprintf( service_group_member->name, sizeof(service_group_member->name),
                  "%s", db_service_group_member->name );
        snprintf( service_group_member->service_name,
                  sizeof(service_group_member->service_name),
                  "%s", db_service_group_member->service_name );
        service_group_member->service_state = SM_SERVICE_STATE_INITIAL;
        service_group_member->service_status = SM_SERVICE_STATUS_NONE;
        service_group_member->service_condition = SM_SERVICE_CONDITION_NONE;
        service_group_member->service_failure_impact
            = db_service_group_member->service_failure_impact;
        service_group_member->service_failure_timestamp = 0;
 
        SM_LIST_PREPEND( _service_group_members, 
                         (SmListEntryDataPtrT) service_group_member );

    } else {
        service_group_member->id = db_service_group_member->id;
        service_group_member->service_failure_impact
            = db_service_group_member->service_failure_impact;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - Load
// =================================
SmErrorT sm_service_group_member_table_load( void )
{
    char db_query[SM_DB_QUERY_STATEMENT_MAX_CHAR]; 
    SmDbServiceGroupMemberT service_group_member;
    SmErrorT error;

    snprintf( db_query, sizeof(db_query), "%s = 'yes'",
              SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_PROVISIONED );

    error = sm_db_foreach( SM_DATABASE_NAME,
                           SM_SERVICE_GROUP_MEMBERS_TABLE_NAME,
                           db_query, &service_group_member,
                           sm_db_service_group_members_convert,
                           sm_service_group_member_table_add, NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over service group memberss in database, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - Persist
// ====================================
SmErrorT sm_service_group_member_table_persist(
    SmServiceGroupMemberT* service_group_member )
{
    SmDbServiceGroupMemberT db_service_group_member;
    SmErrorT error;

    memset( &db_service_group_member, 0, sizeof(db_service_group_member) );

    db_service_group_member.id = service_group_member->id;
    snprintf( db_service_group_member.name, 
              sizeof(db_service_group_member.name),
              "%s", service_group_member->name );
    snprintf( db_service_group_member.service_name,
              sizeof(db_service_group_member.service_name),
              "%s", service_group_member->service_name );
    db_service_group_member.service_failure_impact
        = service_group_member->service_failure_impact;

    error = sm_db_service_group_members_update( _sm_db_handle, 
                                                &db_service_group_member );
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
// Service Group Member Table - Initialize
// =======================================
SmErrorT sm_service_group_member_table_initialize( void )
{
    SmErrorT error;

    _service_group_members = NULL;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_member_table_load();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to load service group members from database, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - Finalize
// =====================================
SmErrorT sm_service_group_member_table_finalize( void )
{
    SmErrorT error;

    SM_LIST_CLEANUP_ALL( _service_group_members );

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
