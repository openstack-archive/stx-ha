//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_DOMAIN_ASSIGNMENTS_H__
#define __SM_DB_SERVICE_DOMAIN_ASSIGNMENTS_H__

#include <stdint.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_uuid.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_NAME                      "SERVICE_DOMAIN_ASSIGNMENTS"
#define SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_ID                 "ID"
#define SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_UUID               "UUID"
#define SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NAME               "NAME"
#define SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_NODE_NAME          "NODE_NAME"
#define SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_SERVICE_GROUP_NAME "SERVICE_GROUP_NAME"
#define SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_DESIRED_STATE      "DESIRED_STATE"
#define SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATE              "STATE"
#define SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_STATUS             "STATUS"
#define SM_SERVICE_DOMAIN_ASSIGNMENTS_TABLE_COLUMN_CONDITION          "CONDITION"

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
} SmDbServiceDomainAssignmentT;

// ****************************************************************************
// Database Service Domain Assignments - Convert
// =============================================
extern SmErrorT sm_db_service_domain_assignments_convert( const char* col_name,
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Read
// ==========================================
extern SmErrorT sm_db_service_domain_assignments_read(
    SmDbHandleT* sm_db_handle, char name[], char node_name[], 
    char service_group_name[], SmDbServiceDomainAssignmentT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Read By Identifier
// ========================================================
extern SmErrorT  sm_db_service_domain_assignments_read_by_id(
    SmDbHandleT* sm_db_handle, int64_t id,
    SmDbServiceDomainAssignmentT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Query
// ===========================================
extern SmErrorT sm_db_service_domain_assignments_query( 
    SmDbHandleT* sm_db_handle, const char* db_query,
    SmDbServiceDomainAssignmentT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Count
// ===========================================
extern SmErrorT sm_db_service_domain_assignments_count( 
    SmDbHandleT* sm_db_handle, const char* db_distinct_columns,
    const char* db_query, int* count );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Sum
// =========================================
extern SmErrorT sm_db_service_domain_assignments_sum( 
    SmDbHandleT* sm_db_handle, const char* db_sum_column,
    const char* db_query, int* sum );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Insert
// ============================================
extern SmErrorT sm_db_service_domain_assignments_insert(
    SmDbHandleT* sm_db_handle, SmDbServiceDomainAssignmentT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Update
// ============================================
extern SmErrorT sm_db_service_domain_assignments_update(
    SmDbHandleT* sm_db_handle, SmDbServiceDomainAssignmentT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Delete
// ============================================
extern SmErrorT sm_db_service_domain_assignments_delete(
    SmDbHandleT* sm_db_handle, char name[], char node_name[],
    char service_group_name[] );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Create Table
// ==================================================
extern SmErrorT sm_db_service_domain_assignments_create_table( 
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Delete Table
// ==================================================
extern SmErrorT sm_db_service_domain_assignments_delete_table( 
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Cleanup Table
// ===================================================
extern SmErrorT sm_db_service_domain_assignments_cleanup_table(
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Initialize
// ================================================
extern SmErrorT sm_db_service_domain_assignments_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Assignments - Finalize
// ==============================================
extern SmErrorT sm_db_service_domain_assignments_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_DOMAIN_ASSIGNMENTS_H__
