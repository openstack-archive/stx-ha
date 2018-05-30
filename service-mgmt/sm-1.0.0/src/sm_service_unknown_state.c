//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_unknown_state.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_fsm.h"
#include "sm_service_audit.h"

// ****************************************************************************
// Service Unknown State - Entry
// =============================
SmErrorT sm_service_unknown_state_entry( SmServiceT* service )
{
    SmErrorT error;

    if( service->audit_enabled_exists )
    {
        error = sm_service_audit_enabled( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to audit service (%s), error=%s",
                      service->name, sm_error_str( error ) );
            return( error );
        }
    } else if( service->audit_disabled_exists ) {
        error = sm_service_audit_disabled( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to audit service (%s), error=%s",
                      service->name, sm_error_str( error ) );
            return( error );
        }
    }

    service->action_fail_count = 0;
    service->transition_fail_count = 0;
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Unknown State - Exit
// ============================
SmErrorT sm_service_unknown_state_exit( SmServiceT* service )
{
    SmErrorT error;

    if( SM_SERVICE_ACTION_AUDIT_ENABLED == service->action_running )
    {
        error = sm_service_audit_enabled_abort( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to abort action (%s) of service (%s), error=%s.",
                      sm_service_action_str( service->action_running ),
                      service->name, sm_error_str( error ) );
            return( error );
        }
    }

    if( SM_SERVICE_ACTION_AUDIT_DISABLED == service->action_running )
    {
        error = sm_service_audit_disabled_abort( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to abort action (%s) of service (%s), error=%s.",
                      sm_service_action_str( service->action_running ),
                      service->name, sm_error_str( error ) );
            return( error );
        }

    }

    service->action_fail_count = 0;
    service->transition_fail_count = 0;
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Unknown State - Transition
// ==================================
SmErrorT sm_service_unknown_state_transition( SmServiceT* service,
    SmServiceStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Unknown State - Event Handler
// =====================================
SmErrorT sm_service_unknown_state_event_handler( SmServiceT* service,
    SmServiceEventT event, void* event_data[] )
{
    SmServiceStateT state;
    SmErrorT error;

    switch( event )
    {
        case SM_SERVICE_EVENT_AUDIT:
            if(( service->audit_enabled_exists )&&
               ( SM_SERVICE_ACTION_AUDIT_ENABLED != service->action_running ))
            {
                error = sm_service_audit_enabled( service );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to audit service (%s), error=%s",
                              service->name, sm_error_str( error ) );
                    return( error );
                }
            }
            
            if(( service->audit_disabled_exists )&&
               ( !service->audit_enabled_exists )&&
               ( SM_SERVICE_ACTION_AUDIT_DISABLED != service->action_running ))
            {
                error = sm_service_audit_disabled( service );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to audit service (%s), error=%s",
                              service->name, sm_error_str( error ) );
                    return( error );
                }
            }

            if( service->transition_fail_count >= service->max_transition_failures )
            {
                if( SM_SERVICE_CONDITION_FATAL_FAILURE != service->condition )
                {
                    service->status = SM_SERVICE_STATUS_FAILED;
                    service->condition = SM_SERVICE_CONDITION_FATAL_FAILURE;

                    DPRINTFI( "Service (%s) is unknown and has reached max "
                              "transition failures (%i).", service->name,
                              service->max_transition_failures );
                }
            }
            else if( service->action_fail_count >= service->max_action_failures )
            {
                if( SM_SERVICE_CONDITION_ACTION_FAILURE != service->condition )
                {
                    service->status = SM_SERVICE_STATUS_FAILED;
                    service->condition = SM_SERVICE_CONDITION_ACTION_FAILURE;

                    DPRINTFI( "Service (%s) is unknown and has reached max "
                              "action failures (%i).", service->name,
                              service->max_action_failures );
                }
            }
        break;

        case SM_SERVICE_EVENT_AUDIT_SUCCESS:
            state = *(SmServiceStateT*)
                    event_data[SM_SERVICE_EVENT_DATA_STATE];

            error = sm_service_fsm_set_state( service, state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to set service (%s) state (%s), error=%s.",
                          service->name, sm_service_state_str( state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_EVENT_AUDIT_FAILED:
        case SM_SERVICE_EVENT_AUDIT_TIMEOUT:
            // Service engine will run the audit action again.
            ++(service->action_fail_count);
            ++(service->transition_fail_count);
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
// Service Unknown State - Initialize
// ==================================
SmErrorT sm_service_unknown_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Unknown State - Finalize
// ================================
SmErrorT sm_service_unknown_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
