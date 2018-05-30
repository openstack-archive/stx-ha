//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_member_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_service_domain_members.h"

static SmListT* _service_domain_members = NULL;
static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Domain Member Table - Read
// ==================================
SmServiceDomainMemberT* sm_service_domain_member_table_read( char name[],
    char service_group_name[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainMemberT* member;

    SM_LIST_FOREACH( _service_domain_members, entry, entry_data )
    {
        member = (SmServiceDomainMemberT*) entry_data;

        if(( 0 == strcmp( name, member->name ) )&&
           ( 0 == strcmp( service_group_name, member->service_group_name ) ))
        {
            return( member );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Read By Identifier
// ================================================
SmServiceDomainMemberT* sm_service_domain_member_table_read_by_id( int64_t id )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainMemberT* member;

    SM_LIST_FOREACH( _service_domain_members, entry, entry_data )
    {
        member = (SmServiceDomainMemberT*) entry_data;

        if( id == member->id )
        {
            return( member );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Read Service Group
// ================================================
SmServiceDomainMemberT* sm_service_domain_member_table_read_service_group(
    char service_group_name[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainMemberT* member;

    SM_LIST_FOREACH( _service_domain_members, entry, entry_data )
    {
        member = (SmServiceDomainMemberT*) entry_data;

        if( 0 == strcmp( service_group_name, member->service_group_name ) )
        {
            return( member );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Count
// ===================================
unsigned int sm_service_domain_member_table_count( char service_domain_name[] )
{
    unsigned int count = 0;
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainMemberT* member;

    SM_LIST_FOREACH( _service_domain_members, entry, entry_data )
    {
        member = (SmServiceDomainMemberT*) entry_data;

        if( 0 == strcmp( service_domain_name, member->name ) )
        {
            ++count;
        }
    }

    return( count );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - For Each
// ======================================
void sm_service_domain_member_table_foreach( char service_domain_name[],
    void* user_data[], SmServiceDomainMemberTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainMemberT* member;

    SM_LIST_FOREACH( _service_domain_members, entry, entry_data )
    {
        member = (SmServiceDomainMemberT*) entry_data;

        if( 0 == strcmp( service_domain_name, member->name ) )
        {
            callback( user_data, member );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - For Each Service Group Aggregate
// ==============================================================
void sm_service_domain_member_table_foreach_service_group_aggregate(
    char service_domain_name[], char service_group_aggregate[],
    void* user_data[], SmServiceDomainMemberTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainMemberT* member;

    SM_LIST_FOREACH( _service_domain_members, entry, entry_data )
    {
        member = (SmServiceDomainMemberT*) entry_data;

        if(( 0 == strcmp( service_domain_name, member->name ) )&&
           ( 0 == strcmp( service_group_aggregate,
                          member->service_group_aggregate ) ))
        {
            callback( user_data, member );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Add
// =================================
static SmErrorT sm_service_domain_member_table_add( void* user_data[],
    void* record )
{
    SmServiceDomainMemberT* member;
    SmDbServiceDomainMemberT* db_member;

    db_member = (SmDbServiceDomainMemberT*) record;

    member = sm_service_domain_member_table_read( db_member->name,
                                            db_member->service_group_name );
    if( NULL == member )
    {
        member = (SmServiceDomainMemberT*)
                     malloc( sizeof(SmServiceDomainMemberT) );
        if( NULL == member )
        {
            DPRINTFE( "Failed to allocate service domain member table entry." );
            return( SM_FAILED );
        }

        memset( member, 0, sizeof(SmServiceDomainMemberT) );

        member->id = db_member->id;
        snprintf( member->name, sizeof(member->name), 
                  "%s", db_member->name );
        snprintf( member->service_group_name,
                  sizeof(member->service_group_name), 
                  "%s", db_member->service_group_name );
        member->redundancy_model = db_member->redundancy_model;
        member->n_active = db_member->n_active;
        member->m_standby = db_member->m_standby;
        snprintf( member->service_group_aggregate,
                  sizeof(member->service_group_aggregate), 
                  "%s", db_member->service_group_aggregate );
        snprintf( member->active_only_if_active,
                  sizeof(member->active_only_if_active), 
                  "%s", db_member->active_only_if_active );
        member->redundancy_log_text[0] = '\0';

        SM_LIST_PREPEND( _service_domain_members,
                         (SmListEntryDataPtrT) member );
    } else { 
        member->id = db_member->id;
        member->redundancy_model = db_member->redundancy_model;
        member->n_active = db_member->n_active;
        member->m_standby = db_member->m_standby;
        snprintf( member->service_group_aggregate,
                  sizeof(member->service_group_aggregate), 
                  "%s", db_member->service_group_aggregate );
        snprintf( member->active_only_if_active,
                  sizeof(member->active_only_if_active), 
                  "%s", db_member->active_only_if_active );
        member->redundancy_log_text[0] = '\0';
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Load
// ==================================
SmErrorT sm_service_domain_member_table_load( void )
{
    char db_query[SM_DB_QUERY_STATEMENT_MAX_CHAR]; 
    SmDbServiceDomainMemberT member;
    SmErrorT error;

    snprintf( db_query, sizeof(db_query), "%s = 'yes'",
              SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_PROVISIONED );

    error = sm_db_foreach( SM_DATABASE_NAME, 
                           SM_SERVICE_DOMAIN_MEMBERS_TABLE_NAME,
                           db_query, &member,
                           sm_db_service_domain_members_convert,
                           sm_service_domain_member_table_add, NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over service domain members in "
                  "database, error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Initialize
// ========================================
SmErrorT sm_service_domain_member_table_initialize( void )
{
    SmErrorT error;

    _service_domain_members = NULL;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_member_table_load();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to load service domain members from database, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Finalize
// ======================================
SmErrorT sm_service_domain_member_table_finalize( void )
{
    SmErrorT error;

    SM_LIST_CLEANUP_ALL( _service_domain_members );

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
