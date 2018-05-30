//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_api.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db.h"
#include "sm_service_domain_table.h"
#include "sm_service_domain_member_table.h"
#include "sm_service_domain_neighbor_table.h"
#include "sm_service_domain_assignment_table.h"
#include "sm_service_domain_utils.h"
#include "sm_service_domain_fsm.h"
#include "sm_service_domain_neighbor_fsm.h"
#include "sm_service_domain_interface_fsm.h"
#include "sm_service_domain_scheduler.h"

static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Domain API - Interface Enabled
// ======================================
SmErrorT sm_service_domain_api_interface_enabled( char service_domain_name[] )
{
    SmServiceDomainEventT event = SM_SERVICE_DOMAIN_EVENT_INTERFACE_ENABLED;
    SmErrorT error;

    error = sm_service_domain_fsm_event_handler( service_domain_name, event,
                                                 NULL, "interface enabled" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) failed to handle event (%s), error=%s.",
                  service_domain_name, sm_service_domain_event_str(event),
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain API - Interface Disabled
// ======================================
SmErrorT sm_service_domain_api_interface_disabled( char service_domain_name[] )
{
    SmServiceDomainEventT event = SM_SERVICE_DOMAIN_EVENT_INTERFACE_DISABLED;
    SmErrorT error;

    error = sm_service_domain_fsm_event_handler( service_domain_name, event,
                                                 NULL, "interface disabled" );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) failed to handle event (%s), error=%s.",
                  service_domain_name, sm_service_domain_event_str(event),
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain API - Send Pause
// ===============================
static void sm_service_domain_api_send_pause( void* user_data[],
    SmServiceDomainT* domain )
{
    int pause_interval = *(int*) user_data[0];
    SmErrorT error;

    error = sm_service_domain_utils_send_pause( domain->name, pause_interval );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Service domain (%s) failed to send pause, error=%s.",
                  domain->name, sm_error_str( error ) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain API - Pause All
// ==============================
void sm_service_domain_api_pause_all( int pause_interval )
{
    void* user_data[] = {&pause_interval};

    sm_service_domain_table_foreach( user_data,
                                     sm_service_domain_api_send_pause );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain API - Interface State Callback
// =============================================
static void sm_service_domain_api_interface_state_callback(
    char service_domain[], char service_domain_interface[],
    SmInterfaceStateT interface_state )
{
    SmErrorT error;

    if( SM_INTERFACE_STATE_ENABLED == interface_state )
    {
        error = sm_service_domain_api_interface_enabled( service_domain );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to notify service domain (%s) that "
                      "interface (%s) is enabled, error=%s.", service_domain,
                      service_domain_interface, sm_error_str( error ) );
            return;
        }

    } else if( SM_INTERFACE_STATE_DISABLED == interface_state ) {

        error = sm_service_domain_api_interface_disabled( service_domain );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to notify service domain (%s) that interface "
                      "(%s) is disabled, error=%s.", service_domain,
                      service_domain_interface, sm_error_str( error ) );
            return;
        }

    } else {
        DPRINTFD( "Ignoring interface (%s) state (%s) from service "
                  "domain (%s).", service_domain_interface,
                  sm_interface_state_str( interface_state ), 
                  service_domain );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain API - Initialize
// ===============================
SmErrorT sm_service_domain_api_initialize( void )
{
    SmErrorT error;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_table_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_member_table_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain member table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_neighbor_table_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain neighbor table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_assignment_table_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain assignment table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_utils_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain utilities, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_fsm_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain fsm, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_neighbor_fsm_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain neighbor fsm, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_scheduler_initialize( _sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain scheduler, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_interface_fsm_register_callback(
                        sm_service_domain_api_interface_state_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for interface state changes, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain API - Finalize 
// =============================
SmErrorT sm_service_domain_api_finalize( void )
{
    SmErrorT error;

    error = sm_service_domain_interface_fsm_deregister_callback(
                        sm_service_domain_api_interface_state_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister for interface state changes, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_scheduler_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain scheduler, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_domain_neighbor_fsm_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain neighbor fsm, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_domain_fsm_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain fsm, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_domain_utils_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain utilities, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_domain_assignment_table_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain assignment table, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_member_table_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain member table, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_neighbor_table_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain neighbor table, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_table_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain table, "
                  "error=%s.", sm_error_str( error ) );
    }

    if( NULL != _sm_db_handle )
    {
        error = sm_db_disconnect( _sm_db_handle );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to disconnect from database (%s), error=%s.",
                      SM_DATABASE_NAME, sm_error_str( error ) );
        }

        _sm_db_handle = NULL;
    }

    return( SM_OKAY );
}
// ****************************************************************************
