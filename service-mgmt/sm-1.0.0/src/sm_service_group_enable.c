//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_enable.h"

#include <stdio.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_service_group_member_table.h"
#include "sm_service_api.h"

// ****************************************************************************
// Service Group Enable - Service
// ==============================
static void sm_service_group_enable_service( void* user_data[],
    SmServiceGroupMemberT* service_group_member )
{
    SmServiceGroupT* service_group = (SmServiceGroupT*) user_data[0];

    DPRINTFD( "Enabling %s of %s", service_group_member->service_name,
              service_group->name );

    // No api to call. 
    return;
}
// ****************************************************************************

// ****************************************************************************
// Service Group Enable
// ====================
SmErrorT sm_service_group_enable( SmServiceGroupT* service_group )
{
    void* user_data[] = { service_group };

    sm_service_group_member_table_foreach_member( service_group->name,
                    user_data, sm_service_group_enable_service );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Enable - Service Complete
// =======================================
static void sm_service_group_enable_service_complete( void* user_data[],
    SmServiceGroupMemberT* service_group_member )
{
    bool* complete_overall = (bool*) user_data[0];

    if(( SM_SERVICE_STATE_ENABLED_ACTIVE     == service_group_member->service_state )||
       ( SM_SERVICE_STATE_ENABLED_GO_ACTIVE  == service_group_member->service_state )||
       ( SM_SERVICE_STATE_ENABLED_GO_STANDBY == service_group_member->service_state )||
       ( SM_SERVICE_STATE_ENABLED_STANDBY    == service_group_member->service_state ))
    {
        DPRINTFD( "Enable of service (%s) for service group (%s) complete, "
                  "state=%s.", service_group_member->service_name, 
                  service_group_member->name,
                  sm_service_state_str( service_group_member->service_state ) );
    } else {
        *complete_overall = false;

        DPRINTFD( "Enable of service (%s) for service group (%s) not yet "
                  "complete, state=%s.", service_group_member->service_name, 
                  service_group_member->name,
                  sm_service_state_str( service_group_member->service_state ) );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Enable - Complete
// ===============================
SmErrorT sm_service_group_enable_complete( SmServiceGroupT* service_group,
    bool* complete )
{
    bool enable_complete = true;
    void* user_data[] = { &enable_complete };

    *complete = false;

    sm_service_group_member_table_foreach_member( service_group->name,
                    user_data, sm_service_group_enable_service_complete );

    *complete = enable_complete;

    if( enable_complete )
    {
        DPRINTFI( "All services enabled for service group (%s) "
                  "are complete.", service_group->name );
    } else {
        DPRINTFD( "Some service enables for service group (%s) "
                  "are not yet complete.", service_group->name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Enable - Initialize
// =================================

SmErrorT sm_service_group_enable_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Enable - Finalize
// ===============================

SmErrorT sm_service_group_enable_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
