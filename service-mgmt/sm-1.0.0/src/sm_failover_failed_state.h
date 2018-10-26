//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_FAILOVER_FAILED_STATE_H__
#define __SM_FAILOVER_FAILED_STATE_H__
#include "sm_types.h"
#include "sm_failover_fsm.h"
#include "sm_timer.h"

class SmFailoverFailedState : public SmFSMState
{
    public:
        SmFailoverFailedState(SmFailoverFSM& fsm) : SmFSMState(fsm){}

    protected:
        SmErrorT event_handler(SmFailoverEventT event, const ISmFSMEventData* event_data);

};


#endif //__SM_FAILOVER_FAILED_STATE_H__