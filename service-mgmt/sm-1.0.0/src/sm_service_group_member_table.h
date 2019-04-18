//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_MEMBER_TABLE_H__
#define __SM_SERVICE_GROUP_MEMBER_TABLE_H__

#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int64_t id;
    char name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    char service_name[SM_SERVICE_NAME_MAX_CHAR];
    SmServiceStateT service_state;
    SmServiceStatusT service_status;
    SmServiceConditionT service_condition;
    SmServiceSeverityT service_failure_impact;
    long service_failure_timestamp;
    bool provisioned;
} SmServiceGroupMemberT;

typedef void (*SmServiceGroupMemberTableForEachCallbackT)
    (void* user_data[], SmServiceGroupMemberT* service_group_member);

// ****************************************************************************
// Service Group Member Table - Read
// =================================
extern SmServiceGroupMemberT* sm_service_group_member_table_read(
    char service_group_name[], char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - Read By Identifier
// ===============================================
extern SmServiceGroupMemberT* sm_service_group_member_table_read_by_id( 
    int64_t service_group_member_id );
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - Read By Service
// ============================================
extern SmServiceGroupMemberT* sm_service_group_member_table_read_by_service( 
    char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - For Each
// =====================================
extern void sm_service_group_member_table_foreach( void* user_data[], 
    SmServiceGroupMemberTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - For Each Member
// ============================================
extern void sm_service_group_member_table_foreach_member( 
    char service_group_name[], void* user_data[],
    SmServiceGroupMemberTableForEachCallbackT callback );
// ****************************************************************************

extern SmErrorT sm_service_group_member_provision(char service_group_name[], char service_name[]);

extern SmErrorT sm_service_group_member_deprovision(char service_group_name[], char service_name[]);

// ****************************************************************************
// Service Group Member Table - Load
// =================================
extern SmErrorT sm_service_group_member_table_load( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - Persist
// ====================================
extern SmErrorT sm_service_group_member_table_persist( 
    SmServiceGroupMemberT* service_group_member );
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - Initialize
// =======================================
extern SmErrorT sm_service_group_member_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Member Table - Finalize
// =====================================
extern SmErrorT sm_service_group_member_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_MEMBER_TABLE_H__
