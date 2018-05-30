//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GO_ACTIVE_H__
#define __SM_SERVICE_GO_ACTIVE_H__

#include <stdbool.h>

#include "sm_types.h"
#include "sm_service_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Go-Active
// =================
extern SmErrorT sm_service_go_active( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Go-Active - Abort
// =========================
extern SmErrorT sm_service_go_active_abort( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Go-Active - Exists
// ==========================
extern SmErrorT sm_service_go_active_exists( SmServiceT* service, bool* exists );
// ****************************************************************************

// ****************************************************************************
// Service Go-Active - Initialize
// ==============================
extern SmErrorT sm_service_go_active_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Go-Active - Finalize
// ============================
extern SmErrorT sm_service_go_active_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GO_ACTIVE_H__
