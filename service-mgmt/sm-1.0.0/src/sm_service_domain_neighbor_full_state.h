//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_NEIGHBOR_FULL_STATE_H__
#define __SM_SERVICE_DOMAIN_NEIGHBOR_FULL_STATE_H__

#include "sm_types.h"
#include "sm_service_domain_neighbor_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Domain Neighbor Full State - Entry
// ==========================================
extern SmErrorT sm_service_domain_neighbor_full_state_entry(
    SmServiceDomainNeighborT* neighbor );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Full State - Exit
// =========================================
extern SmErrorT sm_service_domain_neighbor_full_state_exit(
    SmServiceDomainNeighborT* neighbor );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Full State - Transition
// ===============================================
extern SmErrorT sm_service_domain_neighbor_full_state_transition(
    SmServiceDomainNeighborT* neighbor, 
    SmServiceDomainNeighborStateT from_state );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Full State - Event Handler
// ==================================================
extern SmErrorT sm_service_domain_neighbor_full_state_event_handler(
    SmServiceDomainNeighborT* neighbor, SmServiceDomainNeighborEventT event,
    void* event_data[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Full State - Initialize
// ===============================================
extern SmErrorT sm_service_domain_neighbor_full_state_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Full State - Finalize
// =============================================
extern SmErrorT sm_service_domain_neighbor_full_state_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_NEIGHBOR_FULL_STATE_H__
