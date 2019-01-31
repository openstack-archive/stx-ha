//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_backup_state.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_node_api.h"
#include "sm_service_domain_utils.h"
#include "sm_service_domain_fsm.h"

// ****************************************************************************
// Service Domain Backup State - Entry
// ===================================
SmErrorT sm_service_domain_backup_state_entry( SmServiceDomainT* domain )
{
    domain->designation = SM_DESIGNATION_TYPE_BACKUP;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Backup State - Exit
// ==================================
SmErrorT sm_service_domain_backup_state_exit( SmServiceDomainT* domain )
{
    domain->designation = SM_DESIGNATION_TYPE_UNKNOWN;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Backup State - Transition
// ========================================
SmErrorT sm_service_domain_backup_state_transition( SmServiceDomainT* domain,
    SmServiceDomainStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Backup State - Event Handler
// ===========================================
SmErrorT sm_service_domain_backup_state_event_handler( SmServiceDomainT* domain,
    SmServiceDomainEventT event, void* event_data[] )
{
    bool enabled, elected;
    char* leader_name = NULL;
    char* neighbor_name = NULL;
    char leader[SM_NODE_NAME_MAX_CHAR];
    char hostname[SM_NODE_NAME_MAX_CHAR];
    const char* new_leader;
    int generation = 0;    
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmServiceDomainStateT state = SM_SERVICE_DOMAIN_STATE_NIL;
    SmErrorT error;

    error = sm_service_domain_utils_service_domain_enabled( domain->name,
                                                            &enabled );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to determine if service domain (%s) is enabled, "
                  "error=%s.", domain->name, sm_error_str( error ) );
        return( error );
    }

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    switch( event )
    {
        case SM_SERVICE_DOMAIN_EVENT_HELLO_MSG:
            neighbor_name
                = (char*) event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_NODE_NAME];            
            generation 
                = *(int*) event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_GENERATION];           
            leader_name
                = (char*) event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_LEADER];
            
            if( 0 == strcmp( hostname, leader_name ) )
            {
                state = SM_SERVICE_DOMAIN_STATE_WAITING;

                snprintf( reason_text, sizeof(reason_text),
                          "hello received from %s indicating %s as leader",
                          neighbor_name, leader_name );

            } else if( 0 == strcmp( neighbor_name, leader_name ) ) {
                if( generation < domain->generation ) 
                {
                    state = SM_SERVICE_DOMAIN_STATE_WAITING;

                    snprintf( reason_text, sizeof(reason_text), 
                              "hello received from %s indicating leader, "
                              "but from lesser generation", neighbor_name );
                }
            }

            if( SM_SERVICE_DOMAIN_STATE_NIL != state )
            {
                error = sm_service_domain_fsm_set_state( domain->name,
                                                         state, reason_text );
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
            neighbor_name
                = (char*) event_data[SM_SERVICE_DOMAIN_EVENT_DATA_MSG_NODE_NAME];

            if( 0 == strcmp( neighbor_name, domain->leader ) )
            {
                error = sm_service_domain_utils_hold_election( domain->name,
                                                           leader, &elected );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to hold election for service domain "
                              "(%s), error=%s.", domain->name,
                              sm_error_str( error ) );
                    return( error );
                }

                if( 0 != strcmp( leader, domain->leader ) )
                {
                    state = SM_SERVICE_DOMAIN_STATE_WAITING;

                    snprintf( reason_text, sizeof(reason_text),
                              "neighbor %s ageout of leader", neighbor_name );

                    error = sm_service_domain_fsm_set_state( domain->name,
                                                        state, reason_text );
                    if( SM_OKAY != error )
                    {
                        DPRINTFE( "Set state (%s) of service domain (%s) "
                                  "failed, error=%s.",
                                  sm_service_domain_state_str( state ),
                                  domain->name, sm_error_str( error ) );
                        return( error );
                    }
                }
            }
        break;

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

            if(0 != strncmp(hostname, new_leader, SM_NODE_NAME_MAX_CHAR))
            {
                //Ignore
            }else
            {
                strncpy(domain->leader, new_leader, SM_NODE_NAME_MAX_CHAR);
                state = SM_SERVICE_DOMAIN_STATE_LEADER;
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
                      domain->name,
                      sm_service_domain_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Backup State - Initialize
// ========================================
SmErrorT sm_service_domain_backup_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Backup State - Finalize
// ======================================
SmErrorT sm_service_domain_backup_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
