//
// Copyright (c) 2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_failover.h"

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <json-c/json.h>

#include "sm_service_domain_interface_table.h"
#include "sm_debug.h"
#include "sm_hw.h"
#include "sm_node_utils.h"
#include "sm_db_nodes.h"
#include "sm_timer.h"
#include "sm_service_group_table.h"
#include "sm_node_api.h"
#include "sm_utils.h"
#include "sm_node_fsm.h"
#include "sm_service_domain_scheduler.h"
#include "sm_service_domain_table.h"
#include "sm_service_domain_assignment_table.h"
#include "sm_service_domain_utils.h"
#include "sm_service_domain_neighbor_fsm.h"
#include "sm_service_domain_member_table.h"
#include "sm_service_domain_interface_fsm.h"
#include "sm_service_domain_fsm.h"
#include "sm_heartbeat_msg.h"
#include "sm_node_swact_monitor.h"
#include "sm_util_types.h"
#include "sm_failover_ss.h"
#include "sm_failover_utils.h"
#include "sm_failover_fsm.h"
#include "sm_api.h"
#include "sm_cluster_hbs_info_msg.h"

#define SM_FAILOVER_STATE_TRANSITION_TIME_IN_MS 2000
#define SM_FAILOVER_MULTI_FAILURE_WAIT_TIMER_IN_MS 2000
#define SM_FAILOVER_RECOVERY_INTERVAL_IN_SEC 100
#define SM_FAILOVER_INTERFACE_STATE_REPORT_INTERVAL_MS 20000

typedef enum
{
    SM_FAILOVER_ACTION_RESULT_OK,
    SM_FAILOVER_ACTION_RESULT_NO_ACTION,
    SM_FAILOVER_ACTION_RESULT_FAILED,
}SmFailoverActionResultT;

typedef struct
{
    int active_controller_action;
    int standby_controller_action;
}SmFailoverActionPairT;

class SmFailoverInterfaceInfo
{
    private:
        SmServiceDomainInterfaceT* interface;
        SmFailoverInterfaceStateT state;
        struct timespec last_update;
    public:
        SmFailoverInterfaceInfo()
        {
            this->interface = NULL;
            this->state = SM_FAILOVER_INTERFACE_UNKNOWN;
        }
        virtual ~SmFailoverInterfaceInfo()
        {
        }

        void set_interface(SmServiceDomainInterfaceT* interface)
        {
            this->interface = interface;
            clock_gettime( CLOCK_MONOTONIC_RAW, &(this->last_update) );
        }

        SmServiceDomainInterfaceT* get_interface() const
        {
            return this->interface;
        }

        SmFailoverInterfaceStateT get_state() const
        {
            return this->state;
        }

        bool set_state(SmFailoverInterfaceStateT state)
        {
            if(this->state != state)
            {
                this->state = state;
                clock_gettime( CLOCK_MONOTONIC_RAW, &(this->last_update) );
                return true;
            }
            return false;
        }

        bool state_in_transition() const
        {
            return (SM_FAILOVER_STATE_TRANSITION_TIME_IN_MS < this->time_since_last_state_change_ms());
        }

        int time_since_last_state_change_ms() const
        {
            struct timespec now;
            int sec, nsec;
            clock_gettime( CLOCK_MONOTONIC_RAW, &now );
            sec = now.tv_sec - this->last_update.tv_sec;
            nsec = now.tv_nsec - this->last_update.tv_nsec;
            return (sec * 1000000000 + nsec) / 1000000;
        }
};

SmTimerIdT failover_audit_timer_id;
static char _host_name[SM_NODE_NAME_MAX_CHAR];
static char _peer_name[SM_NODE_NAME_MAX_CHAR];
static SmHeartbeatMsgIfStateT _peer_if_state = 0;
static SmHeartbeatMsgIfStateT _peer_if_state_at_last_action = 0;
static bool _retry = true;
static bool _recheck = true;

static int _degraded_flag = 0;

static SmFailoverInterfaceInfo *_end_of_list = NULL;
static SmFailoverInterfaceInfo _my_if_list[SM_INTERFACE_MAX];
static unsigned int _total_interfaces;
static SmFailoverInterfaceInfo* _oam_interface_info = NULL;
static SmFailoverInterfaceInfo* _mgmt_interface_info = NULL;
static SmFailoverInterfaceInfo* _infra_interface_info = NULL;
static SmFailoverInterfaceInfo _peer_if_list[SM_INTERFACE_MAX];
static pthread_mutex_t _mutex;
static SmDbHandleT* _sm_db_handle = NULL;

static SmNodeScheduleStateT _host_state;
static SmNodeScheduleStateT _host_state_at_last_action; // host state when action was taken last time
static int64_t _node_comm_state = -1;
static int _prev_if_state_flag = -1;
time_t _last_if_state_ms = 0;
static SmNodeScheduleStateT _prev_host_state= SM_NODE_STATE_UNKNOWN;

static bool _hello_msg_alive = true;
static SmSystemModeT _system_mode;
static time_t _last_report_ts = 0;
static int _heartbeat_count = 0;


static bool _if_state_changed = false;

SmErrorT sm_exec_json_command(const char* cmd, char result_buf[], int result_len);
SmErrorT sm_failover_get_node_oper_state(char* node_name, SmNodeOperationalStateT *state);
void _log_nodes_state();

// ****************************************************************************
// Failover - interface check
// ==================
static void sm_failover_interface_check( void* user_data[],
    SmServiceDomainInterfaceT* interface )
{
    unsigned int* count = (unsigned int*)user_data[0];
    if (*count < sizeof(_my_if_list))
    {
        if( 0 == strcmp(SM_SERVICE_DOMAIN_OAM_INTERFACE, interface->service_domain_interface ))
        {
            sm_node_utils_get_oam_interface( interface->interface_name );
            _my_if_list[*count].set_interface(interface);
            _oam_interface_info = _my_if_list + (*count);
            (*count) ++;
        }else if( 0 == strcmp(SM_SERVICE_DOMAIN_MGMT_INTERFACE, interface->service_domain_interface ))
        {
            sm_node_utils_get_mgmt_interface( interface->interface_name );
            _my_if_list[*count].set_interface(interface);
            _mgmt_interface_info = _my_if_list + (*count);
            (*count) ++;
        }else if( 0 == strcmp(SM_SERVICE_DOMAIN_INFRA_INTERFACE, interface->service_domain_interface ))
        {
            SmErrorT error = sm_node_utils_get_infra_interface(interface->interface_name);
            if(SM_OKAY == error)
            {
                _my_if_list[*count].set_interface(interface);
                _infra_interface_info = _my_if_list + (*count);
                (*count) ++;
            } else if (SM_NOT_FOUND != error )
            {
                DPRINTFE( "Failed to look up infrastructure interface, error=%s.",
                      sm_error_str(error) );
            }
        }else
        {
            DPRINTFE( "Unknown interface %s", interface->interface_name );
        }
    }
    else
    {
        DPRINTFE("More domain interfaces than it was expected.");
    }
}
// ****************************************************************************

// ****************************************************************************
// Failover - find interface info
// ==================
static SmFailoverInterfaceInfo* find_interface_info( SmFailoverInterfaceT* interface )
{
    SmServiceDomainInterfaceT* domain_interface = sm_service_domain_interface_table_read(
        interface->service_domain,
        interface->service_domain_interface
    );
    if ( NULL == domain_interface )
    {
        DPRINTFE("Unknown interface (%s %s)", interface->service_domain, interface->service_domain_interface);
        return NULL;
    }

    int64_t if_id = domain_interface->id;

    SmFailoverInterfaceInfo* iter;
    for(iter = _my_if_list; iter < _my_if_list + _total_interfaces; iter ++)
    {
        if(iter->get_interface()->id == if_id)
        {
            return iter;
        }
    }
    DPRINTFE("Unknown interface id (%u)", if_id);
    return NULL;
}
// ****************************************************************************

// ****************************************************************************
// Failover - lost Hello msg
// ==================
void sm_failover_lost_hello_msg()
{
    mutex_holder holder(&_mutex);

    if(_hello_msg_alive)
    {
        DPRINTFI( "Neighbor (%s) declared dead.", _peer_name );
        _hello_msg_alive = false;
    }
}

// ****************************************************************************
// Failover - Hello msg restore
// ==================
void sm_failover_hello_msg_restore()
{
    mutex_holder holder(&_mutex);

    SmFailoverInterfaceInfo* iter;
    for(iter = _my_if_list; iter < _end_of_list; iter ++)
    {
        if ( SM_FAILOVER_INTERFACE_OK == iter->get_state() )
        {
            _hello_msg_alive = true;
            break;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Failover - lost heartbeat
// ==================
void sm_failover_lost_heartbeat( SmFailoverInterfaceT* interface )
{
    mutex_holder holder(&_mutex);

    SmFailoverInterfaceInfo* if_info = find_interface_info( interface );
    if ( NULL == if_info )
    {
        return;
    }

    SmFailoverInterfaceStateT state = if_info->get_state();
    if( SM_FAILOVER_INTERFACE_MISSING_HEARTBEAT == state )
    {
        return;
    }
    else if( SM_FAILOVER_INTERFACE_DOWN == state )
    {
        return;
    }

    if(if_info->set_state(SM_FAILOVER_INTERFACE_MISSING_HEARTBEAT))
    {
        DPRINTFI("Interface %s lose heartbeat.", interface->interface_name);
    }

    _if_state_changed = true;
}
// ****************************************************************************

// ****************************************************************************
// Failover - heartbeat restore
// ==================
void sm_failover_heartbeat_restore( SmFailoverInterfaceT* interface )
{
    mutex_holder holder(&_mutex);

    SmFailoverInterfaceInfo* if_info = find_interface_info( interface );
    if ( NULL == if_info )
    {
        return;
    }

    SmFailoverInterfaceStateT state = if_info->get_state();
    if( SM_FAILOVER_INTERFACE_OK == state )
    {
        return;
    }
    else if( SM_FAILOVER_INTERFACE_DOWN == state )
    {
        bool enabled = true;
        SmErrorT error = sm_hw_get_if_state(interface->interface_name, &enabled);
        if(SM_OKAY != error)
        {
           DPRINTFE("Couldn't get interface (%s) state. ", interface->interface_name);
        }
        if(!enabled)
        {
            // need to wait for if up
            return;
        }else
        {
            DPRINTFI("Interface %s is verified as UP after receiving heartbeat.", interface->interface_name);
        }
    }

    if(if_info->set_state(SM_FAILOVER_INTERFACE_OK))
    {
        DPRINTFI("Interface %s heartbeat is OK.", interface->interface_name);
    }

    _if_state_changed = true;
}
// ****************************************************************************

// ****************************************************************************
// Failover - interface down
// ==================
void sm_failover_interface_down( const char* const interface_name )
{
    mutex_holder holder(&_mutex);

    SmFailoverInterfaceInfo* iter;
    int impacted = 0;
    for(iter = _my_if_list; iter < _end_of_list; iter ++)
    {
        SmServiceDomainInterfaceT* interface = iter->get_interface();
        if( 0 == strcmp(interface_name, interface->interface_name) )
        {
            impacted ++;
            if(iter->set_state(SM_FAILOVER_INTERFACE_DOWN))
            {
                DPRINTFI("Domain interface %s is down, due to i/f %s is down.",
                        interface->service_domain_interface, interface_name);
            }
        }
    }

    if( 0 == impacted )
    {
        DPRINTFD("No domain interface is impacted as i/f %s is down.",
            interface_name);
    }
    else
    {
        DPRINTFI("%d domain interfaces are impacted as i/f %s is down.",
            impacted, interface_name);

        _if_state_changed = true;
    }
}
// ****************************************************************************

// ****************************************************************************
// Failover - interface up
// ==================
void sm_failover_interface_up( const char* const interface_name )
{
    mutex_holder holder(&_mutex);

    SmFailoverInterfaceInfo* iter;
    int impacted = 0;
    for(iter = _my_if_list; iter < _my_if_list + _total_interfaces; iter ++)
    {
        SmServiceDomainInterfaceT* interface = iter->get_interface();
        if( 0 == strcmp(interface_name, interface->interface_name) )
        {
            impacted ++;
            if(iter->set_state(SM_FAILOVER_INTERFACE_RECOVERING))
            {
                DPRINTFI("Domain interface %s is recovering, as i/f %s is now up.",
                        interface->service_domain_interface, interface_name);
            }
        }
    }

    if( 0 == impacted )
    {
        DPRINTFI("No domain interface is impacted as i/f %s is up.",
            interface_name);
    }
    else
    {
        DPRINTFI("%d domain interfaces are impacted as i/f %s is up.",
            impacted, interface_name);

        _if_state_changed = true;
    }
}
// ****************************************************************************

// ****************************************************************************
// Failover - degrade
// ==================
SmErrorT sm_failover_degrade(SmFailoverDegradeSourceT source)
{
    _degraded_flag |= source;
    if(0 != _degraded_flag)
    {
        return sm_utils_indicate_degraded();
    }
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Failover - degraded clear
// ==================
SmErrorT sm_failover_degrade_clear(SmFailoverDegradeSourceT source)
{
    if(_degraded_flag)
    {
        _degraded_flag &= (~source);
        if(0 == _degraded_flag)
        {
            DPRINTFI("Degrade clear");
            return sm_utils_clear_degraded();
        }
    }
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Failover - peer interface state update
// ==================
void sm_failover_if_state_update(const char node_name[], SmHeartbeatMsgIfStateT if_state)
{
    if( 0 == strcmp(node_name, _peer_name) )
    {
        mutex_holder holder(&_mutex);

        if( _peer_if_state != if_state )
        {
            DPRINTFI("%s I/F state changed %d => %d", node_name, _peer_if_state, if_state);
            _peer_if_state = if_state;
        }
    }else
    {
        DPRINTFE("If state updated by unknown host %s", node_name);
    }
}
// ****************************************************************************

// ****************************************************************************
// Failover - is infra configured
// ==================
bool is_infra_configured()
{
    if (NULL == _infra_interface_info ||
        _infra_interface_info->get_interface()->service_domain_interface[0] == '\0' )
    {
        return false;
    }

    return true;
}
// ****************************************************************************

// ****************************************************************************
// Failover - get interface state
// ==================
int sm_failover_get_if_state()
{
    SmFailoverInterfaceStateT mgmt_state = _mgmt_interface_info->get_state();
    SmFailoverInterfaceStateT oam_state = _oam_interface_info->get_state();
    SmFailoverInterfaceStateT infra_state;
    int if_state_flag  = 0;
    if ( is_infra_configured() )
    {
        infra_state = _infra_interface_info->get_state();
        if( SM_FAILOVER_INTERFACE_OK == infra_state )
        {
            if_state_flag |= SM_FAILOVER_HEARTBEAT_ALIVE;
        }
        else if ( SM_FAILOVER_INTERFACE_DOWN == infra_state )
        {
            if_state_flag |= SM_FAILOVER_INFRA_DOWN;
        }
    }

    if( SM_FAILOVER_INTERFACE_OK == mgmt_state )
    {
        if_state_flag |= SM_FAILOVER_HEARTBEAT_ALIVE;
    }
    else if ( SM_FAILOVER_INTERFACE_DOWN == mgmt_state )
    {
        if_state_flag |= SM_FAILOVER_MGMT_DOWN;
    }

    if( SM_FAILOVER_INTERFACE_OK == oam_state )
    {
        if_state_flag |= SM_FAILOVER_HEARTBEAT_ALIVE;
    }
    else if ( SM_FAILOVER_INTERFACE_DOWN == oam_state )
    {
        if_state_flag |= SM_FAILOVER_OAM_DOWN;
    }

    return if_state_flag;
}
// ****************************************************************************

// ****************************************************************************
// Failover - get interface state
// ==================
SmHeartbeatMsgIfStateT sm_failover_if_state_get()
{
    mutex_holder holder(&_mutex);
    int if_state_flag = sm_failover_get_if_state();
    return (if_state_flag & 0b0111); //the lower 3 bits i/f state flag
}
// ****************************************************************************

// ****************************************************************************
// Failover - get peer node interface state
// ==================
SmHeartbeatMsgIfStateT sm_failover_get_peer_if_state()
{
    mutex_holder holder(&_mutex);

    return (_peer_if_state & 0b0111); //the lower 3 bits i/f state flag
}
// ****************************************************************************

// ****************************************************************************
// Failover - get controller state
// ==================
SmNodeScheduleStateT get_controller_state()
{
    return sm_get_controller_state(_host_name);
}
// ****************************************************************************

// ****************************************************************************
// Failover - is active controller
// ==================
bool sm_is_active_controller()
{
    return SM_NODE_STATE_ACTIVE == _host_state;
}
// ****************************************************************************

// ****************************************************************************
// Failover - interface is in transit state
// ==================
bool sm_failover_if_transit_state(SmFailoverInterfaceInfo* if_info)
{
    SmFailoverInterfaceStateT if_state = if_info->get_state();
    if( SM_FAILOVER_INTERFACE_RECOVERING == if_state )
    {
        const SmServiceDomainInterfaceT* interface = if_info->get_interface();
        if ( if_info->state_in_transition() )
        {
            DPRINTFI( "If %s is reconvering, wait for either trun OK or missing heartbeat",
                interface->service_domain_interface);
            return true;
        }else
        {
            if_info->set_state(SM_FAILOVER_INTERFACE_MISSING_HEARTBEAT);
            DPRINTFI( "If %s missing heartbeat", interface->service_domain_interface);
            return false;
        }
    }
    return false;
}
// ****************************************************************************

// ****************************************************************************
// Failover - swact controller
// ==================
SmFailoverActionResultT sm_failover_swact()
{
    SmUuidT request_uuid;
    sm_uuid_create( request_uuid );
    DPRINTFI("Uncontrolled swact start");

    SmErrorT error = sm_node_api_swact(_host_name, true);
    if (SM_OKAY == error)
    {
        return  SM_FAILOVER_ACTION_RESULT_OK;
    }
    return SM_FAILOVER_ACTION_RESULT_FAILED;
}
// ****************************************************************************

// ****************************************************************************
// Failover - fail self
// ==================
SmErrorT sm_failover_fail_self()
{
    DPRINTFI("To disable %s", _host_name);
    SmErrorT error = sm_node_fsm_event_handler(
        _host_name, SM_NODE_EVENT_DISABLED, NULL, "Host is failed" );
    if( SM_OKAY != error )
    {
        DPRINTFE("Failed to disable %s, error: %s", _host_name, sm_error_str(error));
        return SM_FAILED;
    }

    sm_node_utils_set_unhealthy();

    error = sm_node_api_fail_node( _host_name );
    if (SM_OKAY != error )
    {
        DPRINTFE("Failed to set %s failed, error %s.", _host_name, sm_error_str(error));
        return SM_FAILED;
    }
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Failover - disable node
// ==================
SmErrorT sm_failover_disable_node(char* node_name)
{
    DPRINTFI("To disable %s", node_name);

    SmErrorT error;
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR];
    snprintf(reason_text, sizeof(reason_text), "%s has failed", node_name);
    error = sm_node_fsm_event_handler(
        node_name, SM_NODE_EVENT_DISABLED, NULL, reason_text );

    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to disable node %s, error=%s.",
                      node_name, sm_error_str( error ) );
        return SM_FAILED;
    }
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Failover - active self callback
// =======================
static void active_self_callback(void* user_data[], SmServiceDomainT* domain)
{
    char reason_text[] = "Neighbor dead";

    sm_service_domain_neighbor_fsm_event_handler(_peer_name, domain->name,
        SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DOWN, NULL, reason_text);
    sm_service_domain_utils_service_domain_active_self(domain->name);
}
// ****************************************************************************

// ****************************************************************************
// Failover - active self
// =======================
SmErrorT sm_failover_activate_self()
{
    DPRINTFI("Uncontrolled swact start (active local only)");
    SmNodeSwactMonitor::SwactStart(SM_NODE_STATE_ACTIVE);
    sm_service_domain_table_foreach( NULL, active_self_callback);

    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Failover - get node
// =======================
SmErrorT sm_failover_get_node(char* node_name, SmDbNodeT& node)
{
    char query[SM_DB_QUERY_STATEMENT_MAX_CHAR] = {0};
    snprintf(query, sizeof(query), "%s == '%s'", SM_NODES_TABLE_COLUMN_NAME, node_name);
    SmErrorT error = sm_db_nodes_query(_sm_db_handle, query, &node);
    return error;
}
// ****************************************************************************

// ****************************************************************************
// Failover - get node operational state
// =======================
SmErrorT sm_failover_get_node_oper_state(char* node_name, SmNodeOperationalStateT *state)
{
    SmDbNodeT node;
    SmErrorT error = sm_failover_get_node(node_name, node);
    if( SM_OKAY == error )
    {
        *state = node.oper_state;
    }
    else if( SM_NOT_FOUND != error )
    {
        DPRINTFE("Failed to read node table. %s", sm_error_str(error));
    }

    return error;
}
// ****************************************************************************

// ****************************************************************************
// Failover - is node enabled
// =======================
bool _is_node_enabled(char* node_name)
{
    SmNodeOperationalStateT state;
    if (SM_OKAY == sm_failover_get_node_oper_state(node_name, &state))
    {
        return state == SM_NODE_OPERATIONAL_STATE_ENABLED;
    }
    return false;
}
// ****************************************************************************

// ****************************************************************************
// Failover - is peer enabled
// =======================
bool peer_controller_enabled()
{
    return _is_node_enabled(_peer_name);
}
// ****************************************************************************

// ****************************************************************************
// Failover - is this controller enabled
// =======================
bool this_controller_enabled()
{
    return _is_node_enabled(_host_name);
}
// ****************************************************************************

// ****************************************************************************
// Failover - get node admin state
// =======================
SmErrorT sm_failover_get_node_admin_state(char* node_name, SmNodeAdminStateT *admin_state)
{
    char query[SM_DB_QUERY_STATEMENT_MAX_CHAR] = {0};
    SmDbNodeT node;
    snprintf(query, sizeof(query), "%s == '%s'", SM_NODES_TABLE_COLUMN_NAME, node_name);
    SmErrorT error = sm_db_nodes_query(_sm_db_handle, query, &node);
    if( SM_OKAY == error )
    {
        *admin_state = node.admin_state;
    }
    else if( SM_NOT_FOUND != error )
    {
        DPRINTFE("Failed to read node table. %s", sm_error_str(error));
    }

    return error;

}
// ****************************************************************************

// ****************************************************************************
// Failover - is node unlocked
// =======================
bool _is_node_unlocked(char* node_name)
{
    SmNodeAdminStateT admin_state;
    if (SM_OKAY == sm_failover_get_node_admin_state(node_name, &admin_state))
    {
        return admin_state == SM_NODE_ADMIN_STATE_UNLOCKED;
    }
    return false;
}
// ****************************************************************************

// ****************************************************************************
// Failover - is peer unlocked
// =======================
bool peer_controller_unlocked()
{
    return _is_node_unlocked(_peer_name);
}
// ****************************************************************************

// ****************************************************************************
// Failover - is this controller unlocked
// =======================
bool this_controller_unlocked()
{
    return _is_node_unlocked(_host_name);
}
// ****************************************************************************

static SmErrorT sm_ensure_leader_scheduler()
{
    char controller_domain[] = "controller";
    char reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "Loss of heartbeat";

    SmErrorT error = sm_service_domain_fsm_set_state(
        controller_domain,
        SM_SERVICE_DOMAIN_STATE_LEADER,
        reason_text );
    if(SM_OKAY != error)
    {
        DPRINTFE("Failed to ensure leader scheduler. Error %s", sm_error_str(error));
    }
    return error;
}
// ****************************************************************************
// Failover - set system to scheduled status
// ==================
SmErrorT sm_failover_set_system(const SmSystemFailoverStatus& failover_status)
{
    //valid combinations of target system scheduling are:
    // active/standby
    // active/failed
    SmFailoverActionResultT result;
    SmNodeScheduleStateT host_target_state, peer_target_state;
    host_target_state = failover_status.get_host_schedule_state();
    peer_target_state = failover_status.get_peer_schedule_state();
    SmHeartbeatStateT heartbeat_state = failover_status.get_heartbeat_state();
    if(SM_HEARTBEAT_OK != heartbeat_state)
    {
        if(SM_OKAY != sm_ensure_leader_scheduler())
        {
            DPRINTFE("Failed to set new leader scheduler to local");
            return SM_FAILED;
        }
    }

    if(SM_NODE_STATE_ACTIVE == host_target_state)
    {
        if(SM_NODE_STATE_STANDBY == _host_state &&
            SM_NODE_STATE_FAILED == peer_target_state)
        {
            if(SM_OKAY != sm_failover_activate_self())
            {
                DPRINTFE("Failed to activate %s.", _host_name);
                return SM_FAILED;
            }
            if(SM_OKAY != sm_failover_disable_node(_peer_name))
            {
                DPRINTFE("Failed to disable node %s.", _peer_name);
                return SM_FAILED;
            }
        }
    }else if(SM_NODE_STATE_STANDBY == host_target_state)
    {
        if(SM_NODE_STATE_ACTIVE == _host_state)
        {
            result = sm_failover_swact();
            if(SM_FAILOVER_ACTION_RESULT_FAILED == result)
            {
                DPRINTFE("Failed to swact.");
                return SM_FAILED;
            }
        }
    }
    else if(SM_NODE_STATE_FAILED == host_target_state)
    {
        if(SM_OKAY != sm_failover_fail_self())
        {
            DPRINTFE("Failed disable host %s.", _host_name);
            return SM_FAILED;
        }
    }
    else
    {
        DPRINTFE("Runtime error. Unexpected scheduling state %s for host %s",
            sm_node_schedule_state_str(host_target_state),
            _host_name);
        return SM_FAILED;
    }

    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Failover - audit
// =======================
void sm_failover_audit()
{
    mutex_holder holder(&_mutex);

    timespec now;
    time_t now_ms;
    clock_gettime( CLOCK_MONOTONIC_RAW, &now );
    now_ms = now.tv_sec * 1000 + now.tv_nsec / 1000000;
    _host_state = get_controller_state();
    if( SM_NODE_STATE_INIT == _host_state ||
        SM_NODE_STATE_UNKNOWN == _host_state )
    {
        if ( _prev_host_state != _host_state )
        {
            DPRINTFD("Wait for scheduler to decided my role. host state = %d", _host_state);
            _prev_host_state = _host_state;
        }
        return;
    }

    if ( _prev_host_state != _host_state )
    {
        DPRINTFI("host state is %d", _host_state);
        _prev_host_state = _host_state;
    }

    bool in_transition = false;
    bool infra_configured = is_infra_configured();
    in_transition = in_transition &&
            sm_failover_if_transit_state(_mgmt_interface_info);
    in_transition = in_transition &&
            sm_failover_if_transit_state(_oam_interface_info);
    if( infra_configured )
    {
        in_transition = in_transition &&
            sm_failover_if_transit_state(_infra_interface_info);
    }

    if(in_transition)
    {
        //if state in transition, wait for next audit
        return;
    }

    int if_state_flag = sm_failover_get_if_state();
    if(if_state_flag & SM_FAILOVER_HEARTBEAT_ALIVE)
    {
        _heartbeat_count ++;
    }
    else
    {
        _heartbeat_count = 0;
    }
    if( _prev_if_state_flag != if_state_flag)
    {
        _last_report_ts = now_ms;
        DPRINTFI("Interface state flag %d", if_state_flag);

        if( SM_FAILOVER_MULTI_FAILURE_WAIT_TIMER_IN_MS > now_ms - _last_if_state_ms )
        {
            DPRINTFD("interface state just changed. wait %d ms for concurrent changes",
                SM_FAILOVER_MULTI_FAILURE_WAIT_TIMER_IN_MS);
            return;
        }else
        {
            _last_if_state_ms = now_ms;
            _prev_if_state_flag = if_state_flag;
        }
    }

    if(now_ms - _last_report_ts > SM_FAILOVER_INTERFACE_STATE_REPORT_INTERVAL_MS)
    {
        _last_report_ts = now_ms;
        DPRINTFD("Interface state flag %d", if_state_flag);
    }

    if(!peer_controller_enabled())
    {
        _recheck = true;
        if( 1 != _heartbeat_count )
        {
            return;
        }
    }
    if (!this_controller_enabled() || !this_controller_unlocked())
    {
        _recheck = true;
        DPRINTFD("This controller isn't unlocked, no action is taken.");
        return;
    }

    if(_if_state_changed)
    {
        SmIFStateChangedEventData event_data;
        event_data.set_interface_state(SM_INTERFACE_OAM, _oam_interface_info->get_state());
        event_data.set_interface_state(SM_INTERFACE_MGMT, _mgmt_interface_info->get_state());
        if( NULL != _infra_interface_info)
        {
            event_data.set_interface_state(SM_INTERFACE_INFRA, _infra_interface_info->get_state());
        }else
        {
            event_data.set_interface_state(SM_INTERFACE_INFRA, SM_FAILOVER_INTERFACE_UNKNOWN);
        }
        SmFailoverFSM::get_fsm().send_event(SM_FAILOVER_EVENT_IF_STATE_CHANGED, &event_data);
        _if_state_changed = false;
    }

    int64_t curr_node_state = if_state_flag;

    if( _hello_msg_alive )
    {
        curr_node_state = curr_node_state | SM_FAILOVER_HELLO_MSG_ALIVE;
    }

    if( !_retry && !_recheck &&
         ( _node_comm_state == curr_node_state &&
            _host_state == _host_state_at_last_action &&
            _peer_if_state == _peer_if_state_at_last_action ))
    {
        return;
    }

    _recheck = false;
    _retry = false;

    _node_comm_state = curr_node_state;
    _host_state_at_last_action = _host_state;
    _peer_if_state_at_last_action = _peer_if_state;

    _log_nodes_state();
}
// ****************************************************************************

// ****************************************************************************
// Failover - audit timeout callback
// =======================
static bool sm_failover_audit_timeout(SmTimerIdT timer_id,
    int64_t user_data )
{
    sm_failover_audit();
    return true;
}
// ****************************************************************************

// ****************************************************************************
// Failover - exec mtce command
// =======================
SmErrorT sm_exec_mtce_command(const char cmd[], char result_buf[], int result_len)
{
    FILE* fp;
    DPRINTFD("Executing command:\n%s\n", cmd);
    fp = popen(cmd, "r");
    if(NULL == fp)
    {
        DPRINTFE("Failed to run command \n%s\n", cmd);
        return SM_FAILED;
    }

    SmErrorT result = SM_FAILED;
    if( NULL != fgets(result_buf, result_len, fp) )
    {
        struct json_object *raw_obj = json_tokener_parse( result_buf );
        if( !raw_obj )
        {
            DPRINTFE("Mtce api returns invalid result. [%s]", result_buf);
            result = SM_FAILED;
        }
        else
        {
            json_object_object_foreach(raw_obj, key, val)
            {
                const char status_key[] = "status";
                if( 0 == strncmp(status_key, key, sizeof(status_key)) )
                {
                    const char* val_str = json_object_get_string(val);
                    const char pass_val[] = "pass";
                    if( 0 == strncmp(pass_val, val_str, sizeof(pass_val)))
                    {
                        DPRINTFI("Mtce api completed successfully.");
                        result = SM_OKAY;
                    }
                }
            }
            if(result != SM_OKAY)
            {
                const char* json_string = json_object_get_string(raw_obj);
                DPRINTFE("Mtce api return error. [%s]", json_string);
            }
        }
    }
    else
    {
        DPRINTFE("Mtce api not available, will retry in next cycle.");
    }
    fclose(fp);
    return result;
}
// ****************************************************************************

// ****************************************************************************
// Failover - log current state
// =======================
void _log_nodes_state()
{
    SmDbNodeT host, peer;
    SmErrorT error = sm_failover_get_node(_host_name, host);
    SmErrorT error2 = sm_failover_get_node(_peer_name, peer);
    if (SM_OKAY == error && SM_OKAY == error2)
    {
        DPRINTFI("%s %s-%s-%s, %s %s-%s-%s",
            _host_name,
            sm_node_admin_state_str(host.admin_state),
            sm_node_oper_state_str(host.oper_state),
            sm_node_avail_status_str(host.avail_status),
            _peer_name,
            sm_node_admin_state_str(peer.admin_state),
            sm_node_oper_state_str(peer.oper_state),
            sm_node_avail_status_str(peer.avail_status)
        );
    }
    DPRINTFI("Host state %d, I/F state %d, peer I/F state %d",
            _node_comm_state,
            _host_state,
            _peer_if_state
        );
}
// ****************************************************************************


// ****************************************************************************
// Failover - disable peer node
// =======================
SmErrorT sm_failover_disable_peer()
{
    char result_buf[1024];
    char cmd_buf[1024];
    snprintf(cmd_buf, sizeof(cmd_buf), "curl --header \"Content-Type:application/json\"  "
        "--header \"Accept: application/json\" --header \"User-Agent: sm/1.0\" "
        "--data '{\"action\": \"event\", \"hostname\":\"%s\", \"operational\": \"disabled\", \"availability\": \"failed\"}'  "
        "\"http://localhost:2112/v1/hosts/%s/\"", _peer_name, _peer_name);

    SmErrorT error = sm_exec_mtce_command(cmd_buf, result_buf, sizeof(result_buf));
    if (SM_OKAY != error )
    {
        DPRINTFE("Failed to disable peer %s", _peer_name);
        return SM_FAILED;
    }
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Failover - get node interface info
// ==================
SmFailoverInterfaceStateT sm_failover_get_interface_info(SmInterfaceTypeT interface_type)
{
    SmFailoverInterfaceInfo* res = NULL;
    switch(interface_type)
    {
        case SM_INTERFACE_MGMT:
            res = _mgmt_interface_info;
            break;
        case SM_INTERFACE_INFRA:
            res = _infra_interface_info;
            break;
        case SM_INTERFACE_OAM:
            res = _oam_interface_info;
            break;
        case SM_INTERFACE_UNKNOWN:
            break;
    }
    if(NULL != res)
    {
        return res->get_state();
    }
    return SM_FAILOVER_INTERFACE_UNKNOWN;
}
// ****************************************************************************

static void sm_failover_interface_change_callback(
    SmHwInterfaceChangeDataT* if_change )
{
    switch ( if_change->interface_state )
    {
        case SM_INTERFACE_STATE_DISABLED:
            DPRINTFI("Interface %s is down", if_change->interface_name);
            sm_failover_interface_down(if_change->interface_name);
            break;
        case SM_INTERFACE_STATE_ENABLED:
            DPRINTFI("Interface %s is up", if_change->interface_name);
            sm_failover_interface_up(if_change->interface_name);
            break;
        default:
            DPRINTFI("Interface %s state changed to %d",
                if_change->interface_name, if_change->interface_state);
            break;
    }
}

// ****************************************************************************
// Failover - Initialize
// ======================
SmErrorT sm_failover_initialize( void )
{
    void* user_data[1] = {&_total_interfaces};
    bool enabled;
    SmErrorT error;

    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int res = pthread_mutex_init(&_mutex, &attr);
    if( 0 != res )
    {
        DPRINTFE("Failed to initialize mutex, error %d", res);
        return SM_FAILED;
    }

    SmHwCallbacksT callbacks;
    memset( &callbacks, 0, sizeof(callbacks) );
    callbacks.interface_change = sm_failover_interface_change_callback;
    error = sm_hw_initialize( &callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize hardware module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = SmFailoverFSM::initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize failover FSM, error %s.", sm_error_str( error ) );
        return error;
    }

    _system_mode = sm_node_utils_get_system_mode();
    DPRINTFI("System mode %s", sm_system_mode_str(_system_mode));

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_node_utils_get_hostname( _host_name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return error;
    }

    if( 0 == strcmp( SM_NODE_CONTROLLER_0_NAME, _host_name ) )
    {
        snprintf( _peer_name, sizeof(_peer_name), "%s", SM_NODE_CONTROLLER_1_NAME );
    }else if ( 0 == strcmp( SM_NODE_CONTROLLER_1_NAME, _host_name ) )
    {
        snprintf( _peer_name, sizeof(_peer_name), "%s", SM_NODE_CONTROLLER_0_NAME );
    }else
    {
        DPRINTFE( "Unknown host name (not one of [%s, %s]",
            SM_NODE_CONTROLLER_0_NAME, SM_NODE_CONTROLLER_1_NAME);
        return SM_FAILED;
    }

    _total_interfaces = 0;
    sm_service_domain_interface_table_foreach(
        user_data,
        sm_failover_interface_check
    );

    if ( _oam_interface_info == NULL || _mgmt_interface_info == NULL )
    {
        DPRINTFE( "%s and %s must be configured.",
            SM_SERVICE_DOMAIN_MGMT_INTERFACE, SM_SERVICE_DOMAIN_OAM_INTERFACE );
        return SM_FAILED;
    }

    _end_of_list = _my_if_list + _total_interfaces;
    DPRINTFI("Total domain interfaces %d", _total_interfaces);
    SmFailoverInterfaceInfo *iter;
    int i = 0;
    for( iter = _my_if_list; iter < _end_of_list; iter ++)
    {
        const SmServiceDomainInterfaceT* interface = iter->get_interface();

        DPRINTFI( "interface[%d]: %s %s", i, interface->service_domain_interface, interface->interface_name );
        i ++;
    }

    for( iter = _my_if_list; iter < _end_of_list; iter ++)
    {
        const SmServiceDomainInterfaceT* interface = iter->get_interface();
        error = sm_hw_get_if_state(interface->interface_name, &enabled);
        if(SM_OKAY != error)
        {
           DPRINTFE("Couldn't get interface (%s) state. ", interface->interface_name);
        }
        else
        {
           if(enabled)
           {
               if(iter->set_state(SM_FAILOVER_INTERFACE_OK))
               {
                   DPRINTFI("Interface %d [%s, %s] is UP",
                       interface->id,
                       interface->service_domain_interface,
                       sm_path_type_str(interface->path_type));
               }
           }
           else
           {
               if(iter->set_state(SM_FAILOVER_INTERFACE_DOWN))
               {
                   DPRINTFE("Interface %d [%s, %s] is DOWN",
                       interface->id,
                       interface->service_domain_interface,
                       sm_path_type_str(interface->path_type));
               }
           }
        }
    }

    error = sm_timer_register( "failover audit", 50,
           sm_failover_audit_timeout,
           0, &failover_audit_timer_id );

    if(SM_OKAY != error)
    {
        DPRINTFE("Failed to register failover audit timer, error %s",
            sm_error_str(error));
        return SM_FAILED;
    }

    error = SmClusterHbsInfoMsg::initialize();
    if(SM_OKAY != error)
    {
        DPRINTFE("Failed to initialize cluster hbs info messaging");
    }

    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Failover - Finalize
// ====================
SmErrorT sm_failover_finalize( void )
{
    _total_interfaces = 0;

    SmErrorT error;
    error = SmClusterHbsInfoMsg::finalize();
    if(SM_OKAY != error)
    {
        DPRINTFE("Failed to finalize cluster hbs info messaging");
    }

    sm_timer_deregister( failover_audit_timer_id );
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

    error = SmFailoverFSM::finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize failover FSM, error %s.", sm_error_str( error ) );
        return error;
    }

    pthread_mutex_destroy(&_mutex);
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
/* code below is for debugging only. There is no real feature/function
*******************************************************************************/
#define MAX_MAPPING_STR_LEN 40
typedef struct
{
    int value;
    char str[MAX_MAPPING_STR_LEN];
} KeyStringMapT;

static const char* get_map_str( KeyStringMapT mappings[],
    unsigned int num_mappings, int value )
{
    unsigned int map_i;
    for( map_i=0; num_mappings > map_i; ++map_i )
    {
        if( value == mappings[map_i].value )
        {
            return( &(mappings[map_i].str[0]) );
        }
    }

    return( "???" );
}

void dump_host_state(FILE* fp)
{
    const char* state = sm_node_schedule_state_str(_host_state);
    fprintf(fp, "           host state:   %s\n", state);
}

static KeyStringMapT if_state_map[] = {
    {SM_FAILOVER_INTERFACE_UNKNOWN, "Unknown"},
    {SM_NODE_STATE_ACTIVE, "Active"},
    {SM_FAILOVER_INTERFACE_MISSING_HEARTBEAT, "Missing heartbeat"},
    {SM_FAILOVER_INTERFACE_DOWN, "Down"},
    {SM_FAILOVER_INTERFACE_RECOVERING, "Recovering" }
};

void dump_if_state(FILE* fp, SmFailoverInterfaceInfo* interface, const char* if_name )
{
    const char* state;
    if(interface)
    {
        state = get_map_str( if_state_map,
                                sizeof(if_state_map) / sizeof(KeyStringMapT),
                                interface->get_state() );
    }
    else
    {
        state = "Not configured";
    }
    fprintf(fp, "      %s Interface:   %s\n", if_name, state);
}

void dump_interfaces_state(FILE* fp)
{
    dump_if_state(fp, _oam_interface_info,   "  OAM");
    dump_if_state(fp, _mgmt_interface_info,  " MGMT");
    dump_if_state(fp, _infra_interface_info, "INFRA");
}

void dump_peer_if_state(FILE* fp)
{
    fprintf(fp, " Peer Interface state:   %d\n", _peer_if_state);
}

void dump_failover_fsm_state(FILE* fp)
{
    SmFailoverStateT state = SmFailoverFSM::get_fsm().get_state();
    fprintf(fp, "   Failover FSM state:   %s\n", sm_failover_state_str(state));
}

// ****************************************************************************
// Failover - dump state
// ======================
void sm_failover_dump_state(FILE* fp)
{
    fprintf( fp, "Failover \n\n" );
    dump_host_state(fp);
    dump_failover_fsm_state(fp);
    dump_interfaces_state(fp);
    dump_peer_if_state(fp);
}
// ****************************************************************************
