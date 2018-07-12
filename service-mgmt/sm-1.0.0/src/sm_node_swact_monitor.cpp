//
// Copyright (c) 2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_node_swact_monitor.h"
#include "sm_debug.h"
#include "sm_node_utils.h"
#include "sm_swact_state.h"
#include "sm_service_enable.h"

bool SmNodeSwactMonitor::swact_started = false;
static bool duplex = false;
SmNodeScheduleStateT SmNodeSwactMonitor::my_role = SM_NODE_STATE_UNKNOWN;

void SmNodeSwactMonitor::SwactStart(SmNodeScheduleStateT my_role)
{
    sm_node_utils_is_aio_duplex(&duplex);
    if( duplex )
    {
        // Set the swact state to start so task affining thread
        // can affine tasks to all idle cores to speed up swact activity.
        DPRINTFI("Start of swact: affining tasks to idle cores...");
        sm_set_swact_state(SM_SWACT_STATE_START);
        sm_service_enable_throttle_add_cores(true);
    }
    SmNodeSwactMonitor::swact_started = true;
    SmNodeSwactMonitor::my_role = my_role;
    DPRINTFI("Swact has started, host will be %s", sm_node_schedule_state_str(my_role));
}

void SmNodeSwactMonitor::SwactUpdate(const char* hostname, SmNodeScheduleStateT node_state)
{
    if( SmNodeSwactMonitor::swact_started )
    {
        DPRINTFI("Swact update: %s is now %s", hostname,
            sm_node_schedule_state_str(node_state));
        if( SM_NODE_STATE_ACTIVE == node_state )
        {
            SmNodeSwactMonitor::SwactCompleted(
                SM_NODE_STATE_ACTIVE == SmNodeSwactMonitor::my_role );
        }

    }
}

void SmNodeSwactMonitor::SwactCompleted(bool result)
{
    if( duplex )
    {
        // Set the swact state to end so task affining thread
        // can reaffine tasks back to the platform cores.
        DPRINTFI("End of swact: reaffining tasks back to platform cores...");
        sm_set_swact_state(SM_SWACT_STATE_END);
        sm_service_enable_throttle_add_cores(false);
    }
    DPRINTFI("Swact has %s.", result ? "completed successfully": "failed");
}
