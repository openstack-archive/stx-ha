//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_neighbor_down_state.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_domain_neighbor_fsm.h"

// ****************************************************************************
// Service Domain Neighbor Down State - Entry
// ==========================================
SmErrorT sm_service_domain_neighbor_down_state_entry(
    SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    error = sm_service_domain_neighbor_fsm_stop_dead_timer( neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to stop dead timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Down State - Exit
// =========================================
SmErrorT sm_service_domain_neighbor_down_state_exit( 
    SmServiceDomainNeighborT* neighbor )
{
    SmErrorT error;

    error = sm_service_domain_neighbor_fsm_start_dead_timer( neighbor );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to start dead timer for service domain (%s) "
                  "neighbor (%s), error=%s.", neighbor->service_domain,
                  neighbor->name, sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Down State - Transition
// ===============================================
SmErrorT sm_service_domain_neighbor_down_state_transition(
    SmServiceDomainNeighborT* neighbor,
    SmServiceDomainNeighborStateT from_state )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Down State - Event Handler
// ==================================================
SmErrorT sm_service_domain_neighbor_down_state_event_handler(
    SmServiceDomainNeighborT* neighbor,
    SmServiceDomainNeighborEventT event, void* event_data[] )
{
    SmServiceDomainNeighborStateT state;
    SmErrorT error;

    switch( event )
    {
        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_HELLO_MSG:
            state = SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE_START;

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

        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_MSG:
        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_TIMER:
        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_MSG:
        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_TIMEOUT:
        case SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DOWN:
            // Ignore.
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
// Service Domain Neighbor Down State - Initialize
// ===============================================
SmErrorT sm_service_domain_neighbor_down_state_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Down State - Finalize
// =============================================
SmErrorT sm_service_domain_neighbor_down_state_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
