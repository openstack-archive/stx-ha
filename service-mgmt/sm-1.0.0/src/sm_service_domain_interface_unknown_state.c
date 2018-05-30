//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_interface_unknown_state.h"

#include <stdio.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_hw.h"
#include "sm_log.h"
#include "sm_heartbeat.h"
#include "sm_service_domain_interface_fsm.h"

// ****************************************************************************
// Service Domain Interface Unknown State - Entry
// ==============================================
SmErrorT sm_service_domain_interface_unknown_state_entry(
    SmServiceDomainInterfaceT* interface )
{
    SmErrorT error;

    error = sm_heartbeat_add_interface( interface );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to add messaging interface for service domain (%s), "
                  "error=%s.", interface->service_domain,
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Unknown State - Exit
// =============================================
SmErrorT sm_service_domain_interface_unknown_state_exit(
    SmServiceDomainInterfaceT* interface )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Unknown State - Transition
// ===================================================
SmErrorT sm_service_domain_interface_unknown_state_transition(
    SmServiceDomainInterfaceT* interface, SmInterfaceStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Unknown State - Event Handler
// ======================================================
SmErrorT sm_service_domain_interface_unknown_state_event_handler(
    SmServiceDomainInterfaceT* interface, SmServiceDomainInterfaceEventT event,
    void* event_data[] )
{
    bool enabled;
    SmErrorT error;
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = {0};

    error = sm_hw_get_if_state( interface->interface_name, &enabled );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to audit hardware state of interface (%s), "
                  "error=%s", interface->interface_name,
                  sm_error_str( error ) );
        return( error );
    }

    switch( event )
    {
        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_NODE_ENABLED:
            if( enabled )
            {
                char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";

                snprintf( reason_text, sizeof(reason_text), "node and %s "
                          "enabled", interface->interface_name );

                error = sm_service_domain_interface_fsm_set_state(
                                                interface->service_domain,
                                                interface->service_domain_interface,
                                                SM_INTERFACE_STATE_ENABLED, 
                                                reason_text );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Set state (%s) of service domain (%s) interface "
                              "(%s) failed, error=%s.",
                              sm_interface_state_str( SM_INTERFACE_STATE_ENABLED ),
                              interface->service_domain,
                              interface->service_domain_interface,
                              sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_ENABLED:
            // Ignore. Need the node state before transitioning to enabled.
        break;

        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_NODE_DISABLED:
            error = sm_service_domain_interface_fsm_set_state( 
                                            interface->service_domain,
                                            interface->service_domain_interface,
                                            SM_INTERFACE_STATE_DISABLED, 
                                            "node disabled" );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Set state (%s) of service domain (%s) interface (%s) "
                          "failed, error=%s.",
                          sm_interface_state_str( SM_INTERFACE_STATE_DISABLED ),
                          interface->service_domain,
                          interface->service_domain_interface,
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_DISABLED:
            if( !enabled )
            {
                char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";

                snprintf( reason_text, sizeof(reason_text), "%s disabled",
                          interface->interface_name );

                error = sm_service_domain_interface_fsm_set_state( 
                                            interface->service_domain,
                                            interface->service_domain_interface,
                                            SM_INTERFACE_STATE_DISABLED, 
                                            reason_text );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Set state (%s) of service domain (%s) interface "
                              "(%s) failed, error=%s.",
                              sm_interface_state_str( SM_INTERFACE_STATE_DISABLED ),
                              interface->service_domain,
                              interface->service_domain_interface,
                              sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_AUDIT:
            if( !enabled )
            {
                char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";

                snprintf( reason_text, sizeof(reason_text), "%s disabled",
                          interface->interface_name );

                error = sm_service_domain_interface_fsm_set_state( 
                                                interface->service_domain,
                                                interface->service_domain_interface,
                                                SM_INTERFACE_STATE_DISABLED, 
                                                reason_text );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Set state (%s) of service domain (%s) "
                              "interface (%s) failed, error=%s.",
                              sm_interface_state_str( SM_INTERFACE_STATE_DISABLED ),
                              interface->service_domain,
                              interface->service_domain_interface,
                              sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_DOMAIN_INTERFACE_EVENT_NOT_IN_USE:
            error = sm_service_domain_interface_fsm_set_state(
                                        interface->service_domain,
                                        interface->service_domain_interface,
                                        SM_INTERFACE_STATE_NOT_IN_USE,
                                        reason_text );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Set state (%s) of service domain (%s) "
                          "interface (%s) failed, error=%s.",
                          sm_interface_state_str( SM_INTERFACE_STATE_ENABLED ),
                          interface->service_domain,
                          interface->service_domain_interface,
                          sm_error_str( error ) );
                return( error );
            }
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
// Service Domain Interface Unknown State - Initialize
// ===================================================
SmErrorT sm_service_domain_interface_unknown_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Unknown State - Finalize
// =================================================
SmErrorT sm_service_domain_interface_unknown_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
