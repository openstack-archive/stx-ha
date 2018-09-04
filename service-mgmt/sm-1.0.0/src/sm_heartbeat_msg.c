//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_heartbeat_msg.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_selobj.h"
#include "sm_hw.h"
#include "sm_sha512.h"
#include "sm_failover.h"

#define SM_HEARTBEAT_MSG_MAX_SIZE                               128
#define SM_HEARTBEAT_MSG_BUFFER_MAX_SIZE                        512

typedef enum
{
    SM_HEARTBEAT_MSG_TYPE_UNKNOWN,
    SM_HEARTBEAT_MSG_TYPE_ALIVE,
} SmHeartbeatMsgTypeT;

typedef struct
{
    uint16_t version;
    uint16_t revision;
    uint16_t msg_len;
    uint16_t msg_type;
    char node_name[SM_NODE_NAME_MAX_CHAR];
    uint32_t auth_type;
    uint8_t auth_vector[SM_AUTHENTICATION_VECTOR_MAX_CHAR];
} __attribute__ ((packed)) SmHeartbeatMsgHeaderT;

typedef struct
{
} __attribute__ ((packed)) SmHeartbeatMsgAliveT;

typedef struct
{
    uint16_t    msg_size;
    uint32_t    if_state;
} __attribute__ ((packed)) SmHeartbeatMsgAliveRev2T;

typedef struct
{
    SmHeartbeatMsgHeaderT header;

    union
    {
        SmHeartbeatMsgAliveT alive;
        SmHeartbeatMsgAliveRev2T if_state_msg;
        char raw_msg[SM_HEARTBEAT_MSG_MAX_SIZE-sizeof(SmHeartbeatMsgHeaderT)];
    } u;
} __attribute__ ((packed)) SmHeartbeatMsgT;

static char _tx_control_buffer[SM_HEARTBEAT_MSG_BUFFER_MAX_SIZE] __attribute__((aligned));
static char _rx_control_buffer[SM_HEARTBEAT_MSG_BUFFER_MAX_SIZE] __attribute__((aligned));
static SmListT* _callbacks = NULL;

// ****************************************************************************
// Heartbeat Messaging - Register Callbacks
// ========================================
SmErrorT sm_heartbeat_msg_register_callbacks(
    SmHeartbeatMsgCallbacksT* callbacks )
{
    SM_LIST_PREPEND( _callbacks, (SmListEntryDataPtrT) callbacks );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Deregister Callbacks
// ==========================================
SmErrorT sm_heartbeat_msg_deregister_callbacks(
    SmHeartbeatMsgCallbacksT* callbacks )
{
    SM_LIST_REMOVE( _callbacks, (SmListEntryDataPtrT) callbacks );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Send message from source
// ==============================================
static int sm_heartbeat_msg_sendmsg_src_ipv6(int socket, void* msg, size_t msg_len,
    int flags, struct sockaddr_in6* dst_addr, struct in6_addr* src_Addr)
{
    struct msghdr msg_hdr = {};
    struct cmsghdr *cmsg;
    struct in6_pktinfo *pktinfo;
    struct iovec iov = {msg, msg_len};


    memset( &_tx_control_buffer, 0, sizeof(_tx_control_buffer) );
    msg_hdr.msg_iov = &iov;
    msg_hdr.msg_iovlen = 1;
    msg_hdr.msg_name = dst_addr;
    msg_hdr.msg_namelen = sizeof(struct sockaddr_in6);
    msg_hdr.msg_control = _tx_control_buffer;
    msg_hdr.msg_controllen = CMSG_LEN(sizeof(struct in6_pktinfo));
    cmsg = CMSG_FIRSTHDR(&msg_hdr);
    cmsg->cmsg_level = IPPROTO_IPV6;
    cmsg->cmsg_type = IPV6_PKTINFO;
    cmsg->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
    pktinfo = (struct in6_pktinfo*) CMSG_DATA(cmsg);
    pktinfo->ipi6_ifindex = 0;
    pktinfo->ipi6_addr = *src_Addr;

    return sendmsg( socket, &msg_hdr, flags );
}
// ****************************************************************************

// ****************************************************************************
// initialize socket
// ==============================================
static SmErrorT sm_initialize_socket(int& sock)
{
    int result;
    int flags;

    // Set socket to non-blocking.
    flags = fcntl( sock, F_GETFL, 0 );
    if( 0 > flags )
    {
        DPRINTFE( "Failed to get flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    if( 0 > fcntl( sock, F_SETFL, flags | O_NONBLOCK ) )
    {
        DPRINTFE( "Failed to set flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Close on exec.
    flags = fcntl( sock, F_GETFD, 0 );
    if( 0 > flags )
    {
        DPRINTFE( "Failed to get flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    if( 0 > fcntl( sock, F_SETFD, flags | FD_CLOEXEC ) )
    {
        DPRINTFE( "Failed to set flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Allow address reuse on socket.
    flags = 1;
    result = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR,
                         (void*) &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set reuseaddr socket option, "
                  "errno=%s.", strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Send Alive
// ================================
SmErrorT sm_heartbeat_msg_send_alive( SmNetworkTypeT network_type, char node_name[],
    SmNetworkAddressT* network_address, SmNetworkAddressT* dst_addr,
    int network_port, char interface_name[], SmAuthTypeT auth_type, char auth_key[],
    int sender_socket )
{
    struct sockaddr_in dst_addr4;
    struct sockaddr_in6 dst_addr6;
    SmHeartbeatMsgT heartbeat_msg;
    SmIpv4AddressT* ipv4_address = &(dst_addr->u.ipv4);
    SmIpv6AddressT* ipv6_address = &(dst_addr->u.ipv6);
    int result;

    memset( &heartbeat_msg, 0, sizeof(SmHeartbeatMsgT) );

    heartbeat_msg.header.version = htons(SM_VERSION);
    heartbeat_msg.header.revision = htons(SM_REVISION);
    heartbeat_msg.header.msg_len = htons(sizeof(SmHeartbeatMsgT));
    heartbeat_msg.header.msg_type = htons(SM_HEARTBEAT_MSG_TYPE_ALIVE);
    snprintf( heartbeat_msg.header.node_name,
              sizeof(heartbeat_msg.header.node_name), "%s", node_name );

    heartbeat_msg.u.if_state_msg.msg_size = htons((uint16_t)sizeof(SmHeartbeatMsgAliveRev2T));
    SmHeartbeatMsgIfStateT if_state;
    if_state =  sm_failover_if_state_get();
    heartbeat_msg.u.if_state_msg.if_state = htonl(if_state);

    if( SM_AUTH_TYPE_HMAC_SHA512 == auth_type )
    {
        SmSha512HashT hash;

        heartbeat_msg.header.auth_type = htonl(auth_type);
        sm_sha512_hmac( &heartbeat_msg, sizeof(heartbeat_msg), auth_key,
                        strlen(auth_key), &hash );
        memcpy( &(heartbeat_msg.header.auth_vector[0]), &(hash.bytes[0]),
                SM_SHA512_HASH_SIZE );
    } else {
        heartbeat_msg.header.auth_type = htonl(SM_AUTH_TYPE_NONE);
    }

    if( SM_NETWORK_TYPE_IPV4_UDP == network_type )
    {
        memset( &dst_addr4, 0, sizeof(dst_addr4) );

        dst_addr4.sin_family = AF_INET;
        dst_addr4.sin_port = htons(network_port);
        dst_addr4.sin_addr.s_addr = ipv4_address->sin.s_addr;

        result = sendto( sender_socket, &heartbeat_msg, sizeof(SmHeartbeatMsgT),
                         0, (struct sockaddr *) &dst_addr4, sizeof(dst_addr4) );
        if( 0 > result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );

    } else if( SM_NETWORK_TYPE_IPV6_UDP == network_type ) {
        memset( &dst_addr6, 0, sizeof(dst_addr6) );

        dst_addr6.sin6_family = AF_INET6;
        dst_addr6.sin6_port = htons(network_port);
        dst_addr6.sin6_addr = ipv6_address->sin6;

        result = sm_heartbeat_msg_sendmsg_src_ipv6( sender_socket, (void*) &heartbeat_msg,
        	                                    sizeof(SmHeartbeatMsgT),0, &dst_addr6,
        	                                    &network_address->u.ipv6.sin6 );

        if( 0 > result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );
    } else {
        DPRINTFE( "Unsupported network type (%s).",
                  sm_network_type_str( network_type ) );
        return( SM_FAILED );
    }
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Get Ancillary Data 
// ========================================
static void* sm_heartbeat_msg_get_ancillary_data( struct msghdr* msg_hdr,
    int cmsg_level, int cmsg_type )
{
    struct cmsghdr* cmsg;

    for( cmsg = CMSG_FIRSTHDR(msg_hdr); NULL != cmsg;
         cmsg = CMSG_NXTHDR( msg_hdr, cmsg) )
    {
        if(( cmsg_level == cmsg->cmsg_level )&&
           ( cmsg_type == cmsg->cmsg_type ))
        {
            return( CMSG_DATA(cmsg) );
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Dispatch Message
// ======================================
static void sm_heartbeat_msg_dispatch_msg( SmHeartbeatMsgT* heartbeat_msg,
    SmNetworkAddressT* network_address,
    int network_port, char* interface_name )
{
    SmHeartbeatMsgCallbacksT* callbacks;
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;

    if( SM_VERSION != ntohs(heartbeat_msg->header.version) )
    {
        DPRINTFD( "Received unsupported version (%i).",
                  ntohs(heartbeat_msg->header.version) );
        return;
    }

    if( SM_AUTH_TYPE_HMAC_SHA512 == ntohl(heartbeat_msg->header.auth_type) )
    {
        uint8_t auth_vector[SM_AUTHENTICATION_VECTOR_MAX_CHAR];

        memcpy( auth_vector, &(heartbeat_msg->header.auth_vector[0]),
                SM_AUTHENTICATION_VECTOR_MAX_CHAR );
        memset( &(heartbeat_msg->header.auth_vector[0]), 0,
                SM_AUTHENTICATION_VECTOR_MAX_CHAR );

        SM_LIST_FOREACH( _callbacks, entry, entry_data )
        {
            callbacks = (SmHeartbeatMsgCallbacksT*) entry_data;

            if( NULL == callbacks->auth )
                continue;

            if( !(callbacks->auth( interface_name, network_address,
                                   network_port, heartbeat_msg,
                                   sizeof(SmHeartbeatMsgT), auth_vector )) )
            {
                DPRINTFD( "Authentication check failed on message (%i) from "
                          "node (%s).", ntohs(heartbeat_msg->header.msg_type),
                          heartbeat_msg->header.node_name );
                return;
            } else {
                DPRINTFD( "Authentication check passed on message (%i) from "
                          "node (%s).", ntohs(heartbeat_msg->header.msg_type),
                          heartbeat_msg->header.node_name );
            }
        }
    }

    uint16_t                msg_size;
    SmHeartbeatMsgIfStateT  if_state = 0;
    bool        perform_if_exg = false;
    if(2 <= ntohs(heartbeat_msg->header.revision))
    {
        msg_size = ntohs(heartbeat_msg->u.if_state_msg.msg_size);
        if(sizeof(SmHeartbeatMsgAliveRev2T) == msg_size )
        {
            if_state = ntohl(heartbeat_msg->u.if_state_msg.if_state);
            perform_if_exg = true;
        }else
        {
            DPRINTFE("Invalid interface state package. size (%d) vs expected (%d)",
                msg_size, sizeof(SmHeartbeatMsgAliveRev2T));
        }
    }

    switch( ntohs(heartbeat_msg->header.msg_type) )
    {
        case SM_HEARTBEAT_MSG_TYPE_ALIVE:
            DPRINTFD( "Alive message received." );

            SM_LIST_FOREACH( _callbacks, entry, entry_data )
            {
                callbacks = (SmHeartbeatMsgCallbacksT*) entry_data;

                if( NULL != callbacks->alive )
                {
                    callbacks->alive( heartbeat_msg->header.node_name,
                                      network_address, network_port,
                                      ntohs(heartbeat_msg->header.version),
                                      ntohs(heartbeat_msg->header.revision),
                                      interface_name );
                }

                if(perform_if_exg)
                {
                    if(NULL != callbacks->if_state)
                    {
                        callbacks->if_state(heartbeat_msg->header.node_name,
                                            if_state);
                    }
                    else
                    {
                        DPRINTFD("No handler for if state package");
                    }
                }
            }
        break;

        default:
            DPRINTFI( "Unsupported message type (%i) received.",
                      ntohs(heartbeat_msg->header.msg_type) );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Dispatch Ipv4 Udp
// =======================================
static void sm_heartbeat_msg_dispatch_ipv4_udp( int selobj, int64_t unused )
{
    int network_port;
    SmNetworkAddressT network_address;
    char interface_name[SM_INTERFACE_NAME_MAX_CHAR];
    SmHeartbeatMsgT heartbeat_msg;
    SmErrorT error;
    struct msghdr msg_hdr;
    struct sockaddr_in src_addr;
    struct iovec iovec = {(void*)&heartbeat_msg, sizeof(heartbeat_msg)};
    int bytes_read;

    memset( _rx_control_buffer, 0, sizeof(_rx_control_buffer) );
    memset( &heartbeat_msg, 0, sizeof(SmHeartbeatMsgT) );
    memset( &msg_hdr, 0, sizeof(msg_hdr) );

    msg_hdr.msg_name = &src_addr;
    msg_hdr.msg_namelen= sizeof(src_addr);
    msg_hdr.msg_iov = &iovec;
    msg_hdr.msg_iovlen = 1;
    msg_hdr.msg_control = _rx_control_buffer;
    msg_hdr.msg_controllen = sizeof(_rx_control_buffer);

    int retry_i;

    for( retry_i = 5; retry_i != 0; --retry_i )
    {
        bytes_read = recvmsg( selobj, &msg_hdr, MSG_NOSIGNAL | MSG_DONTWAIT );
        if( 0 < bytes_read )
        {
            break;

        } else if( 0 == bytes_read ) {
            // For connection oriented sockets, this indicates that the peer 
            // has performed an orderly shutdown.
            return;

        } else if(( 0 > bytes_read )&&( EINTR != errno )) {
            DPRINTFE( "Failed to receive message, errno=%s.",
                      strerror( errno ) );
            return;
        }

        DPRINTFD( "Interrupted while receiving message, retry=%d, errno=%s.",
                  retry_i, strerror( errno ) );
    }

    if( AF_INET == src_addr.sin_family )
    {
        char network_address_str[SM_NETWORK_ADDRESS_MAX_CHAR];
        struct in_pktinfo* pkt_info;

        pkt_info = (struct in_pktinfo*) 
            sm_heartbeat_msg_get_ancillary_data( &msg_hdr, SOL_IP, IP_PKTINFO );
        if( NULL == pkt_info )
        {
            DPRINTFD( "No packet information available." );
            return;
        }

        error = sm_hw_get_if_name( pkt_info->ipi_ifindex, interface_name );
        if( SM_OKAY != error )
        {
            if( SM_NOT_FOUND == error )
            {
                DPRINTFD( "Failed to get interface name for interface index "
                          "(%i), error=%s.", pkt_info->ipi_ifindex,
                          sm_error_str(error) );
            } else {
                DPRINTFE( "Failed to get interface name for interface index "
                          "(%i), error=%s.", pkt_info->ipi_ifindex,
                          sm_error_str(error) );
            }
            return;
        }

        network_address.type = SM_NETWORK_TYPE_IPV4_UDP;
        network_address.u.ipv4.sin.s_addr = src_addr.sin_addr.s_addr;
        network_port = ntohs(src_addr.sin_port);

        sm_network_address_str( &network_address, network_address_str );

        DPRINTFD( "Received message from ip (%s), port (%i) on "
                  "interface (%s).", network_address_str, network_port,
                  interface_name );

    } else {
        DPRINTFE( "Received unsupported network address type (%i).",
                  src_addr.sin_family );
        return;
    }

    // Once the message is received, pass it to protocol-independent handler
    sm_heartbeat_msg_dispatch_msg( &heartbeat_msg, &network_address,
                                   network_port, interface_name );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Dispatch IPv6 Udp
// =======================================
static void sm_heartbeat_msg_dispatch_ipv6_udp( int selobj, int64_t unused )
{
    int network_port;
    SmNetworkAddressT network_address;
    char interface_name[SM_INTERFACE_NAME_MAX_CHAR];
    SmHeartbeatMsgT heartbeat_msg;
    SmErrorT error;
    struct msghdr msg_hdr;
    struct sockaddr_in6 src_addr;
    struct iovec iovec = {(void*)&heartbeat_msg, sizeof(heartbeat_msg)};
    int bytes_read;

    memset( _rx_control_buffer, 0, sizeof(_rx_control_buffer) );
    memset( &heartbeat_msg, 0, sizeof(SmHeartbeatMsgT) );
    memset( &msg_hdr, 0, sizeof(msg_hdr) );

    msg_hdr.msg_name = &src_addr;
    msg_hdr.msg_namelen= sizeof(src_addr);
    msg_hdr.msg_iov = &iovec;
    msg_hdr.msg_iovlen = 1;
    msg_hdr.msg_control = _rx_control_buffer;
    msg_hdr.msg_controllen = sizeof(_rx_control_buffer);

    int retry_i;
    for( retry_i = 5; retry_i != 0; --retry_i )
    {
        bytes_read = recvmsg( selobj, &msg_hdr, MSG_NOSIGNAL | MSG_DONTWAIT );
        if( 0 < bytes_read )
        {
            break;

        } else if( 0 == bytes_read ) {
            // For connection oriented sockets, this indicates that the peer
            // has performed an orderly shutdown.
            return;

        } else if(( 0 > bytes_read )&&( EINTR != errno )) {
            DPRINTFE( "Failed to receive message, errno=%s.",
                      strerror( errno ) );
            return;
        }

        DPRINTFD( "Interrupted while receiving message, retry=%d, errno=%s.",
                  retry_i, strerror( errno ) );
    }

    if( AF_INET6 == src_addr.sin6_family )
    {
        char network_address_str[SM_NETWORK_ADDRESS_MAX_CHAR];
        struct in6_pktinfo* pkt_info;

        pkt_info = (struct in6_pktinfo*)
            sm_heartbeat_msg_get_ancillary_data( &msg_hdr, SOL_IPV6,
                                                 IPV6_PKTINFO );
        if( NULL == pkt_info )
        {
            DPRINTFD( "No packet information available." );
            return;
        }

        error = sm_hw_get_if_name( pkt_info->ipi6_ifindex, interface_name );
        if( SM_OKAY != error )
        {
            if( SM_NOT_FOUND == error )
            {
                DPRINTFD( "Failed to get interface name for interface index "
                          "(%i), error=%s.", pkt_info->ipi6_ifindex,
                          sm_error_str(error) );
            } else {
                DPRINTFE( "Failed to get interface name for interface index "
                          "(%i), error=%s.", pkt_info->ipi6_ifindex,
                          sm_error_str(error) );
            }
            return;
        }

        network_address.type = SM_NETWORK_TYPE_IPV6_UDP;
        network_address.u.ipv6.sin6 = src_addr.sin6_addr;
        network_port = ntohs(src_addr.sin6_port);

        sm_network_address_str( &network_address, network_address_str );

        DPRINTFD( "Received message from ip (%s), port (%i) on "
                  "interface (%s).", network_address_str, network_port,
                  interface_name );

    } else {
        DPRINTFE( "Received unsupported network address type (%i).",
                  src_addr.sin6_family );
        return;
    }

    // Once the message is received, pass it to protocol-independent handler
    sm_heartbeat_msg_dispatch_msg( &heartbeat_msg, &network_address,
                                   network_port, interface_name );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Open Ipv4 UDP Multicast Socket
// ====================================================
static SmErrorT sm_heartbeat_msg_open_ipv4_udp_multicast_socket(
    SmNetworkAddressT* network_address, SmNetworkAddressT* network_multicast,
    int network_port, char interface_name[], int* multicast_socket )
{
    int flags;
    int sock;
    int result;
    struct ifreq ifr;
    struct sockaddr_in addr;
    SmIpv4AddressT* ipv4_unicast = &(network_address->u.ipv4);
    SmIpv4AddressT* ipv4_multicast =  &(network_multicast->u.ipv4);

    *multicast_socket = -1;

    // Create socket.
    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to open socket for interface (%s), errno=%s.",
                  interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    SmErrorT error = sm_initialize_socket(sock);
    if( SM_OKAY != error )
    {
        DPRINTFI("Initialize socket on interface %s failed.", interface_name );
        return error;
    }

    // Bind address to socket.
    memset( &addr, 0, sizeof(addr) );

    addr.sin_family = AF_INET;
    addr.sin_port = htons(network_port);
    addr.sin_addr.s_addr = ipv4_multicast->sin.s_addr;

    result = bind( sock, (struct sockaddr *) &addr, sizeof(addr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind multicast address to socket for "
                  "interface (%s), error=%s.", interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Set multicast interface on socket.
    result = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_IF,
                        (struct in_addr*) &(ipv4_unicast->sin.s_addr),
                        sizeof(struct in_addr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set unicast address on socket for "
                  "interface (%s), error=%s.", interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind socket to interface.
    memset(&ifr, 0, sizeof(ifr));
    snprintf( ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface_name );
    
    result = setsockopt( sock, SOL_SOCKET, SO_BINDTODEVICE,
                         (void *)&ifr, sizeof(ifr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind socket to interface (%s), errno=%s.",
                  interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set multicast TTL.
    flags = 1;
    result = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_TTL, &flags,
                         sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set multicast ttl for interface (%s), errno=%s.",
                  interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Disable looping of sent multicast packets on socket.
    flags = 0;
    result = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_LOOP,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to stop looping of multicast messages for "
                  "interface (%s), errno=%s.", interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket send priority on interface.
    flags = IPTOS_CLASS_CS6;
    result = setsockopt( sock, IPPROTO_IP, IP_TOS, &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "multicast messages, errno=%s.", interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    flags = 6;
    result = setsockopt( sock, SOL_SOCKET, SO_PRIORITY, &flags,
                         sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "multicast messages, errno=%s.", interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket receive packet information.
    flags = 1;
    result = setsockopt( sock, SOL_IP, IP_PKTINFO, &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket receive packet information for "
                  "interface (%s) multicast messages, errno=%s.",
                  interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    *multicast_socket = sock;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Open Ipv4 UDP unicast Socket
// ====================================================
static SmErrorT sm_heartbeat_msg_open_ipv4_udp_unicast_socket(
    SmNetworkAddressT* network_address,
    int network_port, char interface_name[], int* unicast_socket )
{
    int flags;
    int sock;
    int result;
    struct ifreq ifr;
    struct sockaddr_in addr;
    SmIpv4AddressT* ipv4_unicast = &(network_address->u.ipv4);

    *unicast_socket = -1;

    // Create socket.
    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to open socket for interface (%s), errno=%s.",
                  interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    SmErrorT error = sm_initialize_socket(sock);
    if( SM_OKAY != error )
    {
        DPRINTFI("Initialize socket on interface %s failed.", interface_name );
        return error;
    }

    // Bind address to socket.
    memset( &addr, 0, sizeof(addr) );

    addr.sin_family = AF_INET;
    addr.sin_port = htons(network_port);
    addr.sin_addr.s_addr = ipv4_unicast->sin.s_addr;

    result = bind( sock, (struct sockaddr *) &addr, sizeof(addr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind address to socket for "
                  "interface (%s), error=%s.", interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind socket to interface.
    memset(&ifr, 0, sizeof(ifr));
    snprintf( ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface_name );

    result = setsockopt( sock, SOL_SOCKET, SO_BINDTODEVICE,
                         (void *)&ifr, sizeof(ifr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind socket to interface (%s), errno=%s.",
                  interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket send priority on interface.
    flags = IPTOS_CLASS_CS6;
    result = setsockopt( sock, IPPROTO_IP, IP_TOS, &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "messages, errno=%s.", interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    flags = 6;
    result = setsockopt( sock, SOL_SOCKET, SO_PRIORITY, &flags,
                         sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "messages, errno=%s.", interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket receive packet information.
    flags = 1;
    result = setsockopt( sock, SOL_IP, IP_PKTINFO, &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket receive packet information for "
                  "interface (%s) messages, errno=%s.",
                  interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    *unicast_socket = sock;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Open Ipv6 UDP Multicast Socket
// ====================================================
static SmErrorT sm_heartbeat_msg_open_ipv6_udp_multicast_socket(
    SmNetworkAddressT* network_address, SmNetworkAddressT* network_multicast,
    int network_port, char interface_name[], int* multicast_socket )
{
    int flags;
    int sock;
    int result;
    int64_t ifindex;
    struct ifreq ifr;
    struct sockaddr_in6 addr;
    SmIpv6AddressT* ipv6_multicast =  &(network_multicast->u.ipv6);
    *multicast_socket = -1;

    // Create socket.
    sock = socket( AF_INET6, SOCK_DGRAM, IPPROTO_UDP );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to open socket for interface (%s), errno=%s.",
                  interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    SmErrorT error = sm_initialize_socket(sock);
    if( SM_OKAY != error )
    {
        DPRINTFI("Initialize socket on interface %s failed.", interface_name );
        return error;
    }

    // Bind address to socket.
    memset( &addr, 0, sizeof(addr) );

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(network_port);
    addr.sin6_addr = ipv6_multicast->sin6;

    result = bind( sock, (struct sockaddr *) &addr, sizeof(addr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind multicast address to socket for "
                  "interface (%s), error=%s.", interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Set multicast interface on socket.
    ifindex = if_nametoindex( interface_name );
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                        &ifindex, sizeof(ifindex) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set unicast address on socket for "
                  "interface (%s), error=%s.", interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind socket to interface.
    memset( &ifr, 0, sizeof(ifr) );
    snprintf( ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface_name );

    result = setsockopt( sock, SOL_SOCKET, SO_BINDTODEVICE,
                         (void *)&ifr, sizeof(ifr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind socket to interface (%s), errno=%s.",
                  interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set multicast TTL.
    flags = 1;
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &flags,
                         sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set multicast ttl for interface (%s), errno=%s.",
                  interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Disable looping of sent multicast packets on socket.
    flags = 0;
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to stop looping of multicast messages for "
                  "interface (%s), errno=%s.", interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket send priority on interface.
    flags = IPTOS_CLASS_CS6;
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_TCLASS, &flags,
                         sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "multicast messages, errno=%s.", interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    flags = 6;
    result = setsockopt( sock, SOL_SOCKET, SO_PRIORITY, &flags,
                         sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "multicast messages, errno=%s.", interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket receive packet information.
    flags = 1;
    result = setsockopt( sock, SOL_IPV6, IPV6_RECVPKTINFO, &flags,
                         sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket receive packet information for "
                  "interface (%s) multicast messages, errno=%s.",
                  interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    *multicast_socket = sock;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Open Ipv6 UDP Unicast Socket
// ====================================================
static SmErrorT sm_heartbeat_msg_open_ipv6_udp_unicast_socket(
    SmNetworkAddressT* network_address,
    int network_port, char interface_name[], int* multicast_socket )
{
    int flags;
    int sock;
    int result;
    struct ifreq ifr;
    struct sockaddr_in6 addr;
    SmIpv6AddressT* ipv6_unicast =  &(network_address->u.ipv6);
    *multicast_socket = -1;

    // Create socket.
    sock = socket( AF_INET6, SOCK_DGRAM, IPPROTO_UDP );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to open socket for interface (%s), errno=%s.",
                  interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    SmErrorT error = sm_initialize_socket(sock);
    if( SM_OKAY != error )
    {
        DPRINTFI("Initialize socket on interface %s failed.", interface_name );
        return error;
    }

    // Bind address to socket.
    memset( &addr, 0, sizeof(addr) );

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(network_port);
    addr.sin6_addr = ipv6_unicast->sin6;

    result = bind( sock, (struct sockaddr *) &addr, sizeof(addr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind multicast address to socket for "
                  "interface (%s), error=%s.", interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    if( 0 > result )
    {
        DPRINTFE( "Failed to set unicast address on socket for "
                  "interface (%s), error=%s.", interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind socket to interface.
    memset( &ifr, 0, sizeof(ifr) );
    snprintf( ifr.ifr_name, sizeof(ifr.ifr_name), "%s", interface_name );

    result = setsockopt( sock, SOL_SOCKET, SO_BINDTODEVICE,
                         (void *)&ifr, sizeof(ifr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind socket to interface (%s), errno=%s.",
                  interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set multicast TTL.
    flags = 1;
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &flags,
                         sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set multicast ttl for interface (%s), errno=%s.",
                  interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Disable looping of sent multicast packets on socket.
    flags = 0;
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to stop looping of multicast messages for "
                  "interface (%s), errno=%s.", interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket send priority on interface.
    flags = IPTOS_CLASS_CS6;
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_TCLASS, &flags,
                         sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "multicast messages, errno=%s.", interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    flags = 6;
    result = setsockopt( sock, SOL_SOCKET, SO_PRIORITY, &flags,
                         sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "multicast messages, errno=%s.", interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket receive packet information.
    flags = 1;
    result = setsockopt( sock, SOL_IPV6, IPV6_RECVPKTINFO, &flags,
                         sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket receive packet information for "
                  "interface (%s) multicast messages, errno=%s.",
                  interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    *multicast_socket = sock;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Open Sockets
// ==================================
SmErrorT sm_heartbeat_msg_open_sockets( SmNetworkTypeT network_type,
    SmNetworkAddressT* network_address, SmNetworkAddressT* network_multicast,
    int network_port, char interface_name[], int* multicast_socket, int* unicast_socket )
{
    SmErrorT error;

    if( -1 < *multicast_socket )
    {
        DPRINTFD( "Multicast socket for interface (%s) already opened.",
                  interface_name );
        return( SM_OKAY );
    }

    *multicast_socket = -1;

    if( SM_NETWORK_TYPE_IPV4_UDP == network_type )
    {
        int socket_fd;

        if ( network_multicast->type != SM_NETWORK_TYPE_NIL )
        {
            error = sm_heartbeat_msg_open_ipv4_udp_multicast_socket(
                            network_address, network_multicast, network_port,
                            interface_name, &socket_fd );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to open multicast socket for interface (%s), "
                          "error=%s.", interface_name, sm_error_str(error) );
                return( error );
            }

            error = sm_selobj_register( socket_fd,
                                        sm_heartbeat_msg_dispatch_ipv4_udp,
                                        (int64_t) 0 );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to register selection object for "
                          "interface (%s), error=%s.", interface_name,
                          sm_error_str(error) );
                close( socket_fd );
                return( SM_FAILED );
            }

            *multicast_socket = socket_fd;
        } else
        {
            DPRINTFD("Multicast ip for %s is not configured.", interface_name);
            *multicast_socket = -1;
        }

        if( NULL != unicast_socket )
        {
            error = sm_heartbeat_msg_open_ipv4_udp_unicast_socket(
                            network_address, network_port,
                            interface_name, &socket_fd );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to open multicast socket for interface (%s), "
                          "error=%s.", interface_name, sm_error_str(error) );
                return( error );
            }

            error = sm_selobj_register( socket_fd,
                                        sm_heartbeat_msg_dispatch_ipv4_udp, (int64_t) 0);
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to register selection object for "
                          "interface (%s), error=%s.", interface_name,
                          sm_error_str(error) );
                close( socket_fd );
                return( SM_FAILED );
            }

            *unicast_socket = socket_fd;
         }


        return( SM_OKAY );

    } else if( SM_NETWORK_TYPE_IPV6_UDP == network_type )
    {
        int socket_fd;

        if ( network_multicast->type != SM_NETWORK_TYPE_NIL )
        {
            error = sm_heartbeat_msg_open_ipv6_udp_multicast_socket(
                            network_address, network_multicast, network_port,
                            interface_name, &socket_fd );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to open multicast socket for interface (%s), "
                          "error=%s.", interface_name, sm_error_str(error) );
                return( error );
            }

            error = sm_selobj_register( socket_fd,
                                        sm_heartbeat_msg_dispatch_ipv6_udp, 0 );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to register selection object for "
                          "interface (%s), error=%s.", interface_name,
                          sm_error_str(error) );
                close( socket_fd );
                return( SM_FAILED );
            }

            *multicast_socket = socket_fd;
        } else
        {
            DPRINTFD("Multicast ip for %s is not configured.", interface_name);
            *multicast_socket = -1;
        }

        if( NULL != unicast_socket )
        {
            error = sm_heartbeat_msg_open_ipv6_udp_unicast_socket(
                            network_address, network_port,
                            interface_name, &socket_fd );

            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to open unicast socket for interface (%s), "
                          "error=%s.", interface_name, sm_error_str(error) );
                return( error );
            }

            error = sm_selobj_register( socket_fd,
                                        sm_heartbeat_msg_dispatch_ipv6_udp, 0 );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to register selection object for "
                          "interface (%s), error=%s.", interface_name,
                          sm_error_str(error) );
                close( socket_fd );
                return( SM_FAILED );
            }

            *unicast_socket = socket_fd;
         }
        return( SM_OKAY );

    } else {
        DPRINTFE( "Unsupported network type (%s).",
                  sm_network_type_str( network_type ) );
        return( SM_FAILED );
    }
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Close Sockets
// ===================================
SmErrorT sm_heartbeat_msg_close_sockets( int* multicast_socket )
{
    SmErrorT error;

    if( -1 < *multicast_socket )
    {
        error = sm_selobj_deregister( *multicast_socket );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object, error=%s.",
                      sm_error_str(error) );
        }

        close( *multicast_socket );
        *multicast_socket = -1;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat Message - Initialize
// ==============================
SmErrorT sm_heartbeat_msg_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeart Message - Finalize
// =============================
SmErrorT sm_heartbeat_msg_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
