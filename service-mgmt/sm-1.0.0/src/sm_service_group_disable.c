//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_disable.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_group_member_table.h"
#include "sm_service_api.h"

// ****************************************************************************
// Service Group Disable - Service
// ===============================
static void sm_service_group_disable_service( void* user_data[],
    SmServiceGroupMemberT* service_group_member )
{
    SmServiceGroupT* service_group = (SmServiceGroupT*) user_data[0];
    SmErrorT error;
    
    DPRINTFD( "Disabling %s of %s", service_group_member->service_name,
              service_group->name );

    error = sm_service_api_disable( service_group_member->service_name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to send disable to service (%s) of "
                  "service group (%s), error=%s.", 
                  service_group_member->service_name,
                  service_group->name, sm_error_str( error ) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Disable
// =====================
SmErrorT sm_service_group_disable( SmServiceGroupT* service_group )
{
    void* user_data[] = { service_group };

    sm_service_group_member_table_foreach_member( service_group->name,
                    user_data, sm_service_group_disable_service );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Disable - Service Complete
// ========================================
static void sm_service_group_disable_service_complete( void* user_data[],
    SmServiceGroupMemberT* service_group_member )
{
    bool* complete_overall = (bool*) user_data[0];
    
    if( SM_SERVICE_STATE_DISABLED == service_group_member->service_state )
    {
        DPRINTFD( "Disable of service (%s) for service group (%s) complete, "
                  "state=%s.", service_group_member->service_name, 
                  service_group_member->name,
                  sm_service_state_str( service_group_member->service_state ) );
    } else {
        *complete_overall = false;

        DPRINTFD( "Disable of service (%s) for service group (%s) not yet "
                  "complete, state=%s.", service_group_member->service_name, 
                  service_group_member->name,
                  sm_service_state_str( service_group_member->service_state ) );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Disable - Complete
// ================================
SmErrorT sm_service_group_disable_complete( SmServiceGroupT* service_group,
    bool* complete )
{
    bool disable_complete = true;
    void* user_data[] = { &disable_complete };

    *complete = false;

    sm_service_group_member_table_foreach_member( service_group->name,
                    user_data, sm_service_group_disable_service_complete );

    *complete = disable_complete;

    if( disable_complete )
    {
        DPRINTFI( "All services disabled for service group (%s) are "
                  "complete.", service_group->name );
    } else {
        DPRINTFD( "Some service disables for service group (%s) are "
                  "not yet complete.", service_group->name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Disable - Initialize
// ==================================
SmErrorT sm_service_group_disable_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Disable - Finalize
// ================================
SmErrorT sm_service_group_disable_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
