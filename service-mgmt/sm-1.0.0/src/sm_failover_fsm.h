//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_FAILOVER_FSM_H__
#define __SM_FAILOVER_FSM_H__
#include "sm_types.h"

typedef int SmFSMEventDataTypeT;

class ISmFSMEventData
{
    public:
        virtual SmFSMEventDataTypeT get_event_data_type() const = 0;
};

class SmFailoverFSM;

class SmFSMState
{
    public:
        SmFSMState(SmFailoverFSM& fsm) : fsm(fsm) {};
        virtual ~SmFSMState(){};
        virtual SmErrorT enter_state();
        virtual SmErrorT exit_state();
    protected:
        SmFailoverFSM& fsm;
        virtual SmErrorT event_handler(SmFailoverEventT event, const ISmFSMEventData* event_data)=0;
    friend SmFailoverFSM;
};

class SmFailoverFSM
{
    public:
        SmFailoverFSM();
        virtual ~SmFailoverFSM();

        SmErrorT send_event(SmFailoverEventT event, const ISmFSMEventData* event_data);

        SmErrorT register_fsm_state(SmFailoverStateT state, SmFSMState* state_handler);

        SmErrorT set_state(SmFailoverStateT state);
        inline SmFailoverStateT get_state() const {return this->_current_state;}

        static SmErrorT initialize();
        static SmErrorT finalize();
        inline static SmFailoverFSM& get_fsm() {
            return _the_fsm;
        }

    private:
        static const int MaxState = SM_FAILOVER_STATE_MAX;
        SmFSMState* _state_handlers[MaxState];
        SmFailoverStateT _current_state;

        static SmFailoverFSM _the_fsm;
        SmErrorT init_state();
        void deregister_states();
};

class SmIFStateChangedEventData: public ISmFSMEventData
{
    public:
        static const SmFSMEventDataTypeT SmIFStateChangedEventDataType = 2;
        SmFSMEventDataTypeT get_event_data_type() const;
        SmIFStateChangedEventData();
        void set_interface_state(
            SmInterfaceTypeT interface_type, SmFailoverInterfaceStateT interface_state);
        SmFailoverInterfaceStateT get_interface_state(SmInterfaceTypeT interface_type) const;

    private:
        SmFailoverInterfaceStateT _mgmt_state;
        SmFailoverInterfaceStateT _cluster_host_state;
        SmFailoverInterfaceStateT _oam_state;
};

#endif //__SM_FAILOVER_FSM_H__
