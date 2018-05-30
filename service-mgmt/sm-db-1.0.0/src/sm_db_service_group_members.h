//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_GROUP_MEMBERS_H__
#define __SM_DB_SERVICE_GROUP_MEMBERS_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_GROUP_MEMBERS_TABLE_NAME                             "SERVICE_GROUP_MEMBERS"
#define SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_ID                        "ID"
#define SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_PROVISIONED               "PROVISIONED"
#define SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_NAME                      "NAME"
#define SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_NAME              "SERVICE_NAME"
#define SM_SERVICE_GROUP_MEMBERS_TABLE_COLUMN_SERVICE_FAILURE_IMPACT    "SERVICE_FAILURE_IMPACT"

typedef struct
{
    int64_t id;
    bool provisioned;
    char name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    char service_name[SM_SERVICE_NAME_MAX_CHAR];
    SmServiceSeverityT service_failure_impact;
} SmDbServiceGroupMemberT;

// ****************************************************************************
// Database Service Group Members - Convert
// ========================================
extern SmErrorT sm_db_service_group_members_convert( const char* col_name,
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Read
// =====================================
extern SmErrorT sm_db_service_group_members_read( SmDbHandleT* sm_db_handle,
    char name[], char service_name[], SmDbServiceGroupMemberT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Query
// ======================================
extern SmErrorT sm_db_service_group_members_query( SmDbHandleT* sm_db_handle,
    const char* db_query, SmDbServiceGroupMemberT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Insert
// =======================================
extern SmErrorT sm_db_service_group_members_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceGroupMemberT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Update
// =======================================
extern SmErrorT sm_db_service_group_members_update( SmDbHandleT* sm_db_handle,
    SmDbServiceGroupMemberT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Delete
// =======================================
extern SmErrorT sm_db_service_group_members_delete( SmDbHandleT* sm_db_handle,
    char name[], char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Create Table
// =============================================
extern SmErrorT sm_db_service_group_members_create_table( 
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Delete Table
// =============================================
extern SmErrorT sm_db_service_group_members_delete_table( 
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Cleanup Table
// ==============================================
extern SmErrorT sm_db_service_group_members_cleanup_table( 
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Initialize
// ===========================================
extern SmErrorT sm_db_service_group_members_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Group Members - Finalize
// =========================================
extern SmErrorT sm_db_service_group_members_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_GROUP_MEMBERS_H__
