//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//

#include "sm_failover_ss.h"
#include <time.h>
#include "sm_debug.h"
#include "sm_limits.h"
#include "sm_node_utils.h"
#include "sm_node_api.h"
#include "sm_failover_utils.h"
#include "sm_node_utils.h"
#include "sm_node_api.h"
#include "sm_failover.h"

SmSystemFailoverStatus::SmSystemFailoverStatus()
{
    SmErrorT error;
    char host_name[SM_NODE_NAME_MAX_CHAR];
    char peer_name[SM_NODE_NAME_MAX_CHAR];
    error = sm_node_utils_get_hostname(host_name);
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return;
    }
    error = sm_node_api_get_peername(peer_name);
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get peername, error=%s.",
                  sm_error_str( error ) );
        return;
    }
    _host_schedule_state = sm_get_controller_state(host_name);
    _peer_schedule_state = sm_get_controller_state(peer_name);
}

SmSystemFailoverStatus::~SmSystemFailoverStatus()
{
}

bool SmSystemFailoverStatus::_is_valid_schedule_state(SmNodeScheduleStateT state)
{
    return (SM_NODE_STATE_ACTIVE == state ||
            SM_NODE_STATE_STANDBY == state ||
            SM_NODE_STATE_FAILED == state);
}

void SmSystemFailoverStatus::set_host_schedule_state(SmNodeScheduleStateT state)
{
    if(_is_valid_schedule_state(state))
    {
        if(_host_schedule_state != state)
        {
            _host_schedule_state=state;
            _changed = true;
        }
    }else
    {
        DPRINTFE("Runtime error, schedule state unknown %d", state);
    }
}

void SmSystemFailoverStatus::set_peer_schedule_state(SmNodeScheduleStateT state)
{
    if(_is_valid_schedule_state(state))
    {
        if(_peer_schedule_state != state)
        {
            _peer_schedule_state = state;
            _changed = true;
        }
    }else
    {
        DPRINTFE("Runtime error, schedule state unknown %d", state);
    }
}

typedef enum
{
    SM_FAILOVER_INFRA_DOWN = 1,
    SM_FAILOVER_MGMT_DOWN = 2,
    SM_FAILOVER_OAM_DOWN = 4,
}SmFailoverCommFaultBitFlagT;

// ****************************************************************************
// sm_failover_ss get_node_if_healthy_score - get node interface healthy score
// ===================
static int get_node_if_healthy_score(unsigned int interface_state)
{
    int healthy_score = 0;
    if(interface_state & SM_FAILOVER_OAM_DOWN)
    {
        healthy_score -= 1;
    }
    if(interface_state & SM_FAILOVER_INFRA_DOWN)
    {
        healthy_score -= 2;
    }
    if(interface_state & SM_FAILOVER_INFRA_DOWN)
    {
        healthy_score -= 4;
    }

    return healthy_score;
}
// ****************************************************************************

SmErrorT _get_system_status(SmSystemStatusT& sys_status, char host_name[], char peer_name[])
{
    SmErrorT error;
    SmNodeScheduleStateT host_state;

    error = sm_node_utils_get_hostname(host_name);
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return error;
    }
    error = sm_node_api_get_peername(peer_name);
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get peername, error=%s.",
                  sm_error_str( error ) );
        return error;
    }
    host_state = sm_get_controller_state(host_name);

    sys_status.system_mode = sm_node_utils_get_system_mode();
    sys_status.host_status.mgmt_state = sm_failover_get_interface_info(SM_INTERFACE_MGMT);
    sys_status.host_status.infra_state = sm_failover_get_interface_info(SM_INTERFACE_INFRA);
    sys_status.host_status.oam_state = sm_failover_get_interface_info(SM_INTERFACE_OAM);

    if(SM_FAILOVER_INTERFACE_OK == sys_status.host_status.mgmt_state ||
       SM_FAILOVER_INTERFACE_OK == sys_status.host_status.oam_state ||
       SM_FAILOVER_INTERFACE_OK == sys_status.host_status.infra_state)
    {
        sys_status.heartbeat_state = SM_HEARTBEAT_OK;
    }else
    {
        sys_status.heartbeat_state = SM_HEARTBEAT_LOSS;
    }

    sys_status.host_status.node_name = host_name;
    sys_status.host_status.interface_state = sm_failover_if_state_get();
    sys_status.host_status.current_schedule_state = host_state;
    sys_status.peer_status.node_name = peer_name;

    sys_status.peer_status.interface_state = sm_failover_get_peer_if_state();
    sys_status.peer_status.current_schedule_state = sm_get_controller_state(peer_name);
    return SM_OKAY;
}

// ****************************************************************************
// sm_failover_ss_get_survivor - select the failover survivor
// This is the main entry/container for the failover logic to determine how
// to schedule the controllers, i.e, active/standby or active/failure.
// ===================
SmErrorT sm_failover_ss_get_survivor(SmSystemFailoverStatus& selection)
{
    SmSystemStatusT sys_status;
    char host_name[SM_NODE_NAME_MAX_CHAR];
    char peer_name[SM_NODE_NAME_MAX_CHAR];

    SmErrorT error = _get_system_status(sys_status, host_name, peer_name);
    if(SM_OKAY != error)
    {
        DPRINTFE("Failed to retrieve system status. Error %s", sm_error_str(error));
        return error;
    }
    return sm_failover_ss_get_survivor(sys_status, selection);
}

SmErrorT sm_failover_ss_get_survivor(const SmSystemStatusT& system_status, SmSystemFailoverStatus& selection)
{
    selection.set_host_schedule_state(system_status.host_status.current_schedule_state);
    selection.set_peer_schedule_state(system_status.peer_status.current_schedule_state);
    if(SM_HEARTBEAT_OK == system_status.heartbeat_state)
    {
        int host_healthy_score, peer_healthy_score;
        host_healthy_score = get_node_if_healthy_score(system_status.host_status.interface_state);
        peer_healthy_score = get_node_if_healthy_score(system_status.peer_status.interface_state);
        if( peer_healthy_score < host_healthy_score )
        {
            //host is more healthy
            selection.set_host_schedule_state(SM_NODE_STATE_ACTIVE);
            selection.set_peer_schedule_state(SM_NODE_STATE_STANDBY);
        }else if(peer_healthy_score > host_healthy_score)
        {
            //peer is more healthy
            selection.set_host_schedule_state(SM_NODE_STATE_STANDBY);
            selection.set_peer_schedule_state(SM_NODE_STATE_ACTIVE);
        }
    }

    if(system_status.host_status.current_schedule_state != selection.get_host_schedule_state() ||
        system_status.peer_status.current_schedule_state != selection.get_peer_schedule_state() )
    {
        DPRINTFI("Uncontrolled swact starts. Host from %s to %s, Peer from %s to %s.",
            sm_node_schedule_state_str(system_status.host_status.current_schedule_state),
            sm_node_schedule_state_str(selection.get_host_schedule_state()),
            sm_node_schedule_state_str(system_status.peer_status.current_schedule_state),
            sm_node_schedule_state_str(selection.get_peer_schedule_state())
        );
    }
    return SM_OKAY;
}
// ****************************************************************************
