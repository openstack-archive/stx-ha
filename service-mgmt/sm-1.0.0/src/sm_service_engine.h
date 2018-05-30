//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_ENGINE_H__
#define __SM_SERVICE_ENGINE_H__

#include "sm_types.h"
#include "sm_service_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Engine - Signal
// =======================
extern SmErrorT sm_service_engine_signal( SmServiceT* service  );
// ****************************************************************************

// ****************************************************************************
// Service Engine - Initialize
// ===========================
extern SmErrorT sm_service_engine_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Engine - Finalize
// =========================
extern SmErrorT sm_service_engine_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_ENGINE_H__
