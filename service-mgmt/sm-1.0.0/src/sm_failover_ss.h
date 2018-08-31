//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __SM_FAILOVER_SS_H__
#define __SM_FAILOVER_SS_H__
#include <stdio.h>
#include "sm_types.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const char* node_name;
    unsigned int interface_state;
    SmNodeScheduleStateT current_schedule_state;
}SmNodeStatusT;

typedef enum
{
    //heartbeat ok
    SM_HEARTBEAT_OK,
    //single node situation
    SM_HEARTBEAT_NA,
    //other nodes report heartbeat with peer, no direct heartbeat
    SM_HEARTBEAT_INDIRECT,
    //no heartbeat
    SM_HEARTBEAT_LOSS
}SmHeartbeatStatusT;

typedef struct
{
    SmNodeStatusT host_status;
    SmNodeStatusT peer_status;
    SmHeartbeatStatusT heartbeat_state;
    SmSystemModeT system_mode;
}SmSystemStatusT;


typedef struct
{
    SmNodeScheduleStateT host_schedule_state;
    SmNodeScheduleStateT peer_schedule_state;
}SmSystemFailoverStatus;

// ****************************************************************************
// sm_failover_ss_get_survivor - select the failover survivor
// ===================
SmErrorT sm_failover_ss_get_survivor(const SmSystemStatusT& system_status, SmSystemFailoverStatus& selection);

#ifdef __cplusplus
}
#endif
#endif // __SM_FAILOVER_SS_H__
