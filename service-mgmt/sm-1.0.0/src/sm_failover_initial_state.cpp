//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_failover_initial_state.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_failover_fsm.h"

SmErrorT SmFailoverInitialState::event_handler(SmFailoverEventT event, const ISmFSMEventData* event_data)
{
    switch (event)
    {
        case SM_FAILOVER_EVENT_HEARTBEAT_ENABLED:
            this->fsm.set_state(SM_FAILOVER_STATE_NORMAL);
            break;
        case SM_FAILOVER_EVENT_IF_STATE_CHANGED:
        case SM_FAILOVER_EVENT_FAIL_PENDING_TIMEOUT:
        case SM_FAILOVER_EVENT_NODE_ENABLED:
            break;
        default:
            DPRINTFE("Runtime error, unexpected event %s, at state %s",
                sm_failover_event_str(event),
                sm_failover_state_str(this->fsm.get_state()));
    }
    return SM_OKAY;
}
