//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_ACTIONS_H__
#define __SM_DB_SERVICE_ACTIONS_H__

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_ACTIONS_TABLE_NAME                           "SERVICE_ACTIONS"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_SERVICE_NAME            "SERVICE_NAME"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_ACTION                  "ACTION"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_TYPE             "PLUGIN_TYPE"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_CLASS            "PLUGIN_CLASS"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_NAME             "PLUGIN_NAME"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_COMMAND          "PLUGIN_COMMAND"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_PLUGIN_PARAMETERS       "PLUGIN_PARAMETERS"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_FAILURE_RETRIES     "MAX_FAILURE_RETRIES"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_TIMEOUT_RETRIES     "MAX_TIMEOUT_RETRIES"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_MAX_TOTAL_RETRIES       "MAX_TOTAL_RETRIES"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_TIMEOUT_IN_SECS         "TIMEOUT_IN_SECS"
#define SM_SERVICE_ACTIONS_TABLE_COLUMN_INTERVAL_IN_SECS        "INTERVAL_IN_SECS"

typedef struct
{
    char service_name[SM_SERVICE_NAME_MAX_CHAR];
    SmServiceActionT action;
    char plugin_type[SM_SERVICE_ACTION_PLUGIN_TYPE_MAX_CHAR];
    char plugin_class[SM_SERVICE_ACTION_PLUGIN_CLASS_MAX_CHAR];
    char plugin_name[SM_SERVICE_ACTION_PLUGIN_NAME_MAX_CHAR];
    char plugin_command[SM_SERVICE_ACTION_PLUGIN_COMMAND_MAX_CHAR];
    char plugin_params[SM_SERVICE_ACTION_PLUGIN_PARAMS_MAX_CHAR];
    int max_failure_retries;
    int max_timeout_retries;
    int max_total_retries;
    int timeout_in_secs;
    int interval_in_secs;
} SmDbServiceActionT;

// ****************************************************************************
// Database Service Actions - Convert
// ==================================
extern SmErrorT sm_db_service_actions_convert( const char* col_name, 
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Actions - Read
// ===============================
extern SmErrorT sm_db_service_actions_read( SmDbHandleT* sm_db_handle,
    char service_name[], SmServiceActionT action, SmDbServiceActionT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Actions - Insert
// =================================
extern SmErrorT sm_db_service_actions_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceActionT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Actions - Update
// =================================
extern SmErrorT sm_db_service_actions_update( SmDbHandleT* sm_db_handle,
    SmDbServiceActionT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Actions - Delete
// =================================
extern SmErrorT sm_db_service_actions_delete( SmDbHandleT* sm_db_handle,
    char service_name[], SmServiceActionT action );
// ****************************************************************************

// ****************************************************************************
// Database Service Actions - Create Table
// =======================================
extern SmErrorT sm_db_service_actions_create_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Actions - Delete Table
// =======================================
extern SmErrorT sm_db_service_actions_delete_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Actions - Initialize
// =====================================
extern SmErrorT sm_db_service_actions_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Actions - Finalize
// ===================================
extern SmErrorT sm_db_service_actions_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_ACTIONS_H__
