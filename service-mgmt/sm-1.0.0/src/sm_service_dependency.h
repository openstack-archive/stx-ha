//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DEPENDENCY_H__
#define __SM_SERVICE_DEPENDENCY_H__

#include "sm_types.h"
#include "sm_service_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Dependency - Go-Active Met 
// ==================================
extern SmErrorT sm_service_dependency_go_active_met( SmServiceT* service,
    bool* met );
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Go-Standby Met 
// ===================================
extern SmErrorT sm_service_dependency_go_standby_met( SmServiceT* service,
    bool* met );
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Enable Met 
// ===============================
extern SmErrorT sm_service_dependency_enable_met( SmServiceT* service,
    bool* met );
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Disable Met 
// ================================
extern SmErrorT sm_service_dependency_disable_met( SmServiceT* service,
    bool* met );
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Enabled Active State Met 
// =============================================
extern SmErrorT sm_service_dependency_enabled_active_state_met(
    SmServiceT* service, bool* met );
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Enabled Standby State Met 
// ==============================================
extern SmErrorT sm_service_dependency_enabled_standby_state_met(
    SmServiceT* service, bool* met );
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Dependents Require Disable
// ===============================================
extern SmErrorT sm_service_dependency_dependents_require_disable(
    SmServiceT* service, bool* disable_required );
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Dependents Require Notification
// ====================================================
extern SmErrorT sm_service_dependency_dependents_require_notification(
    SmServiceT* service, bool* notification );
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Initialize
// ===============================
extern SmErrorT sm_service_dependency_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Finalize
// =============================
extern SmErrorT sm_service_dependency_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DEPENDENCY_H__
