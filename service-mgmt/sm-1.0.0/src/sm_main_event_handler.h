//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_MAIN_EVENT_HANDLER_H__
#define __SM_MAIN_EVENT_HANDLER_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Main Event Handler - Reload Data
// ================================
extern void sm_main_event_handler_reload_data( void );
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - Initialize
// ===============================
extern SmErrorT sm_main_event_handler_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Main Event Handler - Finalize
// =============================
extern SmErrorT sm_main_event_handler_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_MAIN_EVENT_HANDLER_H__
