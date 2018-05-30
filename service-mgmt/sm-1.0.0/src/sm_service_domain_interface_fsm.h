//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_INTERFACE_FSM_H__
#define __SM_SERVICE_DOMAIN_INTERFACE_FSM_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmServiceDomainInterfaceFsmCallbackT) (char service_domain[],
        char service_domain_interface[], SmInterfaceStateT interface_state);

// ****************************************************************************
// Service Domain Interface FSM - Register Callback
// ================================================
extern SmErrorT sm_service_domain_interface_fsm_register_callback(
    SmServiceDomainInterfaceFsmCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Deregister Callback
// ==================================================
extern SmErrorT sm_service_domain_interface_fsm_deregister_callback(
    SmServiceDomainInterfaceFsmCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Set State
// ========================================
extern SmErrorT sm_service_domain_interface_fsm_set_state(
    char service_domain[], char service_domain_interface[],
    SmInterfaceStateT state, const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Event Handler
// ============================================
extern SmErrorT sm_service_domain_interface_fsm_event_handler(
    char service_domain[], char service_domain_interface[],
    SmServiceDomainInterfaceEventT event, void* event_data[],
    const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Initialize
// =========================================
extern SmErrorT sm_service_domain_interface_fsm_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Finalize
// =======================================
extern SmErrorT sm_service_domain_interface_fsm_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_INTERFACE_FSM_H__
