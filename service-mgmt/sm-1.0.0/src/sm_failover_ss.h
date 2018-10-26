//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//

#ifndef __SM_FAILOVER_SS_H__
#define __SM_FAILOVER_SS_H__
#include <stdio.h>
#include "sm_types.h"

typedef struct
{
    const char* node_name;
    unsigned int interface_state;
    unsigned int heartbeat_state;
    SmFailoverInterfaceStateT mgmt_state;
    SmFailoverInterfaceStateT infra_state;
    SmFailoverInterfaceStateT oam_state;
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


class SmSystemFailoverStatus
{
    public:
        virtual ~SmSystemFailoverStatus();
        inline SmNodeScheduleStateT get_host_schedule_state() const {
            return _host_schedule_state;
        }
        inline SmNodeScheduleStateT get_host_pre_failure_schedule_state() const {
            return _host_pre_failure_schedule_state;
        }
        void set_host_schedule_state(SmNodeScheduleStateT state);
        void set_host_pre_failure_schedule_state(SmNodeScheduleStateT state);
        inline SmNodeScheduleStateT get_peer_schedule_state() const {
            return _peer_schedule_state;
        }
        inline SmNodeScheduleStateT get_peer_pre_failure_schedule_state() const {
            return _peer_pre_failure_schedule_state;
        }
        void set_peer_schedule_state(SmNodeScheduleStateT state);
        void set_peer_pre_failure_schedule_state(SmNodeScheduleStateT state);
        void serialize();
        void deserialize();
        static SmSystemFailoverStatus& get_status();
    private:
        SmSystemFailoverStatus();
        SmNodeScheduleStateT _host_pre_failure_schedule_state;
        SmNodeScheduleStateT _peer_pre_failure_schedule_state;
        SmNodeScheduleStateT _host_schedule_state;
        SmNodeScheduleStateT _peer_schedule_state;
        static const char filename[];
        static const char file_format[];

        bool _is_valid_schedule_state(SmNodeScheduleStateT state);
        static SmSystemFailoverStatus _failover_status;
};

// ****************************************************************************
// sm_failover_ss_get_survivor - select the failover survivor
// ===================
SmErrorT sm_failover_ss_get_survivor(SmSystemFailoverStatus& selection);

SmErrorT sm_failover_ss_get_survivor(const SmSystemStatusT& system_status, SmSystemFailoverStatus& selection);

#endif // __SM_FAILOVER_SS_H__
