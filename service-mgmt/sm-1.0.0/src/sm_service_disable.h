//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DISABLE_H__
#define __SM_SERVICE_DISABLE_H__

#include <stdbool.h>

#include "sm_types.h"
#include "sm_service_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Disable
// ===============
extern SmErrorT sm_service_disable( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Disable - Abort
// =======================
extern SmErrorT sm_service_disable_abort( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Disable - Exists
// ========================
extern SmErrorT sm_service_disable_exists( SmServiceT* service, bool* exists );
// ****************************************************************************

// ****************************************************************************
// Service Disable - Initialize
// ============================
extern SmErrorT sm_service_disable_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Disable - Finalize
// ==========================
extern SmErrorT sm_service_disable_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DISABLE_H__
