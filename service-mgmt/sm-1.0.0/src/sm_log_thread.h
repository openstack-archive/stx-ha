//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_LOG_THREAD_H__
#define __SM_LOG_THREAD_H__

#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_log_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SM_LOG_THREAD_MSG_NODE_REBOOT_LOG,
    SM_LOG_THREAD_MSG_NODE_REBOOT_FORCE_LOG,
    SM_LOG_THREAD_MSG_NODE_STATE_CHANGE_LOG,
    SM_LOG_THREAD_MSG_LICENSE_CHANGE_LOG,
    SM_LOG_THREAD_MSG_COMMUNICATION_STATE_CHANGE_LOG,
    SM_LOG_THREAD_MSG_INTERFACE_STATE_CHANGE_LOG,
    SM_LOG_THREAD_MSG_NEIGHBOR_STATE_CHANGE_LOG,
    SM_LOG_THREAD_MSG_SERVICE_DOMAIN_STATE_CHANGE_LOG,
    SM_LOG_THREAD_MSG_SERVICE_GROUP_REDUNDANCY_CHANGE_LOG,
    SM_LOG_THREAD_MSG_SERVICE_GROUP_STATE_CHANGE_LOG,
    SM_LOG_THREAD_MSG_SERVICE_STATE_CHANGE_LOG,
} SmLogThreadMsgTypeT;

typedef struct
{
    struct timespec ts_real;
    SmLogNodeNameT node_name;
    SmLogEntityNameT entity_name;
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR];
} SmLogThreadMsgRebootLogT;

typedef struct
{
    struct timespec ts_real;
    SmLogNodeNameT node_name;
    SmLogDomainNameT domain_name;
    SmLogServiceGroupNameT service_group_name;
    SmLogEntityNameT entity_name;
    char prev_state[SM_LOG_ENTITY_STATE_MAX_CHAR];
    char prev_status[SM_LOG_ENTITY_STATUS_MAX_CHAR];
    char prev_condition[SM_LOG_ENTITY_CONDITION_MAX_CHAR];
    char state[SM_LOG_ENTITY_STATE_MAX_CHAR];
    char status[SM_LOG_ENTITY_STATUS_MAX_CHAR];
    char condition[SM_LOG_ENTITY_CONDITION_MAX_CHAR];
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR];
} SmLogThreadMsgStateChangeLogT;

typedef struct
{
    SmLogThreadMsgTypeT type;

    union
    {
        SmLogThreadMsgRebootLogT reboot;
        SmLogThreadMsgStateChangeLogT state_change;
    } u;
} SmLogThreadMsgT;

// ****************************************************************************
// Log Thread - Start
// ==================
extern SmErrorT sm_log_thread_start( int server_fd );
// ****************************************************************************

// ****************************************************************************
// Log Thread - Stop
// =================
extern SmErrorT sm_log_thread_stop( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_LOG_THREAD_H__
