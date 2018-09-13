//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_FAILOVER_NORMAL_STATE_H__
#define __SM_FAILOVER_NORMAL_STATE_H__
#include "sm_types.h"
#include "sm_failover_fsm.h"

class SmFailoverNormalState : public SmFSMState
{
    public:
        SmFailoverNormalState(SmFailoverFSM& fsm) : SmFSMState(fsm){}
        SmErrorT enter_state();
        SmErrorT exit_state();
    protected:
        SmErrorT event_handler(SmFailoverEventT event, const ISmFSMEventData* event_data);
};


#endif //__SM_FAILOVER_NORMAL_STATE_H__