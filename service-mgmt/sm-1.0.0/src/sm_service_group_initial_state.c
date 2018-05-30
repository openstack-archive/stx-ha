//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_initial_state.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_group_fsm.h"
#include "sm_service_group_go_active.h"
#include "sm_service_group_go_standby.h"

// ****************************************************************************
// Service Group Initial State - Entry
// ===================================
SmErrorT sm_service_group_initial_state_entry( SmServiceGroupT* service_group )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Initial State - Exit
// ==================================
SmErrorT sm_service_group_initial_state_exit( SmServiceGroupT* service_group )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Initial State - Transition
// ========================================
SmErrorT sm_service_group_initial_state_transition(
    SmServiceGroupT* service_group, SmServiceGroupStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Initial State - Event Handler
// ===========================================
SmErrorT sm_service_group_initial_state_event_handler(
    SmServiceGroupT* service_group, SmServiceGroupEventT event,
    void* event_data[] )
{
    SmServiceGroupStateT state;
    SmErrorT error;

    error = sm_service_group_fsm_get_next_state( service_group->name,
                                                 event, &state );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to determine next state for service group (%s), "
                  "error=%s", service_group->name, sm_error_str( error ) );
        return( error );
    }

    switch( event )
    {
        case SM_SERVICE_GROUP_EVENT_GO_ACTIVE:
        case SM_SERVICE_GROUP_EVENT_GO_STANDBY:
        case SM_SERVICE_GROUP_EVENT_DISABLE:
            error = sm_service_group_fsm_set_state( service_group->name,
                                                    state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service group (%s) state (%s), "
                          "error=%s.", service_group->name, 
                          sm_service_group_state_str( state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_EVENT_SHUTDOWN:
            error = sm_service_group_fsm_set_state( service_group->name,
                                    SM_SERVICE_GROUP_STATE_SHUTDOWN );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service group (%s) state (%s), "
                          "error=%s.", service_group->name, 
                          sm_service_group_state_str(
                                    SM_SERVICE_GROUP_STATE_SHUTDOWN ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFD( "Service group (%s) ignoring event (%s).", 
                      service_group->name,
                      sm_service_group_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Initial State - Initialize
// ========================================
SmErrorT sm_service_group_initial_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Initial State - Finalize
// ======================================
SmErrorT sm_service_group_initial_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
