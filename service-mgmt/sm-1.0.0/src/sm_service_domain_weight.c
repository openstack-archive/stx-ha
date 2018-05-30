//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_weight.h"

#include <stdio.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_nodes.h"
#include "sm_service_domain_assignment_table.h"

static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Domain Weight - Cleanup
// ===============================
static void sm_service_domain_weight_cleanup( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    assignment->sched_weight = SM_SERVICE_DOMAIN_WEIGHT_MINIMUM;
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Weight - Assignments
// ===================================
static void sm_service_domain_weight_assignments( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    int weight = -1;

    switch( assignment->sched_list )
    {
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE:
            if( SM_SERVICE_GROUP_STATE_ACTIVE == assignment->desired_state )
            {
                if( SM_SERVICE_GROUP_STATE_ACTIVE == assignment->state )
                {
                    weight = 50;
                } else {
                    weight = 25;
                }
            }
        break;

        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY:
            if( SM_SERVICE_GROUP_STATE_STANDBY == assignment->desired_state )
            {
                if( SM_SERVICE_GROUP_STATE_STANDBY == assignment->state )
                {
                    weight = 50;
                } else {
                    weight = 25;
                }
            }
        break;

        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED:
            if( SM_SERVICE_GROUP_STATE_DISABLED == assignment->desired_state )
            {
                if( SM_SERVICE_GROUP_STATE_DISABLED == assignment->state )
                {
                    weight = 50;
                } else {
                    weight = 25;
                }
            }
        break;

        default:
            // Ignore.
        break;
    }

    if( -1 != weight )
    {
        assignment->sched_weight += weight;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Weight -  Add Location
// =====================================
static void sm_service_domain_weight_add_location( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    int weight_multipler = *(int*) user_data[0];
    SmDbNodeT* node = (SmDbNodeT*) user_data[1];

#ifdef SM_CONFIG_OPTION_STANDBY_ALL_SERVICES_ON_A_LOCKED_NODE
    if( SM_NODE_ADMIN_STATE_LOCKED == node->admin_state )
    {
        assignment->sched_weight = SM_SERVICE_DOMAIN_WEIGHT_UNSELECTABLE_ACTIVE;
    } else {
        if( SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT != assignment->sched_state )
        {
            assignment->sched_weight += weight_multipler * 100;
        }
    }
#else
    assignment->sched_weight += weight_multipler * 100;
#endif // SM_CONFIG_OPTION_STANDBY_ALL_SERVICES_ON_A_LOCKED_NODE
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Weight -  Add Location
// =====================================
static void sm_service_domain_weight_check_availability( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    SmDbNodeT* node = (SmDbNodeT*) user_data[0];

    if( SM_NODE_AVAIL_STATUS_FAILED == node->avail_status )
    {
        DPRINTFD("Node %s is FAILED", node->name);
        assignment->sched_weight = SM_SERVICE_DOMAIN_WEIGHT_UNSELECTABLE_ACTIVE;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Weight - By Location
// ===================================
static SmErrorT sm_service_domain_weight_by_location( void* user_data[],
    void* record )
{
    char* service_domain_name = (char*) user_data[0];
    int* weight_multipler = (int*) user_data[1];    
    SmDbNodeT* node = (SmDbNodeT*) record;
    void* user_data2[] = {weight_multipler, node};

    sm_service_domain_assignment_table_foreach_node_in_service_domain(
                                service_domain_name, node->name, user_data2,
                                sm_service_domain_weight_add_location );

    ++(*weight_multipler);

    return( SM_OKAY );
}
// ****************************************************************************

static SmErrorT sm_service_domain_weight_by_availability(void* user_data[],
    void* record )
{
    char* service_domain_name = (char*) user_data[0];
    SmDbNodeT* node = (SmDbNodeT*) record;
    void* user_data2[] = {node};

    sm_service_domain_assignment_table_foreach_node_in_service_domain(
                                service_domain_name, node->name, user_data2,
                                sm_service_domain_weight_check_availability );

    return( SM_OKAY );
}

// ****************************************************************************
// Service Domain Weight - Apply
// =============================
SmErrorT sm_service_domain_weight_apply( char service_domain_name[] )
{
    int weight_multipler = 1;
    SmDbNodeT node;
    void* user_data[] = {service_domain_name, &weight_multipler};
    SmErrorT error;

    // Weight - Cleanup
    sm_service_domain_assignment_table_foreach( service_domain_name,
                    user_data, sm_service_domain_weight_cleanup );

    // Weight - By Assignment.
    sm_service_domain_assignment_table_foreach( service_domain_name,
                    user_data, sm_service_domain_weight_assignments );

    // Weight - By Location.
    error = sm_db_foreach( SM_DATABASE_NAME, SM_NODES_TABLE_NAME,
                           NULL, &node, &sm_db_nodes_convert,
                           sm_service_domain_weight_by_location,
                           user_data );

    error = sm_db_foreach( SM_DATABASE_NAME, SM_NODES_TABLE_NAME,
                           NULL, &node, &sm_db_nodes_convert,
                           sm_service_domain_weight_by_availability,
                           user_data );

    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over nodes, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }
    
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Weight - Initialize
// ==================================
SmErrorT sm_service_domain_weight_initialize( SmDbHandleT* sm_db_handle )
{
    _sm_db_handle = sm_db_handle;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Weight - Finalize
// ================================
SmErrorT sm_service_domain_weight_finalize( void )
{
    _sm_db_handle = NULL;

    return( SM_OKAY );
}
// ****************************************************************************
