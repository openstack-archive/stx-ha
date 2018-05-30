//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_shutdown_state.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_group_notification.h"
#include "sm_service_group_fsm.h"

// ****************************************************************************
// Service Group Shutdown State - Entry
// ====================================
SmErrorT sm_service_group_shutdown_state_entry(
    SmServiceGroupT* service_group )
{
    SmErrorT error;

    error = sm_service_group_notification_notify(
                    service_group, SM_SERVICE_GROUP_NOTIFICATION_SHUTDOWN );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send service group (%s) notification, error=%s",
                  service_group->name, sm_error_str( error ) );
    }

    service_group->status = SM_SERVICE_GROUP_STATUS_NONE;
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Shutdown State - Exit
// ===================================
SmErrorT sm_service_group_shutdown_state_exit( 
    SmServiceGroupT* service_group )
{
    SmErrorT error;

    error = sm_service_group_notification_abort( service_group );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to abort service group (%s) notification, error=%s",
                  service_group->name, sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Shutdown State - Transition
// =========================================
SmErrorT sm_service_group_shutdown_state_transition(
    SmServiceGroupT* service_group, SmServiceGroupStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Shutdown State - Event Handler
// ============================================
SmErrorT sm_service_group_shutdown_state_event_handler(
    SmServiceGroupT* service_group, SmServiceGroupEventT event,
    void* event_data[] )
{
    switch( event )
    {
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
// Service Group Shutdown State - Initialize
// =========================================
SmErrorT sm_service_group_shutdown_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Shutdown State - Finalize
// =======================================
SmErrorT sm_service_group_shutdown_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
