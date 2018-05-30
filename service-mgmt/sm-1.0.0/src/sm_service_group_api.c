//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_api.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_node_api.h"
#include "sm_service_group_table.h"
#include "sm_service_group_member_table.h"
#include "sm_service_group_engine.h"
#include "sm_service_group_fsm.h"

static SmListT* _callbacks = NULL;

// ****************************************************************************
// Service Group API - Register Callback
// =====================================
SmErrorT sm_service_group_api_register_callback( 
    SmServiceGroupApiCallbackT callback )
{
    SM_LIST_PREPEND( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group API - Deregister Callback
// =======================================
SmErrorT sm_service_group_api_deregister_callback(
    SmServiceGroupApiCallbackT callback )
{
    SM_LIST_REMOVE( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group API - Service Group Callback
// ==========================================
static void sm_service_group_api_service_group_callback( 
    char service_group_name[], SmServiceGroupStateT state,
    SmServiceGroupStatusT status, SmServiceGroupConditionT condition,
    int64_t health, const char reason_text[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceGroupApiCallbackT callback;

    SM_LIST_FOREACH( _callbacks, entry, entry_data )
    {
        callback = (SmServiceGroupApiCallbackT) entry_data;

        callback( service_group_name, state, status, condition, health,
                  reason_text );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group API - Go-Active
// =============================
SmErrorT sm_service_group_api_go_active( char service_group_name[] )
{
    SmServiceGroupT* service_group;
    SmErrorT error;

    service_group = sm_service_group_table_read( service_group_name );
    if( NULL == service_group )
    {
        DPRINTFE( "Failed to read service group (%s), error=%s.",
                  service_group_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    if( SM_SERVICE_GROUP_STATE_ACTIVE != service_group->desired_state )
    {
        service_group->desired_state = SM_SERVICE_GROUP_STATE_ACTIVE;

        error = sm_service_group_table_persist( service_group );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service group (%s) data, error=%s.",
                      service_group->name, sm_error_str( error ) );
            return( error );
        }

        error = sm_service_group_engine_signal( service_group );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to signal service group (%s), error=%s.",
                      service_group->name, sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group API - Go-Standby
// ==============================
SmErrorT sm_service_group_api_go_standby( char service_group_name[] )
{
    SmServiceGroupT* service_group;
    SmErrorT error;

    service_group = sm_service_group_table_read( service_group_name );
    if( NULL == service_group )
    {
        DPRINTFE( "Failed to read service group (%s), error=%s.",
                  service_group_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    if( SM_SERVICE_GROUP_STATE_STANDBY != service_group->desired_state )
    {
        service_group->desired_state = SM_SERVICE_GROUP_STATE_STANDBY;

        error = sm_service_group_table_persist( service_group );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service group (%s) data, error=%s.",
                      service_group->name, sm_error_str( error ) );
            return( error );
        }

        error = sm_service_group_engine_signal( service_group );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to signal service group (%s), error=%s.",
                      service_group->name, sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group API - Disable
// ===========================
SmErrorT sm_service_group_api_disable( char service_group_name[] )
{
    SmServiceGroupT* service_group;
    SmErrorT error;

    service_group = sm_service_group_table_read( service_group_name );
    if( NULL == service_group )
    {
        DPRINTFE( "Failed to read service group (%s), error=%s.",
                  service_group_name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    if( SM_SERVICE_GROUP_STATE_DISABLED != service_group->desired_state )
    {
        service_group->desired_state = SM_SERVICE_GROUP_STATE_DISABLED;

        error = sm_service_group_table_persist( service_group );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service group (%s) data, error=%s.",
                      service_group->name, sm_error_str( error ) );
            return( error );
        }

        error = sm_service_group_engine_signal( service_group );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to signal service group (%s), error=%s.",
                      service_group->name, sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group API - Recover
// ===========================
SmErrorT sm_service_group_api_recover( char service_group_name[],
    bool escalate_recovery, bool clear_fatal_condition )
{
    if(( clear_fatal_condition )&&( escalate_recovery ))
    {
        SmServiceGroupT* service_group;
        SmErrorT error;

        service_group = sm_service_group_table_read( service_group_name );
        if( NULL == service_group )
        {
            DPRINTFE( "Failed to read service group (%s), error=%s.",
                      service_group_name, sm_error_str(SM_NOT_FOUND) );
            return( SM_NOT_FOUND );
        }

        if( service_group->fatal_error_reboot )
        {
            char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";

            DPRINTFI( "Service group (%s) recovery from fatal condition "
                      "escalated to a reboot.", service_group->name );

            snprintf( reason_text, sizeof(reason_text), "service group (%s) "
                      "recovery from fatal condition escalated to a reboot.",
                      service_group->name );

            error = sm_node_api_reboot( reason_text );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to issue reboot to recover service "
                          "group (%s), error=%s.", service_group->name,
                          sm_error_str(SM_NOT_FOUND) );
                return( error );
            }
        }
    }

    return( sm_service_group_fsm_recover( service_group_name, 
                                          clear_fatal_condition ) );
}
// ****************************************************************************

// ****************************************************************************
// Service Group API - Initialize
// ==============================
SmErrorT sm_service_group_api_initialize( void )
{
    SmErrorT error;

    error = sm_service_group_table_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }
    
    error = sm_service_group_member_table_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to intialize service group member table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_group_fsm_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to intialize service group fsm, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }
    
    error = sm_service_group_engine_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to intialize service group engine, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }
   
    error = sm_service_group_fsm_register_callback(
                        sm_service_group_api_service_group_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for service group fsm state changes, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group API - Finalize
// ============================
SmErrorT sm_service_group_api_finalize( void )
{
    SmErrorT error;

    error = sm_service_group_fsm_deregister_callback(
                        sm_service_group_api_service_group_callback );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister for service group fsm state changes, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_group_fsm_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group fsm, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_group_engine_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group engine, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_group_table_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group table, error=%s.",
                  sm_error_str( error ) );
    }
    
    error = sm_service_group_member_table_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group member table, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
