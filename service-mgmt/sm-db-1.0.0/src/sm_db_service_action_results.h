//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_ACTION_RESULTS_H__
#define __SM_DB_SERVICE_ACTION_RESULTS_H__

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_ACTION_RESULTS_TABLE_NAME                     "SERVICE_ACTION_RESULTS"
#define SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_TYPE       "PLUGIN_TYPE"
#define SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_NAME       "PLUGIN_NAME"
#define SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_COMMAND    "PLUGIN_COMMAND"
#define SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_PLUGIN_EXIT_CODE  "PLUGIN_EXIT_CODE"
#define SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_ACTION_RESULT     "ACTION_RESULT"
#define SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_STATE     "SERVICE_STATE"
#define SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_STATUS    "SERVICE_STATUS"
#define SM_SERVICE_ACTION_RESULTS_TABLE_COLUMN_SERVICE_CONDITION "SERVICE_CONDITION"

typedef struct
{
    char plugin_type[SM_SERVICE_ACTION_PLUGIN_TYPE_MAX_CHAR];
    char plugin_name[SM_SERVICE_ACTION_PLUGIN_NAME_MAX_CHAR];
    char plugin_command[SM_SERVICE_ACTION_PLUGIN_COMMAND_MAX_CHAR];
    char plugin_exit_code[SM_SERVICE_ACTION_PLUGIN_EXIT_CODE_MAX_CHAR];
    SmServiceActionResultT action_result;
    SmServiceStateT service_state;
    SmServiceStatusT service_status;
    SmServiceConditionT service_condition;
} SmDbServiceActionResultT;

// ****************************************************************************
// Database Service Action Results - Convert
// =========================================
extern SmErrorT sm_db_service_action_results_convert( const char* col_name,
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Action Results - Read
// ======================================
extern SmErrorT sm_db_service_action_results_read( SmDbHandleT* sm_db_handle,
    char plugin_type[], char plugin_name[], char plugin_command[],
    char plugin_exit_code[], SmDbServiceActionResultT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Action Results - Insert
// ========================================
extern SmErrorT sm_db_service_action_results_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceActionResultT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Action Results - Update
// ========================================
extern SmErrorT sm_db_service_results_update( SmDbHandleT* sm_db_handle,
    SmDbServiceActionResultT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Action Results - Delete
// ========================================
extern SmErrorT sm_db_service_action_results_delete( SmDbHandleT* sm_db_handle,
    char plugin_type[], char plugin_name[], char plugin_command[],
    char plugin_exit_code[] );
// ****************************************************************************

// ****************************************************************************
// Database Service Action Results - Create Table
// ==============================================
extern SmErrorT sm_db_service_action_results_create_table(
   SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Action Results - Delete Table
// ==============================================
extern SmErrorT sm_db_service_action_results_delete_table(
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Action Results - Initialize
// ============================================
extern SmErrorT sm_db_service_action_results_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Action Results - Finalize
// ==========================================
extern SmErrorT sm_db_service_action_results_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_ACTION_RESULTS_H__
