//
// Copyright (c) 2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_FAILOVER_THREAD_H__
#define __SM_FAILOVER_THREAD_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif


// ****************************************************************************
// Failover Thread - Start
// ================================
extern SmErrorT sm_failover_thread_start( void );
// ****************************************************************************

// ****************************************************************************
// Failover Thread - Stop
// ===============================
extern SmErrorT sm_failover_thread_stop( void );
// ****************************************************************************


#ifdef __cplusplus
}
#endif

#endif