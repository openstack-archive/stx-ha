//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_FAILOVER_UTILS_H__
#define __SM_FAILOVER_UTILS_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Failover Utilities - get node controller state
// ==============================
SmNodeScheduleStateT sm_get_controller_state(const char node_name[]);
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_FAILOVER_UTILS_H__
