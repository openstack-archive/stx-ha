//
// Copyright (c) 2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_TASK_AFFINING_THREAD_H__
#define __SM_TASK_AFFINING_THREAD_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Task Affining Thread - Start
// ================================
extern SmErrorT sm_task_affining_thread_start( void );
// ****************************************************************************

// ****************************************************************************
// Task Affining Thread - Stop
// ===============================
extern SmErrorT sm_task_affining_thread_stop( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif
