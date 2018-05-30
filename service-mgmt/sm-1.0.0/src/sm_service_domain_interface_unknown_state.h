//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_INTERFACE_UNKNOWN_STATE_H__
#define __SM_SERVICE_DOMAIN_INTERFACE_UNKNOWN_STATE_H__

#include "sm_types.h"
#include "sm_service_domain_interface_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Domain Interface Unknown State - Entry
// ==============================================
extern SmErrorT sm_service_domain_interface_unknown_state_entry(
    SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Unknown State - Exit
// =============================================
extern SmErrorT sm_service_domain_interface_unknown_state_exit(
    SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Unknown State - Transition
// ===================================================
extern SmErrorT sm_service_domain_interface_unknown_state_transition(
    SmServiceDomainInterfaceT* interface, SmInterfaceStateT from_state );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Unknown State - Event Handler
// ======================================================
extern SmErrorT sm_service_domain_interface_unknown_state_event_handler(
    SmServiceDomainInterfaceT* interface, 
    SmServiceDomainInterfaceEventT event, void* event_data[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Unknown State - Initialize
// ===================================================
extern SmErrorT sm_service_domain_interface_unknown_state_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Unknown State - Finalize
// =================================================
extern SmErrorT sm_service_domain_interface_unknown_state_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_INTERFACE_UNKNOWN_STATE_H__
