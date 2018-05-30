//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GO_STANDBY_H__
#define __SM_SERVICE_GO_STANDBY_H__

#include <stdbool.h>

#include "sm_types.h"
#include "sm_service_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Go-Standby
// ==================
extern SmErrorT sm_service_go_standby( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Go-Standby - Abort
// ==========================
extern SmErrorT sm_service_go_standby_abort( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Go-Standby - Exists
// ===========================
extern SmErrorT sm_service_go_standby_exists( SmServiceT* service, bool* exists );
// ****************************************************************************

// ****************************************************************************
// Service Go-Standby - Initialize
// ===============================
extern SmErrorT sm_service_go_standby_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Go-Standby - Finalize
// =============================
extern SmErrorT sm_service_go_standby_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GO_STANDBY_H__
