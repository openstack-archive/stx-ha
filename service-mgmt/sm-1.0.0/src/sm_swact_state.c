// Copyright (c) 2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_swact_state.h"
#include "pthread.h"

volatile SmSwactStateT _swact_state = SM_SWACT_STATE_NONE;
static pthread_mutex_t _state_mutex;

// ****************************************************************************
// Swact State - Swact State Setter
// ============================================
void sm_set_swact_state(SmSwactStateT state)
{
    // The state is set by sm main thread and task affining thread.
    pthread_mutex_lock(&_state_mutex);
     _swact_state = state;
    pthread_mutex_unlock(&_state_mutex);
}

// ****************************************************************************
// Swact State - Swact State Getter
// ============================================
SmSwactStateT sm_get_swact_state( void )
{
    return _swact_state;
}
