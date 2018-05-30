//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_DOMAIN_MEMBERS_H__
#define __SM_DB_SERVICE_DOMAIN_MEMBERS_H__

#include <stdbool.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_DOMAIN_MEMBERS_TABLE_NAME                            "SERVICE_DOMAIN_MEMBERS"
#define SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_ID                       "ID"
#define SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_PROVISIONED              "PROVISIONED"
#define SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_NAME                     "NAME"
#define SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_NAME       "SERVICE_GROUP_NAME"
#define SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_REDUNDANCY_MODEL         "REDUNDANCY_MODEL"
#define SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_N_ACTIVE                 "N_ACTIVE"
#define SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_M_STANDBY                "M_STANDBY"
#define SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_SERVICE_GROUP_AGGREGATE  "SERVICE_GROUP_AGGREGATE"
#define SM_SERVICE_DOMAIN_MEMBERS_TABLE_COLUMN_ACTIVE_ONLY_IF_ACTIVE    "ACTIVE_ONLY_IF_ACTIVE"

typedef struct
{
    int64_t id;
    bool provisioned;
    char name[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char service_group_name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    SmServiceDomainMemberRedundancyModelT redundancy_model;
    int n_active;
    int m_standby;
    char service_group_aggregate[SM_SERVICE_GROUP_AGGREGATE_NAME_MAX_CHAR];
    char active_only_if_active[SM_SERVICE_GROUP_NAME_MAX_CHAR];
} SmDbServiceDomainMemberT;

// ****************************************************************************
// Database Service Domain Members - Convert
// =========================================
extern SmErrorT sm_db_service_domain_members_convert( const char* col_name,
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Read
// ======================================
extern SmErrorT sm_db_service_domain_members_read( SmDbHandleT* sm_db_handle,
    char name[], char service_group_name[], SmDbServiceDomainMemberT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Query
// =======================================
extern SmErrorT sm_db_service_domain_members_query( SmDbHandleT* sm_db_handle,
    const char* db_query, SmDbServiceDomainMemberT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Count
// =======================================
extern SmErrorT sm_db_service_domain_members_count( SmDbHandleT* sm_db_handle,
    const char* db_distinct_columns, const char* db_query, int* count );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Insert
// ========================================
extern SmErrorT sm_db_service_domain_members_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceDomainMemberT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Update
// ========================================
extern SmErrorT sm_db_service_domain_members_update( SmDbHandleT* sm_db_handle,
    SmDbServiceDomainMemberT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Delete
// ========================================
extern SmErrorT sm_db_service_domain_members_delete( SmDbHandleT* sm_db_handle,
    char name[], char service_group_name[] );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Create Table
// ==============================================
extern SmErrorT sm_db_service_domain_members_create_table( 
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Delete Table
// ==============================================
extern SmErrorT sm_db_service_domain_members_delete_table( 
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Initialize
// ============================================
extern SmErrorT sm_db_service_domain_members_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Finalize
// ==========================================
extern SmErrorT sm_db_service_domain_members_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_DOMAIN_MEMBERS_H__
