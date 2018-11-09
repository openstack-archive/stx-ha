//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_FAILOVER_FAIL_PENDING_STATE_H__
#define __SM_FAILOVER_FAIL_PENDING_STATE_H__
#include "sm_types.h"
#include "sm_failover_fsm.h"
#include "sm_timer.h"

class SmFailoverFailPendingState : public SmFSMState
{
    public:
        SmFailoverFailPendingState(SmFailoverFSM& fsm);
        virtual ~SmFailoverFailPendingState();
        SmErrorT enter_state();
        SmErrorT exit_state();

    protected:
        SmErrorT event_handler(SmFailoverEventT event, const ISmFSMEventData* event_data);

    private:
        SmTimerIdT _pending_timer_id;
        SmTimerIdT _delay_query_hbs_timer_id;

        static bool _fail_pending_timeout(SmTimerIdT timer_id, int64_t user_data);
        static bool _delay_query_hbs_timeout(SmTimerIdT timer_id, int64_t user_data);
        static void cluster_hbs_response_callback();
        SmErrorT _register_timer();
        SmErrorT _deregister_timer();
};


#endif //__SM_FAILOVER_FAIL_PENDING_STATE_H__