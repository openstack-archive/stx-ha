//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_assignment_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_service_domain_assignments.h"

static SmListT* _service_domain_assignments = NULL;
static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Domain Assignment Table - Read
// ======================================
SmServiceDomainAssignmentT* sm_service_domain_assignment_table_read( 
    char name[], char node_name[], char service_group_name[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainAssignmentT* assignment;

    SM_LIST_FOREACH( _service_domain_assignments, entry, entry_data )
    {
        assignment = (SmServiceDomainAssignmentT*) entry_data;

        if(( 0 == strcmp( name, assignment->name ) )&&
           ( 0 == strcmp( node_name, assignment->node_name ) )&&
           ( 0 == strcmp( service_group_name, assignment->service_group_name ) ))
        {
            return( assignment );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Read By Identifier
// ====================================================
SmServiceDomainAssignmentT* sm_service_domain_assignment_table_read_by_id(
    int64_t id )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainAssignmentT* assignment;

    SM_LIST_FOREACH( _service_domain_assignments, entry, entry_data )
    {
        assignment = (SmServiceDomainAssignmentT*) entry_data;

        if( id == assignment->id )
        {
            return( assignment );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Count
// =======================================
unsigned int sm_service_domain_assignment_table_count( 
    char service_domain_name[], void* user_data[],
    SmServiceDomainAssignmentTableCountCallbackT callback )
{
    unsigned int count = 0;
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainAssignmentT* assignment;

    SM_LIST_FOREACH( _service_domain_assignments, entry, entry_data )
    {
        assignment = (SmServiceDomainAssignmentT*) entry_data;

        if( 0 == strcmp( service_domain_name, assignment->name ) )
        {
            if( callback( user_data, assignment ) )
            {
                ++count;
            }
        }
    }

    return( count );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Count Service Group
// =====================================================
unsigned int sm_service_domain_assignment_table_count_service_group(
    char service_domain_name[], char service_group_name[], void* user_data[],
    SmServiceDomainAssignmentTableCountCallbackT callback )
{
    unsigned int count = 0;
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainAssignmentT* assignment;

    SM_LIST_FOREACH( _service_domain_assignments, entry, entry_data )
    {
        assignment = (SmServiceDomainAssignmentT*) entry_data;

        if(( 0 == strcmp( service_domain_name, assignment->name ) ) &&
           ( 0 == strcmp( service_group_name, assignment->service_group_name ) ))
        {
            if( callback( user_data, assignment ) )
            {
                ++count;
            }
        }
    }

    return( count );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - For Each
// ==========================================
void sm_service_domain_assignment_table_foreach( char service_domain_name[],
    void* user_data[], SmServiceDomainAssignmentTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListT* next = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainAssignmentT* assignment;

    SM_LIST_FOREACH_SAFE( _service_domain_assignments, entry, next, entry_data )
    {
        assignment = (SmServiceDomainAssignmentT*) entry_data;

        if( 0 == strcmp( service_domain_name, assignment->name ) )
        {
            callback( user_data, assignment );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - For Each Service Group
// ========================================================
void sm_service_domain_assignment_table_foreach_service_group(
    char service_domain_name[], char service_group_name[], void* user_data[],
    SmServiceDomainAssignmentTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListT* next = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainAssignmentT* assignment;

    SM_LIST_FOREACH_SAFE( _service_domain_assignments, entry, next, entry_data )
    {
        assignment = (SmServiceDomainAssignmentT*) entry_data;

        if(( 0 == strcmp( service_domain_name, assignment->name ) )&&
           ( 0 == strcmp( service_group_name, assignment->service_group_name ) ))
        {
            callback( user_data, assignment );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - For Each Node in Service Domain
// =================================================================
void sm_service_domain_assignment_table_foreach_node_in_service_domain(
    char service_domain_name[], char node_name[], void* user_data[],
    SmServiceDomainAssignmentTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListT* next = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainAssignmentT* assignment;

    SM_LIST_FOREACH_SAFE( _service_domain_assignments, entry, next, entry_data )
    {
        assignment = (SmServiceDomainAssignmentT*) entry_data;

        if(( 0 == strcmp( service_domain_name, assignment->name ) )&&
           ( 0 == strcmp( node_name, assignment->node_name ) ))
        {
            callback( user_data, assignment );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - For Each Node
// ===============================================
void sm_service_domain_assignment_table_foreach_node( char node_name[],
    void* user_data[], SmServiceDomainAssignmentTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListT* next = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainAssignmentT* assignment;

    SM_LIST_FOREACH_SAFE( _service_domain_assignments, entry, next, entry_data )
    {
        assignment = (SmServiceDomainAssignmentT*) entry_data;

        if( 0 == strcmp( node_name, assignment->node_name ) )
        {
            callback( user_data, assignment );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - For Each Schedule List
// ========================================================
void sm_service_domain_assignment_table_foreach_schedule_list(
    char service_domain_name[], SmServiceDomainSchedulingListT sched_list,
    void* user_data[], SmServiceDomainAssignmentTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListT* next = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainAssignmentT* assignment;

    SM_LIST_FOREACH_SAFE( _service_domain_assignments, entry, next, entry_data )
    {
        assignment = (SmServiceDomainAssignmentT*) entry_data;

        if(( 0 == strcmp( service_domain_name, assignment->name ) )&&
           ( sched_list == assignment->sched_list ))
        {
            callback( user_data, assignment );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Sort
// ======================================
void sm_service_domain_assignment_table_sort( 
    SmServiceDomainAssignmentTableCompareCallbackT callback )
{
    SM_LIST_SORT( _service_domain_assignments, callback );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Sort Assignments
// ===========================================
static int sm_service_domain_assignment_table_sort_by_id( const void* lhs,
    const void* rhs )
{
    SmServiceDomainAssignmentT* assignment_lhs;
    SmServiceDomainAssignmentT* assignment_rhs;

    assignment_lhs = (SmServiceDomainAssignmentT*) lhs;
    assignment_rhs = (SmServiceDomainAssignmentT*) rhs;

    if( assignment_lhs->id < assignment_rhs->id )
    {
        return( -1 );  // lhs before rhs
    }

    if( assignment_lhs->id > assignment_rhs->id )
    {
        return( 1 ); // lhs after rhs
    }

    return( 0 ); // lhs equal to rhs
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Get Next Node
// ===============================================
SmServiceDomainAssignmentT* sm_service_domain_assignment_table_get_next_node(
    char service_domain_name[], char node_name[], int64_t last_id )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainAssignmentT* assignment;

    SM_LIST_SORT( _service_domain_assignments,
                  sm_service_domain_assignment_table_sort_by_id );

    SM_LIST_FOREACH( _service_domain_assignments, entry, entry_data )
    {
        assignment = (SmServiceDomainAssignmentT*) entry_data;

        if(( 0 == strcmp( service_domain_name, assignment->name ) )&&
           ( 0 == strcmp( node_name, assignment->node_name ) )&&
           ( last_id < assignment->id ))
        {
            return( assignment );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Get Last Node
// ===============================================
SmServiceDomainAssignmentT* sm_service_domain_assignment_table_get_last_node(
    char service_domain_name[], char node_name[], int64_t last_id )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainAssignmentT* assignment;

    SM_LIST_SORT( _service_domain_assignments,
                  sm_service_domain_assignment_table_sort_by_id );

    SM_LIST_FOREACH( _service_domain_assignments, entry, entry_data )
    {
        assignment = (SmServiceDomainAssignmentT*) entry_data;

        if(( 0 == strcmp( service_domain_name, assignment->name ) )&&
           ( 0 == strcmp( node_name, assignment->node_name ) )&&
           ( last_id == assignment->id ))
        {
            return( assignment );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Add
// =====================================
static SmErrorT sm_service_domain_assignment_table_add( void* user_data[],
    void* record )
{
    SmServiceDomainAssignmentT* assignment;
    SmDbServiceDomainAssignmentT* db_assignment;
    struct timespec ts;

    db_assignment = (SmDbServiceDomainAssignmentT*) record;

    assignment = sm_service_domain_assignment_table_read(
                                        db_assignment->name,
                                        db_assignment->node_name,
                                        db_assignment->service_group_name );
    if( NULL == assignment )
    {
        assignment = (SmServiceDomainAssignmentT*)
                     malloc( sizeof(SmServiceDomainAssignmentT) );
        if( NULL == assignment )
        {
            DPRINTFE( "Failed to allocate service domain assignment table "
                      "entry." );
            return( SM_FAILED );
        }

        memset( assignment, 0, sizeof(SmServiceDomainAssignmentT) );

        clock_gettime( CLOCK_MONOTONIC_RAW, &ts );

        assignment->id = db_assignment->id;
        snprintf( assignment->uuid, sizeof(assignment->uuid), 
                  "%s", db_assignment->uuid );
        snprintf( assignment->name, sizeof(assignment->name), 
                  "%s", db_assignment->name );
        snprintf( assignment->node_name, sizeof(assignment->node_name),
                  "%s", db_assignment->node_name );
        snprintf( assignment->service_group_name,
                  sizeof(assignment->service_group_name), 
                  "%s", db_assignment->service_group_name );
        assignment->desired_state = db_assignment->desired_state;
        assignment->state = db_assignment->state;
        assignment->status = db_assignment->status;
        assignment->condition = db_assignment->condition;
        assignment->health = 0;
        assignment->last_state_change = (long) ts.tv_sec;
        assignment->sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_NONE;
        assignment->sched_weight = 0;
        assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_NIL;
        assignment->reason_text[0] = '\0';
        assignment->exchanged = false;

        SM_LIST_PREPEND( _service_domain_assignments,
                         (SmListEntryDataPtrT) assignment );
    } else { 
        assignment->id = db_assignment->id;
        snprintf( assignment->uuid, sizeof(assignment->uuid), 
                  "%s", db_assignment->uuid );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Insert
// ========================================
SmErrorT sm_service_domain_assignment_table_insert( char name[],
    char node_name[], char service_group_name[],
    SmServiceGroupStateT desired_state, SmServiceGroupStateT state,
    SmServiceGroupStatusT status, SmServiceGroupConditionT condition,
    int64_t health, long last_state_change, const char reason_text[] )
{
    SmServiceDomainAssignmentT* assignment;
    SmDbServiceDomainAssignmentT db_assignment;
    SmErrorT error;

    memset( &db_assignment, 0, sizeof(SmDbServiceDomainAssignmentT) );

    snprintf( db_assignment.name, sizeof(db_assignment.name), "%s", name );
    snprintf( db_assignment.node_name, sizeof(db_assignment.node_name), "%s",
              node_name );
    snprintf( db_assignment.service_group_name,
              sizeof(db_assignment.service_group_name), "%s", 
              service_group_name );
    db_assignment.desired_state = desired_state;
    db_assignment.state = state;
    db_assignment.status = status;
    db_assignment.condition = condition;

    error = sm_db_service_domain_assignments_insert( _sm_db_handle,
                                                     &db_assignment );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to insert assignment (%s) for service domain "
                  "(%s) node (%s), error=%s.", service_group_name, name,
                  node_name, sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_assignments_read( _sm_db_handle, name,
                            node_name, service_group_name, &db_assignment );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to read assignment (%s) for service domain "
                  "(%s) node (%s), error=%s.", service_group_name, name,
                  node_name, sm_error_str( error ) );
        return( error );
    }

    assignment = (SmServiceDomainAssignmentT*)
                 malloc( sizeof(SmServiceDomainAssignmentT) );
    if( NULL == assignment )
    {
        DPRINTFE( "Failed to allocate service domain assignment table entry." );
        return( SM_FAILED );
    }

    assignment->id = db_assignment.id;
    snprintf( assignment->uuid, sizeof(assignment->uuid), 
              "%s", db_assignment.uuid );
    snprintf( assignment->name, sizeof(assignment->name), 
              "%s", db_assignment.name );
    snprintf( assignment->node_name, sizeof(assignment->node_name),
              "%s", db_assignment.node_name );
    snprintf( assignment->service_group_name,
              sizeof(assignment->service_group_name), 
              "%s", db_assignment.service_group_name );
    assignment->desired_state = db_assignment.desired_state;
    assignment->state = db_assignment.state;
    assignment->status = db_assignment.status;
    assignment->condition = db_assignment.condition;
    assignment->health = health;
    assignment->last_state_change = last_state_change;
    assignment->sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_NONE;
    assignment->sched_weight = 0;
    assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_NIL;
    snprintf( assignment->reason_text, sizeof(assignment->reason_text),
              "%s", reason_text );
    assignment->exchanged = false;

    SM_LIST_PREPEND( _service_domain_assignments,
                 (SmListEntryDataPtrT) assignment );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Delete
// ========================================
SmErrorT sm_service_domain_assignment_table_delete( char name[],
    char node_name[], char service_group_name[] )
{
    SmServiceDomainAssignmentT* assignment;
    SmErrorT error;

    assignment = sm_service_domain_assignment_table_read( name, node_name,
                                                          service_group_name );
    if( NULL != assignment )
    {
        error = sm_db_service_domain_assignments_delete( _sm_db_handle, name,
                                            node_name, service_group_name );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to delete assignment (%s) for service domain "
                      "(%s) node (%s), error=%s.", service_group_name, name,
                      node_name, sm_error_str( error ) );
            return( error );
        }

        SM_LIST_REMOVE( _service_domain_assignments,
                        (SmListEntryDataPtrT) assignment );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Load
// ======================================
SmErrorT sm_service_domain_assignment_table_load( void )
{
    SmDbServiceDomainAssignmentT assignment;
    SmErrorT error;

    error = sm_db_foreach( SM_DATABASE_NAME, 
                           SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME,
                           NULL, &assignment,
                           sm_db_service_domain_assignments_convert,
                           sm_service_domain_assignment_table_add, NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over service domain assignments in "
                  "database, error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Persist
// =========================================
SmErrorT sm_service_domain_assignment_table_persist(
    SmServiceDomainAssignmentT* assignment )
{
    SmDbServiceDomainAssignmentT db_assignment;
    SmErrorT error;

    memset( &db_assignment, 0, sizeof(db_assignment) );

    db_assignment.id = assignment->id;
    snprintf( db_assignment.uuid, sizeof(db_assignment.uuid),
              "%s", assignment->uuid );
    snprintf( db_assignment.name, sizeof(db_assignment.name),
              "%s", assignment->name );
    snprintf( db_assignment.node_name, sizeof(db_assignment.node_name),
              "%s", assignment->node_name );
    snprintf( db_assignment.service_group_name,
              sizeof(db_assignment.service_group_name), 
              "%s", assignment->service_group_name );
    db_assignment.desired_state = assignment->desired_state;
    db_assignment.state = assignment->state;
    db_assignment.status = assignment->status;
    db_assignment.condition = assignment->condition;

    error = sm_db_service_domain_assignments_update( _sm_db_handle, 
                                                     &db_assignment );
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
// Service Domain Assignment Table - Initialize
// ============================================
SmErrorT sm_service_domain_assignment_table_initialize( void )
{
    SmErrorT error;

    _service_domain_assignments = NULL;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_assignment_table_load();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to load service domain assignments from database, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Finalize
// ==========================================
SmErrorT sm_service_domain_assignment_table_finalize( void )
{
    SmErrorT error;

    SM_LIST_CLEANUP_ALL( _service_domain_assignments );

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
