//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_failover_survived_state.h"
#include <stdlib.h>
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_failover_fsm.h"
#include "sm_failover_ss.h"

static void _audit_failover_state()
{
    SmSystemFailoverStatus& failover_status = SmSystemFailoverStatus::get_status();
    SmErrorT error = sm_failover_ss_get_survivor(failover_status);
    SmNodeScheduleStateT host_state = failover_status.get_host_schedule_state();
    SmNodeScheduleStateT peer_state = failover_status.get_peer_schedule_state();
    if(SM_OKAY != error)
    {
        DPRINTFE("Failed to get failover survivor. Error %s", sm_error_str(error));
        return;
    }

    if(SM_NODE_STATE_ACTIVE == host_state && SM_NODE_STATE_FAILED == peer_state)
    {
        // nothing changed yet
    }
    else if(SM_NODE_STATE_ACTIVE == host_state && SM_NODE_STATE_STANDBY == peer_state)
    {
        // Active is the only possible state to be scheduled to from survived state
        SmFailoverFSM::get_fsm().set_state(SM_FAILOVER_STATE_NORMAL);
    }else
    {
        DPRINTFE("Runtime error: unexpected scheduling state: host %s, peer %s",
            sm_node_schedule_state_str(host_state),
            sm_node_schedule_state_str(peer_state));
    }
}

SmErrorT SmFailoverSurvivedState::event_handler(SmFailoverEventT event, const ISmFSMEventData* event_data)
{
    switch (event)
    {
        case SM_FAILOVER_EVENT_IF_STATE_CHANGED:
            break;
        case SM_FAILOVER_EVENT_NODE_ENABLED:
            _audit_failover_state();
            break;
        default:
            DPRINTFE("Runtime error, unexpected event %s, at state %s",
                sm_failover_event_str(event),
                sm_failover_state_str(this->fsm.get_state()));
    }
    return SM_OKAY;
}

