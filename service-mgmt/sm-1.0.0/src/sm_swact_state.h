//
// Copyright (c) 2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SWACT_STATE_H__
#define __SM_SWACT_STATE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SM_SWACT_STATE_NONE,
    SM_SWACT_STATE_START,
    SM_SWACT_STATE_END,
} SmSwactStateT;

// ****************************************************************************
// Swact State - Setter
// ==========================================
// ****************************************************************************
extern void sm_set_swact_state( SmSwactStateT state );

// ****************************************************************************
// Swact State - Getter
// ==========================================
extern SmSwactStateT sm_get_swact_state( void );

#ifdef __cplusplus
}
#endif

#endif
