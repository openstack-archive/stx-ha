//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_NEIGHBOR_DOWN_STATE_H__
#define __SM_SERVICE_DOMAIN_NEIGHBOR_DOWN_STATE_H__

#include "sm_types.h"
#include "sm_service_domain_neighbor_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Domain Neighbor Down State - Entry
// ==========================================
extern SmErrorT sm_service_domain_neighbor_down_state_entry(
    SmServiceDomainNeighborT* neighbor );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Down State - Exit
// =========================================
extern SmErrorT sm_service_domain_neighbor_down_state_exit(
    SmServiceDomainNeighborT* neighbor );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Down State - Transition
// ===============================================
extern SmErrorT sm_service_domain_neighbor_down_state_transition(
    SmServiceDomainNeighborT* neighbor, 
    SmServiceDomainNeighborStateT from_state );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Down State - Event Handler
// ==================================================
extern SmErrorT sm_service_domain_neighbor_down_state_event_handler(
    SmServiceDomainNeighborT* neighbor, 
    SmServiceDomainNeighborEventT event, void* event_data[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Down State - Initialize
// ===============================================
extern SmErrorT sm_service_domain_neighbor_down_state_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Down State - Finalize
// =============================================
extern SmErrorT sm_service_domain_neighbor_down_state_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_NEIGHBOR_DOWN_STATE_H__
