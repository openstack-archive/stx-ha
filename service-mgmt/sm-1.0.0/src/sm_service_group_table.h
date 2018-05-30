//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_TABLE_H__
#define __SM_SERVICE_GROUP_TABLE_H__

#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int64_t id;
    char name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    bool auto_recover;
    bool core;
    SmServiceGroupStateT desired_state;
    SmServiceGroupStateT state;
    SmServiceGroupStatusT status;
    SmServiceGroupConditionT condition;
    int64_t health;
    int failure_debounce_in_ms;
    bool fatal_error_reboot;
    char reason_text[SM_SERVICE_GROUP_REASON_TEXT_MAX_CHAR];
    SmServiceGroupNotificationT notification;
    const char* notification_str;
    int notification_pid;
    SmTimerIdT notification_timer_id;
    bool notification_complete;
    bool notification_failed;
    bool notification_timeout;
} SmServiceGroupT;

typedef void (*SmServiceGroupTableForEachCallbackT) 
    (void* user_data[], SmServiceGroupT* service_group);

// ****************************************************************************
// Service Group Table - Read
// ==========================
extern SmServiceGroupT* sm_service_group_table_read( 
    char service_group_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Read By Identifier
// ========================================
extern SmServiceGroupT* sm_service_group_table_read_by_id( 
    int64_t service_group_id );
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Read By Notification Pid
// ==============================================
extern SmServiceGroupT* sm_service_group_table_read_by_notification_pid(
    int pid );
// ****************************************************************************

// ****************************************************************************
// Service Group Table - For Each
// ==============================
extern void sm_service_group_table_foreach( void* user_data[],
    SmServiceGroupTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Load
// ==========================
extern SmErrorT sm_service_group_table_load( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Persist
// =============================
extern SmErrorT sm_service_group_table_persist( SmServiceGroupT* service_group );
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Initialize
// ================================
extern SmErrorT sm_service_group_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Table - Finalize
// ==============================
extern SmErrorT sm_service_group_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_TABLE_H__
