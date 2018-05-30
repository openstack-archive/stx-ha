//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_initial_state.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_fsm.h"
#include "sm_service_audit.h"

// ****************************************************************************
// Service Initial State - Entry
// =============================
SmErrorT sm_service_initial_state_entry( SmServiceT* service )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Initial State - Exit
// ============================
SmErrorT sm_service_initial_state_exit( SmServiceT* service )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Initial State - Transition
// ==================================
SmErrorT sm_service_initial_state_transition( SmServiceT* service,
    SmServiceStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Initial State - Event Handler
// =====================================
SmErrorT sm_service_initial_state_event_handler( SmServiceT* service,
    SmServiceEventT event, void* event_data[] )
{
    SmErrorT error;

    switch( event )
    {
        case SM_SERVICE_EVENT_AUDIT:
            error = sm_service_fsm_set_state( service,
                                              SM_SERVICE_STATE_UNKNOWN );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service (%s) state (%s), "
                          "error=%s.", service->name, 
                          sm_service_state_str( SM_SERVICE_STATE_UNKNOWN ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_EVENT_SHUTDOWN:
            error = sm_service_fsm_set_state( service,
                                              SM_SERVICE_STATE_SHUTDOWN );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service (%s) state (%s), error=%s.",
                          service->name,
                          sm_service_state_str(SM_SERVICE_STATE_SHUTDOWN),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFD( "Service (%s) ignoring event (%s).", service->name,
                      sm_service_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Initial State - Initialize
// ==================================
SmErrorT sm_service_initial_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Initial State - Finalize
// ================================
SmErrorT sm_service_initial_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
