//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_ASSIGNMENT_TABLE_H__
#define __SM_SERVICE_DOMAIN_ASSIGNMENT_TABLE_H__

#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_uuid.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int64_t id;
    SmUuidT uuid;
    char name[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char node_name[SM_NODE_NAME_MAX_CHAR];
    char service_group_name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    SmServiceGroupStateT desired_state;
    SmServiceGroupStateT state;
    SmServiceGroupStatusT status;
    SmServiceGroupConditionT condition;
    int64_t health;
    long last_state_change;
    SmServiceDomainSchedulingStateT sched_state;
    int sched_weight;
    SmServiceDomainSchedulingListT sched_list;
    char reason_text[SM_SERVICE_GROUP_REASON_TEXT_MAX_CHAR];
    bool exchanged;
} SmServiceDomainAssignmentT;

typedef int (*SmServiceDomainAssignmentTableCompareCallbackT)
    (const void* assignment_lhs, const void* assignment_rhs);

typedef bool (*SmServiceDomainAssignmentTableCountCallbackT)
    (void* user_data[], SmServiceDomainAssignmentT* assignment);

typedef void (*SmServiceDomainAssignmentTableForEachCallbackT)
    (void* user_data[], SmServiceDomainAssignmentT* assignment);

// ****************************************************************************
// Service Domain Assignment Table - Read
// ======================================
extern SmServiceDomainAssignmentT*
sm_service_domain_assignment_table_read( char name[], char node_name[],
    char service_group_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Read By Identifier
// ====================================================
extern SmServiceDomainAssignmentT* 
sm_service_domain_assignment_table_read_by_id( int64_t id );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Count
// =======================================
extern unsigned int sm_service_domain_assignment_table_count( 
    char service_domain_name[], void* user_data[],
    SmServiceDomainAssignmentTableCountCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Count Service Group
// =====================================================
extern unsigned int sm_service_domain_assignment_table_count_service_group(
    char service_domain_name[], char service_group_name[], void* user_data[],
    SmServiceDomainAssignmentTableCountCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - For Each
// ==========================================
extern void sm_service_domain_assignment_table_foreach( 
    char service_domain_name[], void* user_data[],
    SmServiceDomainAssignmentTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - For Each Service Group
// ========================================================
extern void sm_service_domain_assignment_table_foreach_service_group( 
    char service_domain_name[], char service_group_name[], void* user_data[],
    SmServiceDomainAssignmentTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - For Each Node in Service Domain
// =================================================================
extern void sm_service_domain_assignment_table_foreach_node_in_service_domain(
    char service_domain_name[], char node_name[], void* user_data[],
    SmServiceDomainAssignmentTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - For Each Node
// ===============================================
extern void sm_service_domain_assignment_table_foreach_node( 
    char node_name[], void* user_data[],
    SmServiceDomainAssignmentTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - For Each Schedule List
// ========================================================
extern void sm_service_domain_assignment_table_foreach_schedule_list(
    char service_domain_name[], SmServiceDomainSchedulingListT sched_list,
    void* user_data[], SmServiceDomainAssignmentTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Sort
// ======================================
extern void sm_service_domain_assignment_table_sort( 
    SmServiceDomainAssignmentTableCompareCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Get Next Node
// ===============================================
extern SmServiceDomainAssignmentT* 
sm_service_domain_assignment_table_get_next_node(
    char service_domain_name[], char node_name[], int64_t last_id );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Get Last Node
// ===============================================
extern SmServiceDomainAssignmentT*
sm_service_domain_assignment_table_get_last_node(
    char service_domain_name[], char node_name[], int64_t last_id );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Insert
// ========================================
extern SmErrorT sm_service_domain_assignment_table_insert( char name[],
    char node_name[], char service_group_name[],
    SmServiceGroupStateT desired_state, SmServiceGroupStateT state,
    SmServiceGroupStatusT status, SmServiceGroupConditionT condition,
    int64_t health, long last_state_change, const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Delete
// ========================================
extern SmErrorT sm_service_domain_assignment_table_delete( char name[],
    char node_name[], char service_group_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Load
// ======================================
extern SmErrorT sm_service_domain_assignment_table_load( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Persist
// =========================================
extern SmErrorT sm_service_domain_assignment_table_persist(
    SmServiceDomainAssignmentT* assignment );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Initialize
// ============================================
extern SmErrorT sm_service_domain_assignment_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Assignment Table - Finalize
// ==========================================
extern SmErrorT sm_service_domain_assignment_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_ASSIGNMENT_TABLE_H__
