//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_GROUPS_H__
#define __SM_DB_SERVICE_GROUPS_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_GROUPS_TABLE_NAME                      "SERVICE_GROUPS"
#define SM_SERVICE_GROUPS_TABLE_COLUMN_ID                 "ID"
#define SM_SERVICE_GROUPS_TABLE_COLUMN_PROVISIONED        "PROVISIONED"
#define SM_SERVICE_GROUPS_TABLE_COLUMN_NAME               "NAME"
#define SM_SERVICE_GROUPS_TABLE_COLUMN_AUTO_RECOVER       "AUTO_RECOVER"
#define SM_SERVICE_GROUPS_TABLE_COLUMN_CORE               "CORE"
#define SM_SERVICE_GROUPS_TABLE_COLUMN_DESIRED_STATE      "DESIRED_STATE"
#define SM_SERVICE_GROUPS_TABLE_COLUMN_STATE              "STATE"
#define SM_SERVICE_GROUPS_TABLE_COLUMN_STATUS             "STATUS"
#define SM_SERVICE_GROUPS_TABLE_COLUMN_CONDITION          "CONDITION"
#define SM_SERVICE_GROUPS_TABLE_COLUMN_FAILURE_DEBOUNCE   "FAILURE_DEBOUNCE"
#define SM_SERVICE_GROUPS_TABLE_COLUMN_FATAL_ERROR_REBOOT "FATAL_ERROR_REBOOT"

typedef struct
{
    int64_t id;
    bool provisioned;
    char name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    bool auto_recover;
    bool core;
    SmServiceGroupStateT desired_state;
    SmServiceGroupStateT state;
    SmServiceGroupStatusT status;
    SmServiceGroupConditionT condition;    
    int failure_debounce_in_ms;
    bool fatal_error_reboot;
} SmDbServiceGroupT;

// ****************************************************************************
// Database Service Groups - Convert
// =================================
extern SmErrorT sm_db_service_groups_convert( const char* col_name, 
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Groups - Read
// ==============================
extern SmErrorT sm_db_service_groups_read( SmDbHandleT* sm_db_handle,
    char name[], SmDbServiceGroupT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Groups - Insert
// ================================
extern SmErrorT sm_db_service_groups_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceGroupT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Groups - Update
// ================================
extern SmErrorT sm_db_service_groups_update( SmDbHandleT* sm_db_handle,
    SmDbServiceGroupT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Groups - Delete
// ================================
extern SmErrorT sm_db_service_groups_delete( SmDbHandleT* sm_db_handle,
    char name[] );
// ****************************************************************************

// ****************************************************************************
// Database Service Groups - Create Table
// ======================================
extern SmErrorT sm_db_service_groups_create_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Groups - Delete Table
// ======================================
extern SmErrorT sm_db_service_groups_delete_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Groups - Cleanup Table
// =======================================
extern SmErrorT sm_db_service_groups_cleanup_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Groups - Initialize
// ====================================
extern SmErrorT sm_db_service_groups_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Groups - Finalize
// ==================================
extern SmErrorT sm_db_service_groups_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_GROUPS_H__
