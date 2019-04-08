//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//

#include "sm_failover_ss.h"
#include <string.h>
#include <time.h>
#include "sm_debug.h"
#include "sm_limits.h"
#include "sm_node_utils.h"
#include "sm_node_api.h"
#include "sm_failover_utils.h"
#include "sm_node_utils.h"
#include "sm_node_api.h"
#include "sm_failover.h"

// uncomment when debugging this module to enabled DPRINTFD output to log file
// #define __DEBUG__MSG__

#ifdef __DEBUG__MSG__
#undef DPRINTFD
#define DPRINTFD DPRINTFI
#endif

//
SmErrorT _get_survivor_dc(const SmSystemStatusT& system_status, SmSystemFailoverStatus& selection);

// select standby as failed
SmErrorT _fail_standby(const SmSystemStatusT& system_status, SmSystemFailoverStatus& selection)
{
    if(SM_NODE_STATE_STANDBY == system_status.host_status.current_schedule_state)
    {
        selection.set_host_schedule_state(SM_NODE_STATE_FAILED);
        selection.set_peer_schedule_state(SM_NODE_STATE_ACTIVE);
    }else if(SM_NODE_STATE_STANDBY == system_status.peer_status.current_schedule_state)
    {
        selection.set_peer_schedule_state(SM_NODE_STATE_FAILED);
        selection.set_host_schedule_state(SM_NODE_STATE_ACTIVE);
    }else
    {
        DPRINTFE("Runtime error. Unexpected scheduling state: host %s, peer %s (no standby)",
        sm_node_schedule_state_str(system_status.host_status.current_schedule_state),
        sm_node_schedule_state_str(system_status.peer_status.current_schedule_state));
        return SM_FAILED;
    }
    return SM_OKAY;
}

const char SmSystemFailoverStatus::filename[] = "/var/lib/sm/failover.status";
const char SmSystemFailoverStatus::file_format[] =
    "This is a very important system file.\n"
    "Any modification is strictly forbidden and will cause serious consequence.\n"
    "host_schedule_state=%1d\n"
    "peer_schedule_state=%1d\n"
    "last_update=%19s\n";

SmSystemFailoverStatus SmSystemFailoverStatus::_failover_status;

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

SmSystemFailoverStatus& SmSystemFailoverStatus::get_status()
{
    return _failover_status;
}

bool SmSystemFailoverStatus::_is_valid_schedule_state(SmNodeScheduleStateT state)
{
    return (SM_NODE_STATE_ACTIVE == state ||
            SM_NODE_STATE_STANDBY == state ||
            SM_NODE_STATE_FAILED == state ||
            SM_NODE_STATE_INIT == state);
}

void SmSystemFailoverStatus::set_host_schedule_state(SmNodeScheduleStateT state)
{
    if(_is_valid_schedule_state(state))
    {
        if(_host_schedule_state != state)
        {
            _host_schedule_state=state;
        }
    }else
    {
        DPRINTFE("Runtime error, schedule state unknown %d", state);
    }
}

void SmSystemFailoverStatus::set_host_pre_failure_schedule_state(SmNodeScheduleStateT state)
{
    if(_is_valid_schedule_state(state))
    {
        if(_host_pre_failure_schedule_state != state)
        {
            _host_pre_failure_schedule_state = state;
        }
    }else
    {
        DPRINTFE("Runtime error, schedule state unknown %d", state);
    }
}

void SmSystemFailoverStatus::set_cluster_hbs_state(const SmClusterHbsStateT& state)
{
    if( !is_valid(state) )
    {
        DPRINTFE("Runtime error. Invalid cluster hbs state");
        return;
    }
    _cluster_hbs_state = state;
}

void SmSystemFailoverStatus::set_pre_failure_cluster_hbs_state(const SmClusterHbsStateT& state)
{
    if( !is_valid(state) )
    {
        DPRINTFE("Runtime error. Invalid cluster hbs state");
        return;
    }
    _pre_failure_cluster_hbs_state = state;
}

void SmSystemFailoverStatus::set_peer_schedule_state(SmNodeScheduleStateT state)
{
    if(_is_valid_schedule_state(state))
    {
        if(_peer_schedule_state != state)
        {
            _peer_schedule_state = state;
        }
    }else
    {
        DPRINTFE("Runtime error, schedule state unknown %d", state);
    }
}

void SmSystemFailoverStatus::set_peer_pre_failure_schedule_state(SmNodeScheduleStateT state)
{
    if(_is_valid_schedule_state(state))
    {
        if(_peer_pre_failure_schedule_state != state)
        {
            _peer_pre_failure_schedule_state = state;
        }
    }else
    {
        DPRINTFE("Runtime error, schedule state unknown %d", state);
    }
}

void SmSystemFailoverStatus::serialize()
{
    FILE* f;
    time_t last_update;
    struct tm* local_time;
    time(&last_update);
    local_time = localtime(&last_update);
    char timestamp[20];


    sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02d", local_time->tm_year + 1900,
                local_time->tm_mon + 1, local_time->tm_mday, local_time->tm_hour, local_time->tm_min, local_time->tm_sec);

    f = fopen(filename, "w");
    fprintf(f, file_format, _host_schedule_state, _peer_schedule_state, timestamp);
    for(int i = 0; i < 72; i ++)
    {
       fputs(".", f);
    }
    fclose(f);
}

void SmSystemFailoverStatus::deserialize()
{
//    FILE* f;
//    char timestamp[20];
//
//    _host_schedule_state = _peer_schedule_state = SM_NODE_STATE_UNKNOWN;
//    int host_state, peer_state;
//    f = fopen(filename, "r");
//    if(NULL != f)
//    {
//        DPRINTFI("Loading schedule state from %s", filename);
//        int cnt = fscanf(f, file_format, &host_state, &peer_state, timestamp);
//        fclose(f);
//        if(cnt != 3)
//        {
//            DPRINTFE("Runtime error, %s has been modified.", filename);
//        }else
//        {
//            set_host_schedule_state((SmNodeScheduleStateT)host_state);
//            set_peer_schedule_state((SmNodeScheduleStateT)peer_state);
//        }
//    }
}

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
    if(interface_state & SM_FAILOVER_CLUSTER_HOST_DOWN)
    {
        healthy_score -= 2;
    }
    if(interface_state & SM_FAILOVER_MGMT_DOWN)
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
    sys_status.host_status.cluster_host_state = sm_failover_get_interface_info(SM_INTERFACE_CLUSTER_HOST);
    sys_status.host_status.oam_state = sm_failover_get_interface_info(SM_INTERFACE_OAM);

    if(SM_FAILOVER_INTERFACE_OK == sys_status.host_status.mgmt_state ||
       SM_FAILOVER_INTERFACE_OK == sys_status.host_status.oam_state ||
       SM_FAILOVER_INTERFACE_OK == sys_status.host_status.cluster_host_state)
    {
        sys_status.heartbeat_state = SM_HEARTBEAT_OK;
    }else
    {
        sys_status.heartbeat_state = SM_HEARTBEAT_LOSS;
    }

    SmSystemFailoverStatus::get_status().set_heartbeat_state(sys_status.heartbeat_state);

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
    DPRINTFI("get survivor %s %s, %s %s",
        system_status.host_status.node_name,
        sm_node_schedule_state_str(system_status.host_status.current_schedule_state),
        system_status.peer_status.node_name,
        sm_node_schedule_state_str(system_status.peer_status.current_schedule_state));

    selection.set_host_schedule_state(system_status.host_status.current_schedule_state);
    selection.set_peer_schedule_state(system_status.peer_status.current_schedule_state);
    if(SM_HEARTBEAT_OK == system_status.heartbeat_state)
    {
        DPRINTFI("Heartbeat alive");
        int host_healthy_score, peer_healthy_score;
        host_healthy_score = get_node_if_healthy_score(system_status.host_status.interface_state);
        peer_healthy_score = get_node_if_healthy_score(system_status.peer_status.interface_state);
        if( peer_healthy_score < host_healthy_score )
        {
            //host is more healthy
            selection.set_host_schedule_state(SM_NODE_STATE_ACTIVE);
            selection.set_peer_schedule_state(SM_NODE_STATE_STANDBY);
            if(system_status.peer_status.interface_state & SM_FAILOVER_MGMT_DOWN)
            {
                DPRINTFI("Disable peer, host go active");
                selection.set_peer_schedule_state(SM_NODE_STATE_FAILED);
            }
        }else if(peer_healthy_score > host_healthy_score)
        {
            //peer is more healthy
            selection.set_host_schedule_state(SM_NODE_STATE_STANDBY);
            selection.set_peer_schedule_state(SM_NODE_STATE_ACTIVE);
            if(system_status.host_status.interface_state & SM_FAILOVER_MGMT_DOWN)
            {
                DPRINTFI("Disable host, peer go active");
                selection.set_host_schedule_state(SM_NODE_STATE_FAILED);
            }
        }
    }else
    {
        DPRINTFI("Loss of heartbeat ALL");
        bool expect_storage_0 = false;
        SmClusterHbsStateT pre_failure_cluster_hbs_state = selection.get_pre_failure_cluster_hbs_state();
        SmClusterHbsStateT current_cluster_hbs_state = selection.get_cluster_hbs_state();
        bool has_cluser_info = true;
        int max_nodes_available = 0;
        if(is_valid(pre_failure_cluster_hbs_state))
        {
            expect_storage_0 = pre_failure_cluster_hbs_state.storage0_enabled;
            for(unsigned int i = 0; i < max_controllers; i ++)
            {
                if(max_nodes_available < pre_failure_cluster_hbs_state.controllers[i].number_of_node_reachable)
                {
                    max_nodes_available = pre_failure_cluster_hbs_state.controllers[i].number_of_node_reachable;
                }
            }
        }else if(is_valid(current_cluster_hbs_state))
        {
            expect_storage_0 = current_cluster_hbs_state.storage0_enabled;
            for(unsigned int i = 0; i < max_controllers; i ++)
            {
                if(max_nodes_available < pre_failure_cluster_hbs_state.controllers[i].number_of_node_reachable)
                {
                    max_nodes_available = pre_failure_cluster_hbs_state.controllers[i].number_of_node_reachable;
                }
            }
        }else
        {
            has_cluser_info = false;
        }

        if(has_cluser_info && max_nodes_available > 1)
        {
            DPRINTFD("storage-0 is %s", expect_storage_0 ? "enabled":"not enabled");
            int this_controller_index, peer_controller_index;

            char host_name[SM_NODE_NAME_MAX_CHAR];
            SmErrorT error = sm_node_utils_get_hostname(host_name);
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to get hostname, error=%s.",
                          sm_error_str( error ) );
                return SM_FAILED;
            }

            if(0 == strncmp(SM_NODE_CONTROLLER_0_NAME, host_name, sizeof(SM_NODE_CONTROLLER_0_NAME)))
            {
                this_controller_index = 0;
                peer_controller_index = 1;
            }else
            {
                this_controller_index = 1;
                peer_controller_index = 0;
            }

            bool survivor_selected = false;
            if(expect_storage_0)
            {
                if(current_cluster_hbs_state.controllers[this_controller_index].storage0_responding &&
                    !current_cluster_hbs_state.controllers[peer_controller_index].storage0_responding)
                {
                    DPRINTFI("peer cannot reach storage-0. host can. host will be survivor");
                    selection.set_host_schedule_state(SM_NODE_STATE_ACTIVE);
                    selection.set_peer_schedule_state(SM_NODE_STATE_FAILED);
                    survivor_selected = true;
                }else if(!current_cluster_hbs_state.controllers[this_controller_index].storage0_responding &&
                         current_cluster_hbs_state.controllers[peer_controller_index].storage0_responding)
                {
                    DPRINTFI("host cannot reach storage-0. peer can. peer will be survivor");
                    selection.set_host_schedule_state(SM_NODE_STATE_FAILED);
                    selection.set_peer_schedule_state(SM_NODE_STATE_ACTIVE);
                    survivor_selected = true;
                }
            }

            if(!survivor_selected)
            {
                // so no storage-0 or storage-0 state same on both side
                if(current_cluster_hbs_state.controllers[this_controller_index].number_of_node_reachable >
                   current_cluster_hbs_state.controllers[peer_controller_index].number_of_node_reachable)
                {
                    DPRINTFI("host reaches %d nodes, peer reaches %d nodes, host will be survivor",
                        current_cluster_hbs_state.controllers[this_controller_index].number_of_node_reachable,
                        current_cluster_hbs_state.controllers[peer_controller_index].number_of_node_reachable
                    );
                    selection.set_host_schedule_state(SM_NODE_STATE_ACTIVE);
                    selection.set_peer_schedule_state(SM_NODE_STATE_FAILED);
                    survivor_selected = true;
                }else if (current_cluster_hbs_state.controllers[this_controller_index].number_of_node_reachable <
                   current_cluster_hbs_state.controllers[peer_controller_index].number_of_node_reachable)
                {
                    DPRINTFI("host reaches %d nodes, peer reaches %d nodes, peer will be survivor",
                        current_cluster_hbs_state.controllers[this_controller_index].number_of_node_reachable,
                        current_cluster_hbs_state.controllers[peer_controller_index].number_of_node_reachable
                    );
                    selection.set_host_schedule_state(SM_NODE_STATE_FAILED);
                    selection.set_peer_schedule_state(SM_NODE_STATE_ACTIVE);
                    survivor_selected = true;
                }else
                {
                    if(pre_failure_cluster_hbs_state != current_cluster_hbs_state)
                    {
                        if(0 == current_cluster_hbs_state.controllers[this_controller_index].number_of_node_reachable)
                        {
                            // Cannot reach any nodes, I am dead
                            DPRINTFI("host cannot reach any nodes, peer will be survivor",
                                    current_cluster_hbs_state.controllers[this_controller_index].number_of_node_reachable,
                                    current_cluster_hbs_state.controllers[peer_controller_index].number_of_node_reachable
                                );
                            selection.set_host_schedule_state(SM_NODE_STATE_FAILED);
                            selection.set_peer_schedule_state(SM_NODE_STATE_ACTIVE);
                        }else
                        {
                            // equaly split, failed the standby
                            if(SM_NODE_STATE_ACTIVE == system_status.host_status.current_schedule_state)
                            {
                                DPRINTFI("host reaches %d nodes, peer reaches %d nodes, host will be survivor",
                                    current_cluster_hbs_state.controllers[this_controller_index].number_of_node_reachable,
                                    current_cluster_hbs_state.controllers[peer_controller_index].number_of_node_reachable
                                );
                                selection.set_peer_schedule_state(SM_NODE_STATE_FAILED);
                            }else
                            {
                                DPRINTFI("host reaches %d nodes, peer reaches %d nodes, peer will be survivor",
                                    current_cluster_hbs_state.controllers[this_controller_index].number_of_node_reachable,
                                    current_cluster_hbs_state.controllers[peer_controller_index].number_of_node_reachable
                                );
                                selection.set_host_schedule_state(SM_NODE_STATE_FAILED);
                            }
                        }
                    }
                    else
                    {
                        // no connectivity status changed? peer sm is not responding
                        DPRINTFI("Peer SM is not responding, host will be survivor");
                        selection.set_host_schedule_state(SM_NODE_STATE_ACTIVE);
                        selection.set_peer_schedule_state(SM_NODE_STATE_FAILED);
                    }
                }
            }
        }
        else
        {
            // no cluster info, peer is assumed down
            // the connecting to majority nodes rule is postponed
            DPRINTFI("No cluster hbs info, host will be survivor");
            selection.set_host_schedule_state(SM_NODE_STATE_ACTIVE);
            selection.set_peer_schedule_state(SM_NODE_STATE_FAILED);
        }
    }

    if(SM_SYSTEM_MODE_CPE_DUPLEX == system_status.system_mode)
    {
    }

    if(SM_SYSTEM_MODE_CPE_DUPLEX_DC == system_status.system_mode)
    {
        return _get_survivor_dc(system_status, selection);
    }

    SmNodeScheduleStateT host_schedule_state, peer_schedule_state;
    host_schedule_state = selection.get_host_schedule_state();
    peer_schedule_state = selection.get_peer_schedule_state();
    DPRINTFI("Host from %s to %s, Peer from %s to %s.",
        sm_node_schedule_state_str(system_status.host_status.current_schedule_state),
        sm_node_schedule_state_str(host_schedule_state),
        sm_node_schedule_state_str(system_status.peer_status.current_schedule_state),
        sm_node_schedule_state_str(peer_schedule_state)
    );

    if((system_status.host_status.current_schedule_state == SM_NODE_STATE_ACTIVE &&
        host_schedule_state != SM_NODE_STATE_ACTIVE) ||
        (system_status.peer_status.current_schedule_state == SM_NODE_STATE_ACTIVE &&
        peer_schedule_state != SM_NODE_STATE_ACTIVE))
    {
        DPRINTFI("Uncontrolled swact starts. Host from %s to %s, Peer from %s to %s.",
            sm_node_schedule_state_str(system_status.host_status.current_schedule_state),
            sm_node_schedule_state_str(selection.get_host_schedule_state()),
            sm_node_schedule_state_str(system_status.peer_status.current_schedule_state),
            sm_node_schedule_state_str(selection.get_peer_schedule_state())
        );
    }

    selection.serialize();
    return SM_OKAY;
}
// ****************************************************************************


// ****************************************************************************
// Direct connected
SmErrorT _get_survivor_dc(const SmSystemStatusT& system_status, SmSystemFailoverStatus& selection)
{
    if(SM_SYSTEM_MODE_CPE_DUPLEX_DC != system_status.system_mode)
    {
        DPRINTFE("Runtime error, not the right system mode %d", system_status.system_mode);
        return SM_FAILED;
    }

    if(SM_HEARTBEAT_LOSS == system_status.heartbeat_state)
    {
        if(system_status.host_status.mgmt_state == SM_FAILOVER_INTERFACE_DOWN &&
            (system_status.host_status.cluster_host_state == SM_FAILOVER_INTERFACE_DOWN ||
            system_status.host_status.cluster_host_state == SM_FAILOVER_INTERFACE_UNKNOWN))
        {
            if(SM_FAILOVER_INTERFACE_DOWN == system_status.host_status.oam_state)
            {
                selection.set_host_schedule_state(SM_NODE_STATE_FAILED);
                selection.set_peer_schedule_state(SM_NODE_STATE_ACTIVE);
            }else
            {
                selection.set_peer_schedule_state(SM_NODE_STATE_FAILED);
                selection.set_host_schedule_state(SM_NODE_STATE_ACTIVE);
            }
        }else
        {
            selection.set_peer_schedule_state(SM_NODE_STATE_FAILED);
            selection.set_host_schedule_state(SM_NODE_STATE_ACTIVE);
        }
    }
    return SM_OKAY;
}
