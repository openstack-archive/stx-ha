//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_failover_normal_state.h"
#include <stdlib.h>
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_failover_fsm.h"

SmErrorT SmFailoverNormalState::enter_state()
{
    DPRINTFI("Entering normal state");
    return SM_OKAY;
}

SmErrorT SmFailoverNormalState::event_handler(SmFailoverEventT event, const ISmFSMEventData* event_data)
{
    switch (event)
    {
        case SM_HEARTBEAT_LOST:
        case SM_INTERFACE_DOWN:
            this->fsm.set_state(SM_FAILOVER_STATE_FAIL_PENDING);
            break;
        case SM_INTERFACE_UP:
            DPRINTFI("Interface is up");
            break;
        case SM_HEARTBEAT_RECOVER:
            DPRINTFI("Heartbeat is recovered");
            break;
        default:
            DPRINTFE("Runtime error, unexpected event %d, at state %d", event, this->fsm.get_state());
    }
    return SM_OKAY;
}

SmErrorT SmFailoverNormalState::exit_state()
{
    DPRINTFI("Exiting normal state");
    return SM_OKAY;
}
