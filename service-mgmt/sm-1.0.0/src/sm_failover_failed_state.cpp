//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_failover_failed_state.h"
#include <stdlib.h>
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_failover_fsm.h"
#include "sm_failover_ss.h"

SmErrorT SmFailoverFailedState::event_handler(SmFailoverEventT event, const ISmFSMEventData* event_data)
{
    // Currently the only supported scenario to recover from failure is
    // reboot triggered by mtce.
    // So once entering failed state, wait for reboot to reenter the normal state.
    switch (event)
    {
        case SM_FAILOVER_EVENT_IF_STATE_CHANGED:
            // event will be fired, but couldn't bring fsm state back to normal
            break;

        default:
            DPRINTFE("Runtime error, unexpected event %s, at state %s",
                sm_failover_event_str(event),
                sm_failover_state_str(this->fsm.get_state()));
    }
    return SM_OKAY;
}
