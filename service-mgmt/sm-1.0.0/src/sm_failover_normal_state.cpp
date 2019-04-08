//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_failover_normal_state.h"
#include <stdlib.h>
#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_node_api.h"
#include "sm_node_utils.h"
#include "sm_failover_utils.h"
#include "sm_failover_fsm.h"
#include "sm_failover_ss.h"
#include "sm_cluster_hbs_info_msg.h"

SmErrorT SmFailoverNormalState::event_handler(SmFailoverEventT event, const ISmFSMEventData* event_data)
{
    switch (event)
    {
        case SM_FAILOVER_EVENT_IF_STATE_CHANGED:
            if(NULL != event_data &&
                SmIFStateChangedEventData::SmIFStateChangedEventDataType == event_data->get_event_data_type())
            {
                const SmIFStateChangedEventData* data = (const SmIFStateChangedEventData*) event_data;

                SmFailoverInterfaceStateT oam_state, mgmt_state, cluster_host_state;
                oam_state = data->get_interface_state(SM_INTERFACE_OAM);
                mgmt_state = data->get_interface_state(SM_INTERFACE_MGMT);
                cluster_host_state = data->get_interface_state(SM_INTERFACE_CLUSTER_HOST);
                if(oam_state != SM_FAILOVER_INTERFACE_OK ||
                   mgmt_state != SM_FAILOVER_INTERFACE_OK ||
                   (cluster_host_state != SM_FAILOVER_INTERFACE_OK && cluster_host_state != SM_FAILOVER_INTERFACE_UNKNOWN))
                {
                    this->fsm.set_state(SM_FAILOVER_STATE_FAIL_PENDING);
                }
            }else
            {
                DPRINTFE("Runtime error: receive invalid event data type");
            }

            break;

        default:
            DPRINTFE("Runtime error, unexpected event %s, at state %s",
                sm_failover_event_str(event),
                sm_failover_state_str(this->fsm.get_state()));
    }
    return SM_OKAY;
}

SmErrorT SmFailoverNormalState::exit_state()
{
    SmSystemFailoverStatus& failover_status = SmSystemFailoverStatus::get_status();
    char host_name[SM_NODE_NAME_MAX_CHAR];
    SmErrorT error = sm_node_utils_get_hostname(host_name);
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        failover_status.set_host_pre_failure_schedule_state(SM_NODE_STATE_UNKNOWN);
    }else
    {
        SmNodeScheduleStateT host_state = sm_get_controller_state(host_name);
        DPRINTFI("Blind guess pre failure host state %s", sm_node_schedule_state_str(host_state));
        failover_status.set_host_pre_failure_schedule_state(host_state);
    }

    char peer_name[SM_NODE_NAME_MAX_CHAR];
    error = sm_node_api_get_peername(peer_name);
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get peername, error=%s.",
                  sm_error_str( error ) );
        failover_status.set_peer_pre_failure_schedule_state(SM_NODE_STATE_UNKNOWN);
    }else
    {
        SmNodeScheduleStateT peer_state = sm_get_controller_state(peer_name);
        failover_status.set_peer_pre_failure_schedule_state(peer_state);
    }

    const SmClusterHbsStateT& cluster_hbs_state_cur = SmClusterHbsInfoMsg::get_current_state();
    const SmClusterHbsStateT& cluster_hbs_state_pre = SmClusterHbsInfoMsg::get_previous_state();
    SmClusterHbsStateT pre_failure_cluster_hsb_state;
    if(!is_valid(cluster_hbs_state_cur))
    {
        DPRINTFE("No cluster hbs state available");
    }else
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        if(ts.tv_sec - cluster_hbs_state_cur.last_update <= 1 && cluster_hbs_state_pre.last_update != 0)
        {
            // cluster hbs state changed within past 1 second, take the pre state as pre-failure state.
            pre_failure_cluster_hsb_state = cluster_hbs_state_pre;
        }else
        {
            pre_failure_cluster_hsb_state = cluster_hbs_state_cur;
        }

        log_cluster_hbs_state(pre_failure_cluster_hsb_state);
        failover_status.set_pre_failure_cluster_hbs_state(pre_failure_cluster_hsb_state);
    }

    SmFSMState::exit_state();
    return SM_OKAY;
}
