//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_ACTION_RESULT_TABLE_H__
#define __SM_SERVICE_ACTION_RESULT_TABLE_H__

#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

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
} SmServiceActionResultDataT;

// ****************************************************************************
// Service Action Result Table - Read
// ==================================
extern SmServiceActionResultDataT* sm_service_action_result_table_read( 
    char plugin_type[], char plugin_name[], char plugin_command[],
    char plugin_exit_code[] );
// ****************************************************************************

// ****************************************************************************
// Service Action Result Table - Load
// ==================================
extern SmErrorT sm_service_action_result_table_load( void );
// ****************************************************************************

// ****************************************************************************
// Service Action Result Table - Initialize
// ========================================
extern SmErrorT sm_service_action_result_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Action Result Table - Finalize
// ======================================
extern SmErrorT sm_service_action_result_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_ACTION_RESULT_TABLE_H__
