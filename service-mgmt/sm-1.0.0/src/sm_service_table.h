//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_TABLE_H__
#define __SM_SERVICE_TABLE_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int64_t id;
    char name[SM_SERVICE_NAME_MAX_CHAR];
    char instance_name[SM_SERVICE_INSTANCE_NAME_MAX_CHAR];
    char instance_params[SM_SERVICE_INSTANCE_PARAMS_MAX_CHAR];    
    SmServiceStateT desired_state;
    SmServiceStateT state;
    SmServiceStatusT status;
    SmServiceConditionT condition;
    bool recover;
    bool clear_fatal_condition;
    int max_failures;
    int fail_count;
    int fail_countdown;
    int fail_countdown_interval_in_ms;
    SmTimerIdT fail_countdown_timer_id;
    SmTimerIdT audit_timer_id;
    SmServiceActionT action_running;
    int action_pid;
    SmTimerIdT action_timer_id;
    int action_attempts;
    SmTimerIdT action_state_timer_id;
    int pid;
    char pid_file[SM_SERVICE_PID_FILE_MAX_CHAR];
    SmTimerIdT pid_file_audit_timer_id;
    int action_fail_count;
    int max_action_failures;
    int transition_fail_count;
    int max_transition_failures;
    bool go_active_action_exists;
    bool go_standby_action_exists;
    bool enable_action_exists;
    bool disable_action_exists;
    bool audit_enabled_exists;
    bool audit_disabled_exists;
    bool disable_check_dependency;
    //flag to indicate disable a service without disabling its dependency
    bool disable_skip_dependent;
    char group_name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    bool provisioned;
} SmServiceT;

typedef void (*SmServiceTableForEachCallbackT) 
    (void* user_data[], SmServiceT* service);

// ****************************************************************************
// Service Table - Read
// ====================
extern SmServiceT* sm_service_table_read( char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Table - Read By Identifier
// ==================================
extern SmServiceT* sm_service_table_read_by_id( int64_t service_id );
// ****************************************************************************

// ****************************************************************************
// Service Table - Read By Pid
// ===========================
extern SmServiceT* sm_service_table_read_by_pid( int pid );
// ****************************************************************************

// ****************************************************************************
// Service Table - Read By Action Pid
// ==================================
extern SmServiceT* sm_service_table_read_by_action_pid( int pid );
// ****************************************************************************

// ****************************************************************************
// Service Table - For Each
// ========================
extern void sm_service_table_foreach( void* user_data[],
    SmServiceTableForEachCallbackT callback );
// ****************************************************************************

extern SmErrorT sm_service_provision(char service_name[]);

extern SmErrorT sm_service_deprovision(char service_name[]);

// ****************************************************************************
// Service Table - Load
// ====================
extern SmErrorT sm_service_table_load( void );
// ****************************************************************************

// ****************************************************************************
// Service Table - Persist
// =======================
extern SmErrorT sm_service_table_persist( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Table - Initialize
// ==========================
extern SmErrorT sm_service_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Table - Finalize
// ========================
extern SmErrorT sm_service_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_TABLE_H__
