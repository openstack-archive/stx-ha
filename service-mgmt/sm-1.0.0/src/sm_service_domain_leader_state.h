//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_LEADER_STATE_H__
#define __SM_SERVICE_DOMAIN_LEADER_STATE_H__

#include "sm_types.h"
#include "sm_service_domain_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Domain Leader State - Entry
// ===================================
extern SmErrorT sm_service_domain_leader_state_entry(
    SmServiceDomainT* domain );
// ****************************************************************************

// ****************************************************************************
// Service Domain Leader State - Exit
// ==================================
extern SmErrorT sm_service_domain_leader_state_exit(
    SmServiceDomainT* domain );
// ****************************************************************************

// ****************************************************************************
// Service Domain Leader State - Transition
// ========================================
extern SmErrorT sm_service_domain_leader_state_transition(
    SmServiceDomainT* domain, SmServiceDomainStateT from_state );
// ****************************************************************************

// ****************************************************************************
// Service Domain Leader State - Event Handler
// ===========================================
extern SmErrorT sm_service_domain_leader_state_event_handler(
    SmServiceDomainT* domain, SmServiceDomainEventT event,
    void* event_data[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Leader State - Initialize
// ========================================
extern SmErrorT sm_service_domain_leader_state_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Leader State - Finalize
// ======================================
extern SmErrorT sm_service_domain_leader_state_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_LEADER_STATE_H__
