//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_OTHER_STATE_H__
#define __SM_SERVICE_DOMAIN_OTHER_STATE_H__

#include "sm_types.h"
#include "sm_service_domain_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Domain Other State - Entry
// ==================================
extern SmErrorT sm_service_domain_other_state_entry(
    SmServiceDomainT* domain );
// ****************************************************************************

// ****************************************************************************
// Service Domain Other State - Exit
// =================================
extern SmErrorT sm_service_domain_other_state_exit(
    SmServiceDomainT* domain );
// ****************************************************************************

// ****************************************************************************
// Service Domain Other State - Transition
// =======================================
extern SmErrorT sm_service_domain_other_state_transition(
    SmServiceDomainT* domain, SmServiceDomainStateT from_state );
// ****************************************************************************

// ****************************************************************************
// Service Domain Other State - Event Handler
// ==========================================
extern SmErrorT sm_service_domain_other_state_event_handler(
    SmServiceDomainT* domain, SmServiceDomainEventT event, void* event_data[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Other State - Initialize
// =======================================
extern SmErrorT sm_service_domain_other_state_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Other State - Finalize
// =====================================
extern SmErrorT sm_service_domain_other_state_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_OTHER_STATE_H__
