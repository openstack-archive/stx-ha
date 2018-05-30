//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_go_active.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_group_member_table.h"
#include "sm_service_api.h"

// ****************************************************************************
// Service Group Go-Active - Service
// =================================
static void sm_service_group_go_active_service( void* user_data[],
    SmServiceGroupMemberT* service_group_member )
{
    SmServiceGroupT* service_group = (SmServiceGroupT*) user_data[0];
    SmErrorT error;
    
    DPRINTFD( "Go-Active on %s of %s", service_group_member->service_name,
              service_group->name );

    error = sm_service_api_go_active( service_group_member->service_name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send go-active to service (%s) of "
                  "service group (%s), error=%s.", 
                  service_group_member->service_name,
                  service_group->name, sm_error_str( error ) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active
// =======================
SmErrorT sm_service_group_go_active( SmServiceGroupT* service_group )
{
    void* user_data[] = { service_group };

    sm_service_group_member_table_foreach_member( service_group->name,
                    user_data, sm_service_group_go_active_service );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active - Service Complete
// ==========================================
static void sm_service_group_go_active_service_complete( void* user_data[],
    SmServiceGroupMemberT* service_group_member )
{
    bool* complete_overall = (bool*) user_data[0];

    if( SM_SERVICE_STATE_ENABLED_ACTIVE == service_group_member->service_state )
    {
        DPRINTFD( "Go-Active of service (%s) for service group (%s) complete, "
                  "state=%s.", service_group_member->service_name, 
                  service_group_member->name,
                  sm_service_state_str( service_group_member->service_state ) );
    } else {
        *complete_overall = false;

        DPRINTFD( "Go-Active of service (%s) for service group (%s) not yet "
                  "complete, state=%s.", service_group_member->service_name, 
                  service_group_member->name,
                  sm_service_state_str( service_group_member->service_state ) );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active - Complete
// ==================================
SmErrorT sm_service_group_go_active_complete( SmServiceGroupT* service_group,
    bool* complete )
{
    bool go_active_complete = true;
    void* user_data[] = { &go_active_complete };

    *complete = false;

    sm_service_group_member_table_foreach_member( service_group->name,
                    user_data, sm_service_group_go_active_service_complete );

    *complete = go_active_complete;

    if( go_active_complete )
    {
        DPRINTFI( "All services go-active for service group (%s) "
                  "are complete.", service_group->name );
    } else {
        DPRINTFD( "Some service go-actives for service group (%s) "
                  "are not yet complete.", service_group->name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active - Initialize
// ====================================
SmErrorT sm_service_group_go_active_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active - Finalize
// ==================================
SmErrorT sm_service_group_go_active_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
