//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_go_standby.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_group_member_table.h"
#include "sm_service_api.h"

// ****************************************************************************
// Service Group Go-Standby - Service
// ==================================
static void sm_service_group_go_standby_service( void* user_data[],
    SmServiceGroupMemberT* service_group_member )
{
    SmServiceGroupT* service_group = (SmServiceGroupT*) user_data[0];
    SmErrorT error;
    
    DPRINTFD( "Go-Standby on %s of %s", service_group_member->service_name,
              service_group->name );

    error = sm_service_api_go_standby( service_group_member->service_name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send go-standby to service (%s) of "
                  "service group (%s), error=%s.", 
                  service_group_member->service_name,
                  service_group->name, sm_error_str( error ) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Standby
// ========================
SmErrorT sm_service_group_go_standby( SmServiceGroupT* service_group )
{
    void* user_data[] = { service_group };

    sm_service_group_member_table_foreach_member( service_group->name,
                    user_data, sm_service_group_go_standby_service );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Standby - Complete
// ===================================
static void sm_service_group_go_standby_service_complete( void* user_data[],
    SmServiceGroupMemberT* service_group_member )
{
    bool* complete_overall = (bool*) user_data[0];

    if(( SM_SERVICE_STATE_ENABLED_STANDBY   == service_group_member->service_state )||
       ( SM_SERVICE_STATE_ENABLING_THROTTLE == service_group_member->service_state )||
       ( SM_SERVICE_STATE_ENABLING          == service_group_member->service_state )||
       ( SM_SERVICE_STATE_DISABLING         == service_group_member->service_state )||
       ( SM_SERVICE_STATE_DISABLED          == service_group_member->service_state ))
    {
        DPRINTFD( "Go-Standby of service (%s) for service group (%s) complete, "
                  "state=%s.", service_group_member->service_name, 
                  service_group_member->name,
                  sm_service_state_str( service_group_member->service_state ) );
    } else {
        *complete_overall = false;

        DPRINTFD( "Go-Standby of service (%s) for service group (%s) not yet "
                  "complete, state=%s.", service_group_member->service_name, 
                  service_group_member->name,
                  sm_service_state_str( service_group_member->service_state ) );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Standby - Complete
// ===================================
SmErrorT sm_service_group_go_standby_complete( SmServiceGroupT* service_group,
    bool* complete )
{
    bool go_standby_complete = true;
    void* user_data[] = { &go_standby_complete };

    *complete = false;

    sm_service_group_member_table_foreach_member( service_group->name,
                    user_data, sm_service_group_go_standby_service_complete );

    *complete = go_standby_complete;

    if( go_standby_complete )
    {
        DPRINTFI( "All services go-standby for service group (%s) "
                  "are complete.", service_group->name );
    } else {
        DPRINTFD( "Some service go-standby for service group (%s) "
                  "are not yet complete.", service_group->name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Standby - Initialize
// =====================================
SmErrorT sm_service_group_go_standby_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Standby - Finalize
// ===================================
SmErrorT sm_service_group_go_standby_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
