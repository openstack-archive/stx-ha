//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_interface_not_in_use_state.h"

#include <stdio.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_msg.h"
#include "sm_log.h"
#include "sm_service_domain_interface_fsm.h"

// ****************************************************************************
// Service Domain Interface Not In Use State - Entry
// ==============================================
SmErrorT sm_service_domain_interface_niu_state_entry(
    SmServiceDomainInterfaceT* interface )
{
    DPRINTFI("Interface (%s) is not in use.", interface->service_domain_interface);
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Not In Use State - Exit
// =============================================
SmErrorT sm_service_domain_interface_unsued_state_exit(
    SmServiceDomainInterfaceT* interface )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Not In Use State - Transition
// ===================================================
SmErrorT sm_service_domain_interface_niu_state_transition(
    SmServiceDomainInterfaceT* interface, SmInterfaceStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Not In Use State - Event Handler
// ======================================================
SmErrorT sm_service_domain_interface_niu_state_event_handler(
    SmServiceDomainInterfaceT* interface,
    SmServiceDomainInterfaceEventT event, void* event_data[] )
{
    switch( event )
    {
        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_ENABLED:
            // undefined behavior
        break;

        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_NODE_DISABLED:
        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_DISABLED:
        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_NODE_ENABLED:
        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_AUDIT:
        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_NOT_IN_USE:
            // don't care, ignore these events
        break;

        default:
            DPRINTFD( "Service domain (%s) interface (%s) ignoring event (%s).",
                      interface->service_domain,
                      interface->service_domain_interface,
                      sm_service_domain_interface_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Not In Use State - Initialize
// ===================================================
SmErrorT sm_service_domain_interface_niu_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Not In Use State - Finalize
// =================================================
SmErrorT sm_service_domain_interface_niu_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
