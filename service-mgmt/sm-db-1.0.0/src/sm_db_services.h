//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICES_H__
#define __SM_DB_SERVICES_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_timer.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICES_TABLE_NAME                              "SERVICES"
#define SM_SERVICES_TABLE_COLUMN_ID                         "ID"
#define SM_SERVICES_TABLE_COLUMN_PROVISIONED                "PROVISIONED"
#define SM_SERVICES_TABLE_COLUMN_NAME                       "NAME"
#define SM_SERVICES_TABLE_COLUMN_DESIRED_STATE              "DESIRED_STATE"
#define SM_SERVICES_TABLE_COLUMN_STATE                      "STATE"
#define SM_SERVICES_TABLE_COLUMN_STATUS                     "STATUS"
#define SM_SERVICES_TABLE_COLUMN_CONDITION                  "CONDITION"
#define SM_SERVICES_TABLE_COLUMN_MAX_FAILURES               "MAX_FAILURES"
#define SM_SERVICES_TABLE_COLUMN_FAIL_COUNTDOWN             "FAIL_COUNTDOWN"
#define SM_SERVICES_TABLE_COLUMN_FAIL_COUNTDOWN_INTERVAL    "FAIL_COUNTDOWN_INTERVAL"
#define SM_SERVICES_TABLE_COLUMN_MAX_ACTION_FAILURES        "MAX_ACTION_FAILURES"
#define SM_SERVICES_TABLE_COLUMN_MAX_TRANSITION_FAILURES    "MAX_TRANSITION_FAILURES"
#define SM_SERVICES_TABLE_COLUMN_PID_FILE                   "PID_FILE"

typedef struct
{
    int64_t id;
    bool provisioned;
    char name[SM_SERVICE_NAME_MAX_CHAR];
    SmServiceStateT desired_state;
    SmServiceStateT state;
    SmServiceStatusT status;
    SmServiceConditionT condition;
    int max_failures;
    int fail_countdown;
    int fail_countdown_interval_in_ms;
    int max_action_failures;
    int max_transition_failures;    
    char pid_file[SM_SERVICE_PID_FILE_MAX_CHAR];
} SmDbServiceT;

// ****************************************************************************
// Database Services - Convert
// ===========================
extern SmErrorT sm_db_services_convert( const char* col_name,
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Services - Read
// ========================
extern SmErrorT sm_db_services_read( SmDbHandleT* sm_db_handle,
    char name[], SmDbServiceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Services - Read By Identifier
// ======================================
extern SmErrorT sm_db_services_read_by_id( SmDbHandleT* sm_db_handle,
    int64_t id, SmDbServiceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Services - Query
// =========================
extern SmErrorT sm_db_services_query( SmDbHandleT* sm_db_handle,
    const char* db_query, SmDbServiceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Services - Insert
// ==========================
extern SmErrorT sm_db_services_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Services - Update
// ==========================
extern SmErrorT sm_db_services_update( SmDbHandleT* sm_db_handle,
    SmDbServiceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Services - Delete
// ==========================
extern SmErrorT sm_db_services_delete( SmDbHandleT* sm_db_handle,
    char name[] );
// ****************************************************************************

// ****************************************************************************
// Database Services - Create Table
// ================================
extern SmErrorT sm_db_services_create_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Services - Delete Table
// ================================
extern SmErrorT sm_db_services_delete_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Services - Cleanup Table
// =================================
extern SmErrorT sm_db_services_cleanup_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Services - Initialize
// ==============================
extern SmErrorT sm_db_services_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Services - Finalize
// ============================
extern SmErrorT sm_db_services_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICES_H__
