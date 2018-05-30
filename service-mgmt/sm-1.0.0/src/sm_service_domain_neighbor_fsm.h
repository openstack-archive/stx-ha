//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_NEIGHBOR_FSM_H__
#define __SM_SERVICE_DOMAIN_NEIGHBOR_FSM_H__

#include "sm_types.h"
#include "sm_timer.h"
#include "sm_service_domain_neighbor_table.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmServiceDomainNeighborFsmCallbackT) (char service_domain[],
        char node_name[], SmServiceDomainNeighborStateT state);

// ****************************************************************************
// Service Domain Neighbor FSM - Register Callback
// ===============================================
extern SmErrorT sm_service_domain_neighbor_fsm_register_callback(
    SmServiceDomainNeighborFsmCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Deregister Callback
// =================================================
extern SmErrorT sm_service_domain_neighbor_fsm_deregister_callback(
    SmServiceDomainNeighborFsmCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Start Dead Timer
// ==============================================
extern SmErrorT sm_service_domain_neighbor_fsm_start_dead_timer(
    SmServiceDomainNeighborT* neighbor );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Stop Dead Timer
// =============================================
extern SmErrorT sm_service_domain_neighbor_fsm_stop_dead_timer(
    SmServiceDomainNeighborT* neighbor );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Restart Dead Timer
// ================================================
extern SmErrorT sm_service_domain_neighbor_fsm_restart_dead_timer(
    SmServiceDomainNeighborT* neighbor );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Set State
// =======================================
extern SmErrorT sm_service_domain_neighbor_fsm_set_state( char neighbor_name[], 
    char service_domain[], SmServiceDomainNeighborStateT state );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Event Handler
// ===========================================
extern SmErrorT sm_service_domain_neighbor_fsm_event_handler(
    char neighbor_name[], char service_domain[],
    SmServiceDomainNeighborEventT event, void* event_data[],
    const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - dump
// ===========================================
void sm_domain_neighbor_fsm_dump(FILE* fp);
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Initialize
// ========================================
extern SmErrorT sm_service_domain_neighbor_fsm_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor FSM - Finalize
// ======================================
extern SmErrorT sm_service_domain_neighbor_fsm_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_NEIGHBOR_FSM_H__
