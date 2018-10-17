//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_hw.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <ifaddrs.h>
#include <pthread.h>
#include <sys/types.h>
#include <linux/gen_stats.h>
#include <linux/rtnetlink.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_selobj.h"
#include "sm_netlink.h"

typedef struct
{
    bool inuse;
    pthread_t thread_id;
    uint32_t msg_seq;
    int ioctl_socket;
    int netlink_receive_socket;
    SmHwCallbacksT callbacks;
} SmHwThreadInfoT;

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
static SmHwThreadInfoT _threads[SM_THREADS_MAX];

// ****************************************************************************
// Hardware - Find Thread Info
// ===========================
static SmHwThreadInfoT* sm_hw_find_thread_info( void )
{
    pthread_t thread_id = pthread_self();

    unsigned int thread_i;
    for( thread_i=0; SM_THREADS_MAX > thread_i; ++thread_i )
    {
        if( !(_threads[thread_i].inuse) )
            continue;

        if( thread_id == _threads[thread_i].thread_id )
            return( &(_threads[thread_i]) );
    }

    return( NULL );
}
// ****************************************************************************

// ***************************************************************************
// Hardware - Get Interface By Network Address
// ===========================================
SmErrorT sm_hw_get_if_by_network_address( SmNetworkAddressT* network_address,
    char if_name[] )
{
    if_name[0] = '\0';

    if(( SM_NETWORK_TYPE_IPV4 == network_address->type )||
       ( SM_NETWORK_TYPE_IPV4_UDP == network_address->type ))
    {
        struct ifaddrs *if_addr = NULL;
        struct ifaddrs *if_addrs = NULL;
        int result;

        result = getifaddrs( &if_addrs );
        if( 0 > result )
        {
            DPRINTFE( "Failed to get all interface addresses, error=%s.",
                      strerror( errno ) );
            return( SM_FAILED );
        }

        for( if_addr=if_addrs; NULL != if_addr; if_addr=if_addr->ifa_next )
        {
            if( NULL == if_addr->ifa_addr )
                continue;

            if( AF_INET == if_addr->ifa_addr->sa_family )
            {
                struct sockaddr_in* ipv4_addr;
                ipv4_addr = (struct sockaddr_in*) if_addr->ifa_addr;

                if( ipv4_addr->sin_addr.s_addr
                        == network_address->u.ipv4.sin.s_addr )
                {
                    snprintf( if_name, SM_INTERFACE_NAME_MAX_CHAR, "%s",
                              if_addr->ifa_name );
                    return( SM_OKAY );
                }
            }
        }

    } else if(( SM_NETWORK_TYPE_IPV6 == network_address->type )||
    	       ( SM_NETWORK_TYPE_IPV6_UDP == network_address->type ))
    	    {
    	        struct ifaddrs *if_addr = NULL;
    	        struct ifaddrs *if_addrs = NULL;
    	        int result;

    	        result = getifaddrs( &if_addrs );
    	        if( 0 > result )
    	        {
    	            DPRINTFE( "Failed to get all interface addresses, error=%s.",
    	                      strerror( errno ) );
    	            return( SM_FAILED );
    	        }

    	        for( if_addr=if_addrs; NULL != if_addr; if_addr=if_addr->ifa_next )
    	        {
    	            if( NULL == if_addr->ifa_addr )
    	                continue;

    	            if( AF_INET6 == if_addr->ifa_addr->sa_family )
    	            {
    	                struct sockaddr_in6* ipv6_addr;
    	                ipv6_addr = (struct sockaddr_in6*) if_addr->ifa_addr;

    	                if( memcmp( &(ipv6_addr->sin6_addr),
    	                            &(network_address->u.ipv6.sin6),
    	                            sizeof(struct in6_addr) ))
    	                {
    	                    snprintf( if_name, SM_INTERFACE_NAME_MAX_CHAR, "%s",
    	                              if_addr->ifa_name );
    	                    return( SM_OKAY );
    	                }
    	            }
    	        }

    	    } else {

        DPRINTFE( "Unsupported network type (%s).",
                  sm_network_type_str(network_address->type) );
        return( SM_FAILED );
    }

    return( SM_NOT_FOUND );
}
// ***************************************************************************

// ***************************************************************************
// Hardware - Get Interface Index
// ==============================
SmErrorT sm_hw_get_if_index( const char if_name[], int* if_index )
{
    struct ifreq if_data;
    SmHwThreadInfoT* thread_info;

    *if_index = SM_INVALID_INDEX;

    thread_info = sm_hw_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( SM_FAILED );
    }

    memset( &if_data, 0, sizeof(if_data) );

    snprintf( if_data.ifr_name, sizeof(if_data.ifr_name), "%s", if_name );

    if( 0 <= ioctl( thread_info->ioctl_socket, SIOCGIFINDEX, &if_data ) )
    {
        if( 0 != if_data.ifr_ifindex )
        {
            *if_index = if_data.ifr_ifindex;

        } else {
            DPRINTFD( "Interface index is invalid, if_name=%s.", if_name );
            return( SM_FAILED );
        }
    } else {
        DPRINTFD( "Failed to get interface index, if_name=%s, error=%s.",
                  if_name, strerror( errno ) );
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ***************************************************************************

// ***************************************************************************
// Hardware - Get Interface Name
// =============================
SmErrorT sm_hw_get_if_name( int if_index, char if_name[] )
{
    struct ifreq if_data;
    SmHwThreadInfoT* thread_info;

    thread_info = sm_hw_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( SM_FAILED );
    }

    memset( &if_data, 0, sizeof(if_data) );

    if_data.ifr_ifindex = if_index;

    if( 0 <= ioctl( thread_info->ioctl_socket, SIOCGIFNAME, &if_data ) )
    {
        snprintf( if_name, SM_INTERFACE_NAME_MAX_CHAR, "%s", if_data.ifr_name );

    } else {
        DPRINTFD( "Failed to get interface name for interface index (%i), "
                  "error=%s.", if_index, strerror( errno ) );
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ***************************************************************************

// ***************************************************************************
// Sm Hardware - Get Interface State
// =================================
SmErrorT sm_hw_get_if_state( const char if_name[], bool* enabled )
{
    struct ifreq if_data;
    SmHwThreadInfoT* thread_info;

    thread_info = sm_hw_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( SM_FAILED );
    }

    memset( &if_data, 0, sizeof(if_data) );
    snprintf( if_data.ifr_name, sizeof(if_data.ifr_name), "%s", if_name );

    if( 0 <= ioctl( thread_info->ioctl_socket, SIOCGIFFLAGS, &if_data ) )
    {
        if( if_data.ifr_flags & IFF_RUNNING )
        {
            *enabled = true;
        } else {
            *enabled = false;
        }
    } else {
        DPRINTFE( "Failed to get interface state for interface (%s), "
                  "error=%s.", if_name, strerror( errno ) );
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ***************************************************************************

// ***************************************************************************
// Sm Hardware - Get All QDisc Asynchronous
// ========================================
SmErrorT sm_hw_get_all_qdisc_async( void )
{
    struct nlmsghdr hdr;
    struct tcmsg tc;    
    SmHwThreadInfoT* thread_info;
    SmErrorT error;

    thread_info = sm_hw_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return( SM_FAILED );
    }

    memset( &hdr, 0, sizeof(hdr) );
    hdr.nlmsg_len = NLMSG_LENGTH(sizeof(tc));
    hdr.nlmsg_type = RTM_GETQDISC;
    hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
    hdr.nlmsg_seq = ++(thread_info->msg_seq);
    hdr.nlmsg_pid = getpid();

    memset( &tc, 0, sizeof(tc) );
    tc.tcm_family = AF_UNSPEC;

    error = sm_netlink_send_request( thread_info->netlink_receive_socket,
                                     &hdr, &tc, sizeof(tc) );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to request all qdisc info, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    return( SM_OKAY );
}
// ***************************************************************************

// ****************************************************************************
// Hardware - Netlink Interface Message Dispatch
// =============================================
static void sm_hw_netlink_if_msg_dispatch( SmHwThreadInfoT* thread_info,
    struct nlmsghdr* payload )
{
    bool enabled;
    struct ifinfomsg* if_msg;
    int if_msg_size;
    struct rtattr* if_msg_attr[IFLA_MAX+1];
    struct rtattr* if_msg_attr_tmp = NULL;
    SmHwInterfaceChangeDataT if_change_data;
    SmErrorT error;

    if_msg = (struct ifinfomsg*) NLMSG_DATA( payload );
    if_msg_size = payload->nlmsg_len - NLMSG_LENGTH( sizeof(struct ifinfomsg) );

    if( 0 >= if_msg_size )
    {
        DPRINTFE( "Netlink message has the wrong length( %i).", if_msg_size );
        return;
    }

    if( AF_UNSPEC != if_msg->ifi_family )
    {
        DPRINTFE( "Unknown interface address family (%i) received.",
                  if_msg->ifi_family );
        return;
    }

    memset( &if_change_data, 0, sizeof( if_change_data ) );

    int attr_i;
    for( attr_i=0; IFLA_MAX >= attr_i; ++attr_i )
    {
        if_msg_attr[attr_i] = NULL;
    }

    if_msg_attr_tmp = IFLA_RTA( if_msg );

    while( RTA_OK( if_msg_attr_tmp, if_msg_size ) )
    {
        if( IFLA_MAX >= if_msg_attr_tmp->rta_type )
        {
            if_msg_attr[if_msg_attr_tmp->rta_type] = if_msg_attr_tmp;
        }

        if_msg_attr_tmp = RTA_NEXT( if_msg_attr_tmp, if_msg_size );
    }

    if( NULL == if_msg_attr[IFLA_IFNAME] )
    {
        DPRINTFE( "Missing interface name for interface index %i.",
                  if_msg->ifi_index );
        return;
    }

    snprintf( if_change_data.interface_name, 
              sizeof(if_change_data.interface_name), "%s",
              (char*) RTA_DATA( if_msg_attr[IFLA_IFNAME] ) );

    error = sm_hw_get_if_state( if_change_data.interface_name, &enabled );
    if(( SM_OKAY != error )&&( SM_NOT_FOUND != error ))
    {
        DPRINTFE( "Failed to get interface (%s) state information, "
                  "error=%s.", if_change_data.interface_name,
                  sm_error_str( error ) );
        return;
    } else if( SM_NOT_FOUND == error ) {
        if_change_data.type = SM_HW_INTERFACE_CHANGE_TYPE_DELETE;
        if_change_data.interface_state = SM_INTERFACE_STATE_UNKNOWN;
    } else {
        if_change_data.type = SM_HW_INTERFACE_CHANGE_TYPE_ADD;

        if( enabled )
        {
            if_change_data.interface_state = SM_INTERFACE_STATE_ENABLED;
        } else {
            if_change_data.interface_state = SM_INTERFACE_STATE_DISABLED;
        }
    }

    if( NULL != thread_info->callbacks.interface_change )
    {
        thread_info->callbacks.interface_change( &if_change_data );
    }
}
// ****************************************************************************

// ****************************************************************************
// Hardware - Netlink IP Message Dispatch
// ======================================
static void sm_hw_netlink_ip_msg_dispatch( SmHwThreadInfoT* thread_info,
    struct nlmsghdr* payload )
{
    int len;
    struct ifaddrmsg* if_addr_msg;
    struct rtattr* rta;
    SmHwIpChangeDataT ip_info;
    SmErrorT error;

    memset( &ip_info, 0, sizeof(SmHwIpChangeDataT) );

    if( RTM_DELADDR == payload->nlmsg_type )
    {
        ip_info.type = SM_HW_IP_CHANGE_TYPE_DELETE;
    } else {
        ip_info.type = SM_HW_IP_CHANGE_TYPE_ADD;
    }

    if_addr_msg = (struct ifaddrmsg *) NLMSG_DATA( payload );
    len = IFA_PAYLOAD( payload );

    error = sm_hw_get_if_name( if_addr_msg->ifa_index,
                               ip_info.interface_name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to look up interface name, ifindex=%i, "
                  "error=%s.", (int) if_addr_msg->ifa_index,
                  sm_error_str(error) );
        return;
    }

    DPRINTFD( "Interface %s (%i).", ip_info.interface_name,
              if_addr_msg->ifa_index );

    ip_info.prefix_len = if_addr_msg->ifa_prefixlen;

    for( rta = (struct rtattr *) IFA_RTA( if_addr_msg );
         RTA_OK( rta, len ); rta = RTA_NEXT( rta, len ) )
    {
        if(( IFA_ADDRESS == rta->rta_type )||( IFA_LOCAL == rta->rta_type )||
           ( IFA_BROADCAST == rta->rta_type || IFA_ANYCAST == rta->rta_type ))
        {
            char ip_str[SM_NETWORK_ADDRESS_MAX_CHAR];

            if( IFA_ADDRESS == rta->rta_type )
            {
                ip_info.address_type = SM_HW_ADDRESS_TYPE_ADDRESS;
            } else if( IFA_LOCAL == rta->rta_type ){
                ip_info.address_type = SM_HW_ADDRESS_TYPE_LOCAL;
            } else if( IFA_BROADCAST == rta->rta_type ){
                ip_info.address_type = SM_HW_ADDRESS_TYPE_BROADCAST;
            } else if( IFA_ANYCAST == rta->rta_type ){
                ip_info.address_type = SM_HW_ADDRESS_TYPE_ANYCAST;
            } else {
                ip_info.address_type = SM_HW_ADDRESS_TYPE_UNKNOWN;
            }

            if( AF_INET == if_addr_msg->ifa_family )
            {
                ip_info.address.type = SM_NETWORK_TYPE_IPV4;
                ip_info.address.u.ipv4.sin
                    = *((struct in_addr*) RTA_DATA( rta ));
                sm_network_address_str( &(ip_info.address), ip_str );
                DPRINTFD( "IPv4 address=%s/%i.", ip_str, ip_info.prefix_len );

                if( NULL != thread_info->callbacks.ip_change )
                {
                    thread_info->callbacks.ip_change( &ip_info );
                }
            } else if( AF_INET6 == if_addr_msg->ifa_family ) {
                ip_info.address.type = SM_NETWORK_TYPE_IPV6;
                memcpy( &(ip_info.address.u.ipv6.sin6),
                        (struct in6_addr*) RTA_DATA( rta ),
                        sizeof(struct in6_addr) );
                sm_network_address_str( &(ip_info.address), ip_str );
                DPRINTFD( "IPv6 address=%s/%i.", ip_str, ip_info.prefix_len );

                if( NULL != thread_info->callbacks.ip_change )
                {
                    thread_info->callbacks.ip_change( &ip_info );
                }
            }
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Hardware - Netlink QDisc Message Dispatch
// =========================================
static void sm_hw_netlink_qdisc_msg_dispatch( SmHwThreadInfoT* thread_info,
    struct nlmsghdr* payload )
{
    struct tcmsg* tc = (struct tcmsg*) NLMSG_DATA(payload);
    struct rtattr* tb[TCA_MAX+1];
    struct rtattr* rta;
    int len;
    SmHwQdiscInfoT qdisc_info;
    SmErrorT error;

    memset( &qdisc_info, 0, sizeof(qdisc_info) );

    error = sm_hw_get_if_name( tc->tcm_ifindex, qdisc_info.interface_name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to look up interface name, ifindex=%i, error=%s.",
                  (int) tc->tcm_ifindex, sm_error_str(error) );
        return;
    }

    len = payload->nlmsg_len - NLMSG_LENGTH(sizeof(struct tcmsg));
    if( 0 > len )
    {
        DPRINTFE( "Invalid length detected." );
        return;
    }

    memset( tb, 0, sizeof(tb) );

    for( rta = TCA_RTA(tc); RTA_OK(rta, len); rta = RTA_NEXT(rta, len) )
    {
        if( TCA_MAX >= rta->rta_type )
        {
            tb[rta->rta_type] = rta;
        }
    }

    if( NULL == tb[TCA_KIND] )
    {
        DPRINTFE( "Kind of qdisc not set." );
        return;
    }

    snprintf( qdisc_info.queue_type, sizeof(qdisc_info.queue_type),
              "%s", (const char *)RTA_DATA(tb[TCA_KIND]) );

    snprintf( qdisc_info.handle, sizeof(qdisc_info.handle),
              "%x:", tc->tcm_handle >> 16 );

    if( tb[TCA_STATS2] )
    {
        struct rtattr* tb_stats[TCA_STATS_MAX+1];

        memset( tb_stats, 0, sizeof(tb_stats) );
        len = RTA_PAYLOAD(tb[TCA_STATS2]);

        for( rta = (struct rtattr*) RTA_DATA(tb[TCA_STATS2]);
             RTA_OK(rta, len); rta = RTA_NEXT(rta, len) )
        {
            if( TCA_STATS_MAX >= rta->rta_type )
            {
                tb_stats[rta->rta_type] = rta;
            }
        }

        if( tb_stats[TCA_STATS_BASIC] )
        {
            struct gnet_stats_basic basic_stats;
            memset( &basic_stats, 0, sizeof(basic_stats) );

            if( RTA_PAYLOAD(tb_stats[TCA_STATS_BASIC]) > sizeof(basic_stats) )
            {
                memcpy( &basic_stats, RTA_DATA(tb_stats[TCA_STATS_BASIC]),
                        sizeof(basic_stats) );
            } else {
                memcpy( &basic_stats, RTA_DATA(tb_stats[TCA_STATS_BASIC]),
                        RTA_PAYLOAD(tb_stats[TCA_STATS_BASIC]) );
            }

            qdisc_info.bytes   = basic_stats.bytes;
            qdisc_info.packets = basic_stats.packets;
        }

        if( tb_stats[TCA_STATS_QUEUE] )
        {
            struct gnet_stats_queue queue_stats;
            memset( &queue_stats, 0, sizeof(queue_stats) );

            if( RTA_PAYLOAD(tb_stats[TCA_STATS_QUEUE]) > sizeof(queue_stats) )
            {
                memcpy( &queue_stats, RTA_DATA(tb_stats[TCA_STATS_QUEUE]),
                        sizeof(queue_stats) );
            } else {
                memcpy( &queue_stats, RTA_DATA(tb_stats[TCA_STATS_QUEUE]),
                        RTA_PAYLOAD(tb_stats[TCA_STATS_QUEUE]) );
            }

            qdisc_info.q_length   = queue_stats.qlen;
            qdisc_info.backlog    = queue_stats.backlog;
            qdisc_info.drops      = queue_stats.drops;
            qdisc_info.requeues   = queue_stats.requeues;
            qdisc_info.overlimits = queue_stats.overlimits;
        }
    }

    if( NULL != thread_info->callbacks.qdisc_info )
    {
        thread_info->callbacks.qdisc_info( &qdisc_info );
    }
}
// ****************************************************************************

// ****************************************************************************
// Hardware - Netlink Message Dispatch
// ===================================
static void sm_hw_netlink_msg_dispatch( int socket_fd, 
    struct sockaddr_nl* address, struct nlmsghdr* payload,
    void* invocation_data )
{
    SmHwThreadInfoT* thread_info = (SmHwThreadInfoT*) invocation_data;

    switch( payload->nlmsg_type )
    {
        case RTM_NEWLINK:
        case RTM_DELLINK:
        case RTM_GETLINK:
            sm_hw_netlink_if_msg_dispatch( thread_info, payload );
        break;

        case RTM_NEWADDR:
        case RTM_GETADDR:
        case RTM_DELADDR:
            sm_hw_netlink_ip_msg_dispatch( thread_info, payload );
        break;

        case RTM_NEWQDISC:
        case RTM_GETQDISC:
            sm_hw_netlink_qdisc_msg_dispatch( thread_info, payload );
        break;

        case RTM_DELQDISC:
            DPRINTFD( "Ignoring delete qdisc notification." );
        break;

        default:
            DPRINTFI( "Ignoring netlink notification (%i).",
                      (int) payload->nlmsg_type );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Hardware - Netlink Dispatch
// ===========================
static void sm_hw_netlink_dispatch( int selobj, int64_t user_data )
{
    SmHwThreadInfoT* thread_info;
     SmErrorT error;

    thread_info = sm_hw_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        return;
    }

    error = sm_netlink_receive( selobj, sm_hw_netlink_msg_dispatch,
                                thread_info );
    if(( SM_OKAY != error )&&( SM_NO_MSG != error ))
    {
        DPRINTFE( "Failed to receive netlink message, error=%s.",
                  sm_error_str( error ) );
        return;
    }
}
// ***************************************************************************

// ***************************************************************************
// Hardware - Initialize
// =====================
SmErrorT sm_hw_initialize( SmHwCallbacksT* callbacks )
{
    int flags;
    SmHwThreadInfoT* thread_info = NULL;
    SmErrorT error;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    unsigned int thread_i;
    for( thread_i=0; SM_THREADS_MAX > thread_i; ++thread_i )
    {
        if( !(_threads[thread_i].inuse) )
        {
            thread_info = &(_threads[thread_i]);
            break;
        }
    }

    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to allocate thread information." );
        goto ERROR;
    }

    memset( thread_info, 0, sizeof(SmHwThreadInfoT) );

    thread_info->ioctl_socket = socket( PF_PACKET, SOCK_DGRAM, 0 );
    if( 0 > thread_info->ioctl_socket )
    {
        DPRINTFE( "Unable to open a ioctl socket, errno=%s.",
                  strerror( errno ) );
        goto ERROR;
    }

    flags = fcntl( thread_info->ioctl_socket, F_GETFL, 0 );
    if( 0 > flags )
    {
        DPRINTFE( "Failed to get ioctl socket flags, error=%s.",
                  strerror( errno ) );
        close( thread_info->ioctl_socket );
        thread_info->ioctl_socket = -1;
        goto ERROR;
    }

    if( 0 > fcntl( thread_info->ioctl_socket, F_SETFL, flags | O_NONBLOCK ) )
    {
        DPRINTFE( "Failed to set ioctl socket flags, error=%s.",
                  strerror( errno ) );
        close( thread_info->ioctl_socket );
        thread_info->ioctl_socket = -1;
        goto ERROR;
    }

    if( NULL != callbacks )
    {
        error = sm_netlink_open( &(thread_info->netlink_receive_socket),
                                 RTMGRP_LINK | RTMGRP_IPV4_IFADDR | 
                                 RTMGRP_IPV6_IFADDR | RTMGRP_TC );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to open netlink receive socket, error=%s.",
                      sm_error_str( error ) );
            close( thread_info->ioctl_socket );
            thread_info->ioctl_socket = -1;
            goto ERROR;
        }

        error = sm_selobj_register( thread_info->netlink_receive_socket,
                                    sm_hw_netlink_dispatch, 0 );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to register netlink receive selection object, "
                      "error=%s.", sm_error_str( error ) );
            close( thread_info->ioctl_socket );
            thread_info->ioctl_socket = -1;
            sm_netlink_close( thread_info->netlink_receive_socket );
            thread_info->netlink_receive_socket = -1;
            goto ERROR;
        }

        memcpy( &(thread_info->callbacks), callbacks, sizeof(SmHwCallbacksT) );
    }

    thread_info->thread_id = pthread_self();
    thread_info->inuse = true;

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
// ***************************************************************************

// ***************************************************************************
// Hardware - Finalize
// ===================
SmErrorT sm_hw_finalize( void )
{
    SmHwThreadInfoT* thread_info;
    SmErrorT error;

    if( 0 != pthread_mutex_lock( &_mutex ) )
    {
        DPRINTFE( "Failed to capture mutex." );
        return( SM_FAILED );
    }

    thread_info = sm_hw_find_thread_info();
    if( NULL == thread_info )
    {
        DPRINTFE( "Failed to find thread information." );
        goto DONE;
    }

    if( -1 < thread_info->netlink_receive_socket )
    {
        error = sm_selobj_deregister( thread_info->netlink_receive_socket );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister netlink receive selection object, "
                      "error=%s.", sm_error_str( error ) );
        }

        sm_netlink_close( thread_info->netlink_receive_socket );
        thread_info->netlink_receive_socket = -1;
    }

    if( -1 < thread_info->ioctl_socket )
    {
        close( thread_info->ioctl_socket );
        thread_info->ioctl_socket = -1;
    }

DONE:
    if( 0 != pthread_mutex_unlock( &_mutex ) )
    {
        DPRINTFE( "Failed to release mutex." );
    }

    return( SM_OKAY );
}
// ***************************************************************************
