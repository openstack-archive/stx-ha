//
// Copyright (c) 2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_NODE_SWACT_MONITOR_H__
#define __SM_NODE_SWACT_MONITOR_H__
#include "sm_types.h"

class SmNodeSwactMonitor
{
    public:
        static void SwactStart(SmNodeScheduleStateT my_role);
        static void SwactUpdate(const char* hostname, SmNodeScheduleStateT node_state);
        static void SwactCompleted(bool result);

    private:
        static bool swact_started;
        static SmNodeScheduleStateT my_role;
};

#endif
