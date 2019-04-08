//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_failover_fsm.h"
#include <stdlib.h>
#include "sm_debug.h"
#include "sm_failover_initial_state.h"
#include "sm_failover_normal_state.h"
#include "sm_failover_fail_pending_state.h"
#include "sm_failover_failed_state.h"
#include "sm_failover_survived_state.h"

SmFSMEventDataTypeT SmIFStateChangedEventData::get_event_data_type() const
{
    return SmIFStateChangedEventDataType;
}


SmIFStateChangedEventData::SmIFStateChangedEventData()
{
}

void SmIFStateChangedEventData::set_interface_state(
    SmInterfaceTypeT interface_type, SmFailoverInterfaceStateT interface_state)
{
    switch (interface_type)
    {
        case SM_INTERFACE_OAM:
            _oam_state = interface_state;
            break;
        case SM_INTERFACE_MGMT:
            _mgmt_state = interface_state;
            break;
        case SM_INTERFACE_CLUSTER_HOST:
            _cluster_host_state = interface_state;
            break;
        default:
            DPRINTFE("Runtime error: invalid interface type %d", interface_type);
    }
}

SmFailoverInterfaceStateT SmIFStateChangedEventData::get_interface_state(SmInterfaceTypeT interface_type) const
{
    switch (interface_type)
    {
        case SM_INTERFACE_OAM:
            return _oam_state;
        case SM_INTERFACE_MGMT:
            return _mgmt_state;
        case SM_INTERFACE_CLUSTER_HOST:
            return _cluster_host_state;
        default:
            DPRINTFE("Runtime error: invalid interface type %d", interface_type);
            return SM_FAILOVER_INTERFACE_UNKNOWN;
    }
}

SmErrorT SmFSMState::enter_state()
{
    DPRINTFI("Entering %s state", sm_failover_state_str(fsm.get_state()));
    return SM_OKAY;
};

SmErrorT SmFSMState::exit_state()
{
    DPRINTFI("Exiting %s state", sm_failover_state_str(fsm.get_state()));
    return SM_OKAY;
};

SmFailoverFSM SmFailoverFSM::_the_fsm;

void SmFailoverFSM::deregister_states()
{
    for(int i = 0; i < MaxState; i ++)
    {
        SmFSMState* state = _state_handlers[i];
        if(NULL != state)
        {
            _state_handlers[i] = NULL;
            delete state;
        }
    }
}

SmFailoverFSM::SmFailoverFSM()
{
    for(int i = 0; i < MaxState; i ++)
    {
        _state_handlers[i] = NULL;
    }
}

SmFailoverFSM::~SmFailoverFSM()
{
}

SmErrorT SmFailoverFSM::send_event(SmFailoverEventT event, const ISmFSMEventData* event_data)
{
    SmFSMState* state_handler = this->_state_handlers[this->get_state()];
    if(NULL == state_handler)
    {
        DPRINTFE("Runtime error. No handler for state %d", this->get_state());
        return SM_FAILED;
    }
    DPRINTFI("send_event %s\n", sm_failover_event_str(event));
    state_handler->event_handler(event, event_data);
    return SM_OKAY;
}

SmErrorT SmFailoverFSM::set_state(SmFailoverStateT state)
{
    if(state >= MaxState)
    {
        DPRINTFE("Runtime error. Don't understand state %d", state);
        return SM_FAILED;
    }
    if(state == this->get_state())
    {
        DPRINTFI("Already in %d state", state);
        return SM_OKAY;
    }

    SmFSMState* state_handler = this->_state_handlers[this->get_state()];
    if(NULL == state_handler)
    {
        DPRINTFE("State %d was not registered", this->get_state());
        return SM_FAILED;
    }

    SmErrorT error = state_handler->exit_state();
    if(SM_OKAY != error)
    {
        DPRINTFE("Exit state failed (%d), %d", this->get_state(), error);
        return SM_FAILED;
    }

    SmFSMState* new_state_handler = this->_state_handlers[state];
    if(NULL == new_state_handler)
    {
        DPRINTFE("State %d was not registered", state);
        return SM_FAILED;
    }

    this->_current_state = state;

    error = new_state_handler->enter_state();
    if(SM_OKAY != error)
    {
        DPRINTFE("Enter state failed (%d), %d", this->get_state(), error);
        return SM_FAILED;
    }

    return SM_OKAY;
}

SmErrorT SmFailoverFSM::register_fsm_state(SmFailoverStateT state, SmFSMState* state_handler)
{
    if(state >= MaxState)
    {
        DPRINTFE("Runtime error. Invalid state %d", state);
        return SM_FAILED;
    }

    if(NULL != this->_state_handlers[state])
    {
        DPRINTFE("Runtime error. Duplicated state handle for state %d", state);
        return SM_FAILED;
    }

    this->_state_handlers[state] = state_handler;
    return SM_OKAY;
}

SmErrorT SmFailoverFSM::init_state()
{
    SmErrorT error;
    SmFailoverStateT state = SM_FAILOVER_STATE_INITIAL;
    SmFSMState* new_state_handler = _state_handlers[state];
    if(NULL == new_state_handler)
    {
        DPRINTFE("State %d was not registered", state);
        return SM_FAILED;
    }
    error = new_state_handler->enter_state();
    if(SM_OKAY != error)
    {
        DPRINTFE("Enter state failed (%d), %d", this->get_state(), error);
        return SM_FAILED;
    }

    _current_state = state;
    return SM_OKAY;
}

SmErrorT SmFailoverFSM::initialize()
{
    SmFailoverFSM& fsm = SmFailoverFSM::get_fsm();
    fsm.register_fsm_state(SM_FAILOVER_STATE_INITIAL, new SmFailoverInitialState(fsm));
    fsm.register_fsm_state(SM_FAILOVER_STATE_NORMAL, new SmFailoverNormalState(fsm));
    fsm.register_fsm_state(SM_FAILOVER_STATE_FAIL_PENDING, new SmFailoverFailPendingState(fsm));
    fsm.register_fsm_state(SM_FAILOVER_STATE_FAILED, new SmFailoverFailedState(fsm));
    fsm.register_fsm_state(SM_FAILOVER_STATE_SURVIVED, new SmFailoverSurvivedState(fsm));

    return fsm.init_state();
}

SmErrorT SmFailoverFSM::finalize()
{
    SmFailoverFSM::get_fsm().deregister_states();
    return SM_OKAY;
}