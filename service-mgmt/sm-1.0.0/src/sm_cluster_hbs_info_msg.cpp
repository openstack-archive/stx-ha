//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_cluster_hbs_info_msg.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "sm_configuration_table.h"
#include "sm_debug.h"
#include "sm_limits.h"
#include "sm_selobj.h"
#include "sm_worker_thread.h"

// uncomment when debugging this module to enabled DPRINTFD output to log file
// #define __DEBUG__MSG__

#ifdef __DEBUG__MSG__
#undef DPRINTFD
#define DPRINTFD DPRINTFI
#endif

#define LOOPBACK_IP "127.0.0.1"
#define SM_CLIENT_PORT_KEY "sm_client_port"
#define SM_SERVER_PORT_KEY "sm_server_port"
const char json_fmt[] = "{\"origin\":\"sm\",\"service\":\"heartbeat\",\"request\":\"cluster_info\",\"reqid\":\"%d\"}";
const int request_size = sizeof(json_fmt) + 10;

static const unsigned int size_of_msg_header =
                            sizeof(mtce_hbs_cluster_type)
                            - sizeof(mtce_hbs_cluster_history_type) * MTCE_HBS_MAX_HISTORY_ELEMENTS;

bool operator==(const SmClusterHbsInfoT& lhs, const SmClusterHbsInfoT& rhs)
{
    return lhs.storage0_responding == rhs.storage0_responding &&
           lhs.number_of_node_reachable == rhs.number_of_node_reachable;
}

bool operator!=(const SmClusterHbsInfoT& lhs, const SmClusterHbsInfoT& rhs)
{
    return !(lhs == rhs);
}

bool operator==(const SmClusterHbsStateT& lhs, const SmClusterHbsStateT& rhs)
{
    if(lhs.storage0_enabled != rhs.storage0_enabled)
        return false;

    for(unsigned int i = 0; i < max_controllers; i ++)
    {
        if(lhs.controllers[i] != rhs.controllers[i])
        {
            return false;
        }
    }
    return true;
}

bool operator!=(const SmClusterHbsStateT& lhs, const SmClusterHbsStateT& rhs)
{
    return !(lhs == rhs);
}

void log_cluster_hbs_state(const SmClusterHbsStateT& state)
{
    if(0 == state.last_update)
    {
        DPRINTFI("Cluster hbs state not available");
        return;
    }

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int secs_since_update = ts.tv_sec - state.last_update;

    if(state.storage0_enabled)
    {
        DPRINTFI("Cluster hbs last updated %d secs ago, storage-0 is provisioned, "
                 "from controller-0: %d nodes enabled, %d nodes reachable, storage-0 %s responding "
                 "from controller-1: %d nodes enabled, %d nodes reachable, storage-0 %s responding",
                 secs_since_update,
                 state.controllers[0].number_of_node_enabled,
                 state.controllers[0].number_of_node_reachable,
                 state.controllers[0].storage0_responding ? "is" : "is not",
                 state.controllers[1].number_of_node_enabled,
                 state.controllers[1].number_of_node_reachable,
                 state.controllers[1].storage0_responding ? "is" : "is not"
                );
    }else
    {
        DPRINTFI("Cluster hbs last updated %d secs ago, storage-0 is not provisioned, "
                 "from controller-0: %d nodes enabled, %d nodes reachable, "
                 "from controller-1: %d nodes enabled, %d nodes reachable",
                 secs_since_update,
                 state.controllers[0].number_of_node_enabled,
                 state.controllers[0].number_of_node_reachable,
                 state.controllers[1].number_of_node_enabled,
                 state.controllers[1].number_of_node_reachable
                );
    }
}

pthread_mutex_t SmClusterHbsInfoMsg::_mutex;
const unsigned short Invalid_Req_Id = 0;
int SmClusterHbsInfoMsg::_sock = -1;
SmClusterHbsStateT SmClusterHbsInfoMsg::_cluster_hbs_state_current;
SmClusterHbsStateT SmClusterHbsInfoMsg::_cluster_hbs_state_previous;
SmClusterHbsInfoMsg::hbs_query_respond_callback SmClusterHbsInfoMsg::_callbacks;

const SmClusterHbsStateT& SmClusterHbsInfoMsg::get_current_state()
{
    return _cluster_hbs_state_current;
}

const SmClusterHbsStateT& SmClusterHbsInfoMsg::get_previous_state()
{
    return _cluster_hbs_state_previous;
}

bool SmClusterHbsInfoMsg::_process_cluster_hbs_history(mtce_hbs_cluster_history_type history, SmClusterHbsStateT& state)
{
    if(history.controller >= max_controllers)
    {
        DPRINTFE("Invalid controller id %d", history.controller);
        return false;
    }
    if(MTCE_HBS_NETWORKS <= history.network)
    {
        DPRINTFE("Invalid network id %d", history.network);
        return false;
    }
    if(MTCE_HBS_HISTORY_ENTRIES < history.entries)
    {
        DPRINTFE("Invalid entries %d", history.entries);
        return false;
    }
    if(MTCE_HBS_HISTORY_ENTRIES < history.oldest_entry_index)
    {
        DPRINTFE("Invalid oldest entry index %d", history.oldest_entry_index);
        return false;
    }

    int newest_entry_index = (history.oldest_entry_index + history.entries - 1) % MTCE_HBS_HISTORY_ENTRIES;
    mtce_hbs_cluster_entry_type& entry = history.entry[newest_entry_index];

    SmClusterHbsInfoT& controller_state = state.controllers[history.controller];
    controller_state.storage0_responding = history.storage0_responding;
    if(entry.hosts_responding > controller_state.number_of_node_reachable)
    {
        controller_state.number_of_node_reachable = entry.hosts_responding;
        controller_state.number_of_node_enabled = entry.hosts_enabled;
    }

    DPRINTFD("Oldest index %d, entries %d, newest index %d, nodes %d",
        history.oldest_entry_index, history.entries, newest_entry_index, entry.hosts_responding);
    return true;
}

void SmClusterHbsInfoMsg::_cluster_hbs_info_msg_received( int selobj, int64_t user_data )
{
    mtce_hbs_cluster_type msg = {0};
    mutex_holder holder(&_mutex);
    while(true)
    {
        int bytes_read = recv( selobj, &msg, sizeof(msg), MSG_NOSIGNAL | MSG_DONTWAIT );
        if(bytes_read < 0)
        {
            if(EAGAIN != errno)
            {
                DPRINTFE("Failed to read socket. error %s", strerror(errno));
            }
            return;
        }
        DPRINTFD("msg received %d bytes. buffer size %d", bytes_read, sizeof(msg));
        if(size_of_msg_header > (unsigned int)bytes_read)
        {
            DPRINTFE("size not right, msg size %d, expected not less than %d",
                bytes_read, size_of_msg_header);
            return;
        }

        DPRINTFD("msg version %d, revision %d, size %d, reqid %d",
                  msg.version, msg.revision, msg.bytes, msg.reqid);
        DPRINTFD("period %d number of rec %d", msg.period_msec, msg.histories);

        SmClusterHbsStateT state;
        if(msg.histories > 0)
        {
            int expected_size = sizeof(mtce_hbs_cluster_history_type) * msg.histories
                                + size_of_msg_header;
            if(bytes_read != expected_size)
            {
                DPRINTFE("Received size %d not matching %d expected", bytes_read, expected_size);
                return;
            }
            for(int i = 0; i < msg.histories; i ++)
            {
                if(!_process_cluster_hbs_history(msg.history[i], state))
                {
                    return;
                }
            }
        }else
        {
            DPRINTFD("No rbs cluster info history data is received");
        }

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        state.last_update = ts.tv_sec;
        state.storage0_enabled = (bool)msg.storage0_enabled;
        if(state != _cluster_hbs_state_current)
        {
            _cluster_hbs_state_previous = _cluster_hbs_state_current;
            _cluster_hbs_state_current = state;
            DPRINTFD("cluster hbs state changed");
            log_cluster_hbs_state(_cluster_hbs_state_current);
        }
        else
        {
            DPRINTFD("cluster hbs state unchanged");
        }

        while(!_callbacks.empty())
        {
            cluster_hbs_query_ready_callback callback = _callbacks.front();
            _callbacks.pop_front();
            callback();
        }
    }
}

SmErrorT SmClusterHbsInfoMsg::_get_address(const char* port_key, struct sockaddr_in* addr)
{
    struct addrinfo *address = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;          // IPv4 only
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    char port[SM_CONFIGURATION_VALUE_MAX_CHAR + 1];
    if( SM_OKAY != sm_configuration_table_get(port_key, port, sizeof(port) - 1) )
    {
        DPRINTFE("Runtime error: system configuration %s undefined", port_key);
        return SM_FAILED;
    }

    int result = getaddrinfo(LOOPBACK_IP, port, &hints, &address);
    if(result != 0)
    {
        DPRINTFE("Failed to get addrinfo %s:%s", LOOPBACK_IP, port);
        return SM_FAILED;
    }

    memcpy(addr, address->ai_addr, sizeof(struct sockaddr_in));
    freeaddrinfo(address);
    return SM_OKAY;
}

static void send_query(SmSimpleAction&)
{
    SmClusterHbsInfoMsg::cluster_hbs_info_query();
}

static SmSimpleAction _query_hbs_cluster_info_action("send hbs-cluster query", send_query);

// ****************************************************************************
// SmClusterHbsInfoMsg::cluster_hbs_info_query -
//      trigger a query of cluster hbs info.
//      return true if request sent successfully, false otherwise.
// ========================
bool SmClusterHbsInfoMsg::cluster_hbs_info_query(cluster_hbs_query_ready_callback callback)
{
    char server_port[SM_CONFIGURATION_VALUE_MAX_CHAR + 1];
    if( SM_OKAY != sm_configuration_table_get(SM_SERVER_PORT_KEY, server_port, sizeof(server_port) - 1) )
    {
        DPRINTFE("Runtime error: system configuration %s undefined", SM_SERVER_PORT_KEY);
        return false;
    }

    int port = atoi(server_port);
    if(0 > port)
    {
        DPRINTFE("Runtime error: Invalid configuration %s: %s", SM_SERVER_PORT_KEY, server_port);
        return false;
    }

    char query[request_size];
    unsigned short reqid;
    struct timespec ts;
    {
        mutex_holder holder(&_mutex);
        if(0 != clock_gettime(CLOCK_REALTIME, &ts))
        {
            DPRINTFE("Failed to get realtime");
            reqid = (unsigned short)1;
        }else
        {
            unsigned short* v = (unsigned short*)(&ts.tv_nsec);
            reqid = (*v) % 0xFFFE + 1;
        }

        struct sockaddr_in addr;
        if(SM_OKAY != _get_address(SM_SERVER_PORT_KEY, &addr))
        {
            DPRINTFE("Failed to get address");
            return false;
        }

        int msg_size = snprintf(query, sizeof(query), json_fmt, reqid);

        DPRINTFD("send %d bytes %s", msg_size, query);
        if(0 > sendto(_sock, query, msg_size, 0, (sockaddr*)&addr, sizeof(addr)))
        {
            DPRINTFE("Failed to send msg. Error %s", strerror(errno));
            return false;
        }
        if(NULL != callback)
        {
            _callbacks.push_back(callback);
        }
    }
    return true;
}

SmErrorT SmClusterHbsInfoMsg::open_socket()
{
    struct addrinfo *address = NULL;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;          // IPv4 only
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    struct sockaddr_in addr;

    char client_port[SM_CONFIGURATION_VALUE_MAX_CHAR + 1];
    char server_port[SM_CONFIGURATION_VALUE_MAX_CHAR + 1];
    if( SM_OKAY != sm_configuration_table_get(SM_CLIENT_PORT_KEY, client_port, sizeof(client_port) - 1) )
    {
        DPRINTFE("Runtime error: system configuration %s undefined", SM_CLIENT_PORT_KEY);
        return SM_FAILED;
    }

    if( SM_OKAY != sm_configuration_table_get(SM_SERVER_PORT_KEY, server_port, sizeof(server_port) - 1) )
    {
        DPRINTFE("Runtime error: system configuration %s undefined", SM_SERVER_PORT_KEY);
        return SM_FAILED;
    }else
    {
        int port = atoi(server_port);
        if(0 > port)
        {
            DPRINTFE("Invalid configuration %s: %s", SM_SERVER_PORT_KEY, server_port);
            return SM_FAILED;
        }
    }

    int result = getaddrinfo(LOOPBACK_IP, client_port, &hints, &address);
    if(result != 0)
    {
        DPRINTFE("Failed to get addrinfo %s:%s", LOOPBACK_IP, client_port);
        return SM_FAILED;
    }

    memcpy(&addr, address->ai_addr, sizeof(addr));
    freeaddrinfo(address);
    address = NULL;
    int sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( 0 > sock )
    {
        DPRINTFE("Failed to create sock. Error %s", strerror(errno));
        return SM_FAILED;
    }

    int flags = fcntl( sock, F_GETFL, 0 );
    if( 0 > flags )
    {
        DPRINTFE("Failed to get flags, error=%s.", strerror(errno));
        close( sock );
        return SM_FAILED;
    }

    if( 0 > fcntl( sock, F_SETFL, flags | O_NONBLOCK ) )
    {
        DPRINTFE("Failed to set flags, error=%s.", strerror(errno));
        close( sock );
        return SM_FAILED;
    }

    result = bind( sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_in));
    if(0 > result)
    {
        DPRINTFE("Failed to bind. error=%s", strerror( errno));
        close( sock );
        return SM_FAILED;
    }

    SmErrorT error = sm_selobj_register(sock, _cluster_hbs_info_msg_received, 0);
    if(SM_OKAY != error)
    {
        DPRINTFE("Failed to register selobj");
        close( sock );
        return SM_FAILED;
    }

    _sock = sock;
    return SM_OKAY;
}

SmErrorT SmClusterHbsInfoMsg::initialize()
{
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

    error = open_socket();
    if(SM_OKAY != error)
    {
        DPRINTFE("Failed to open sock");
        return SM_FAILED;
    }

    SmWorkerThread::get_worker().add_action(&_query_hbs_cluster_info_action);
    return SM_OKAY;
}

SmErrorT SmClusterHbsInfoMsg::finalize()
{
    mutex_holder holder(&_mutex);
    if(_sock > 0)
    {
        close(_sock);
        _sock = -1;
    }
    pthread_mutex_destroy(&_mutex);
    return SM_OKAY;
}

void SmClusterHbsInfoMsg::dump_hbs_record(FILE* fp)
{
    struct timespec ts;
    time_t t;
    clock_gettime(CLOCK_REALTIME, &ts);
    t = ts.tv_sec - _cluster_hbs_state_current.last_update;
    fprintf(fp, "\ncluster hbs info\n");
    if(0 == _cluster_hbs_state_current.last_update)
    {
        fprintf(fp, "  Current state, no data received yet\n");
    }else
    {
        fprintf(fp, "  Current state, last updated %d seconds ago\n", (int)t);

        fprintf(fp, "  storage-0 is %s configured\n", _cluster_hbs_state_current.storage0_enabled ? "" : "not");
        fprintf(fp, "  From controller-0\n");
        if(_cluster_hbs_state_current.storage0_enabled)
        {
            fprintf(fp, "    storage-0 is %s responding\n", _cluster_hbs_state_current.controllers[0].storage0_responding ? "" : "not");
        }
        fprintf(fp, "    %d nodes are responding\n", _cluster_hbs_state_current.controllers[0].number_of_node_reachable);
        fprintf(fp, "  From controller-1\n");
        if(_cluster_hbs_state_current.storage0_enabled)
        {
            fprintf(fp, "    storage-0 is %s responding\n", _cluster_hbs_state_current.controllers[1].storage0_responding ? "" : "not");
        }
        fprintf(fp, "    %d nodes are responding\n", _cluster_hbs_state_current.controllers[1].number_of_node_reachable);
    }

    if(0 != _cluster_hbs_state_previous.last_update)
    {
        t = ts.tv_sec - _cluster_hbs_state_previous.last_update;
        fprintf(fp, "\n  Previous state, since %d seconds ago\n", (int)t);

        fprintf(fp, "  storage-0 is %s configured\n", _cluster_hbs_state_previous.storage0_enabled ? "" : "not");
        fprintf(fp, "  From controller-0\n");
        if(_cluster_hbs_state_previous.storage0_enabled)
        {
            fprintf(fp, "    storage-0 is %s responding\n", _cluster_hbs_state_previous.controllers[0].storage0_responding ? "" : "not");
        }
        fprintf(fp, "    %d nodes are responding\n", _cluster_hbs_state_previous.controllers[0].number_of_node_reachable);
        fprintf(fp, "  From controller-1\n");
        if(_cluster_hbs_state_previous.storage0_enabled)
        {
            fprintf(fp, "    storage-0 is %s responding\n", _cluster_hbs_state_previous.controllers[1].storage0_responding ? "" : "not");
        }
        fprintf(fp, "    %d nodes are responding\n", _cluster_hbs_state_previous.controllers[1].number_of_node_reachable);
    }
}
