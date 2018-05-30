//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_shutdown_state.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_fsm.h"
#include "sm_service_audit.h"

// ****************************************************************************
// Service Shutdown State - Entry
// ==============================
SmErrorT sm_service_shutdown_state_entry( SmServiceT* service )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Shutdown State - Exit
// =============================
SmErrorT sm_service_shutdown_state_exit( SmServiceT* service )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Shutdown State - Transition
// ===================================
SmErrorT sm_service_shutdown_state_transition( SmServiceT* service,
    SmServiceStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Shutdown State - Event Handler
// ======================================
SmErrorT sm_service_shutdown_state_event_handler( SmServiceT* service,
    SmServiceEventT event, void* event_data[] )
{
    switch( event )
    {
        default:
            DPRINTFD( "Service (%s) ignoring event (%s).", service->name,
                      sm_service_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Shutdown State - Initialize
// ===================================
SmErrorT sm_service_shutdown_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Shutdown State - Finalize
// =================================
SmErrorT sm_service_shutdown_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
