//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_ACTION_TABLE_H__
#define __SM_SERVICE_ACTION_TABLE_H__

#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_time.h"
#include "sm_sha512.h"

#ifdef __cplusplus
extern "C" {
#endif

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
    SmSha512HashT hash;
    SmTimeT last_hash_validate;
} SmServiceActionDataT;

// ****************************************************************************
// Service Action Table - Set Interval Extension
// =============================================
extern void sm_service_action_table_set_interval_extension(
    int interval_extension_in_secs );
// ****************************************************************************

// ****************************************************************************
// Service Action Table - Set Timeout Extension
// ============================================
extern void sm_service_action_table_set_timeout_extension(
    int timeout_extension_in_secs );
// ****************************************************************************

// ****************************************************************************
// Service Action Table - Read
// ===========================
extern SmServiceActionDataT* sm_service_action_table_read( char service_name[],
    SmServiceActionT action );
// ****************************************************************************

// ****************************************************************************
// Service Action Table - Load
// ===========================
extern SmErrorT sm_service_action_table_load( void );
// ****************************************************************************

// ****************************************************************************
// Service Action Table - Initialize
// =================================
extern SmErrorT sm_service_action_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Action Table - Finalize
// ===============================
extern SmErrorT sm_service_action_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_ACTION_TABLE_H__
