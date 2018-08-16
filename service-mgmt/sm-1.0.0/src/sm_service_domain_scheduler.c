//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_scheduler.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_time.h"
#include "sm_timer.h"
#include "sm_msg.h"
#include "sm_db.h"
#include "sm_db_nodes.h"
#include "sm_db_service_domain_neighbors.h"
#include "sm_node_api.h"
#include "sm_service_domain_table.h"
#include "sm_service_domain_member_table.h"
#include "sm_service_domain_assignment_table.h"
#include "sm_service_domain_utils.h"
#include "sm_service_group_api.h"
#include "sm_service_domain_filter.h"
#include "sm_service_domain_weight.h"
#include "sm_alarm.h"
#include "sm_log.h"

#define SM_SERVICE_DOMAIN_SCHEDULE_INTERVAL_IN_MS    3000

static SmDbHandleT* _sm_db_handle = NULL;
static SmTimerIdT _scheduler_timer_id = SM_TIMER_ID_INVALID;
static SmMsgCallbacksT _msg_callbacks = {0};

// ****************************************************************************
// Service Domain Scheduler - Set Scheduling State
// ===============================================
static void sm_service_domain_scheduler_set_assignment_sched_state(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    int* total_set;
    SmServiceDomainSchedulingStateT only_sched_state;
    SmServiceDomainSchedulingStateT sched_state;

    only_sched_state = *(SmServiceDomainSchedulingStateT*) user_data[0];
    sched_state = *(SmServiceDomainSchedulingStateT*) user_data[1];
    total_set = (int*) user_data[2];

    if( SM_SERVICE_DOMAIN_SCHEDULING_STATE_NIL != only_sched_state )
    {
        if(( only_sched_state == assignment->sched_state )&&
           ( sched_state != assignment->sched_state )) 
        {
            assignment->sched_state = sched_state;
            ++(*total_set);
        }
        return;
    }

    if(( SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT == sched_state )||
       ( SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT_FORCE == sched_state ))
    {
        SmServiceDomainMemberT* member;

        member = sm_service_domain_member_table_read( assignment->name,
                                            assignment->service_group_name );
        if( NULL == member )
        {
            DPRINTFE( "Failed to read member info for assignment (%s) for "
                      "service domain (%s) node (%s), error=%s.", 
                      assignment->service_group_name, assignment->name,
                      assignment->node_name, sm_error_str(SM_NOT_FOUND) );
            return;
        }

        if( SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_PLUS_M
                != member->redundancy_model )
        {
            DPRINTFD( "Cannot swact assignment (%s) for service domain (%s) "
                      "node (%s), standby state not supported.",
                      assignment->service_group_name, assignment->name,
                      assignment->node_name );
            return;
        }
    }

    if( sched_state != assignment->sched_state )
    {
        assignment->sched_state = sched_state;
        ++(*total_set);
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Set Scheduling Node State
// ====================================================
static SmErrorT sm_service_domain_scheduler_set_sched_node_state(
    char* node_name, SmServiceDomainSchedulingStateT sched_state )
{
    int total_set = 0;
    SmServiceDomainSchedulingStateT only_sched_state;
    void* user_data[] = {&only_sched_state, &sched_state, &total_set};

    only_sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_NIL;

    sm_service_domain_assignment_table_foreach_node( node_name, user_data,
                    sm_service_domain_scheduler_set_assignment_sched_state );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Set Scheduling State
// ===============================================
static SmErrorT sm_service_domain_scheduler_set_sched_state(
    char* name, SmServiceDomainSchedulingStateT sched_state )
{
    int total_set = 0;
    SmServiceDomainSchedulingStateT only_sched_state;
    void* user_data[] = {&only_sched_state, &sched_state, &total_set};

    only_sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_NIL;
    
    sm_service_domain_assignment_table_foreach( name, user_data,
                    sm_service_domain_scheduler_set_assignment_sched_state );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Count Swacting Assignments
// =====================================================
bool sm_service_domain_scheduler_count_swacting_assignments( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    return((( SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT == assignment->sched_state )||
            ( SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT_FORCE == assignment->sched_state ))&&
            !((( SM_SERVICE_GROUP_STATE_STANDBY == assignment->desired_state )&&
               (( SM_SERVICE_GROUP_STATE_STANDBY  == assignment->state )||
                ( SM_SERVICE_GROUP_STATE_DISABLED == assignment->state )) )||
              (( SM_SERVICE_GROUP_STATE_DISABLED == assignment->desired_state )&&
               ( SM_SERVICE_GROUP_STATE_DISABLED == assignment->state )) ));
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Count Disabling Assignments
// ======================================================
bool sm_service_domain_scheduler_count_disabling_assignments( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    return(( SM_SERVICE_DOMAIN_SCHEDULING_STATE_DISABLE == assignment->sched_state )&&
            !(( SM_SERVICE_GROUP_STATE_DISABLED == assignment->desired_state )&&
              ( SM_SERVICE_GROUP_STATE_DISABLED == assignment->state )) );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Count Transitioning Assignments
// ==========================================================
bool sm_service_domain_scheduler_count_transitioning_assignments(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    return(( SM_SERVICE_GROUP_STATE_GO_STANDBY == assignment->state )||
           ( SM_SERVICE_GROUP_STATE_DISABLING == assignment->state ));
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Count Active Assignments
// ===================================================
bool sm_service_domain_scheduler_count_active_assignments( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    return(( SM_SERVICE_GROUP_STATE_ACTIVE == assignment->state )||
           ( SM_SERVICE_GROUP_STATE_GO_ACTIVE == assignment->state ));
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Count Standby Assignments
// ====================================================
bool sm_service_domain_scheduler_count_standby_assignments( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    return(( SM_SERVICE_GROUP_STATE_STANDBY == assignment->state )||
           ( SM_SERVICE_GROUP_STATE_GO_STANDBY == assignment->state ));
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Count Active Only Assignments
// ========================================================
bool sm_service_domain_scheduler_count_active_only_assignments(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    bool do_count = false;

    if(( SM_SERVICE_GROUP_STATE_ACTIVE == assignment->state )&&
       ( SM_SERVICE_GROUP_STATUS_FAILED != assignment->status ))
    {
        SmDbNodeT node;
        SmErrorT error;

        error = sm_db_nodes_read( _sm_db_handle, assignment->node_name,
                                  &node );
        if( SM_OKAY == error )
        {
            if(( SM_NODE_ADMIN_STATE_UNLOCKED == node.admin_state )&&
               ( SM_NODE_READY_STATE_ENABLED  == node.ready_state ))
            {
                do_count = true;
                DPRINTFD( "Count active assignment (%s) node (%s) for "
                          "domain (%s).", assignment->service_group_name,
                          assignment->node_name, assignment->name );
            } else {
                DPRINTFD( "Skip counting active assignment (%s) node (%s) "
                          "for domain (%s), state=%s.",
                          assignment->service_group_name,
                          assignment->node_name, assignment->name,
                          sm_node_state_str( node.admin_state,
                                             node.ready_state ) );
            }
        }
    }

    return( do_count );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Count Standby Only Assignments
// =========================================================
bool sm_service_domain_scheduler_count_standby_only_assignments(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    bool do_count = false;

    if(( SM_SERVICE_GROUP_STATE_STANDBY == assignment->state )&&
       ( SM_SERVICE_GROUP_STATUS_FAILED != assignment->status ))
    {
        SmDbNodeT node;
        SmErrorT error;

        error = sm_db_nodes_read( _sm_db_handle, assignment->node_name,
                                  &node );
        if( SM_OKAY == error )
        {
            if(( SM_NODE_ADMIN_STATE_UNLOCKED == node.admin_state )&&
               ( SM_NODE_READY_STATE_ENABLED  == node.ready_state ))
            {
                do_count = true;
                DPRINTFD( "Count standby assignment (%s) node (%s) for "
                          "domain (%s).", assignment->service_group_name,
                          assignment->node_name, assignment->name );
            } else {
                DPRINTFD( "Skip counting standby assignment (%s) node (%s) "
                          "for domain (%s), state=%s.",
                          assignment->service_group_name,
                          assignment->node_name, assignment->name,
                          sm_node_state_str( node.admin_state,
                                             node.ready_state ) );
            }
        }
    }

    return( do_count );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Update State
// =======================================
static SmErrorT sm_service_domain_scheduler_update_state( 
    SmServiceDomainT* domain, bool* removing_activity )
{
    int total_set = 0;
    SmServiceDomainSchedulingStateT only_sched_state, sched_state;
    void* user_data[] = {&only_sched_state, &sched_state, &total_set};
    int count;

    // Update Remove Activity Scheduler States
    count = sm_service_domain_assignment_table_count( domain->name,
                NULL, sm_service_domain_scheduler_count_swacting_assignments );
    if( 0 == count )
    {
        *removing_activity = false;

        only_sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT;
        sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_NONE;

        sm_service_domain_assignment_table_foreach( domain->name, user_data,
                sm_service_domain_scheduler_set_assignment_sched_state );

        only_sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT_FORCE;
        sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_NONE;

        sm_service_domain_assignment_table_foreach( domain->name, user_data,
                sm_service_domain_scheduler_set_assignment_sched_state );

        if( 0 < total_set )
        {
            SCHED_LOG( domain->name, "Clear swact scheduling states." );
        }
    } else {
        *removing_activity = true;
    }

    // Update Disable Scheduler States
    total_set = 0;
    count = sm_service_domain_assignment_table_count( domain->name, NULL,
                sm_service_domain_scheduler_count_disabling_assignments );
    if( 0 == count )
    {
        only_sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_DISABLE;
        sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_NONE;

        sm_service_domain_assignment_table_foreach( domain->name, user_data,
                sm_service_domain_scheduler_set_assignment_sched_state );

        if( 0 < total_set )
        {
            SCHED_LOG( domain->name, "Clear disable scheduling states." );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Sort Assignments
// ===========================================
static int sm_service_domain_scheduler_sort_assignments( const void* lhs,
    const void* rhs )
{
    SmServiceDomainAssignmentT* assignment_lhs;
    SmServiceDomainAssignmentT* assignment_rhs;

    assignment_lhs = (SmServiceDomainAssignmentT*) lhs;
    assignment_rhs = (SmServiceDomainAssignmentT*) rhs;

    // Descending Order based on weight.
    if( assignment_lhs->sched_weight > assignment_rhs->sched_weight )
    {
        return( -1 );  // lhs before rhs
    }

    if( assignment_lhs->sched_weight == assignment_rhs->sched_weight )
    {
        // Ascending Order based on last state change.
        if( assignment_lhs->last_state_change < assignment_rhs->last_state_change )
        {
            return( -1 ); // lhs before rhs
        }

        // Ascending Order based on last state change.
        if( assignment_lhs->last_state_change == assignment_rhs->last_state_change )
        {
            return( 0 ); // lhs before rhs
        }

        // Ascending Order based on last state change.
        if( assignment_lhs->last_state_change > assignment_rhs->last_state_change )
        {
            return( 1 ); // lhs after rhs
        }
    }

    if( assignment_lhs->sched_weight < assignment_rhs->sched_weight )
    {
        return( 1 ); // lhs after rhs
    }

    return( 0 ); // lhs before rhs
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Disable Active Member
// ================================================
static void sm_service_domain_scheduler_disable_active_member(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    if(( SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE == assignment->sched_list ) ||
       ( SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE == assignment->sched_list ))
    {
        assignment->sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_DISABLE;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Unselect Active Member
// =================================================
static void sm_service_domain_scheduler_unselect_active_member(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    SmServiceDomainMemberT* member = (SmServiceDomainMemberT*) user_data[0];
    int* current_active = (int*) user_data[1];

    if( 0 != strcmp( member->service_group_name, assignment->service_group_name ) )
    {
        return;
    }
    
    if( SM_SERVICE_DOMAIN_WEIGHT_UNSELECTABLE_ACTIVE == assignment->sched_weight )
    {
        assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY;
        DPRINTFD( "Unselect assignment (%s-%s-%s), unselectable.",
                  assignment->name, assignment->node_name,
                  assignment->service_group_name );
        return;
    }

    if( *current_active < member->n_active )
    {
        ++(*current_active);

        DPRINTFD( "Selected assignment (%s-%s-%s).", assignment->name, 
                  assignment->node_name, assignment->service_group_name );
    } else {
        assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY;

        DPRINTFD( "Unselected assignment (%s-%s-%s).", assignment->name, 
                  assignment->node_name, assignment->service_group_name );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Select Active Member
// ===============================================
static void sm_service_domain_scheduler_select_active_member( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    SmServiceDomainMemberT* member = (SmServiceDomainMemberT*) user_data[0];
    int* current_active = (int*) user_data[1];

    if( 0 != strcmp( member->service_group_name, assignment->service_group_name ) )
    {
        return;
    }
    
    if( SM_SERVICE_DOMAIN_WEIGHT_UNSELECTABLE_ACTIVE == assignment->sched_weight )
    {
        DPRINTFD( "Skipping assignment (%s-%s-%s), unselectable.",
                  assignment->name, assignment->node_name,
                  assignment->service_group_name );
        return;
    }

    if( *current_active < member->n_active )
    {
        ++(*current_active );
        assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE;

        DPRINTFD( "Selected assignment (%s-%s-%s).", assignment->name, 
                  assignment->node_name, assignment->service_group_name );
    } else {
        DPRINTFD( "Not selected assignment (%s-%s-%s).", assignment->name, 
                  assignment->node_name, assignment->service_group_name );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Select Active Members
// ================================================
static void sm_service_domain_scheduler_select_active_members( void* user_data[],
    SmServiceDomainMemberT* member )
{
    int count;
    int current_active;
    SmServiceDomainT* domain = (SmServiceDomainT*) user_data[0];
    void* user_data2[] = {member, &current_active};

    sm_service_domain_assignment_table_sort(
                        sm_service_domain_scheduler_sort_assignments );

    // Count members transitioning to standby or disabled.
    count = sm_service_domain_assignment_table_count_service_group(
                member->name, member->service_group_name, NULL,
                sm_service_domain_scheduler_count_transitioning_assignments );
    if( 0 != count )
    {
        SCHED_LOG( domain->name, "Members transitioning to standby "
                   "or disabled, count=%i.", count );
        return;
    }

    // Count active and transitioning to active members.
    count = sm_service_domain_assignment_table_count_service_group(
                member->name, member->service_group_name, NULL,
                sm_service_domain_scheduler_count_active_assignments );

    if(( SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_DISABLE_ALL_ACTIVE
            == domain->split_brain_recovery )&&( count > member->n_active ))
    {
        SCHED_LOG( domain->name, "Too many active service domain (%s) "
                   "members (%s), disable all and recover, count=%i.",
                   member->name, member->service_group_name, count );

        sm_service_domain_assignment_table_foreach_service_group(
                        member->name, member->service_group_name, NULL, 
                        sm_service_domain_scheduler_disable_active_member );
    }
    else if( count > member->n_active )
    {
        current_active = 0;
        // Go-Active List
        sm_service_domain_assignment_table_foreach_schedule_list( member->name,
                    SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE, user_data2,
                    sm_service_domain_scheduler_unselect_active_member );

        // Active List
        sm_service_domain_assignment_table_foreach_schedule_list( member->name,
                    SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE, user_data2,
                    sm_service_domain_scheduler_unselect_active_member );

    } else if( count < member->n_active ) {
        current_active = count;

        // Standby List.
        sm_service_domain_assignment_table_foreach_schedule_list( member->name,
                    SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY, user_data2,
                    sm_service_domain_scheduler_select_active_member );

        if( current_active < member->n_active ) 
        {
            // Disabled List.
            sm_service_domain_assignment_table_foreach_schedule_list( member->name,
                        SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED, user_data2,
                        sm_service_domain_scheduler_select_active_member );
        }
    } else {
        SCHED_LOG( domain->name, "Active service domain (%s) members "
                   "(%s) already selected.", member->name,
                   member->service_group_name );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Select Active
// ========================================
static SmErrorT sm_service_domain_scheduler_select_active( 
    SmServiceDomainT* domain )
{
    void* user_data[] = {domain};

    sm_service_domain_member_table_foreach( domain->name, user_data,
                        sm_service_domain_scheduler_select_active_members );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Unselect Standby Member
// ==================================================
static void sm_service_domain_scheduler_unselect_standby_member(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    SmServiceDomainMemberT* member = (SmServiceDomainMemberT*) user_data[0];
    int* current_standby = (int*) user_data[1];

    if( 0 != strcmp( member->service_group_name, assignment->service_group_name ) )
    {
        return;
    }
    
    if(( *current_standby < member->m_standby )&&
       ( SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT != assignment->sched_state )&&
       ( SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT_FORCE != assignment->sched_state ))
    {
        ++(*current_standby);

    } else {
        assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Select Standby Member
// ================================================
static void sm_service_domain_scheduler_select_standby_member(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    SmServiceDomainMemberT* member = (SmServiceDomainMemberT*) user_data[0];
    int* current_standby = (int*) user_data[1];

    if( 0 != strcmp( member->service_group_name, assignment->service_group_name ) )
    {
        return;
    }

    if( *current_standby < member->m_standby )
    {
        ++(*current_standby );
        assignment->sched_list = SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Select Standby Members
// =================================================
static void sm_service_domain_scheduler_select_standby_members( void* user_data[],
    SmServiceDomainMemberT* member )
{
    int count;
    int current_standby;
    SmServiceDomainT* domain = (SmServiceDomainT*) user_data[0];
    void* user_data2[] = {member, &current_standby};

    sm_service_domain_assignment_table_sort(
                        sm_service_domain_scheduler_sort_assignments );

    count = sm_service_domain_assignment_table_count_service_group(
                    member->name, member->service_group_name, NULL,
                    sm_service_domain_scheduler_count_standby_assignments );
    if( count > member->m_standby ) 
    {
        current_standby = 0;
        
        // Standby List.
        sm_service_domain_assignment_table_foreach_schedule_list( member->name,
                    SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY, user_data2,
                    sm_service_domain_scheduler_unselect_standby_member );

    } else if( count < member->m_standby ) {
        current_standby = count;

        // Disabled List.
        sm_service_domain_assignment_table_foreach_schedule_list( member->name,
                    SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED, user_data2,
                    sm_service_domain_scheduler_select_standby_member );

    } else {
        SCHED_LOG( domain->name, "Standby service domain (%s) members "
                   "(%s) already selected.", member->name,
                   member->service_group_name );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Select Standby
// =========================================
static SmErrorT sm_service_domain_scheduler_select_standby( 
    SmServiceDomainT* domain )
{
    void* user_data[] = {domain};

    sm_service_domain_member_table_foreach( domain->name, user_data,
                            sm_service_domain_scheduler_select_standby_members );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Apply Change
// =======================================
static void sm_service_domain_scheduler_apply_change( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    bool recover = false;
    bool escalate_recovery = false;
    bool clear_fatal_condition = false;
    struct timespec ts;
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmServiceGroupStateT desired_state;
    SmServiceGroupActionT action;
    SmServiceGroupActionFlagsT action_flags = 0;
    SmErrorT error;

    switch( assignment->sched_list )
    {
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE:
            desired_state = SM_SERVICE_GROUP_STATE_ACTIVE;
            action = SM_SERVICE_GROUP_ACTION_GO_ACTIVE;

            if(( desired_state == assignment->desired_state )&&
               (( SM_SERVICE_GROUP_STATUS_WARN == assignment->status )||
                ( SM_SERVICE_GROUP_STATUS_DEGRADED == assignment->status )))
            {
                recover = true;
                clear_fatal_condition = true;
            }
        break;

        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE:
            desired_state = SM_SERVICE_GROUP_STATE_ACTIVE;
            action = SM_SERVICE_GROUP_ACTION_GO_ACTIVE;

            if(( SM_SERVICE_GROUP_STATUS_FAILED == assignment->status )&&
               ( SM_SERVICE_GROUP_CONDITION_ACTION_FAILURE
                    == assignment->condition ))
            {
                recover = true;

            } else if(( SM_SERVICE_GROUP_STATUS_WARN == assignment->status )||
                      ( SM_SERVICE_GROUP_STATUS_DEGRADED == assignment->status ))
            {
                recover = true;
                clear_fatal_condition = true;
            }
        break;

        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY:
            desired_state = SM_SERVICE_GROUP_STATE_STANDBY;
            action = SM_SERVICE_GROUP_ACTION_GO_STANDBY;

            if(( desired_state == assignment->desired_state )&&
               (( SM_SERVICE_GROUP_STATUS_WARN == assignment->status )||
                ( SM_SERVICE_GROUP_STATUS_DEGRADED == assignment->status )))
            {
                recover = true;
            }
        break;

        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY:
            desired_state = SM_SERVICE_GROUP_STATE_STANDBY;
            action = SM_SERVICE_GROUP_ACTION_GO_STANDBY;
        break;

        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED:
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING:
        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE:
            desired_state = SM_SERVICE_GROUP_STATE_DISABLED;
            action = SM_SERVICE_GROUP_ACTION_DISABLE;
        break;

        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_FAILED:
            // An assignment can be failed or failed with a fatal error and be
            // on the failed list.  The failed list indicates recovery without
            // escalation should be attempted.
            desired_state = SM_SERVICE_GROUP_STATE_NIL;
            action = SM_SERVICE_GROUP_ACTION_RECOVER;
            recover = true;
            clear_fatal_condition = (SM_SERVICE_GROUP_CONDITION_FATAL_FAILURE
                                     == assignment->condition);
        break;

        case SM_SERVICE_DOMAIN_SCHEDULING_LIST_FATAL:
            // An assignment on this list indicates that recovery with
            // escalation should be attempted as there is another healthier
            // assignment.
            desired_state = SM_SERVICE_GROUP_STATE_NIL;
            action = SM_SERVICE_GROUP_ACTION_RECOVER;
            recover = true;
            escalate_recovery = true;
            clear_fatal_condition = true;
        break;

        default:
            DPRINTFI( "Unknown list specified for member (%s) of service "
                      "domain (%s) node (%s).",
                      assignment->service_group_name, assignment->name,
                      assignment->node_name );
            return;
        break;
    }

    if(( SM_SERVICE_GROUP_STATE_NIL != desired_state )&&
       ( desired_state != assignment->desired_state  ))
    {
        clock_gettime( CLOCK_MONOTONIC_RAW, &ts );

        SCHED_LOG( assignment->name, "Update desired state from (%s) to (%s) "
                   "for member (%s) of node (%s) current_state=%s, "
                   "last_state_change=%ld, recover=%s, escalate_recovery=%s, "
                   "clear_fatal_condition=%s.",
                   sm_service_group_state_str(assignment->desired_state),
                   sm_service_group_state_str(desired_state),
                   assignment->service_group_name, assignment->node_name,
                   sm_service_group_state_str(assignment->state),
                   (long) ts.tv_sec, recover ? "yes" : "no",
                   escalate_recovery ? "yes" : "no",
                   clear_fatal_condition ? "yes" : "no" );

        assignment->desired_state = desired_state;
        assignment->last_state_change = (long) ts.tv_sec;

        error = sm_service_domain_assignment_table_persist( assignment );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to update assignment (%s) for service "
                      "domain (%s) node (%s), error=%s.",
                      assignment->service_group_name, assignment->name,
                      assignment->node_name, sm_error_str( error ) );
            return;
        }
    } else {
        SCHED_LOG( assignment->name, "No update of desired state (%s) "
                   "for member (%s) of node (%s) current_state=%s, " 
                   "last_state_change=%ld, recover=%s, escalate_recovery=%s, "
                   "clear_fatal_condition=%s.",
                   sm_service_group_state_str(assignment->desired_state),
                   assignment->service_group_name, assignment->node_name,
                   sm_service_group_state_str(assignment->state),
                   assignment->last_state_change, recover ? "yes" : "no",
                   escalate_recovery ? "yes" : "no",
                   clear_fatal_condition ? "yes" : "no" );
    }

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return;
    }

    if( 0 == strcmp( hostname, assignment->node_name ) )
    {
        switch( action )
        {
            case SM_SERVICE_GROUP_ACTION_DISABLE:
                error = sm_service_group_api_disable(
                                    assignment->service_group_name );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to perform disable action on member (%s) "
                              "for service domain (%s) on node (%s), error=%s.",
                              assignment->service_group_name,
                              assignment->name, assignment->node_name,
                              sm_error_str(error) );
                    return;
                }
            break;

            case SM_SERVICE_GROUP_ACTION_GO_ACTIVE:
                error = sm_service_group_api_go_active(
                                    assignment->service_group_name );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to perform go-active action on member (%s) "
                              "for service domain (%s) on node (%s), error=%s.",
                              assignment->service_group_name,
                              assignment->name, assignment->node_name,
                              sm_error_str(error) );
                    return;
                }
            break;

            case SM_SERVICE_GROUP_ACTION_GO_STANDBY:
                error = sm_service_group_api_go_standby(
                                    assignment->service_group_name );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to perform go-standby action on member (%s) "
                              "for service domain (%s) on node (%s), error=%s.",
                              assignment->service_group_name,
                              assignment->name, assignment->node_name,
                              sm_error_str(error) );
                    return;
                }
            break;

            case SM_SERVICE_GROUP_ACTION_RECOVER:
                error = sm_service_group_api_recover(
                                    assignment->service_group_name,
                                    escalate_recovery, clear_fatal_condition );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to perform recovery action on member (%s) "
                              "for service domain (%s) on node (%s), error=%s.",
                              assignment->service_group_name,
                              assignment->name, assignment->node_name,
                              sm_error_str(error) );
                    return;
                }
            break;

            default:
                DPRINTFE( "Unknown action (%s) specified for member "
                          "(%s) for service domain (%s) on node (%s).",
                          sm_service_group_action_str(action),
                          assignment->service_group_name, assignment->name,
                          assignment->node_name );
            break;
        }

        if(( recover )&&( SM_SERVICE_GROUP_ACTION_RECOVER != action ))
        {
            error = sm_service_group_api_recover(
                                assignment->service_group_name,
                                escalate_recovery, clear_fatal_condition );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to perform recovery action on member (%s) "
                          "for service domain (%s) on node (%s), error=%s.",
                          assignment->service_group_name,
                          assignment->name, assignment->node_name,
                          sm_error_str(error) );
                return;
            }
        }
    } else {
        if( recover )
        {
            SM_FLAG_SET( action_flags, SM_SERVICE_GROUP_ACTION_FLAG_RECOVER );
        }

        if( escalate_recovery )
        {
            SM_FLAG_SET( action_flags,
                         SM_SERVICE_GROUP_ACTION_FLAG_ESCALATE_RECOVERY );
        }

        if( clear_fatal_condition )
        {
            SM_FLAG_SET( action_flags,
                         SM_SERVICE_GROUP_ACTION_FLAG_CLEAR_FATAL_CONDITION );
        }

        error = sm_service_domain_utils_send_member_request( 
                                assignment->name, assignment->node_name,
                                assignment->id, assignment->service_group_name,
                                action, action_flags );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send member update for service domain "
                      "(%s) node (%s) of member (%s).", assignment->name,
                      assignment->node_name, assignment->service_group_name );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Apply Changes
// ========================================
static SmErrorT sm_service_domain_scheduler_apply_changes( 
    char service_domain_name[] )
{
    sm_service_domain_assignment_table_foreach( service_domain_name, NULL,
                                    sm_service_domain_scheduler_apply_change );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Dump Entry
// =====================================
static void sm_service_domain_scheduler_dump_entry( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    bool* first_one = (bool*) user_data[0];
    
    if( *first_one )
    {
        SmServiceDomainSchedulingListT* sched_list;

        sched_list = (SmServiceDomainSchedulingListT*) user_data[1];

        SCHED_LOG( assignment->name, "List (%s):",
                   sm_service_domain_scheduling_list_str( *sched_list) );
        *first_one = false;
    }

    SCHED_LOG( assignment->name, "  Node (%s) - Service Group (%s) - "
               "desired_state=%s, state=%s, status=%s, condition=%s, "
               "health=%" PRIX64 ", last_state_change=%li, sched_state=%s, "
               "sched_weight=%i.", assignment->node_name,
               assignment->service_group_name,
               sm_service_group_state_str(assignment->desired_state),
               sm_service_group_state_str(assignment->state),
               sm_service_group_status_str(assignment->status),
               sm_service_group_condition_str(assignment->condition),
               assignment->health, assignment->last_state_change,
               sm_service_domain_scheduling_state_str(assignment->sched_state),
               assignment->sched_weight );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Dump
// ===============================
static void sm_service_domain_scheduler_dump( SmServiceDomainT* domain )
{
    bool first_one;
    SmServiceDomainSchedulingListT sched_list;
    void* user_data[] = {&first_one, &sched_list};

    sm_service_domain_assignment_table_sort(
                        sm_service_domain_scheduler_sort_assignments );

    int list;
    for( list=SM_SERVICE_DOMAIN_SCHEDULING_LIST_NIL;
         SM_SERVICE_DOMAIN_SCHEDULING_LIST_MAX > list; ++list )
    {
        first_one = true;
        sched_list = (SmServiceDomainSchedulingListT) list;

        sm_service_domain_assignment_table_foreach_schedule_list(
            domain->name, sched_list, user_data,
            sm_service_domain_scheduler_dump_entry );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Schedule
// ===================================
static SmErrorT sm_service_domain_scheduler_schedule( SmServiceDomainT* domain )
{
    long ms_expired;
    SmTimeT time_prev, time_now;
    bool removing_activity;
    SmServiceDomainFilterCountsT filter_counts;
    SmErrorT error;

    // Apply Preselect Filters.
    sm_time_get( &time_prev );

    error = sm_service_domain_filter_preselect_apply( domain->name,
                                                      &filter_counts );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) filter preselect apply failed, "
                  "error=%s.", domain->name, sm_error_str( error ) );
        return( error );
    }

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - preselect apply filters, took %li ms.",
              ms_expired );

    SCHED_LOG( domain->name, "Filter counts: active(%i) go-active(%i) "
               "standby(%i) go-standby(%i) disabling(%i) disabled (%i) "
               "failed (%i) fatal (%i) unavailable(%i) total(%i)",
               filter_counts.active_members, filter_counts.go_active_members,
               filter_counts.standby_members, filter_counts.go_standby_members,
               filter_counts.disabling_members, filter_counts.disabled_members,
               filter_counts.failed_members, filter_counts.fatal_members,
               filter_counts.unavailable_members, filter_counts.total_members );

    // Apply Weights.
    sm_time_get( &time_prev );

    error = sm_service_domain_weight_apply( domain->name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) weight apply failed, error=%s.",
                  domain->name, sm_error_str( error ) );
        return( error );
    }

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - apply weights, took %li ms.", ms_expired );

    // Update scheduling states.
    sm_time_get( &time_prev );

    error = sm_service_domain_scheduler_update_state( domain,
                                                      &removing_activity );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) scheduler state update failed, "
                  "error=%s.", domain->name, sm_error_str( error ) );
        return( error );
    }

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - update scheduling states, took %li ms.",
              ms_expired );

    // Select Active.
    sm_time_get( &time_prev );

    if( removing_activity )
    {
        SCHED_LOG( domain->name, "Removing activity, active selection "
                   "delayed." );
    } else {
        error = sm_service_domain_scheduler_select_active( domain );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to select active, error=%s.",
                      sm_error_str( error ) );
            return( error );
        }
    }

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - select active, took %li ms.", ms_expired );

    // Select Standby.
    sm_time_get( &time_prev );

    error = sm_service_domain_scheduler_select_standby( domain );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to select standby, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - select standby, took %li ms.", ms_expired );

    // Apply Post Select Filters.
    sm_time_get( &time_prev );

    error = sm_service_domain_filter_post_select_apply( domain->name,
                                                        &filter_counts );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) filter post select apply failed, "
                  "error=%s.", domain->name, sm_error_str( error ) );
        return( error );
    }

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - post select apply filters, took %li ms.",
              ms_expired );

    SCHED_LOG( domain->name, "Filter counts: active(%i) go-active(%i) "
               "standby(%i) go-standby(%i) disabling(%i) disabled (%i) "
               "failed (%i) fatal (%i) unavailable(%i) total(%i)",
               filter_counts.active_members, filter_counts.go_active_members,
               filter_counts.standby_members, filter_counts.go_standby_members,
               filter_counts.disabling_members, filter_counts.disabled_members,
               filter_counts.failed_members, filter_counts.fatal_members,
               filter_counts.unavailable_members, filter_counts.total_members );
    
    sm_service_domain_scheduler_dump( domain );

    // Apply Changes.
    sm_time_get( &time_prev );

    error = sm_service_domain_scheduler_apply_changes( domain->name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) apply changes failed, error=%s.",
                  domain->name, sm_error_str( error ) );
        return( error );
    }

    sm_time_get( &time_now );
    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );
    DPRINTFD( "Scheduler execute - apply changes, took %li ms.", ms_expired );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Assignment State Alarms
// ==================================================
static void sm_service_domain_scheduler_assignment_state_alarms(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    SmAlarmSeverityT severity;
    SmAlarmSpecificProblemTextT problem_text = "";

    switch( assignment->status )
    {
        case SM_SERVICE_GROUP_STATUS_WARN:
            severity = SM_ALARM_SEVERITY_MINOR;
            snprintf( problem_text, sizeof(problem_text), 
                      "service group %s warning",
                      assignment->service_group_name );
        break;

        case SM_SERVICE_GROUP_STATUS_DEGRADED:
            severity = SM_ALARM_SEVERITY_MAJOR;
            snprintf( problem_text, sizeof(problem_text), 
                      "service group %s degraded",
                      assignment->service_group_name );
        break;

        case SM_SERVICE_GROUP_STATUS_FAILED:
            severity = SM_ALARM_SEVERITY_CRITICAL;
            snprintf( problem_text, sizeof(problem_text), 
                      "service group %s failure",
                      assignment->service_group_name );
        break;

        case SM_SERVICE_GROUP_STATUS_NONE:
            severity = SM_ALARM_SEVERITY_CLEARED;
        break;

        default:
            DPRINTFD( "Ignore service domain (%s) assignment (%s) "
                      "status (%s = %i) on node (%s).", assignment->name,
                      assignment->service_group_name,
                      sm_service_group_status_str(assignment->status),
                      assignment->status, assignment->node_name );
            return;
        break;
    }

    if( SM_ALARM_SEVERITY_CLEARED == severity )
    {
        sm_alarm_clear( SM_ALARM_SERVICE_GROUP_STATE, assignment->node_name,
                assignment->name, assignment->service_group_name );
    } else {
        sm_alarm_raise_state_alarm( SM_ALARM_SERVICE_GROUP_STATE, 
                assignment->node_name, assignment->name,
                assignment->service_group_name, severity,
                SM_ALARM_PROBABLE_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE,
                sm_service_group_state_str(assignment->state),
                sm_service_group_status_str(assignment->status),
                sm_service_group_condition_str(assignment->condition),
                problem_text, assignment->reason_text, "", true );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Assignment Redundancy Alarm
// ======================================================
static void sm_service_domain_scheduler_assignment_redundancy_alarm(
    void* user_data[], SmServiceDomainMemberT* member )
{
    SmAlarmSpecificProblemTextT problem_text = "";
    SmAlarmAdditionalTextT additional_text;
    SmAlarmProposedRepairActionT proposed_repair_action;
    char log_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    int count;

    snprintf( problem_text, sizeof(problem_text), "service group %s loss "
              "of redundancy", member->service_group_name );

    snprintf( log_text, sizeof(log_text), "service group %s redundancy "
              "restored", member->service_group_name );

    snprintf( proposed_repair_action, sizeof(proposed_repair_action),
              "bring a controller node back in to service, otherwise "
              "contact next level of support" );

    count = sm_service_domain_assignment_table_count_service_group(
              member->name, member->service_group_name, NULL,
              sm_service_domain_scheduler_count_active_only_assignments );

    if( count == member->n_active )
    {
        if( SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_PLUS_M 
                == member->redundancy_model )
        {
            count = sm_service_domain_assignment_table_count_service_group(
                      member->name, member->service_group_name, NULL,
                      sm_service_domain_scheduler_count_standby_only_assignments );
            if( count < member->m_standby ) 
            {
                if( 0 == count )
                {
                    snprintf( additional_text, sizeof(additional_text),
                              "expected %i standby member%s but no standby "
                              "members available", member->m_standby,
                              (1 == member->m_standby) ? "" : "s" );
                } else {
                    snprintf( additional_text, sizeof(additional_text), 
                              "expected %i standby member%s but only %i "
                              "standby member%s available", member->m_standby,
                              (1 == member->m_standby) ? "" : "s", count,
                              (1 == count) ? "" : "s" );
                }

                sm_alarm_raise_threshold_alarm( SM_ALARM_SERVICE_GROUP_REDUNDANCY,
                    "", member->name, member->service_group_name,
                    SM_ALARM_SEVERITY_MAJOR,
                    SM_ALARM_PROBABLE_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE,
                    member->m_standby, count, problem_text, additional_text,
                    proposed_repair_action, true );

                snprintf( log_text, sizeof(log_text), "%s; %s", problem_text,
                          additional_text );

            } else {
                sm_alarm_clear( SM_ALARM_SERVICE_GROUP_REDUNDANCY, "",
                                member->name, member->service_group_name );
            }
        } else {
            sm_alarm_clear( SM_ALARM_SERVICE_GROUP_REDUNDANCY, "",
                            member->name, member->service_group_name );
        }
    } else {
        if( count < member->n_active )
        {
            SmAlarmSeverityT alarm_severity;

            if( 0 == count )
            {
                snprintf( problem_text, sizeof(problem_text), "service group "
                          "%s has no active members available",
                          member->service_group_name );

                snprintf( additional_text, sizeof(additional_text),
                          "expected %i active member%s", member->n_active,
                          (1 == member->n_active) ? "" : "s" );

                alarm_severity = SM_ALARM_SEVERITY_CRITICAL;

            } else {
                snprintf( additional_text, sizeof(additional_text),
                          "expected %i active member%s but only %i "
                          "active member%s available", member->n_active,
                          (1 == member->n_active) ? "" : "s", count,
                          (1 == count) ? "" : "s" );

                alarm_severity = SM_ALARM_SEVERITY_MAJOR;
            }

            sm_alarm_raise_threshold_alarm( SM_ALARM_SERVICE_GROUP_REDUNDANCY,
                "", member->name, member->service_group_name, 
                alarm_severity,
                SM_ALARM_PROBABLE_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE,
                member->n_active, count, problem_text, additional_text,
                proposed_repair_action, true );

            snprintf( log_text, sizeof(log_text), "%s; %s", problem_text,
                      additional_text );

        } else {
            sm_alarm_clear( SM_ALARM_SERVICE_GROUP_REDUNDANCY, "",
                            member->name, member->service_group_name );
        }
    }

    if( 0 != strcmp( member->redundancy_log_text, log_text ) )
    {
        snprintf( member->redundancy_log_text,
                  sizeof(member->redundancy_log_text), "%s", log_text );

        sm_log_service_group_redundancy_change( member->service_group_name,
                                                member->redundancy_log_text );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Assignment Cleanup Alarms
// ====================================================
static void sm_service_domain_scheduler_assignment_cleanup_alarms(
    void* user_data[], SmServiceDomainMemberT* member )
{
    member->redundancy_log_text[0] = '\0';
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Manage Alarms
// ========================================
static SmErrorT sm_service_domain_scheduler_manage_alarms(
    SmServiceDomainT* domain )
{
    if( SM_SERVICE_DOMAIN_STATE_LEADER == domain->state )
    {
        // Manage Service Domain Assignment State Alarms
        sm_service_domain_assignment_table_foreach( domain->name, NULL,
                    sm_service_domain_scheduler_assignment_state_alarms );

        // Manage Service Domain Assignment Redundancy Alarms
        sm_service_domain_member_table_foreach( domain->name, NULL,
                    sm_service_domain_scheduler_assignment_redundancy_alarm );

    } else {
        sm_service_domain_member_table_foreach( domain->name, NULL,
                    sm_service_domain_scheduler_assignment_cleanup_alarms );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Schedule Service Domain
// ==================================================
void sm_service_domain_scheduler_schedule_service_domain(
    SmServiceDomainT* domain )
{
    bool syncing;
    SmErrorT error;

    SCHED_LOG_START( domain->name, "--start------------------------------" );
    SCHED_LOG( domain->name, "%s.",
               sm_service_domain_state_str(domain->state) );

    if( SM_SERVICE_DOMAIN_STATE_LEADER == domain->state )
    {
        error = sm_service_domain_utils_service_domain_neighbors_syncing(
                                                    domain->name, &syncing );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to determine if service domain (%s) neighbors "
                      "are syncing, error=%s.", domain->name,
                      sm_error_str( error ) );
            goto DONE;
        }
    
        if( syncing )
        {
            DPRINTFI( "Service domain (%s) neighbors are syncing.",
                      domain->name );
            SCHED_LOG( domain->name, "neighbors are syncing.");
            goto DONE;
        }

        error = sm_service_domain_scheduler_schedule( domain );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to schedule service domain (%s), error=%s.",
                      domain->name, sm_error_str( error ) );
            goto DONE;
        }

    } else {
        error = sm_service_domain_scheduler_set_sched_state(
                    domain->name, SM_SERVICE_DOMAIN_SCHEDULING_STATE_NONE );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to set scheduling state for (%s), error=%s.",
                      domain->name, sm_error_str( error ) );
            goto DONE;
        }
    }

    error = sm_service_domain_scheduler_manage_alarms( domain );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to manage alarms service domain (%s), "
                  "error=%s.", domain->name, sm_error_str( error ) );
        goto DONE;
    }

DONE:
    SCHED_LOG_DONE( domain->name, "--complete---------------------------" );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Schedule Service Domain Timeout
// ==========================================================
static void sm_service_domain_scheduler_schedule_service_domain_timeout(
    void* user_data[], SmServiceDomainT* domain )
{
    sm_service_domain_scheduler_schedule_service_domain( domain );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Timeout
// ==================================
static bool sm_service_domain_scheduler_timeout( SmTimerIdT timer_id,
    int64_t user_data )
{
    long ms_expired;
    SmTimeT time_prev, time_now;

    sm_time_get( &time_prev );

    sm_service_domain_table_foreach( NULL, 
            sm_service_domain_scheduler_schedule_service_domain_timeout );

    sm_time_get( &time_now );

    ms_expired = sm_time_delta_in_ms( &time_now, &time_prev );

    DPRINTFD( "Scheduler execute, took %li ms.", ms_expired );

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Swact Node
// =====================================
SmErrorT sm_service_domain_scheduler_swact_node( char node_name[], bool force,
    SmUuidT request_uuid )
{
    char hostname[SM_NODE_NAME_MAX_CHAR] = "";
    char log_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceDomainSchedulingStateT sched_state;
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
    }

    if( force )
    {
        sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT_FORCE;
        snprintf( log_text, sizeof(log_text), "issued against host %s",
                  node_name );
        sm_log_node_state_change( hostname, "", "swact-force", log_text );

    } else {
        sched_state = SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT;
        snprintf( log_text, sizeof(log_text), "issued against host %s",
                  node_name );
        sm_log_node_state_change( hostname, "", "swact", log_text );
    }

    DPRINTFI("Swact from (%s) to (%s) start", node_name, hostname);
    error = sm_service_domain_scheduler_set_sched_node_state( node_name,
                                                              sched_state );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to set node scheduling state, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Node Swact Message Callback
// ======================================================
static void sm_service_domain_scheduler_node_swact_msg_callback(
    SmNetworkAddressT* network_address, int network_port, int version,
    int revision, char node_name[], bool force, SmUuidT request_uuid )
{
    SmErrorT error;

    error = sm_service_domain_scheduler_swact_node( node_name, force,
                                                    request_uuid );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to set node scheduling state, error=%s.",
                  sm_error_str( error ) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Member Request Message Callback
// ==========================================================
static void sm_service_domain_scheduler_member_request_msg_callback(
    SmNetworkAddressT* network_address, int network_port, int version,
    int revision, char service_domain_name[], char node_name[],
    char member_node_name[], int64_t member_id, char member_name[],
    SmServiceGroupActionT member_action,
    SmServiceGroupActionFlagsT member_action_flags )
{
    bool insync;
    bool recover, escalate_recovery, clear_fatal_condition;
    SmServiceGroupStateT member_desired_state;
    SmServiceDomainAssignmentT* assignment;
    SmErrorT error;

    assignment = sm_service_domain_assignment_table_read( service_domain_name,
                                            member_node_name, member_name );
    if( NULL == assignment )
    {
        DPRINTFE( "Failed to read service domain (%s) assignment for node "
                  "(%s) of member (%s), error=%s.", service_domain_name,
                  node_name, member_name,  sm_error_str(SM_NOT_FOUND) );
        return;
    }

    error = sm_service_domain_utils_service_domain_neighbor_insync(
                                        service_domain_name, node_name,
                                        &insync );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to determine if service domain (%s) neighbor (%s) "
                  "is insync for assignment (%s).", service_domain_name,
                  node_name, member_name );
        return;
    }

    if( !insync )
    {
        DPRINTFI( "Service domain (%s) neighbor (%s) is not insync for "
                  "assignment (%s).", service_domain_name, member_node_name,
                  member_name );
        goto SEND_UPDATE;
    }

    recover = SM_FLAG_IS_SET( member_action_flags,
                              SM_SERVICE_GROUP_ACTION_FLAG_RECOVER );
    escalate_recovery = SM_FLAG_IS_SET( member_action_flags,
                              SM_SERVICE_GROUP_ACTION_FLAG_ESCALATE_RECOVERY );
    clear_fatal_condition = SM_FLAG_IS_SET( member_action_flags,
                              SM_SERVICE_GROUP_ACTION_FLAG_CLEAR_FATAL_CONDITION );

    switch( member_action )
    {
        case SM_SERVICE_GROUP_ACTION_DISABLE:
            member_desired_state = SM_SERVICE_GROUP_STATE_DISABLED;

            error = sm_service_domain_utils_update_assignment( 
                         service_domain_name, member_node_name, member_name,
                         member_desired_state, assignment->state,
                         assignment->status, assignment->condition,
                         assignment->health, assignment->reason_text );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) neighbor (%s) unable to update "
                          "assignment (%s), error=%s.", service_domain_name, 
                          member_node_name, member_name, 
                          sm_error_str( error ) );
                return;
            }

            error = sm_service_group_api_disable( member_name );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to perform action (%s) on member (%s) for "
                          "service domain (%s) on node (%s), error=%s.",
                          sm_service_group_action_str(member_action),
                          member_name, service_domain_name, member_node_name,
                          sm_error_str(error) );
                return;
            }
        break;

        case SM_SERVICE_GROUP_ACTION_GO_ACTIVE:
            member_desired_state = SM_SERVICE_GROUP_STATE_ACTIVE;

            error = sm_service_domain_utils_update_assignment( 
                         service_domain_name, member_node_name, member_name,
                         member_desired_state, assignment->state,
                         assignment->status, assignment->condition,
                         assignment->health, assignment->reason_text );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) neighbor (%s) unable to update "
                          "assignment (%s), error=%s.", service_domain_name, 
                          member_node_name, member_name, 
                          sm_error_str( error ) );
                return;
            }

            error = sm_service_group_api_go_active( member_name );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to perform action (%s) on member (%s) for "
                          "service domain (%s) on node (%s), error=%s.",
                          sm_service_group_action_str(member_action),
                          member_name, service_domain_name, member_node_name,
                          sm_error_str(error) );
                return;
            }
        break;

        case SM_SERVICE_GROUP_ACTION_GO_STANDBY:
            member_desired_state = SM_SERVICE_GROUP_STATE_STANDBY;

            error = sm_service_domain_utils_update_assignment( 
                         service_domain_name, member_node_name, member_name,
                         member_desired_state, assignment->state,
                         assignment->status, assignment->condition,
                         assignment->health, assignment->reason_text );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) neighbor (%s) unable to update "
                          "assignment (%s), error=%s.", service_domain_name, 
                          member_node_name, member_name,
                          sm_error_str( error ) );
                return;
            }

            error = sm_service_group_api_go_standby( member_name );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to perform action (%s) on member (%s) for "
                          "service domain (%s) on node (%s), error=%s.",
                          sm_service_group_action_str(member_action),
                          member_name, service_domain_name, member_node_name,
                          sm_error_str(error) );
                return;
            }
        break;

        case SM_SERVICE_GROUP_ACTION_AUDIT:
            // Will send the current member state below.
        break;

        case SM_SERVICE_GROUP_ACTION_RECOVER:
            error = sm_service_group_api_recover( member_name, escalate_recovery,
                                                  clear_fatal_condition );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to perform action (%s) on member (%s) for "
                          "service domain (%s) on node (%s), error=%s.",
                          sm_service_group_action_str(member_action),
                          member_name, service_domain_name, member_node_name,
                          sm_error_str(error) );
                return;
            }
        break;

        default:
            DPRINTFE( "Service domain (%s) unknown action (%s) received for "
                      "member (%s).", service_domain_name,
                      sm_service_group_action_str(member_action),
                      member_name );
            return;

        break;
    }

    if(( recover )&&( SM_SERVICE_GROUP_ACTION_RECOVER != member_action ))
    {
        error = sm_service_group_api_recover( member_name, escalate_recovery,
                                              clear_fatal_condition );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to perform recovery action on member (%s) for "
                      "service domain (%s) on node (%s), error=%s.",
                      member_name, service_domain_name, member_node_name,
                      sm_error_str(error) );
            return;
        }
    }

SEND_UPDATE:
    error = sm_service_domain_utils_send_member_update( service_domain_name,
                member_node_name, member_id, member_name,
                assignment->desired_state, assignment->state,
                assignment->status, assignment->condition, assignment->health,
                assignment->reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send member update for service domain (%s) node "
                  "(%s) of member (%s).", service_domain_name, member_node_name,
                  member_name );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Member Update Message Callback
// =========================================================
static void sm_service_domain_scheduler_member_update_msg_callback(
    SmNetworkAddressT* network_address, int network_port, int version,
    int revision, char service_domain_name[], char node_name[],
    char member_node_name[], int64_t member_id, char member_name[],
    SmServiceGroupStateT member_desired_state,
    SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
    SmServiceGroupConditionT member_condition, int64_t health,
    char reason_text[] )
{
    bool insync;
    SmErrorT error;

    if( '\0' == member_name[0] )
    {
        DPRINTFD( "Empty member name received from neighbor (%s).",
                  member_node_name );
        return;
    }

    error = sm_service_domain_utils_service_domain_neighbor_insync(
                                        service_domain_name, node_name,
                                        &insync );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to determine if service domain (%s) neighbor (%s) "
                  "is insync for assignment (%s).", service_domain_name,
                  node_name, member_name );
        return;
    }
    
    if( !insync )
    {
        DPRINTFI( "Service domain (%s) neighbor (%s) is not insync for "
                  "assignment (%s).", service_domain_name, member_node_name,
                  member_name );
        return;
    }

    error = sm_service_domain_utils_update_assignment( service_domain_name,
                member_node_name, member_name, member_desired_state,
                member_state, member_status, member_condition, health,
                reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) neighbor (%s) unable to update "
                  "assignment (%s), error=%s.", service_domain_name, 
                  member_node_name, member_name, sm_error_str( error ) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Service Group State Callback
// =======================================================
void sm_service_domain_scheduler_service_group_state_callback(
    char service_group_name[], SmServiceGroupStateT state,
    SmServiceGroupStatusT status, SmServiceGroupConditionT condition,
    int64_t health, const char reason_text[] )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmServiceDomainMemberT* member;
    SmServiceDomainAssignmentT* assignment;
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return;
    }

    member = sm_service_domain_member_table_read_service_group(
                                                        service_group_name );
    if( NULL == member )
    {
        DPRINTFD( "Failed to query service domain membership for service "
                  "group (%s), error=%s.", service_group_name, 
                  sm_error_str( error ) );
        return;
    }

    assignment = sm_service_domain_assignment_table_read( member->name,
                                            hostname, service_group_name );
    if( NULL == assignment )
    {
        DPRINTFE( "Failed to read service domain (%s) assignment for node "
                  "(%s) of member (%s).", member->name, hostname,
                  service_group_name );
        return;
    }

    error = sm_service_domain_utils_update_assignment( 
                member->name, hostname, service_group_name,
                assignment->desired_state, state, status, condition,
                health, reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) neighbor (%s) unable to update "
                  "assignment (%s), error=%s.", member->name, hostname,
                  service_group_name, sm_error_str( error ) );
        return;
    }

    error = sm_service_domain_utils_send_member_update( assignment->name,
                assignment->node_name, assignment->id,
                assignment->service_group_name, assignment->desired_state,
                assignment->state, assignment->status, assignment->condition,
                assignment->health, assignment->reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send member update for service domain (%s) node "
                  "(%s) of member (%s), error=%s.", assignment->name,
                  assignment->node_name, assignment->service_group_name,
                  sm_error_str( error ) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Initialize
// =====================================
SmErrorT sm_service_domain_scheduler_initialize( SmDbHandleT* sm_db_handle )
{
    SmErrorT error;

    _sm_db_handle = sm_db_handle;

    error = sm_timer_register( "service domain scheduler",
                               SM_SERVICE_DOMAIN_SCHEDULE_INTERVAL_IN_MS,
                               sm_service_domain_scheduler_timeout,
                               0, &_scheduler_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create scheduler timer, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    _msg_callbacks.node_swact
        = sm_service_domain_scheduler_node_swact_msg_callback;
    _msg_callbacks.member_request
        = sm_service_domain_scheduler_member_request_msg_callback ;
    _msg_callbacks.member_update 
        = sm_service_domain_scheduler_member_update_msg_callback;

    error = sm_msg_register_callbacks( &_msg_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register message callbacks, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_api_register_callback(
                    sm_service_domain_scheduler_service_group_state_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for service group state callbacks, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_filter_initialize( _sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain filtering, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_weight_initialize( _sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to intialize service domain weighting, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Finalize
// ===================================
SmErrorT sm_service_domain_scheduler_finalize( void )
{
    SmErrorT error;

    _sm_db_handle = NULL;

    error = sm_service_domain_filter_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain filtering, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_domain_weight_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain weighting, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_group_api_deregister_callback(
                    sm_service_domain_scheduler_service_group_state_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister for service group state callbacks, "
                  "error=%s.", sm_error_str( error ) );
    }

    if( SM_TIMER_ID_INVALID != _scheduler_timer_id )
    {
        error = sm_timer_deregister( _scheduler_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel scheduler timer, error=%s.", 
                      sm_error_str( error ) );
        }

        _scheduler_timer_id = SM_TIMER_ID_INVALID;
    }

    error = sm_msg_deregister_callbacks( &_msg_callbacks ); 
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister message callbacks, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
