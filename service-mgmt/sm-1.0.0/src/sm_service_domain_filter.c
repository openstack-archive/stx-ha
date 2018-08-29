//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_filter.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_time.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_nodes.h"
#include "sm_service_domain_member_table.h"
#include "sm_service_domain_neighbor_table.h"
#include "sm_service_domain_assignment_table.h"
#include "sm_node_api.h"
#include "sm_node_swact_monitor.h"
#include "sm_node_utils.h"
#include "sm_failover_utils.h"

static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Domain Filter - Cleanup
// ===============================
static void sm_service_domain_filter_cleanup( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED;
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - By Node Ready
// =====================================
static void sm_service_domain_filter_by_node_ready( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    SmServiceDomainFilterCountsT* counts; 
    SmDbNodeT node;
    SmServiceDomainSchedulingListT list;
    SmErrorT error;

    counts = (SmServiceDomainFilterCountsT*) user_data[0];

    list = assignment->sched_list;

    error = sm_db_nodes_read( _sm_db_handle, assignment->node_name, &node );
    if( SM_OKAY == error )
    {

        if( SM_NODE_READY_STATE_ENABLED != node.ready_state )
        {
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE;
            ++(counts->unavailable_members);
        }
    } else if( SM_NOT_FOUND == error ) {
        list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE;
        ++(counts->unavailable_members);

    } else {
        DPRINTFE( "Failed to read node (%s) info, error = %s.", 
                  assignment->node_name, sm_error_str(error) );
        return;
    }

    if( list != assignment->sched_list )
    {
        assignment->sched_list = list;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - By Node Administrative State
// ====================================================
static void sm_service_domain_filter_by_node_admin_state( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    SmServiceDomainFilterCountsT* counts; 
    SmDbNodeT node;
    SmServiceDomainSchedulingListT list;
    SmErrorT error;

    counts = (SmServiceDomainFilterCountsT*) user_data[0];

    list = assignment->sched_list;

    error = sm_db_nodes_read( _sm_db_handle, assignment->node_name, &node );
    if( SM_OKAY == error )
    {
        if( SM_NODE_ADMIN_STATE_LOCKED == node.admin_state )
        {
#ifdef SM_CONFIG_OPTION_STANDBY_ALL_SERVICES_ON_A_LOCKED_NODE
            switch( list )
            {
                case SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE:
                    --(counts->active_members);
                    list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY;
                    ++(counts->go_standby_members);
                break;
                case SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE:
                    --(counts->go_active_members);
                    list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY;
                    ++(counts->go_standby_members);
                break;
                default:
                    //Ignore.
                break;
            }
#else
            // Adjust counts.
            switch( list )
            {
                case SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE:
                    --(counts->active_members);
                break;
                case SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE:
                    --(counts->go_active_members);
                break;
                case SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY:
                    --(counts->go_standby_members);
                break;
                case SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY:
                    --(counts->standby_members);
                break;
                case SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING:
                    --(counts->disabling_members);
                break;
                case SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED:
                    --(counts->disabled_members);
                break;
                case SM_SERVICE_DOMAIN_SCHEDULING_LIST_FAILED:
                    --(counts->failed_members);
                break;
                case SM_SERVICE_DOMAIN_SCHEDULING_LIST_FATAL:
                case SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE:
                default:
                    //Ignore.
                break;
            }

            if(( SM_SERVICE_DOMAIN_SCHEDULING_LIST_FATAL != list )&&
               ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE != list ))
            {
                list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE;
                ++(counts->unavailable_members);
            }
#endif // SM_CONFIG_OPTION_STANDBY_ALL_SERVICES_ON_A_LOCKED_NODE
        }
    } else if( SM_NOT_FOUND != error ) {
        DPRINTFE( "Failed to read node (%s) info, error = %s.", 
                  assignment->node_name, sm_error_str(error) );
        return;
    }

    if( list != assignment->sched_list )
    {
        assignment->sched_list = list;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - By Neighbor
// ===================================
static void sm_service_domain_filter_by_neighbor( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    SmServiceDomainFilterCountsT* counts; 
    SmServiceDomainSchedulingListT list;
    SmServiceDomainNeighborT* neighbor;
    SmErrorT error;

    counts = (SmServiceDomainFilterCountsT*) user_data[0];

    list = assignment->sched_list;

    neighbor = sm_service_domain_neighbor_table_read( assignment->node_name,
                                                      assignment->name );
    if( NULL != neighbor )
    {
        if( SM_SERVICE_DOMAIN_NEIGHBOR_STATE_FULL != neighbor->state )
        {
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE;
            ++(counts->unavailable_members);

        } else {
            if( SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE == list )
            {
                list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED;
                // Don't increment here as the counts will be 
                // thrown off later, assignment filter will do
                // the increment.
            }
        }
    } else {
        char hostname[SM_NODE_NAME_MAX_CHAR];
    
        error = sm_node_api_get_hostname( hostname );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to get hostname, error=%s.",
                      sm_error_str( error ) );
            return;
        }

        if( 0 != strcmp( hostname, assignment->node_name ) )
        {
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE;
            ++(counts->unavailable_members);
        }
    }

    if( list != assignment->sched_list )
    {
        assignment->sched_list = list;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - By Assignment
// =====================================
static void sm_service_domain_filter_by_assignment( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    SmServiceDomainFilterCountsT* counts; 
    SmServiceDomainSchedulingListT list;

    counts = (SmServiceDomainFilterCountsT*) user_data[0];

    list = assignment->sched_list;

    ++(counts->total_members);

    // Previous filter might have already decided that this assignment
    // needs to be put on the unavailable or disabled list (node checks).
    if( SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE == assignment->sched_list )
    {
        DPRINTFD( "Member assignment (%s) for service domain (%s) node (%s) "
                  "is unavailable.", assignment->service_group_name,
                  assignment->name, assignment->node_name );
        return;
    }

    // The scheduler decided that this assignment needs to be disabled.
    if( SM_SERVICE_DOMAIN_SCHEDULING_STATE_DISABLE == assignment->sched_state )
    {
        // Assignment is failed, move to the failed scheduling list.
        if( SM_SERVICE_GROUP_STATUS_FAILED == assignment->status )
        {
            // Fatal condition is looked at later when the health of all
            // assignments is compared.
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_FAILED;
            ++(counts->failed_members);
        } else {
            // Put it on the unavailable list, so that it can't be chosen.
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE;
            ++(counts->unavailable_members);
        }
        goto UPDATE;
    }

    // Request to swact from this assignment has been indicated.
    if(( SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT == assignment->sched_state )||
       ( SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT_FORCE == assignment->sched_state ))
    {
        if(( SM_SERVICE_GROUP_STATE_ACTIVE    == assignment->state )||
           ( SM_SERVICE_GROUP_STATE_GO_ACTIVE == assignment->state ))
        {
            SmServiceDomainMemberT* member;

            member = sm_service_domain_member_table_read( assignment->name,
                                            assignment->service_group_name );
            if( NULL == member )
            {
                list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY;
                ++(counts->go_standby_members);

                DPRINTFE( "Failed to read service domain (%s) member (%s), "
                          "error=%s.", assignment->name, 
                          assignment->service_group_name, 
                          sm_error_str(SM_NOT_FOUND) );
                goto UPDATE;
            }

            if( 0 < member->m_standby )
            {
                list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY;
                ++(counts->go_standby_members);
            } else {
                list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED;
                ++(counts->disabled_members);
            }
        } else if( SM_SERVICE_GROUP_STATE_STANDBY == assignment->state ) {
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY;
            ++(counts->standby_members);

        } else {
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED;
            ++(counts->disabled_members);
        }
        goto UPDATE;
    }

    // Assignment is failed, move to the failed scheduling list.
    if( SM_SERVICE_GROUP_STATUS_FAILED == assignment->status )
    {
        // Fatal condition is looked at later when the health of all
        // assignments is compared.
        char hostname[SM_NODE_NAME_MAX_CHAR];
        SmErrorT error = sm_node_utils_get_hostname( hostname );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to get hostname, error=%s.",
                      sm_error_str( error ) );
            hostname[0] = '\0';
            goto UPDATE;
        }
        SmNodeScheduleStateT current_schedule_state = sm_get_controller_state( hostname );
        SmNodeScheduleStateT to_schedule_state;
        if(0 == strcmp(hostname, assignment->node_name))
        {
            to_schedule_state = SM_NODE_STATE_STANDBY;
        }else
        {
            to_schedule_state = SM_NODE_STATE_ACTIVE;
        }
        if(current_schedule_state != to_schedule_state)
        {
            DPRINTFI("Uncontrolled swact start");
            SmNodeSwactMonitor::SwactStart(to_schedule_state);
        }

        list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_FAILED;
        ++(counts->failed_members);
        goto UPDATE;
    }

    // Select scheduling list based on desired-state and operational state.
    if( SM_SERVICE_GROUP_STATE_ACTIVE == assignment->desired_state )
    {
        if( SM_SERVICE_GROUP_STATE_ACTIVE == assignment->state )
        {
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE;
            ++(counts->active_members);
        } else {
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE;
            ++(counts->go_active_members);
        }
    }
    else if( SM_SERVICE_GROUP_STATE_STANDBY == assignment->desired_state )
    {
        if( SM_SERVICE_GROUP_STATE_STANDBY == assignment->state )
        {
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY;
            ++(counts->standby_members);
        } else {
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY;
            ++(counts->go_standby_members);
        }
    }
    else if( SM_SERVICE_GROUP_STATE_DISABLED == assignment->desired_state )
    {
        if( SM_SERVICE_GROUP_STATE_DISABLED == assignment->state )
        {
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED;
            ++(counts->disabled_members);
        } else {
            list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING;
            ++(counts->disabling_members);
        }
    }
    else 
    {
        list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED;
        ++(counts->disabled_members);
    }

UPDATE:
    if( list != assignment->sched_list )
    {
        assignment->sched_list = list;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Count Healthier Assignments
// ===================================================
static void sm_service_domain_filter_count_healthier_assignments(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    int* count;
    SmDbNodeT node;
    SmServiceDomainAssignmentT* target_assignment;

    count = (int*) user_data[0];
    target_assignment = (SmServiceDomainAssignmentT*) user_data[1];
    SmErrorT error;

    error = sm_db_nodes_read( _sm_db_handle, assignment->node_name, &node );
    if( SM_OKAY == error )
    {
        if( SM_NODE_ADMIN_STATE_LOCKED == node.admin_state )
        {
            DPRINTFD( "Node (%s) is locked.", assignment->node_name );
            return;
        }
    } else if( SM_NOT_FOUND == error ) {
        DPRINTFD( "Node (%s) not found.", assignment->node_name );
        return;
    } else {
        DPRINTFE( "Failed to read node (%s) info, error = %s.", 
                  assignment->node_name, sm_error_str(error) );
        return;
    }

    if(( 0 == strcmp( assignment->name, target_assignment->name ) )&&
       ( 0 != strcmp( assignment->node_name, target_assignment->node_name ) )&&
       ( 0 == strcmp( assignment->service_group_name, 
                      target_assignment->service_group_name ) )&&
       ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_FAILED != assignment->sched_list )&&
       ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_FATAL != assignment->sched_list )&&
       ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE != assignment->sched_list )&&
       ( assignment->health > target_assignment->health ))
    {
        ++(*count);
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - By Assignment Health
// ============================================
static void sm_service_domain_filter_by_assignment_health( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    int count = 0;
    char* service_domain_name = (char*) user_data[1];
    SmServiceDomainFilterCountsT* counts;
    SmServiceDomainMemberT* member;
    void* user_data2[] = {&count, assignment };

    counts = (SmServiceDomainFilterCountsT*) user_data[0];

    if( SM_SERVICE_DOMAIN_SCHEDULING_LIST_FAILED != assignment->sched_list )
    {
        // Only looking at the failed list.
        return;
    }

    member = sm_service_domain_member_table_read( assignment->name,
                                            assignment->service_group_name );
    if( NULL == member )
    {
        DPRINTFE( "Failed to read service domain (%s) member (%s), error=%s.",
                  assignment->name, assignment->service_group_name,
                  sm_error_str(SM_NOT_FOUND) );
        return;
    }

    sm_service_domain_assignment_table_foreach( service_domain_name,
            user_data2, sm_service_domain_filter_count_healthier_assignments );

    if( member->n_active <= count )
    {
        // Healthier assignment available.
        switch( assignment->condition )
        {
            case SM_SERVICE_GROUP_CONDITION_ACTION_FAILURE:
                // Already on the failed scheduling list.
            break;

            case SM_SERVICE_GROUP_CONDITION_FATAL_FAILURE:
                assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_FATAL;
                --(counts->failed_members);
                ++(counts->fatal_members);
            break;

            default:
                assignment->sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_DISABLE;
                assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE;
                --(counts->failed_members);
                ++(counts->unavailable_members);
            break;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Service Group Aggregate Recover
// =======================================================
static SmErrorT sm_service_domain_filter_service_group_aggregate_recover(
    bool* recover, SmServiceDomainAssignmentT* assignment )
{
    if(( SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT == assignment->sched_state )&&
       ( SM_SERVICE_GROUP_CONDITION_ACTION_FAILURE == assignment->condition  ))
    {    
        if((( SM_SERVICE_GROUP_STATE_STANDBY == assignment->desired_state )&&
            ( SM_SERVICE_GROUP_STATE_GO_STANDBY == assignment->state ))||
           (( SM_SERVICE_GROUP_STATE_DISABLED == assignment->desired_state )&&
            ( SM_SERVICE_GROUP_STATE_DISABLING == assignment->state )))
        {
            // Fail back to from-side back.
            *recover = true;
            DPRINTFE( "Failback service domain (%s) member (%s).",
                      assignment->name, assignment->service_group_name );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Service Group Aggregate Recover Apply
// =============================================================
static SmErrorT sm_service_domain_filter_service_group_aggregate_recover_apply(
    SmServiceDomainFilterCountsT* counts,
    SmServiceDomainAssignmentT* assignment )
{
    assignment->sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_NONE;

    switch( assignment->sched_list )
    {
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE:
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE:
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE:
            // Nothing to recover.
        break;
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY:
            --(counts->standby_members);
            ++(counts->go_active_members);
            assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE;
        break;
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY:
            --(counts->go_standby_members);
            ++(counts->go_active_members);
            assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE;
        break;
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED:
            --(counts->disabled_members);
            ++(counts->go_active_members);
            assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE;
        break;
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING:
            --(counts->disabling_members);
            ++(counts->go_active_members);
            assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE;
        break;
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_FAILED:
            --(counts->failed_members);
            ++(counts->go_active_members);
            assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE;
        break;
        default:
            //Ignore.
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Get Service Group Aggregate List
// ========================================================
static SmErrorT sm_service_domain_filter_get_service_group_aggregate_list(
    SmServiceDomainSchedulingListT* list,
    SmServiceDomainAssignmentT* assignment )
{
    // Failed and Fatal scheduling list is not checked below.  This is to 
    // allow partial service scenarios, otherwise we would pull down all
    // co-located assignments.

    if( SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE == *list )
    {
        if(( SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE   == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY  == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY     == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING   == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED    == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE == assignment->sched_list ))
        {
            *list = assignment->sched_list;
        }
    } else if( SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE == *list )
    {
        if(( SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY  == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY     == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING   == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED    == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE == assignment->sched_list ))
        {
            *list = assignment->sched_list;
        }
    } else if( SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY == *list )
    {
        if(( SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY     == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING   == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED    == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE == assignment->sched_list ))
        {
            *list = assignment->sched_list;
        }
    } else if( SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY == *list )
    {
        if(( SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING   == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED    == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE == assignment->sched_list ))
        {
            *list = assignment->sched_list;
        }
    } else if( SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING == *list )
    {
        if(( SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED    == assignment->sched_list )||
           ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE == assignment->sched_list ))
        {
            *list = assignment->sched_list;
        }
    } else if( SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED == *list )
    {
        if( SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE == assignment->sched_list )
        {
            *list = assignment->sched_list;
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Apply Service Group Aggregate List
// ==========================================================
static SmErrorT sm_service_domain_filter_apply_service_group_aggregate_list(
    SmServiceDomainFilterCountsT* counts, SmServiceDomainSchedulingListT* list,
    SmServiceDomainAssignmentT* assignment )
{
    if(( *list != assignment->sched_list )&&
       ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_FAILED != assignment->sched_list )&&
       ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_FATAL  != assignment->sched_list ))
    {
        // Adjust counts.
        switch( assignment->sched_list )
        {
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE:
                --(counts->active_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE:
                --(counts->go_active_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY:
                --(counts->go_standby_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY:
                --(counts->standby_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING:
                --(counts->disabling_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED:
                --(counts->disabled_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE:
                --(counts->unavailable_members);
            break;
            default:
                //Ignore.
            break;
        }

        switch( *list )
        {
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE:
                ++(counts->active_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE:
                ++(counts->go_active_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY:
                ++(counts->go_standby_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY:
                ++(counts->standby_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING:
                ++(counts->disabling_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED:
                ++(counts->disabled_members);
            break;
            case SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE:
                ++(counts->unavailable_members);
            break;
            default:
                //Ignore.
            break;
        }

        assignment->sched_list = *list;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Service Group Aggregate Member
// ======================================================
void sm_service_domain_filter_service_group_aggregate_member( void* user_data[],
    SmServiceDomainMemberT* member )
{
    bool apply;
    bool* recover;
    SmDbNodeT* node;
    char* service_domain_name;
    SmServiceDomainFilterCountsT* counts;
    SmServiceDomainSchedulingListT* list;
    SmServiceDomainAssignmentT* assignment;
    SmErrorT error;

    counts = (SmServiceDomainFilterCountsT*) user_data[0];
    service_domain_name = (char*) user_data[1];
    node = (SmDbNodeT*) user_data[2];
    list = (SmServiceDomainSchedulingListT*) user_data[3];
    apply = *(bool*) user_data[4];
    recover = (bool*) user_data[5];

    assignment = sm_service_domain_assignment_table_read( service_domain_name,
                                    node->name, member->service_group_name );
    if( NULL == assignment )
    {
        // Possible that the node has been provisioned, but not configured yet.
        // This is possible during initial commissioning of the second
        // controller.
        DPRINTFD( "Failed to query service domain (%s) assignment (%s) for "
                  "node (%s), error=%s.", member->name, 
                  member->service_group_name, node->name,
                  sm_error_str(SM_NOT_FOUND) );
        return;
    }

    if( apply )
    {
        if( *recover )
        {
            error = sm_service_domain_filter_service_group_aggregate_recover_apply(
                        counts, assignment );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to apply service domain (%s) assignment (%s) "
                          "recovery for node (%s), error=%s.", member->name,
                          member->service_group_name, node->name,
                          sm_error_str( error ) );
                return;
            }
        } else {
            error = sm_service_domain_filter_apply_service_group_aggregate_list(
                        counts, list, assignment );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to apply service domain (%s) assignment (%s) "
                          "list for node (%s), error=%s.", member->name,
                          member->service_group_name, node->name,
                          sm_error_str( error ) );
                return;
            }
        }
    } else {
        // First determine if recovery is needed.
        error = sm_service_domain_filter_service_group_aggregate_recover(
                    recover, assignment );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to determine if recovery of service domain (%s) "
                      "assignment (%s) is needed for node (%s), error=%s.",
                      member->name, member->service_group_name, node->name,
                      sm_error_str( error ) );
            return;
        }

        if( false == *recover )
        {
            // Recovery not needed, get the common list among the service
            // group aggregate.
            error = sm_service_domain_filter_get_service_group_aggregate_list(
                        list, assignment );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to get service domain (%s) assignment (%s) "
                          "list for node (%s), error=%s.", member->name,
                          member->service_group_name, node->name,
                          sm_error_str( error ) );
                return;
            }
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Service Group Aggregates
// ================================================
void sm_service_domain_filter_service_group_aggregates( void* user_data[],
    SmServiceDomainMemberT* member )
{
    bool apply = false;
    bool recover = false;
    char* service_domain_name = (char*) user_data[1];
    SmServiceDomainSchedulingListT list;
    void* user_data2[] = { user_data[0], user_data[1], user_data[2], 
                           &list, &apply, &recover };

    if( '\0' == member->service_group_aggregate[0] )
    {
        DPRINTFD( "Not aggregated with other service groups." );
        return;
    }

    list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE;

    sm_service_domain_member_table_foreach_service_group_aggregate(
        service_domain_name, member->service_group_aggregate, user_data2,
        sm_service_domain_filter_service_group_aggregate_member );

    apply = true;

    sm_service_domain_member_table_foreach_service_group_aggregate(
            service_domain_name, member->service_group_aggregate, user_data2,
            sm_service_domain_filter_service_group_aggregate_member );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - By Service Group Aggregate
// ==================================================
SmErrorT sm_service_domain_filter_by_service_group_aggregate( void* user_data[],
    void* record )
{
    char* service_domain_name = (char*) user_data[1];
    SmDbNodeT* node = (SmDbNodeT*) record;
    void* user_data2[] = { user_data[0], user_data[1], node };

    sm_service_domain_member_table_foreach( service_domain_name, user_data2,
                            sm_service_domain_filter_service_group_aggregates );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Preselect Apply
// =======================================
SmErrorT sm_service_domain_filter_preselect_apply( char service_domain_name[],
    SmServiceDomainFilterCountsT* counts )
{
    long ms_expired;
    SmTimeT time_prev, time_now;
    void* user_data[] = {counts, service_domain_name};
    SmDbNodeT node;
    SmErrorT error;

    memset( counts, 0, sizeof(SmServiceDomainFilterCountsT) );

    // Filter - Cleanup
    sm_time_get( &time_prev );

    sm_service_domain_assignment_table_foreach( service_domain_name,
                    user_data, sm_service_domain_filter_cleanup );

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - apply filter - cleanup, "
              "took %li ms.", ms_expired );

    // Filter - By Node Ready.
    sm_time_get( &time_prev );

    sm_service_domain_assignment_table_foreach( service_domain_name,
                    user_data, sm_service_domain_filter_by_node_ready );

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - apply filter - node enabled, "
              "took %li ms.", ms_expired );

   // Filter - By Neighbor. 
    sm_time_get( &time_prev );

    sm_service_domain_assignment_table_foreach( service_domain_name,
                    user_data, sm_service_domain_filter_by_neighbor );

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - apply filter - neighbor, "
              "took %li ms.", ms_expired );

    // Filter - By Assignments.
    sm_time_get( &time_prev );

    sm_service_domain_assignment_table_foreach( service_domain_name,
                    user_data, sm_service_domain_filter_by_assignment );

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - apply filter - assignments, "
              "took %li ms.", ms_expired );

    // Filter - By Assignment Health.
    sm_time_get( &time_prev );

    sm_service_domain_assignment_table_foreach( service_domain_name,
                    user_data, sm_service_domain_filter_by_assignment_health );

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - apply filter - assignment health, "
              "took %li ms.", ms_expired );

    // Filter - By Node Administrative State.
    sm_time_get( &time_prev );

    sm_service_domain_assignment_table_foreach( service_domain_name,
                    user_data, sm_service_domain_filter_by_node_admin_state );

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - apply filter - node admin state, "
              "took %li ms.", ms_expired );

    // Filter - By Service Group Aggregate.
    sm_time_get( &time_prev );

    error = sm_db_foreach( SM_DATABASE_NAME, SM_NODES_TABLE_NAME,
                           NULL, &node, &sm_db_nodes_convert,
                           sm_service_domain_filter_by_service_group_aggregate,
                           user_data );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over nodes, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - apply filter - service group "
              "aggregate, took %li ms.", ms_expired );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Post Select Apply
// =========================================
SmErrorT sm_service_domain_filter_post_select_apply( char service_domain_name[],
    SmServiceDomainFilterCountsT* counts )
{
    long ms_expired;
    SmTimeT time_prev, time_now;
    void* user_data[] = {counts, service_domain_name};
    SmDbNodeT node;
    SmErrorT error;

    // Filter - By Service Group Aggregate.
    sm_time_get( &time_prev );

    error = sm_db_foreach( SM_DATABASE_NAME, SM_NODES_TABLE_NAME,
                           NULL, &node, &sm_db_nodes_convert,
                           sm_service_domain_filter_by_service_group_aggregate,
                           user_data );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over nodes, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - post select apply filter - service group "
              "aggregate, took %li ms.", ms_expired );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Initialize
// ==================================
SmErrorT sm_service_domain_filter_initialize( SmDbHandleT* sm_db_handle )
{
    _sm_db_handle = sm_db_handle;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Finalize
// ================================
SmErrorT sm_service_domain_filter_finalize( void )
{
    _sm_db_handle = NULL;

    return( SM_OKAY );
}
// ****************************************************************************
