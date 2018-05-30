//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_AUDIT_H__
#define __SM_SERVICE_AUDIT_H__

#include "sm_types.h"
#include "sm_service_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Audit - Enabled
// =======================
extern SmErrorT sm_service_audit_enabled( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Audit - Disabled
// ========================
extern SmErrorT sm_service_audit_disabled( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Audit - Enabled Abort
// =============================
extern SmErrorT sm_service_audit_enabled_abort( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Audit - Disabled Abort
// ==============================
extern SmErrorT sm_service_audit_disabled_abort( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Audit - Enabled Abort
// =============================
extern SmErrorT sm_service_audit_enabled_abort( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Audit - Disabled Abort
// ==============================
extern SmErrorT sm_service_audit_disabled_abort( SmServiceT* service );
// ****************************************************************************

// ****************************************************************************
// Service Audit - Enabled Interval
// ================================
extern SmErrorT sm_service_audit_enabled_interval( SmServiceT* service,
    int* interval );
// ****************************************************************************

// ****************************************************************************
// Service Audit - Disabled Interval
// =================================
extern SmErrorT sm_service_audit_disabled_interval( SmServiceT* service,
    int* interval );
// ****************************************************************************

// ****************************************************************************
// Service Audit - Enabled Exists
// ==============================
extern SmErrorT sm_service_audit_enabled_exists( SmServiceT* service,
    bool* exists );
// ****************************************************************************

// ****************************************************************************
// Service Audit - Disabled Exists
// ===============================
extern SmErrorT sm_service_audit_disabled_exists( SmServiceT* service,
    bool* exists );
// ****************************************************************************

// ****************************************************************************
// Service Audit - Initialized
// ===========================
extern SmErrorT sm_service_audit_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Audit - Finalize
// ========================
extern SmErrorT sm_service_audit_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_AUDIT_H__
