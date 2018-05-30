//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_neighbor_full_state.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_domain_neighbor_fsm.h"
#include "sm_service_domain_assignment_table.h"
#include "sm_failover.h"

// ****************************************************************************
// Service Domain Neighbor Full State - Delete Assignments
// =======================================================
static void sm_service_domain_neighbor_full_state_delete_assignments(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    SmErrorT error;

    if( !assignment->exchanged )
    {
        error = sm_service_domain_assignment_table_delete( assignment->name,
                assignment->node_name, assignment->service_group_name );
        if( SM_OKAY == error )
        {
            DPRINTFI( "Deleted assignment (%s) from service domain (%s) "
                      "node (%s) as it was not exchanged.",
                      assignment->service_group_name, assignment->name,
                      assignment->node_name );
        } else {
            DPRINTFE( "Failed to delete assignment (%s) for service domain "
                      "(%s) node (%s), error=%s.",
                      assignment->service_group_name, assignment->name,
                      assignment->node_name, sm_error_str( error ) );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Full State - Entry
// ==========================================
SmErrorT sm_service_domain_neighbor_full_state_entry(
    SmServiceDomainNeighborT* neighbor )
{
    sm_service_domain_assignment_table_foreach_node_in_service_domain(
            neighbor->service_domain, neighbor->name, NULL,
            sm_service_domain_neighbor_full_state_delete_assignments );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Full State - Exit
// =========================================
SmErrorT sm_service_domain_neighbor_full_state_exit( 
    SmServiceDomainNeighborT* neighbor )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Full State - Transition
// ===============================================
SmErrorT sm_service_domain_neighbor_full_state_transition(
    SmServiceDomainNeighborT* neighbor,
    SmServiceDomainNeighborStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Full State - Event Handler
// ==================================================
SmErrorT sm_service_domain_neighbor_full_state_event_handler(
    SmServiceDomainNeighborT* neighbor,
    SmServiceDomainNeighborEventT event, void* event_data[] )
{
    SmServiceDomainNeighborStateT state;
    SmErrorT error;

    switch( event )
    {
        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_HELLO_MSG:
            sm_failover_hello_msg_restore();
            error = sm_service_domain_neighbor_fsm_restart_dead_timer( neighbor );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to restart dead timer for service domain (%s) "
                          "neighbor (%s), error=%s.", neighbor->service_domain,
                          neighbor->name, sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_MSG:
        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_MSG:
            state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE_START;

            error = sm_service_domain_neighbor_fsm_set_state( neighbor->name,
                                            neighbor->service_domain, state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) neighbor (%s) unable to set "
                          "state (%s), error=%s.", neighbor->service_domain,
                          neighbor->name,
                          sm_service_domain_neighbor_state_str( state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_TIMER:
        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_TIMEOUT:
            // Ignore.
        break;

        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DOWN:
            state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN;

            error = sm_service_domain_neighbor_fsm_set_state( neighbor->name,
                                            neighbor->service_domain, state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain neighbor (%s) set state (%s) "
                          "failed, error=%s.", neighbor->name,
                          sm_service_domain_neighbor_state_str( state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFD( "Neighbor (%s) ignoring event (%s).", neighbor->name,
                      sm_service_domain_neighbor_event_str( event ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Full State - Initialize
// ===============================================
SmErrorT sm_service_domain_neighbor_full_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Full State - Finalize
// =============================================
SmErrorT sm_service_domain_neighbor_full_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
