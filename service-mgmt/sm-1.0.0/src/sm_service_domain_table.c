//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_service_domains.h"

static SmListT* _service_domains = NULL;
static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Domain Table - Read
// ===========================
SmServiceDomainT* sm_service_domain_table_read( char name[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainT* domain;

    SM_LIST_FOREACH( _service_domains, entry, entry_data )
    {
        domain = (SmServiceDomainT*) entry_data;

        if( 0 == strcmp( name, domain->name ) )
        {
            return( domain );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - Read By Identifier
// =========================================
SmServiceDomainT* sm_service_domain_table_read_by_id( int64_t id )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainT* domain;

    SM_LIST_FOREACH( _service_domains, entry, entry_data )
    {
        domain = (SmServiceDomainT*) entry_data;

        if( id == domain->id )
        {
            return( domain );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - For Each
// ===============================
void sm_service_domain_table_foreach( void* user_data[],
    SmServiceDomainTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;

    SM_LIST_FOREACH( _service_domains, entry, entry_data )
    {
        callback( user_data, (SmServiceDomainT*) entry_data );
    }

}
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - Add
// ==========================
static SmErrorT sm_service_domain_table_add( void* user_data[], void* record )
{
    SmServiceDomainT* domain;
    SmDbServiceDomainT* db_domain;

    db_domain = (SmDbServiceDomainT*) record;

    domain = sm_service_domain_table_read( db_domain->name );
    if( NULL == domain )
    {
        domain = (SmServiceDomainT*) malloc( sizeof(SmServiceDomainT) );
        if( NULL == domain )
        {
            DPRINTFE( "Failed to allocate service domain table entry." );
            return( SM_FAILED );
        }

        memset( domain, 0, sizeof(SmServiceDomainT) );

        domain->id = db_domain->id;
        snprintf( domain->name, sizeof(domain->name), "%s", db_domain->name );
        domain->orchestration = db_domain->orchestration;
        domain->designation = db_domain->designation;
        domain->preempt = db_domain->preempt;
        domain->generation = db_domain->generation;
        domain->priority = db_domain->priority;
        domain->hello_interval = db_domain->hello_interval;
        domain->dead_interval = db_domain->dead_interval;
        domain->wait_interval = db_domain->wait_interval;
        domain->exchange_interval = db_domain->exchange_interval;
        domain->state = db_domain->state;
        domain->split_brain_recovery = db_domain->split_brain_recovery;
        snprintf( domain->leader, sizeof(domain->leader), "%s",
                  db_domain->leader );
        domain->hello_timer_id = SM_TIMER_ID_INVALID;
        domain->wait_timer_id = SM_TIMER_ID_INVALID;

        SM_LIST_PREPEND( _service_domains, (SmListEntryDataPtrT) domain );

    } else { 
        domain->id = db_domain->id;
        domain->orchestration = db_domain->orchestration;
        domain->designation = db_domain->designation;
        domain->preempt = db_domain->preempt;
        domain->generation = db_domain->generation;
        domain->priority = db_domain->priority;
        domain->hello_interval = db_domain->hello_interval;
        domain->dead_interval = db_domain->dead_interval;
        domain->wait_interval = db_domain->wait_interval;
        domain->exchange_interval = db_domain->exchange_interval;
        domain->state = db_domain->state;
        domain->split_brain_recovery = db_domain->split_brain_recovery;
        snprintf( domain->leader, sizeof(domain->leader), "%s",
                  db_domain->leader );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - Load
// ===========================
SmErrorT sm_service_domain_table_load( void )
{
    char db_query[SM_DB_QUERY_STATEMENT_MAX_CHAR]; 
    SmDbServiceDomainT domain;
    SmErrorT error;

    snprintf( db_query, sizeof(db_query), "%s = 'yes'",
              SM_SERVICE_DOMAINS_TABLE_COLUMN_PROVISIONED );

    error = sm_db_foreach( SM_DATABASE_NAME, 
                           SM_SERVICE_DOMAINS_TABLE_NAME, db_query,
                           &domain, sm_db_service_domains_convert,
                           sm_service_domain_table_add, NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over service domains in "
                  "database, error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - Persist
// ==============================
SmErrorT sm_service_domain_table_persist( SmServiceDomainT* domain )
{
    SmDbServiceDomainT db_domain;
    SmErrorT error;

    memset( &db_domain, 0, sizeof(db_domain) );

    db_domain.id = domain->id;
    snprintf( db_domain.name, sizeof(db_domain.name), "%s", domain->name );
    db_domain.orchestration = domain->orchestration;
    db_domain.designation = domain->designation;
    db_domain.preempt = domain->preempt;
    db_domain.generation = domain->generation;
    db_domain.priority = domain->priority;
    db_domain.hello_interval = domain->hello_interval;
    db_domain.dead_interval = domain->dead_interval;
    db_domain.wait_interval = domain->wait_interval;
    db_domain.exchange_interval = domain->exchange_interval;
    db_domain.state = domain->state;
    db_domain.split_brain_recovery = domain->split_brain_recovery;
    snprintf( db_domain.leader, sizeof(db_domain.leader), "%s",
              domain->leader );

    error = sm_db_service_domains_update( _sm_db_handle, &db_domain );
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
// Service Domain Table - Initialize
// =================================
SmErrorT sm_service_domain_table_initialize( void )
{
    SmErrorT error;

    _service_domains = NULL;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_table_load();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to load service domains from database, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - Finalize
// ===============================
SmErrorT sm_service_domain_table_finalize( void )
{
    SmErrorT error;

    SM_LIST_CLEANUP_ALL( _service_domains );

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
