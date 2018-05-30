//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_MEMBER_TABLE_H__
#define __SM_SERVICE_DOMAIN_MEMBER_TABLE_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_limits.h"
#include "sm_types.h"

#ifdef __cplusplus
extern "C" { 
#endif

typedef struct
{
    int64_t id;
    char name[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char service_group_name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    SmServiceDomainMemberRedundancyModelT redundancy_model;
    int n_active;
    int m_standby;
    char service_group_aggregate[SM_SERVICE_GROUP_AGGREGATE_NAME_MAX_CHAR];
    char active_only_if_active[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    char redundancy_log_text[SM_LOG_REASON_TEXT_MAX_CHAR];
} SmServiceDomainMemberT;

typedef void (*SmServiceDomainMemberTableForEachCallbackT)
    (void* user_data[], SmServiceDomainMemberT* member);

// ****************************************************************************
// Service Domain Member Table - Read
// ==================================
extern SmServiceDomainMemberT* sm_service_domain_member_table_read(
    char name[], char service_group_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Read By Identifier
// ================================================
extern SmServiceDomainMemberT* sm_service_domain_member_table_read_by_id(
    int64_t id );
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Read Service Group
// ================================================
extern SmServiceDomainMemberT*
sm_service_domain_member_table_read_service_group( char service_group_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Count
// ===================================
extern unsigned int sm_service_domain_member_table_count( 
    char service_domain_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - For Each
// ======================================
extern void sm_service_domain_member_table_foreach( char service_domain_name[],
    void* user_data[], SmServiceDomainMemberTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - For Each Service Group Aggregate
// ==============================================================
extern void sm_service_domain_member_table_foreach_service_group_aggregate(
    char service_domain_name[], char service_group_aggregate[],
    void* user_data[], SmServiceDomainMemberTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Load
// ==================================
extern SmErrorT sm_service_domain_member_table_load( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Initialize
// ========================================
extern SmErrorT sm_service_domain_member_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Member Table - Finalize
// ======================================
extern SmErrorT sm_service_domain_member_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_MEMBER_TABLE_H__
