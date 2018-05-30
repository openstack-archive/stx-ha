//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_neighbor_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_service_domain_neighbors.h"

static SmListT* _service_domain_neighbors = NULL;
static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Domain Neighbor Table - Read
// ====================================
SmServiceDomainNeighborT* sm_service_domain_neighbor_table_read( 
    char name[], char service_domain_name[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainNeighborT* neighbor;

    SM_LIST_FOREACH( _service_domain_neighbors, entry, entry_data )
    {
        neighbor = (SmServiceDomainNeighborT*) entry_data;

        if(( 0 == strcmp( name, neighbor->name ) )&&
           ( 0 == strcmp( service_domain_name, neighbor->service_domain ) ))
        {
            return( neighbor );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Read By Identifier
// ==================================================
SmServiceDomainNeighborT* sm_service_domain_neighbor_table_read_by_id( 
    int64_t id )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainNeighborT* neighbor;

    SM_LIST_FOREACH( _service_domain_neighbors, entry, entry_data )
    {
        neighbor = (SmServiceDomainNeighborT*) entry_data;

        if( id == neighbor->id )
        {
            return( neighbor );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - For Each
// ========================================
void sm_service_domain_neighbor_table_foreach( char service_domain_name[],
    void* user_data[], SmServiceDomainNeighborTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainNeighborT* neighbor;

    SM_LIST_FOREACH( _service_domain_neighbors, entry, entry_data )
    {
        neighbor = (SmServiceDomainNeighborT*) entry_data;

        if( service_domain_name == neighbor->service_domain )
        {
            callback( user_data, neighbor );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Add
// ===================================
static SmErrorT sm_service_domain_neighbor_table_add( void* user_data[],
    void* record )
{
    SmServiceDomainNeighborT* neighbor;
    SmDbServiceDomainNeighborT* db_neighbor;

    db_neighbor = (SmDbServiceDomainNeighborT*) record;

    neighbor = sm_service_domain_neighbor_table_read( db_neighbor->name,
                                                db_neighbor->service_domain );
    if( NULL == neighbor )
    {
        neighbor = (SmServiceDomainNeighborT*) 
                    malloc( sizeof(SmServiceDomainNeighborT) );
        if( NULL == neighbor )
        {
            DPRINTFE( "Failed to allocate service domain neighbor table "
                      "entry." );
            return( SM_FAILED );
        }

        memset( neighbor, 0, sizeof(SmServiceDomainNeighborT) );

        neighbor->id = db_neighbor->id;
        snprintf( neighbor->name, sizeof(neighbor->name), "%s", 
                  db_neighbor->name );
        snprintf( neighbor->service_domain, sizeof(neighbor->service_domain),
                  "%s", db_neighbor->service_domain );
        snprintf( neighbor->orchestration, sizeof(neighbor->orchestration),
                  "%s", db_neighbor->orchestration );
        snprintf( neighbor->designation, sizeof(neighbor->designation),
                  "%s", db_neighbor->designation );
        neighbor->generation = db_neighbor->generation;        
        neighbor->priority = db_neighbor->priority;
        neighbor->hello_interval = db_neighbor->hello_interval;
        neighbor->dead_interval = db_neighbor->dead_interval;
        neighbor->wait_interval = db_neighbor->wait_interval;
        neighbor->exchange_interval = db_neighbor->exchange_interval;
        neighbor->state = db_neighbor->state;
        neighbor->exchange_master = false;
        neighbor->exchange_seq = 0;
        neighbor->exchange_last_sent_id = 0;
        neighbor->exchange_last_recvd_id = 0;
        neighbor->exchange_timer_id = SM_TIMER_ID_INVALID;
        neighbor->dead_timer_id = SM_TIMER_ID_INVALID;
        neighbor->pause_timer_id = SM_TIMER_ID_INVALID;

        SM_LIST_PREPEND( _service_domain_neighbors, 
                         (SmListEntryDataPtrT) neighbor );

    } else { 
        neighbor->id = db_neighbor->id;
        snprintf( neighbor->service_domain, sizeof(neighbor->service_domain),
                  "%s", db_neighbor->service_domain );
        snprintf( neighbor->orchestration, sizeof(neighbor->orchestration),
                  "%s", db_neighbor->orchestration );
        snprintf( neighbor->designation, sizeof(neighbor->designation),
                  "%s", db_neighbor->designation );
        neighbor->generation = db_neighbor->generation;
        neighbor->priority = db_neighbor->priority;
        neighbor->hello_interval = db_neighbor->hello_interval;
        neighbor->dead_interval = db_neighbor->dead_interval;
        neighbor->wait_interval = db_neighbor->wait_interval;
        neighbor->exchange_interval = db_neighbor->exchange_interval;
        neighbor->state = db_neighbor->state;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Insert
// ======================================
SmErrorT sm_service_domain_neighbor_table_insert( char name[],
    char service_domain_name[], char orchestration[], char designation[],
    int generation, int priority, int hello_interval, int dead_interval,
    int wait_interval, int exchange_interval )
{
    SmServiceDomainNeighborT* neighbor;
    SmDbServiceDomainNeighborT db_neighbor;
    SmErrorT error;

    memset( &db_neighbor, 0, sizeof(SmDbServiceDomainNeighborT) );

    snprintf( db_neighbor.name, sizeof(db_neighbor.name), "%s", name );
    snprintf( db_neighbor.service_domain, sizeof(db_neighbor.service_domain),
              "%s", service_domain_name );
    snprintf( db_neighbor.orchestration, sizeof(db_neighbor.orchestration),
              "%s", orchestration );
    snprintf( db_neighbor.designation, sizeof(db_neighbor.designation),
              "%s", designation );
    db_neighbor.generation = generation;
    db_neighbor.priority = priority;
    db_neighbor.hello_interval = hello_interval;
    db_neighbor.dead_interval = dead_interval;
    db_neighbor.wait_interval = wait_interval;
    db_neighbor.exchange_interval = exchange_interval;
    db_neighbor.state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN;

    error = sm_db_service_domain_neighbors_insert( _sm_db_handle,
                                                   &db_neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to insert neighbor (%s) for service domain "
                  "(%s), error=%s.", name, service_domain_name,
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_neighbors_read( _sm_db_handle, name,
                                        service_domain_name, &db_neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to read neighbor (%s) for service domain "
                  "(%s), error=%s.", name, service_domain_name,
                  sm_error_str( error ) );
        return( error );
    }

    neighbor = (SmServiceDomainNeighborT*)
                 malloc( sizeof(SmServiceDomainNeighborT) );
    if( NULL == neighbor )
    {
        DPRINTFE( "Failed to allocate service domain neighbor table entry." );
        return( SM_FAILED );
    }

    neighbor->id = db_neighbor.id;
    snprintf( neighbor->name, sizeof(neighbor->name), "%s", 
              db_neighbor.name );
    snprintf( neighbor->service_domain, sizeof(neighbor->service_domain),
              "%s", db_neighbor.service_domain );
    snprintf( neighbor->orchestration, sizeof(neighbor->orchestration),
              "%s", db_neighbor.orchestration );
    snprintf( neighbor->designation, sizeof(neighbor->designation),
              "%s", db_neighbor.designation );
    neighbor->generation = db_neighbor.generation;
    neighbor->priority = db_neighbor.priority;
    neighbor->hello_interval = db_neighbor.hello_interval;
    neighbor->dead_interval = db_neighbor.dead_interval;
    neighbor->wait_interval = db_neighbor.wait_interval;
    neighbor->exchange_interval = db_neighbor.exchange_interval;
    neighbor->state = db_neighbor.state;
    neighbor->exchange_master = false;
    neighbor->exchange_seq = 0;
    neighbor->exchange_last_sent_id = 0;
    neighbor->exchange_last_recvd_id = 0;
    neighbor->exchange_timer_id = SM_TIMER_ID_INVALID;
    neighbor->dead_timer_id = SM_TIMER_ID_INVALID;
    neighbor->pause_timer_id = SM_TIMER_ID_INVALID;

    SM_LIST_PREPEND( _service_domain_neighbors,
                     (SmListEntryDataPtrT) neighbor );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Load
// ====================================
SmErrorT sm_service_domain_neighbor_table_load( void )
{
    SmDbServiceDomainNeighborT neighbor;
    SmErrorT error;

    error = sm_db_foreach( SM_DATABASE_NAME, 
                           SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_NAME,
                           NULL, &neighbor, 
                           sm_db_service_domain_neighbors_convert,
                           sm_service_domain_neighbor_table_add, NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over service domain neighbors in "
                  "database, error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Persist
// =======================================
SmErrorT sm_service_domain_neighbor_table_persist(
    SmServiceDomainNeighborT* neighbor )
{
    SmDbServiceDomainNeighborT db_neighbor;
    SmErrorT error;

    memset( &db_neighbor, 0, sizeof(db_neighbor) );

    db_neighbor.id = neighbor->id;
    snprintf( db_neighbor.name, sizeof(db_neighbor.name), "%s", 
              neighbor->name );
    snprintf( db_neighbor.service_domain, sizeof(db_neighbor.service_domain),
              "%s", neighbor->service_domain );
    snprintf( db_neighbor.orchestration, sizeof(db_neighbor.orchestration),
              "%s", neighbor->orchestration );
    snprintf( db_neighbor.designation, sizeof(db_neighbor.designation),
              "%s", neighbor->designation );
    db_neighbor.generation = neighbor->generation;
    db_neighbor.priority = neighbor->priority;
    db_neighbor.hello_interval = neighbor->hello_interval;
    db_neighbor.dead_interval = neighbor->dead_interval;
    db_neighbor.wait_interval = neighbor->wait_interval;
    db_neighbor.exchange_interval = neighbor->exchange_interval;
    db_neighbor.state = neighbor->state;

    error = sm_db_service_domain_neighbors_update( _sm_db_handle,
                                                    &db_neighbor );
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
// Service Domain Neighbor Table - Initialize
// ==========================================
SmErrorT sm_service_domain_neighbor_table_initialize( void )
{
    SmErrorT error;

    _service_domain_neighbors = NULL;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_neighbor_table_load();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to load service domain neighbors from database, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Finalize
// ========================================
SmErrorT sm_service_domain_neighbor_table_finalize( void )
{
    SmErrorT error;

    SM_LIST_CLEANUP_ALL( _service_domain_neighbors );

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
