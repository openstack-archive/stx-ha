//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_go_active_state.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_group_notification.h"
#include "sm_service_group_fsm.h"
#include "sm_service_group_go_active.h"
#include "sm_service_group_audit.h"

// ****************************************************************************
// Service Group Go-Active State - Entry
// =====================================
SmErrorT sm_service_group_go_active_state_entry(
    SmServiceGroupT* service_group )
{
    SmErrorT error;

    error = sm_service_group_notification_notify(
                    service_group, SM_SERVICE_GROUP_NOTIFICATION_GO_ACTIVE );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send service group (%s) notification, error=%s",
                  service_group->name, sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active State - Exit
// ====================================
SmErrorT sm_service_group_go_active_state_exit( 
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
// Service Group Go-Active State - Transition
// ==========================================
SmErrorT sm_service_group_go_active_state_transition(
    SmServiceGroupT* service_group, SmServiceGroupStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active State - Event Handler
// =============================================
SmErrorT sm_service_group_go_active_state_event_handler(
    SmServiceGroupT* service_group, SmServiceGroupEventT event,
    void* event_data[] )
{
    bool complete;
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
            // Ignore.
        break;

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

        case SM_SERVICE_GROUP_EVENT_NOTIFICATION_SUCCESS:
        case SM_SERVICE_GROUP_EVENT_NOTIFICATION_FAILED:
        case SM_SERVICE_GROUP_EVENT_NOTIFICATION_TIMEOUT:
            error = sm_service_group_go_active( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to go-active on service group (%s), "
                          "error=%s", service_group->name,
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_GROUP_EVENT_AUDIT:
        case SM_SERVICE_GROUP_EVENT_SERVICE_SCN:
            error = sm_service_group_audit_status( service_group );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to audit service group (%s), error=%s.",
                          service_group->name, sm_error_str( error ) );
                return( error );
            }

            error = sm_service_group_go_active_complete( service_group,
                                                         &complete );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to determine if go-active complete on "
                          "service group (%s), error=%s", service_group->name,
                          sm_error_str( error ) );
                return( error );
            }

            if( complete )
            {
                error = sm_service_group_fsm_set_state( service_group->name,
                                        SM_SERVICE_GROUP_STATE_ACTIVE );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to set service group (%s) state (%s), "
                              "error=%s.", service_group->name,
                              sm_service_group_state_str( 
                                      SM_SERVICE_GROUP_STATE_ACTIVE ),
                              sm_error_str( error ) );
                    return( error );
                }
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
            DPRINTFI( "Service group (%s) ignoring event (%s).", 
                      service_group->name,
                      sm_service_group_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active State - Initialize
// ==========================================
SmErrorT sm_service_group_go_active_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active State - Finalize
// ========================================
SmErrorT sm_service_group_go_active_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
