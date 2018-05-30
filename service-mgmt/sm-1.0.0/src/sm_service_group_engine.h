//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_ENGINE_H__
#define __SM_SERVICE_GROUP_ENGINE_H__

#include "sm_types.h"
#include "sm_service_group_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Group Engine - Signal
// =============================
extern SmErrorT sm_service_group_engine_signal(
    SmServiceGroupT* service_group  );
// ****************************************************************************

// ****************************************************************************
// Service Group Engine - Initialize
// =================================
extern SmErrorT sm_service_group_engine_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Engine - Finalize
// ===============================
extern SmErrorT sm_service_group_engine_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_ENGINE_H__
