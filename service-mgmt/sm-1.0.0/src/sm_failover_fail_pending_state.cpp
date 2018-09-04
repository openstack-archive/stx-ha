//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_failover_fail_pending_state.h"
#include <stdlib.h>
#include "sm_types.h"
#include "sm_limits.h"
#include "sm_debug.h"
#include "sm_failover.h"
#include "sm_failover_fsm.h"
#include "sm_failover_ss.h"
#include "sm_failover_utils.h"
#include "sm_node_utils.h"
#include "sm_node_api.h"

static const int FAIL_PENDING_TIMEOUT = 2000; //2000ms

SmErrorT SmFailoverFailPendingState::event_handler(SmFailoverEventT event, const ISmFSMEventData* event_data)
{
    //SmFSMEventDataTypeT event_data_type = event_data->get_event_data_type();
    switch (event)
    {
        case SM_HEARTBEAT_LOST:
        case SM_INTERFACE_DOWN:
            //getting worse, extend wait time
        case SM_INTERFACE_UP:
        case SM_HEARTBEAT_RECOVER:
            // Things are improving, extend wait time
            this->_register_timer();
            break;
        case SM_FAIL_PENDING_TIMEOUT:
            {
                SmSystemFailoverStatus failover_status;
                SmErrorT error = sm_failover_ss_get_survivor(failover_status);
                SmNodeScheduleStateT host_state = failover_status.get_host_schedule_state();
                SmNodeScheduleStateT peer_state = failover_status.get_peer_schedule_state();
                if(SM_OKAY != error)
                {
                    DPRINTFE("Failed to get failover survivor. Error %s", sm_error_str(error));
                    return error;
                }

                error = sm_failover_set_system(failover_status);
                if(SM_NODE_STATE_FAILED == host_state)
                {
                    this->fsm.set_state(SM_FAILOVER_STATE_FAILED);
                }
                else if(SM_NODE_STATE_ACTIVE == host_state)
                {
                    if(SM_NODE_STATE_FAILED == peer_state)
                    {
                        this->fsm.set_state(SM_FAILOVER_STATE_SURVIVED);
                    }else
                    {
                        this->fsm.set_state(SM_FAILOVER_STATE_NORMAL);
                    }
                }
                else if(SM_NODE_STATE_STANDBY == host_state)
                {
                    this->fsm.set_state(SM_FAILOVER_STATE_NORMAL);
                }else
                {
                    DPRINTFE("Runtime error: unexpected scheduling state: %s",
                        sm_node_schedule_state_str(host_state));
                }
                return error;
            }
            break;
        default:
            DPRINTFE("Runtime error, unexpected event %d, at state %d", event, this->fsm.get_state());
    }
    return SM_OKAY;
}

bool SmFailoverFailPendingState::_fail_pending_timeout(
    SmTimerIdT timer_id, int64_t user_data)
{
    SmFailoverFSM::get_fsm().send_event(SM_FAIL_PENDING_TIMEOUT, NULL);
    return false;
}

SmErrorT SmFailoverFailPendingState::enter_state()
{
    DPRINTFI("Entering Pending state");
    SmErrorT error = this->_register_timer();
    if(SM_OKAY != error)
    {
        DPRINTFE("Failed to register fail pending timer. Error %s", sm_error_str(error));
    }
    return error;
}

SmErrorT SmFailoverFailPendingState::_register_timer()
{
    SmErrorT error;
    const char* timer_name = "FAIL PENDING TIMER";
    if(SM_TIMER_ID_INVALID != this->_pending_timer_id)
    {
        this->_deregister_timer();
    }

    error = sm_timer_register( timer_name, FAIL_PENDING_TIMEOUT,
                               SmFailoverFailPendingState::_fail_pending_timeout,
                               0, &this->_pending_timer_id);

    return error;
}

SmErrorT SmFailoverFailPendingState::_deregister_timer()
{
    SmErrorT error;
    if(SM_TIMER_ID_INVALID == this->_pending_timer_id)
    {
        return SM_OKAY;
    }

    error = sm_timer_deregister(this->_pending_timer_id);
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to cancel fail pending timer, error=%s.",
                  sm_error_str( error ) );
    }else
    {
        this->_pending_timer_id = SM_TIMER_ID_INVALID;
    }

    return error;
}

SmErrorT SmFailoverFailPendingState::exit_state()
{
    SmErrorT error = this->_deregister_timer();
    if(SM_OKAY != error)
    {
        DPRINTFE("Failed to deregister fail pending timer. Error %s", sm_error_str(error));
    }
    DPRINTFI("Exiting Pending state");
    return error;
}
