//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_leader_state.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_node_api.h"
#include "sm_service_domain_utils.h"
#include "sm_service_domain_fsm.h"

// ****************************************************************************
// Service Domain Leader State - Entry
// ===================================
SmErrorT sm_service_domain_leader_state_entry( SmServiceDomainT* domain )
{
    domain->designation = SM_DESIGNATION_TYPE_LEADER;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Leader State - Exit
// ==================================
SmErrorT sm_service_domain_leader_state_exit( SmServiceDomainT* domain )
{
    domain->designation = SM_DESIGNATION_TYPE_UNKNOWN;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Leader State - Transition
// ========================================
SmErrorT sm_service_domain_leader_state_transition( SmServiceDomainT* domain,
    SmServiceDomainStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Leader State - Event Handler
// ===========================================
SmErrorT sm_service_domain_leader_state_event_handler( SmServiceDomainT* domain,
    SmServiceDomainEventT event, void* event_data[] )
{
    bool enabled;
    char* neighbor_name = NULL;
    char* leader_name = NULL;
    char hostname[SM_NODE_NAME_MAX_CHAR];
    const char* new_leader;
    int generation = 0;
    int priority = 0;
    SmServiceDomainStateT state = SM_SERVICE_DOMAIN_STATE_NIL;
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

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
            neighbor_name
                = (char*) event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_NODE_NAME];
            generation 
                = *(int*) event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_GENERATION];
            priority 
                = *(int*) event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_PRIORITY];
            leader_name
                = (char*) event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_LEADER];

            if(( '\0' != leader_name[0] )&&
               ( 0 != strcmp( leader_name, domain->leader ) ))
            {
                DPRINTFI( "Service domain (%s) node name (%s) leader (%s) "
                          "does not match neighbor (%s) leader (%s).",
                          domain->name, hostname, domain->leader,
                          neighbor_name, leader_name );

                state = SM_SERVICE_DOMAIN_STATE_WAITING;

                snprintf( reason_text, sizeof(reason_text),
                          "leader mismatch with %s neighbor", neighbor_name );

            } else if( generation > domain->generation ) {
                DPRINTFI( "Service domain (%s) node name (%s) generation (%i) "
                          "less than neighbor (%s) generation (%i).",
                          domain->name, hostname, domain->generation,
                          neighbor_name, generation );

                state = SM_SERVICE_DOMAIN_STATE_WAITING;

                snprintf( reason_text, sizeof(reason_text),
                          "preempted by %s neighbor", neighbor_name );

            } else if(( generation == domain->generation )&&( domain->preempt )) {
                if( priority > domain->priority )
                {
                    DPRINTFI( "Service domain (%s) node name (%s) priority (%i) "
                              "less than neighbor (%s) priority (%i).",
                              domain->name, hostname, domain->priority,
                              neighbor_name, priority );

                    state = SM_SERVICE_DOMAIN_STATE_WAITING;

                   snprintf( reason_text, sizeof(reason_text),
                             "preempted by %s neighbor", neighbor_name );

                } else if( priority == domain->priority ) {
                    if( 0 < strcmp( neighbor_name, hostname ) )
                    {
                        DPRINTFI( "Service domain (%s) node name (%s) priority "
                                  "(%i) equal to neighbor (%s) priority (%i), "
                                  "using names as tie breaker.", 
                                  domain->name, hostname, domain->priority,
                                  neighbor_name, priority );

                        state = SM_SERVICE_DOMAIN_STATE_WAITING;

                        snprintf( reason_text, sizeof(reason_text),
                                  "preempted by %s neighbor", neighbor_name );
                    }
                }
            }

            if( SM_SERVICE_DOMAIN_STATE_NIL != state )
            {
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
            new_leader = (const char*) event_data[0];
            error = sm_node_api_get_hostname(hostname);
            if(SM_OKAY != error )
            {
                DPRINTFE("Failed to get hostname. Error %s", sm_error_str(error));
                return SM_FAILED;
            }

            if(0 == strncmp(hostname, new_leader, SM_NODE_NAME_MAX_CHAR))
            {
                //Ignore
            }else
            {
                strncpy(domain->leader, new_leader, SM_NODE_NAME_MAX_CHAR);
                state = SM_SERVICE_DOMAIN_STATE_BACKUP;
                error = sm_service_domain_fsm_set_state( domain->name, state,
                                                         "leader change");
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Set state (%s) of service domain (%s) failed, "
                              "error=%s.", sm_service_domain_state_str( state ),
                              domain->name, sm_error_str( error ) );
                    return( error );
                }
            }
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
// Service Domain Leader State - Initialize
// ========================================
SmErrorT sm_service_domain_leader_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Leader State - Finalize
// ======================================
SmErrorT sm_service_domain_leader_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
