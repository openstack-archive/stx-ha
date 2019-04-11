//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_heartbeat_thread.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sched.h>
#include <pthread.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_list.h"
#include "sm_debug.h"
#include "sm_trap.h"
#include "sm_time.h"
#include "sm_selobj.h"
#include "sm_timer.h"
#include "sm_hw.h"
#include "sm_sha512.h"
#include "sm_thread_health.h"
#include "sm_heartbeat_msg.h"
#include "sm_node_utils.h"
#include "sm_alarm.h"
#include "sm_log.h"
#include "sm_failover.h"

#define SM_HEARTBEAT_THREAD_NAME                                    "sm_heartbeat"
#define SM_HEARTBEAT_THREAD_TICK_INTERVAL_IN_MS                                100
#define SM_HEARTBEAT_THREAD_ALIVE_TIMER_IN_MS                                  100
#define SM_HEARTBEAT_ALARM_DEBOUNCE_IN_MS                                    30000
#define SM_HEARTBEAT_ALARM_THROTTLE_IN_MS                                     5000

typedef enum
{
    SEND_UNICAST    = 1,
    SEND_MULTICAST  = 2
}SmHeartbeatThreadMethod;

typedef struct
{
    int64_t id;
    char service_domain[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char service_domain_interface[SM_SERVICE_DOMAIN_INTERFACE_NAME_MAX_CHAR];
    char interface_name[SM_INTERFACE_NAME_MAX_CHAR];
    SmInterfaceStateT interface_state;
    SmNetworkTypeT network_type;
    SmNetworkAddressT network_multicast;
    SmNetworkAddressT network_address;
    SmNetworkAddressT network_peer_address;
    int network_peer_port;
    int network_port;
    SmAuthTypeT auth_type;
    char auth_key[SM_AUTHENTICATION_KEY_MAX_CHAR];
    int unicast_socket;
    int multicast_socket;
    bool socket_reconfigure;
    int method;
    SmInterfaceTypeT interface_type;
} SmHeartbeatThreadInterfaceT;

typedef struct
{
    int64_t id;
    char interface_name[SM_INTERFACE_NAME_MAX_CHAR];
    SmNetworkAddressT network_address;
    int network_port;
    int dead_interval;
    SmTimeT last_alive_msg;
    SmTimeT alarm_raised_time;
    SmTimeT last_alarm_clear;
    SmTimeT last_alarm_raise;
    SmTimerIdT alarm_timer;
    char log_text[SM_LOG_REASON_TEXT_MAX_CHAR];
} SmHeartbeatThreadPeerInterfaceT;

typedef struct
{
    char node_name[SM_NODE_NAME_MAX_CHAR];
    int version;
    int revision;
    SmTimeT last_alive_msg;
} SmHeartbeatThreadPeerNodeT;

static sig_atomic_t _stay_on;
static bool _thread_created = false;
static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t _heartbeat_thread;
static sig_atomic_t _messaging_enabled = 0;
static sig_atomic_t _heartbeat_required = 1;
static SmListT* _heartbeat_interfaces = NULL;
static SmListT* _heartbeat_peer_interfaces = NULL;
static SmListT* _heartbeat_peer_nodes = NULL;
static SmTimerIdT _alive_timer_id = SM_TIMER_ID_INVALID;
static char _node_name[SM_NODE_NAME_MAX_CHAR];
static SmHeartbeatMsgCallbacksT _callbacks;

// ****************************************************************************
// Heartbeat Thread - SmNetworkAddressT ==
// =========================
static bool operator == ( const SmNetworkAddressT& src, const SmNetworkAddressT& dest )
{
    return ( 0 == memcmp( &src, &dest, sizeof(SmNetworkAddressT) ) );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - SmNetworkAddressT !=
// =========================
static bool operator != ( const SmNetworkAddressT& src, const SmNetworkAddressT& dest )
{
    return !( src == dest );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Disable heartbeat
// =========================
void sm_heartbeat_thread_disable_heartbeat( void )
{
    _heartbeat_required = 0;

    DPRINTFI( "Heartbeat is not required." );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Enable
// =========================
void sm_heartbeat_thread_enable( void )
{
    _messaging_enabled = 1;

    DPRINTFI( "Heartbeat messaging is now enabled." );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Disable
// ==========================
void sm_heartbeat_thread_disable( void )
{
    _messaging_enabled = 0;

    DPRINTFI( "Heartbeat messaging is now disabled." );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Find Interface
// =================================
static SmHeartbeatThreadInterfaceT* sm_heartbeat_thread_find_interface(
    int64_t id )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmHeartbeatThreadInterfaceT* interface;

    SM_LIST_FOREACH( _heartbeat_interfaces, entry, entry_data )
    {
        interface = (SmHeartbeatThreadInterfaceT*) entry_data;

        if( id == interface->id )
        {
            return( interface );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Add Interface
// ================================
SmErrorT sm_heartbeat_thread_add_interface( const SmServiceDomainInterfaceT& domain_interface )
{
    if (0 == _heartbeat_required)
    {
	DPRINTFE( "Heartbeat is not required, skip adding interface." );
	return( SM_OKAY );
    }
    bool configure = false;
    SmHeartbeatThreadInterfaceT* interface;
    SmErrorT error;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    interface = sm_heartbeat_thread_find_interface( domain_interface.id );
    if( NULL == interface )
    {
        interface = (SmHeartbeatThreadInterfaceT*)
                    malloc( sizeof(SmHeartbeatThreadInterfaceT) );
        if( NULL == interface )
        {
            DPRINTFE( "Failed to allocate heartbeat interface (%s).",
                      domain_interface.interface_name );
            goto ERROR;
        }

        SM_LIST_PREPEND( _heartbeat_interfaces,
                         (SmListEntryDataPtrT) interface );

        memset( interface, 0, sizeof(SmHeartbeatThreadInterfaceT) );
        interface->multicast_socket = -1;

        interface->interface_type = sm_get_interface_type(domain_interface.service_domain_interface);
        if ( SM_INTERFACE_OAM == interface->interface_type )
        {
            interface->method = SEND_UNICAST;
        } else
        {
            interface->method = SEND_MULTICAST;
        }
        configure = true;

    } else {
        if(( 0 != strcmp( interface->service_domain, domain_interface.service_domain ) )||
           ( 0 != strcmp( interface->service_domain_interface,
                          domain_interface.service_domain_interface ) )||
           ( 0 != strcmp( interface->interface_name, domain_interface.interface_name ) )||
           ( interface->network_type != domain_interface.network_type ) ||
           ( interface->network_multicast != domain_interface.network_multicast ) ||
           ( interface->network_address != domain_interface.network_address ) ||
           ( interface->network_port != domain_interface.network_heartbeat_port ) ||
           ( interface->network_peer_address != domain_interface.network_peer_address ) ||
           ( interface->network_peer_port != domain_interface.network_peer_heartbeat_port ))
        {
            // Same id, but not the same instance data.
            error = sm_heartbeat_msg_close_sockets( 
                                        &(interface->multicast_socket) );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to close multicast socket for interface "
                          "(%s).", interface->interface_name );
                goto ERROR;
            }
            error = sm_heartbeat_msg_close_sockets(
                                        &(interface->unicast_socket) );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to close unicast socket for interface "
                          "(%s).", interface->interface_name );
                goto ERROR;
            }

            configure = true;

        } else {
            // Update data.
            interface->interface_state = domain_interface.interface_state;
            interface->auth_type = domain_interface.auth_type;
            snprintf( interface->auth_key, sizeof(interface->auth_key),
                      "%s", domain_interface.auth_key );
        }
    }

    if( configure )
    {
        interface->id = domain_interface.id;
        snprintf( interface->service_domain, sizeof(interface->service_domain),
                  "%s", domain_interface.service_domain );
        snprintf( interface->service_domain_interface,
                  sizeof(interface->service_domain_interface),
                  "%s", domain_interface.service_domain_interface );
        snprintf( interface->interface_name, sizeof(interface->interface_name),
                  "%s", domain_interface.interface_name );
        interface->interface_state = domain_interface.interface_state;
        interface->network_type = domain_interface.network_type;
        memcpy( &(interface->network_multicast), &domain_interface.network_multicast,
                sizeof(SmNetworkAddressT) );
        memcpy( &(interface->network_address), &domain_interface.network_address,
                sizeof(SmNetworkAddressT) );
        interface->network_port = domain_interface.network_heartbeat_port;
        memcpy( &(interface->network_peer_address), &domain_interface.network_peer_address,
                sizeof(SmNetworkAddressT) );
        interface->network_peer_port = domain_interface.network_peer_heartbeat_port;
        interface->auth_type = domain_interface.auth_type;
        snprintf( interface->auth_key, sizeof(interface->auth_key), "%s",
                  domain_interface.auth_key );
        interface->multicast_socket = -1;
        interface->unicast_socket = -1;
        interface->socket_reconfigure = false;

        DPRINTFI( "Heartbeat interface (%s) configured.",
                  interface->interface_name );
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_OKAY );

ERROR:
    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_FAILED );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Delete Interface
// ===================================
SmErrorT sm_heartbeat_thread_delete_interface( int64_t id )
{
    SmHeartbeatThreadInterfaceT* interface;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    interface = sm_heartbeat_thread_find_interface( id );
    if( NULL != interface )
    {
        SM_LIST_REMOVE( _heartbeat_interfaces,
                        (SmListEntryDataPtrT) interface );
        free( interface );
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Find Peer Interface
// ======================================
static SmHeartbeatThreadPeerInterfaceT* sm_heartbeat_thread_find_peer_interface(
    char interface_name[], SmNetworkAddressT* network_address, int network_port )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmHeartbeatThreadPeerInterfaceT* peer_interface;

    SM_LIST_FOREACH( _heartbeat_peer_interfaces, entry, entry_data )
    {
        peer_interface = (SmHeartbeatThreadPeerInterfaceT*) entry_data;

        if( 0 != strcmp( interface_name, peer_interface->interface_name ) )
            continue;

        if( network_address->type != peer_interface->network_address.type )
        {
            continue;
        }

        if( SM_NETWORK_TYPE_IPV4 == network_address->type )
        {
            if( network_address->u.ipv4.sin.s_addr
                        == peer_interface->network_address.u.ipv4.sin.s_addr )
            {
                return( peer_interface );
            }
        } else if( SM_NETWORK_TYPE_IPV4_UDP == network_address->type ) {
            if(( network_address->u.ipv4.sin.s_addr
                        == peer_interface->network_address.u.ipv4.sin.s_addr )&&
               ( network_port == peer_interface->network_port ))
            {
                return( peer_interface );
            }
        } else if( SM_NETWORK_TYPE_IPV6 == network_address->type ) {
            if( !memcmp( &(network_address->u.ipv6.sin6),
                         &(peer_interface->network_address.u.ipv6.sin6),
                         sizeof(in6_addr) ))
            {
                return( peer_interface );
            }
        } else if( SM_NETWORK_TYPE_IPV6_UDP == network_address->type ) {
            if( !memcmp( &(network_address->u.ipv6.sin6),
                         &(peer_interface->network_address.u.ipv6.sin6),
                         sizeof(in6_addr) )&&
               ( network_port == peer_interface->network_port ))
            {
                return( peer_interface );
            }
        }
    }
    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Peer Alarm on Interface Timer
// ================================================
static bool sm_heartbeat_peer_alarm_on_interface( SmTimerIdT timer_id,
    int64_t user_data )
{
    bool rearm = true;
    bool raise_alarm = false;
    long ms_expired;
    char hostname[SM_NODE_NAME_MAX_CHAR];
    char network_type[16];
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmAlarmSpecificProblemTextT problem_text;
    SmAlarmProposedRepairActionT proposed_repair_action;
    SmHeartbeatThreadInterfaceT* interface = NULL;
    SmHeartbeatThreadPeerInterfaceT* peer_interface = NULL;
    char log_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    error = sm_node_utils_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( true );
    }

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( true );
    }

    SM_LIST_FOREACH( _heartbeat_peer_interfaces, entry, entry_data )
    {
        SmHeartbeatThreadPeerInterfaceT* tmp;

        tmp = (SmHeartbeatThreadPeerInterfaceT*) entry_data;

        if( timer_id == tmp->alarm_timer )
        {
            peer_interface = tmp;
            break;
        }
    }

    if( NULL == peer_interface )
    {
        DPRINTFE( "Stale peer interface alarm timer detected, peer "
                  "interface not found." );
        rearm = false;
        goto ERROR;
    }

    interface = sm_heartbeat_thread_find_interface( peer_interface->id );
    if( NULL == interface )
    {
        DPRINTFE( "Interface (%s) not found.",
                  peer_interface->interface_name );
        goto ERROR;
    }

    if( 0 == strcmp( SM_SERVICE_DOMAIN_MGMT_INTERFACE,
                     interface->service_domain_interface ) )
    {
        snprintf( network_type, sizeof(network_type), SM_MGMT_INTERFACE_NAME );

    } else if( 0 == strcmp( SM_SERVICE_DOMAIN_OAM_INTERFACE,
                            interface->service_domain_interface ) )
    {
        snprintf( network_type, sizeof(network_type), SM_OAM_INTERFACE_NAME );

    } else if( 0 == strcmp( SM_SERVICE_DOMAIN_CLUSTER_HOST_INTERFACE,
                            interface->service_domain_interface ) )
    {
        snprintf( network_type, sizeof(network_type), SM_CLUSTER_HOST_INTERFACE_NAME );

    } else {
        snprintf( network_type, sizeof(network_type), "unknown" );
    }

    ms_expired = sm_time_get_elapsed_ms( &(peer_interface->last_alive_msg) );
    SmFailoverInterfaceT failover_interface;
    failover_interface.service_domain = interface->service_domain;
    failover_interface.service_domain_interface = interface->service_domain_interface;
    failover_interface.interface_name = interface->interface_name;
    failover_interface.interface_state = interface->interface_state;
    if( ms_expired >= peer_interface->dead_interval )
    {
        sm_failover_lost_heartbeat(&failover_interface);
        raise_alarm = true;
        sm_time_get( &(peer_interface->alarm_raised_time) );
        snprintf( problem_text, sizeof(problem_text),
                  "communication failure detected with peer over "
                  "port %s on host %s", interface->interface_name, hostname );
        snprintf( log_text, sizeof(log_text), "%s", problem_text );

    } else {
        sm_failover_heartbeat_restore(&failover_interface);
        if( SM_HEARTBEAT_ALARM_DEBOUNCE_IN_MS >
            sm_time_get_elapsed_ms( &(peer_interface->alarm_raised_time) ) )
        {
            raise_alarm = true;
            snprintf( problem_text, sizeof(problem_text),
                      "communication failure detected with peer over "
                      "port %s on host %s within the last %i seconds",
                      interface->interface_name, hostname,
                      SM_HEARTBEAT_ALARM_DEBOUNCE_IN_MS/1000 );
            snprintf( log_text, sizeof(log_text), "%s", problem_text );

        } else {
            raise_alarm = false;
            snprintf( log_text, sizeof(log_text), "communication established "
                      "with peer over port %s on host %s",
                      interface->interface_name, hostname );
        }
    }

    if( raise_alarm )
    {
        memset( &(peer_interface->last_alarm_clear), 0, sizeof(SmTimeT) );

        if( SM_HEARTBEAT_ALARM_THROTTLE_IN_MS <=
            sm_time_get_elapsed_ms( &(peer_interface->last_alarm_raise) ) )
        {
            sm_time_get( &(peer_interface->last_alarm_raise) );

            snprintf( proposed_repair_action, sizeof(proposed_repair_action),
                      "check cabling and far-end port configuration "
                      "and status on adjacent equipment" );

            sm_alarm_raise_communication_alarm(
                      SM_ALARM_COMMUNICATION_FAILURE, hostname,
                      network_type, SM_ALARM_SEVERITY_MAJOR,
                      SM_ALARM_PROBABLE_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE,
                      problem_text, "", proposed_repair_action, true );
        }

    } else {
        memset( &(peer_interface->last_alarm_raise), 0, sizeof(SmTimeT) );

        if( SM_HEARTBEAT_ALARM_THROTTLE_IN_MS <=
            sm_time_get_elapsed_ms( &(peer_interface->last_alarm_clear) ) )
        {
            sm_time_get( &(peer_interface->last_alarm_clear) );

            sm_alarm_clear( SM_ALARM_COMMUNICATION_FAILURE, hostname, "",
                            network_type );
        }
    }

    if( 0 != strcmp( peer_interface->log_text, log_text ) )
    {
        snprintf( peer_interface->log_text,
                  sizeof(peer_interface->log_text), "%s", log_text );

        sm_log_communication_state_change( network_type, peer_interface->log_text );
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
    }

    return( true );

ERROR:
    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
    } 

    return( rearm );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Add Peer Interface 
// =====================================
SmErrorT sm_heartbeat_thread_add_peer_interface( int64_t id, 
    char interface_name[], SmNetworkAddressT* network_address,
    int network_port, int dead_interval )
{
    SmHeartbeatThreadPeerInterfaceT* peer_interface;
    SmErrorT error;


    if (0 == _heartbeat_required)
    {
        DPRINTFE( "Heartbeat is not required, skip adding peer interface." );
        return( SM_OKAY );
    }

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    peer_interface = sm_heartbeat_thread_find_peer_interface( interface_name,
                                                network_address, network_port );
    if( NULL == peer_interface )
    {
        char timer_name[80];

        peer_interface = (SmHeartbeatThreadPeerInterfaceT*)
                         malloc( sizeof(SmHeartbeatThreadPeerInterfaceT) );
        if( NULL == peer_interface )
        {
            DPRINTFE( "Failed to allocate peer heartbeat interface (%s).",
                      interface_name );
            goto ERROR;
        }

        memset( peer_interface, 0, sizeof(SmHeartbeatThreadPeerInterfaceT) );

        peer_interface->id = id;
        snprintf( peer_interface->interface_name,
                  sizeof(peer_interface->interface_name), "%s",
                  interface_name );
        memcpy( &(peer_interface->network_address), network_address,
                sizeof(SmNetworkAddressT) );
        peer_interface->network_port = network_port;
        peer_interface->dead_interval = dead_interval;
        sm_time_get( &(peer_interface->last_alive_msg) );
        peer_interface->alarm_timer = SM_TIMER_ID_INVALID;

        snprintf( timer_name, sizeof(timer_name), "peer alarm on interface %s",
                  interface_name );

        error = sm_timer_register( timer_name,
                                   peer_interface->dead_interval,
                                   sm_heartbeat_peer_alarm_on_interface, 0,
                                   &(peer_interface->alarm_timer) );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to create peer alarm timer, error=%s.",
                      sm_error_str( error ) );
        }

        SM_LIST_PREPEND( _heartbeat_peer_interfaces,
                         (SmListEntryDataPtrT) peer_interface );
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_OKAY );

ERROR:
    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_FAILED );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Delete Peer Interface 
// ========================================
SmErrorT sm_heartbeat_thread_delete_peer_interface( char interface_name[],
    SmNetworkAddressT* network_address, int network_port )
{
    SmHeartbeatThreadPeerInterfaceT* peer_interface;
    SmErrorT error;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    peer_interface = sm_heartbeat_thread_find_peer_interface( interface_name,
                                                network_address, network_port );
    if( NULL != peer_interface )
    {
        if( SM_TIMER_ID_INVALID != peer_interface->alarm_timer )
        {
            error = sm_timer_deregister( peer_interface->alarm_timer );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to cancel peer alarm timer, error=%s.",
                          sm_error_str( error ) );
            }

            peer_interface->alarm_timer = SM_TIMER_ID_INVALID;
        }

        SM_LIST_REMOVE( _heartbeat_peer_interfaces,
                        (SmListEntryDataPtrT) peer_interface );

        memset( peer_interface, 0, sizeof(SmHeartbeatThreadPeerInterfaceT) );

        free( peer_interface );
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Find Peer Node
// =================================
static SmHeartbeatThreadPeerNodeT* sm_heartbeat_thread_find_peer_node(
    char node_name[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmHeartbeatThreadPeerNodeT* peer_node;

    SM_LIST_FOREACH( _heartbeat_peer_nodes, entry, entry_data )
    {
        peer_node = (SmHeartbeatThreadPeerNodeT*) entry_data;

        if( 0 == strcmp( node_name, peer_node->node_name ) )
        {
            return( peer_node );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Peer Alive In Period
// =======================================
bool sm_heartbeat_thread_peer_alive_in_period( char node_name[], 
    unsigned int period_in_ms )
{
    bool peer_alive = false;
    SmHeartbeatThreadPeerNodeT* peer_node;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( true );
    }

    peer_node = sm_heartbeat_thread_find_peer_node( node_name );
    if( NULL != peer_node )
    {
        long ms_expired;

        ms_expired = sm_time_get_elapsed_ms( &(peer_node->last_alive_msg) );

        peer_alive = ( period_in_ms >= ms_expired );
    }

    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
    }

    return( peer_alive );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Alive Timer
// ==============================
static bool sm_heartbeat_alive_timer( SmTimerIdT timer_id, int64_t user_data )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmHeartbeatThreadInterfaceT* interface;
    SmErrorT error;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( true );
    }

    if( '\0' == _node_name[0] )
    {
        DPRINTFI( "Node name not set yet." );
        goto DONE;
    }

    if( 0 == _messaging_enabled )
    {
        DPRINTFD( "Messaging is disabled." );
        goto DONE;
    }

    SM_LIST_FOREACH( _heartbeat_interfaces, entry, entry_data )
    {
        interface = (SmHeartbeatThreadInterfaceT*) entry_data;

        if( SM_INTERFACE_STATE_ENABLED != interface->interface_state ||
            interface->socket_reconfigure )
        {
            error = sm_heartbeat_msg_close_sockets(
                                        &(interface->multicast_socket) );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to close multicast socket for interface "
                          "(%s), error=%s.", interface->interface_name,
                          sm_error_str(error) );
                continue;
            }

            if  ( SM_INTERFACE_OAM == interface->interface_type )
            {
                error = sm_heartbeat_msg_close_sockets(
                                            &(interface->unicast_socket) );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to close unicast socket for interface "
                              "(%s), error=%s.", interface->interface_name,
                              sm_error_str(error) );
                    continue;
                }
            }

            if( SM_INTERFACE_STATE_ENABLED != interface->interface_state )
            {
                continue;
            }

            if  ( SM_INTERFACE_OAM == interface->interface_type )
            {
                error = sm_heartbeat_msg_open_sockets( interface->network_type,
                        &(interface->network_address),
                        &(interface->network_multicast),
                        interface->network_port, interface->interface_name,
                        &(interface->multicast_socket), &(interface->unicast_socket) );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to open sockets for interface (%s), "
                              "error=%s.", interface->interface_name,
                              sm_error_str(error) );
                    continue;
                }

            }else
            {
                error = sm_heartbeat_msg_open_sockets( interface->network_type,
                        &(interface->network_address),
                        &(interface->network_multicast),
                        interface->network_port, interface->interface_name,
                        &(interface->multicast_socket), NULL );
                if( SM_OKAY != error )
                {
                    DPRINTFE( "Failed to open sockets for interface (%s), "
                              "error=%s.", interface->interface_name,
                              sm_error_str(error) );
                    continue;
                }
            }
            interface->socket_reconfigure = false;
        }

        if ( interface->network_multicast.type != SM_NETWORK_TYPE_NIL )
        {
            if( SM_INTERFACE_OAM != interface->interface_type )
            {
                error = sm_heartbeat_msg_send_alive( interface->network_type, _node_name,
                            &(interface->network_address), &(interface->network_multicast),
                            interface->network_port, interface->interface_name,
                            interface->auth_type, interface->auth_key,
                            interface->multicast_socket );
                if( SM_OKAY != error )
                {
                    char multicast_addr_str[200];
                    sm_network_address_str(&interface->network_multicast, multicast_addr_str);

                    DPRINTFE( "Failed to send alive on to %s interface (%s), "
                              "error=%s.", multicast_addr_str, interface->interface_name,
                              sm_error_str(error) );
                    interface->socket_reconfigure = true;
                    continue;
                }
            }
        } else
        {
            DPRINTFD( "Multicast not configured for interface %s", interface->interface_name );
        }

        if  ( SM_INTERFACE_OAM == interface->interface_type )
        {
            error = sm_heartbeat_msg_send_alive( interface->network_type, _node_name,
                        &(interface->network_address), &(interface->network_peer_address),
                        interface->network_port, interface->interface_name,
                        interface->auth_type, interface->auth_key,
                        interface->unicast_socket );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to send alive on interface (%s), "
                          "error=%s.", interface->interface_name,
                          sm_error_str(error) );
                interface->socket_reconfigure = true;
                continue;
            }
        }

        DPRINTFD( "Sent alive message for node (%s).", _node_name );
    }

DONE:
    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( true );
    }

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Authenticate Message
// =======================================
static bool sm_heartbeat_thread_auth_message( char interface_name[],
    SmNetworkAddressT* network_address, int network_port,
    void* msg, int msg_size, uint8_t auth_vector[] )
{
    bool auth = false;
    SmHeartbeatThreadPeerInterfaceT* peer_interface;
    SmHeartbeatThreadInterfaceT* interface;
    SmSha512HashT hash;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( true );
    }

    peer_interface = sm_heartbeat_thread_find_peer_interface( interface_name,
                                                network_address, network_port );
    if( NULL != peer_interface )
    {
        interface = sm_heartbeat_thread_find_interface( peer_interface->id );
        if( NULL == interface )
        {
            DPRINTFE( "Interface (%s) not found.",
                      peer_interface->interface_name );
            goto DONE;
        }

        sm_sha512_hmac( msg, msg_size, interface->auth_key,
                        strlen(interface->auth_key), &hash );

        if( 0 == memcmp( &(hash.bytes[0]), auth_vector, SM_SHA512_HASH_SIZE ) )
        {
            auth = true;
        }
        else
        {
            DPRINTFD("auth key not matched, auth key %s", interface->auth_key);
        }
    }


DONE:
    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return( true );
    }

    return( auth );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Receive Alive Message
// ========================================
static void sm_heartbeat_thread_receive_alive_message( char node_name[],
        SmNetworkAddressT* network_address, int network_port, int version,
        int revision, char interface_name[] )
{
    SmHeartbeatThreadPeerInterfaceT* peer_interface;

    DPRINTFD( "Received alive message from node (%s).", node_name );

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return;
    }

    if( '\0' == node_name[0] )
    {
        DPRINTFE( "Node name invalid." );
        goto DONE;
    }

    if( 0 == _messaging_enabled )
    {
        DPRINTFD( "Messaging is disabled." );
        goto DONE;
    }

    peer_interface = sm_heartbeat_thread_find_peer_interface( interface_name,
                                                network_address, network_port );
    if( NULL != peer_interface )
    {
        SmHeartbeatThreadPeerNodeT* peer_node;

        sm_time_get( &(peer_interface->last_alive_msg) );

        peer_node = sm_heartbeat_thread_find_peer_node( node_name );
        if( NULL == peer_node )
        {
            peer_node = (SmHeartbeatThreadPeerNodeT*)
                        malloc( sizeof(SmHeartbeatThreadPeerNodeT) );
            if( NULL == peer_node )
            {
                DPRINTFE( "Failed to allocate peer node (%s).", node_name );
                goto DONE;
            }

            SM_LIST_PREPEND( _heartbeat_peer_nodes,
                             (SmListEntryDataPtrT) peer_node );

            memset( peer_node, 0, sizeof(SmHeartbeatThreadPeerNodeT) );

            snprintf( peer_node->node_name, sizeof(peer_node->node_name),
                      "%s", node_name );

            DPRINTFI( "Peer node (%s) added.", peer_node->node_name );
        }

        peer_node->version = version;
        peer_node->revision = revision;
        peer_node->last_alive_msg = peer_interface->last_alive_msg;
    }

DONE:
    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Receive if_state Message
// ========================================
static void sm_heartbeat_thread_receive_if_state_message( const char node_name[],
        SmHeartbeatMsgIfStateT if_state)
{
    sm_failover_if_state_update(node_name, if_state);
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Initialize Thread
// ====================================
static SmErrorT sm_heartbeat_thread_initialize_thread( void )
{
    SmErrorT error;

    error = sm_selobj_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize selection object module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_timer_initialize( SM_HEARTBEAT_THREAD_TICK_INTERVAL_IN_MS );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize timer module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_heartbeat_msg_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize heartbeat message module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    _callbacks.auth = sm_heartbeat_thread_auth_message;
    _callbacks.alive = sm_heartbeat_thread_receive_alive_message;
    _callbacks.if_state = sm_heartbeat_thread_receive_if_state_message;

    error = sm_heartbeat_msg_register_callbacks( &_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register heartbeat message callbacks, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_timer_register( "heartbeat alive",
                               SM_HEARTBEAT_THREAD_ALIVE_TIMER_IN_MS,
                               sm_heartbeat_alive_timer, 0, &_alive_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create heartbeat alive timer, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_hw_initialize( NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize hardware module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_node_utils_get_hostname( _node_name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get node name, error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Finalize Thread
// ==================================
static SmErrorT sm_heartbeat_thread_finalize_thread( void )
{
    SmErrorT error;

    error = sm_hw_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize hardware module, error=%s.",
                  sm_error_str( error ) );
    }

    if( SM_TIMER_ID_INVALID != _alive_timer_id )
    {
        error = sm_timer_deregister( _alive_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel heartbeat alive timer, error=%s.",
                      sm_error_str( error ) );
        }

        _alive_timer_id = SM_TIMER_ID_INVALID;
    }

    error = sm_heartbeat_msg_deregister_callbacks( &_callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister heartbeat message callbacks, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_heartbeat_msg_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize heartbeat message module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_timer_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize timer module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_selobj_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize selection object module, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Main
// =======================
static void* sm_heartbeat_thread_main( void* arguments )
{
    SmErrorT error;
    struct sched_param param;
    int result;

    pthread_setname_np( pthread_self(), SM_HEARTBEAT_THREAD_NAME );
    sm_debug_set_thread_info();
    sm_trap_set_thread_info();

    memset( &param, 0, sizeof(struct sched_param) );
    param.sched_priority = sched_get_priority_min( SCHED_RR );

    result = pthread_setschedparam( pthread_self(), SCHED_RR, &param );
    if( 0 != result )
    {
        DPRINTFE( "Failed to set heartbeat thread to realtime (priority=%d), "
                  "error=%s (%i).", param.sched_priority, strerror( result ),
                  result );
        pthread_exit( NULL );
    }

    DPRINTFI( "Starting" );

    // Warn after 400 milliseconds, fail after 3 seconds.
    error = sm_thread_health_register( SM_HEARTBEAT_THREAD_NAME, 400, 3000 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register heartbeat thread, error=%s.",
                  sm_error_str( error ) );
        pthread_exit( NULL );
    }

    error = sm_heartbeat_thread_initialize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize heartbeat thread, error=%s.",
                  sm_error_str( error ) );
        pthread_exit( NULL );
    }

    DPRINTFI( "Started." );

    while( _stay_on )
    {
        error = sm_selobj_dispatch( SM_HEARTBEAT_THREAD_TICK_INTERVAL_IN_MS );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Selection object dispatch failed, error=%s.",
                      sm_error_str(error) );
            break;
        }

        error = sm_thread_health_update( SM_HEARTBEAT_THREAD_NAME );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to update heartbeat thread health, error=%s.",
                      sm_error_str(error) );
        }
    }

    DPRINTFI( "Shutting down." );

    error = sm_heartbeat_thread_finalize_thread();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize heartbeat thread, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_thread_health_deregister( SM_HEARTBEAT_THREAD_NAME );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to deregister heartbeat thread, error=%s.",
                  sm_error_str( error ) );
    }

    DPRINTFI( "Shutdown complete." );

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Start
// ========================
SmErrorT sm_heartbeat_thread_start( void )
{
    int result;

    _stay_on = 1;
    _messaging_enabled = 0;
    _thread_created = false;

    result = pthread_create( &_heartbeat_thread, NULL, sm_heartbeat_thread_main,
                             NULL );
    if( 0 != result )
    {
        DPRINTFE( "Failed to start heartbeat thread, error=%s.",
                  strerror(result) );
        return( SM_FAILED );
    }

    _thread_created = true;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Stop
// =======================
SmErrorT sm_heartbeat_thread_stop( void )
{
    _stay_on = 0;
    _messaging_enabled = 0;

    if( _thread_created )
    {
        long ms_expired;
        SmTimeT time_prev;
        int result;

        sm_time_get( &time_prev );

        while( true )
        {
            result = pthread_tryjoin_np( _heartbeat_thread, NULL );
            if(( 0 != result )&&( EBUSY != result ))
            {
                if(( ESRCH != result )&&( EINVAL != result ))
                {
                    DPRINTFE( "Failed to wait for heartbeat thread exit, "
                              "sending kill signal, error=%s.",
                              strerror(result) );
                    pthread_kill( _heartbeat_thread, SIGKILL );
                }
                break;
            }

            ms_expired = sm_time_get_elapsed_ms( &time_prev );
            if( 5000 <= ms_expired )
            {
                DPRINTFE( "Failed to stop heartbeat thread, sending "
                          "kill signal." );
                pthread_kill( _heartbeat_thread, SIGKILL );
                break;
            }

            usleep( 250000 ); // 250 milliseconds.
        }
    }

    _thread_created = false;

    return( SM_OKAY );
}
// ****************************************************************************
