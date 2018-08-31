//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//

#include "sm_failover_ss.h"
#include "sm_debug.h"


typedef enum
{
    SM_FAILOVER_INFRA_DOWN = 1,
    SM_FAILOVER_MGMT_DOWN = 2,
    SM_FAILOVER_OAM_DOWN = 4,
}SmFailoverCommFaultBitFlagT;

// ****************************************************************************
// sm_failover_ss get_node_if_healthy_score - get node interface healthy score
// ===================
static int get_node_if_healthy_score(unsigned int interface_state)
{
    int healthy_score = 0;
    if(interface_state & SM_FAILOVER_OAM_DOWN)
    {
        healthy_score -= 1;
    }
    if(interface_state & SM_FAILOVER_INFRA_DOWN)
    {
        healthy_score -= 2;
    }
    if(interface_state & SM_FAILOVER_INFRA_DOWN)
    {
        healthy_score -= 4;
    }

    return healthy_score;
}
// ****************************************************************************

// ****************************************************************************
// sm_failover_ss_get_survivor - select the failover survivor
// This is the main entry/container for the failover logic to determine how
// to schedule the controllers, i.e, active/standby or active/failure.
// ===================
SmErrorT sm_failover_ss_get_survivor(const SmSystemStatusT& system_status, SmSystemFailoverStatus& selection)
{
    selection.host_schedule_state = system_status.host_status.current_schedule_state;
    selection.peer_schedule_state = system_status.peer_status.current_schedule_state;
    if(SM_HEARTBEAT_OK == system_status.heartbeat_state)
    {
        int host_healthy_score, peer_healthy_score;
        host_healthy_score = get_node_if_healthy_score(system_status.host_status.interface_state);
        peer_healthy_score = get_node_if_healthy_score(system_status.peer_status.interface_state);
        if( peer_healthy_score < host_healthy_score )
        {
            //host is more healthy
            selection.host_schedule_state = SM_NODE_STATE_ACTIVE;
            selection.peer_schedule_state = SM_NODE_STATE_STANDBY;
        }else if(peer_healthy_score > host_healthy_score)
        {
            //peer is more healthy
            selection.host_schedule_state = SM_NODE_STATE_STANDBY;
            selection.peer_schedule_state = SM_NODE_STATE_ACTIVE;
        }
    }

    if(system_status.host_status.current_schedule_state != selection.host_schedule_state ||
        system_status.peer_status.current_schedule_state != selection.peer_schedule_state )
    {
        DPRINTFI("Uncontrolled swact starts. Host from %s to %s, Peer from %s to %s.",
            sm_node_schedule_state_str(system_status.host_status.current_schedule_state),
            sm_node_schedule_state_str(selection.host_schedule_state),
            sm_node_schedule_state_str(system_status.peer_status.current_schedule_state),
            sm_node_schedule_state_str(selection.peer_schedule_state)
        );
    }
    return SM_OKAY;
}
// ****************************************************************************