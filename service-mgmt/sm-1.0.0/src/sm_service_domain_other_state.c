//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_other_state.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_domain_utils.h"
#include "sm_service_domain_fsm.h"

// ****************************************************************************
// Service Domain Other State - Entry
// ==================================
SmErrorT sm_service_domain_other_state_entry( SmServiceDomainT* domain )
{
    domain->designation = SM_DESIGNATION_TYPE_OTHER;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Other State - Exit
// =================================
SmErrorT sm_service_domain_other_state_exit( SmServiceDomainT* domain )
{
    domain->designation = SM_DESIGNATION_TYPE_UNKNOWN;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Other State - Transition
// =======================================
SmErrorT sm_service_domain_other_state_transition( SmServiceDomainT* domain,
    SmServiceDomainStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Other State - Event Handler
// ==========================================
SmErrorT sm_service_domain_other_state_event_handler( SmServiceDomainT* domain,
    SmServiceDomainEventT event, void* event_data[] )
{
    bool enabled;
    SmServiceDomainStateT state;
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    error = sm_service_domain_utils_service_domain_enabled( domain->name,
                                                            &enabled );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to determine if service domain (%s) is enabled, "
                  "error=%s.", domain->name, sm_error_str( error ) );
        return( error );
    }

    switch( event )
    {
        case SM_SERVICE_DOMAIN_EVENT_HELLO_MSG:
        case SM_SERVICE_DOMAIN_EVENT_NEIGHBOR_AGEOUT:
        case SM_SERVICE_DOMAIN_EVENT_INTERFACE_ENABLED:
            // Ignore.
        break;

        case SM_SERVICE_DOMAIN_EVENT_INTERFACE_DISABLED:
            if( !enabled )
            {
                state = SM_SERVICE_DOMAIN_STATE_INITIAL;

                snprintf( reason_text, sizeof(reason_text),
                          "primary interface disabled" );

                error = sm_service_domain_fsm_set_state( domain->name, state,
                                                         reason_text );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Set state (%s) of service domain (%s) failed, "
                              "error=%s.", sm_service_domain_state_str( state ),
                              domain->name, sm_error_str( error ) );
                    return( error );
                }
            }
        break;

        case SM_SERVICE_DOMAIN_EVENT_WAIT_EXPIRED:
            // Ignore.
        break;

        case SM_SERVICE_DOMAIN_EVENT_CHANGING_LEADER:
            DPRINTFE("Received unexpected %s event", sm_service_domain_event_str(event));
        break;

        default:
            DPRINTFD( "Service Domain (%s) ignoring event (%s).", 
                      domain->name, sm_service_domain_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Other State - Initialize
// =======================================
SmErrorT sm_service_domain_other_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Other State - Finalize
// =====================================
SmErrorT sm_service_domain_other_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
