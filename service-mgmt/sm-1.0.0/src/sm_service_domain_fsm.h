//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_FSM_H__
#define __SM_SERVICE_DOMAIN_FSM_H__

#include <stdio.h>
#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Domain FSM - Set State
// ==============================
extern SmErrorT sm_service_domain_fsm_set_state( char name[], 
    SmServiceDomainStateT state, const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - Event Handler
// ==================================
extern SmErrorT sm_service_domain_fsm_event_handler( char name[], 
    SmServiceDomainEventT event, void* event_data[], const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - dump state
// ==================================
extern void sm_service_domain_dump_state(FILE* fp);
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - Initialize
// ===============================
extern SmErrorT sm_service_domain_fsm_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain FSM - Finalize
// =============================
extern SmErrorT sm_service_domain_fsm_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_FSM_H__
