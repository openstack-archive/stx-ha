//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_netlink.h"

#include "sm_types.h"
#include "sm_debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>

#include <linux/rtnetlink.h>

#define SM_NETLINK_TX_MSG_MAX_LEN 2048
#define SM_NETLINK_RX_MSG_MAX_LEN 4096

#define SM_NETLINK_MAX_SOCKET_SIZE 2097152

static char _tx_buffer[SM_NETLINK_TX_MSG_MAX_LEN] __attribute__((aligned));
static char _rx_buffer[SM_NETLINK_RX_MSG_MAX_LEN] __attribute__((aligned));

// ***************************************************************************
// Netlink - Get Tx Buffer
// =======================
void* sm_netlink_get_tx_buffer( void )
{
    memset( _tx_buffer, 0, sizeof(_tx_buffer) );
    return( &(_tx_buffer[0]) );
}
// ***************************************************************************

// ***************************************************************************
// Netlink - Set Blocking
// ======================
static SmErrorT sm_netlink_set_blocking( int socket_fd, bool block )
{
    int flags;

    flags = fcntl( socket_fd, F_GETFL, 0 );
    if( 0 > flags )
    {
        DPRINTFE( "Failed to get flags, error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }

    if( block )
    {
        flags &= ~O_NONBLOCK;
    } else {
        flags |= O_NONBLOCK;
    }

    if( 0 > fcntl( socket_fd, F_SETFL, flags ) )
    {
        DPRINTFE( "Failed to set flags, error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ***************************************************************************

// ***************************************************************************
// Netlink - Add Attribute 
// =======================
void sm_netlink_add_attribute( struct nlmsghdr* payload, int type,
    void* data, int data_length )
{
	struct rtattr* rta;
	int len = RTA_LENGTH( data_length );

	rta = (struct rtattr*) 
          (((char*) payload) + NLMSG_ALIGN( payload->nlmsg_len ));
	rta->rta_type = type;
	rta->rta_len  = len;
	memcpy( RTA_DATA( rta ), data, data_length );
	payload->nlmsg_len = NLMSG_ALIGN( payload->nlmsg_len ) + len;
}
// ***************************************************************************

// ***************************************************************************
// Netlink - Send 
// ==============
SmErrorT sm_netlink_send( int socket_fd, struct nlmsghdr* payload )
{
    struct sockaddr_nl address;
    struct iovec io_vec;
    struct msghdr msg_hdr;

    memset( &address, 0, sizeof(address) );
    address.nl_family = AF_NETLINK;
    address.nl_pad    = 0; // unused
    address.nl_pid    = 0; // kernel 
    address.nl_groups = 0; // kernel notifications

    memset( &io_vec, 0, sizeof(io_vec) );
    io_vec.iov_base = (void*) payload;
    io_vec.iov_len  = payload->nlmsg_len;

    memset( &msg_hdr, 0, sizeof(msg_hdr) );
    msg_hdr.msg_name       = &address;
    msg_hdr.msg_namelen    = sizeof(address);
    msg_hdr.msg_iov        = &io_vec;
    msg_hdr.msg_iovlen     = 1;
    msg_hdr.msg_control    = NULL;
    msg_hdr.msg_controllen = 0;
    msg_hdr.msg_flags      = 0;

    if( 0 > sendmsg( socket_fd, &msg_hdr, 0 ) )
    {
        DPRINTFE( "Send failed, error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }
    
    return( SM_OKAY );
}
// ***************************************************************************

// ***************************************************************************
// Netlink - Send Request
// ======================
SmErrorT sm_netlink_send_request( int socket_fd, struct nlmsghdr* payload,
    void* request, int request_len )
{
    struct sockaddr_nl address;
    struct iovec io_vec[2];
    struct msghdr msg_hdr;

    memset( &address, 0, sizeof(address) );
    address.nl_family = AF_NETLINK;
    address.nl_pad    = 0; // unused
    address.nl_pid    = 0; // kernel 
    address.nl_groups = 0; // kernel notifications

    memset( &io_vec, 0, sizeof(io_vec) );
    io_vec[0].iov_base = (void*) payload;
    io_vec[0].iov_len  = payload->nlmsg_len;
    io_vec[1].iov_base = (void*) request;
    io_vec[1].iov_len  = request_len;

    memset( &msg_hdr, 0, sizeof(msg_hdr) );
    msg_hdr.msg_name       = &address;
    msg_hdr.msg_namelen    = sizeof(address);
    msg_hdr.msg_iov        = &(io_vec[0]);
    msg_hdr.msg_iovlen     = 2;
    msg_hdr.msg_control    = NULL;
    msg_hdr.msg_controllen = 0;
    msg_hdr.msg_flags      = 0;

    if( 0 > sendmsg( socket_fd, &msg_hdr, 0 ) )
    {
        DPRINTFE( "Send failed, error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }
    
    return( SM_OKAY );
}
// ***************************************************************************

// ***************************************************************************
// Netlink - Receive 
// =================
SmErrorT sm_netlink_receive( int socket_fd, SmNetlinkCallbackT callback,
    void* invocation_data )
{
    struct sockaddr_nl address;
    struct msghdr msg_hdr;
    struct iovec io_vec;
    struct nlmsghdr* payload;

    int retry_i;
    for( retry_i = 10; 0 != retry_i; --retry_i )
    {
        memset( &address, 0, sizeof(struct sockaddr_nl) );

        address.nl_family = AF_NETLINK;
        address.nl_pad    = 0;
        address.nl_pid    = 0;
        address.nl_groups = 0;

        memset( &io_vec, 0, sizeof(io_vec) );
        memset( &msg_hdr, 0, sizeof(msg_hdr) );
        memset( _rx_buffer, 0, sizeof(_rx_buffer) );

        io_vec.iov_base      = _rx_buffer;
        io_vec.iov_len       = sizeof(_rx_buffer);

        msg_hdr.msg_name       = (void *) &address;
        msg_hdr.msg_namelen    = sizeof(address);
        msg_hdr.msg_iov        = &io_vec;
        msg_hdr.msg_iovlen     = 1;
        msg_hdr.msg_control    = NULL;
        msg_hdr.msg_controllen = 0;
        msg_hdr.msg_flags      = 0;

        unsigned int length = recvmsg( socket_fd, &msg_hdr, 0 );
        if( 0 > length ) 
        {
            if( EINTR == errno )
            {
                DPRINTFD( "Message receive interrupted, retry=%d, "
                          "errno=%s.", retry_i, strerror( errno ) );
                continue;

            } else if( EAGAIN == errno ) {
                DPRINTFD( "Message receive error, error=%s", strerror( errno ) );
                return( SM_NO_MSG );
            }

            DPRINTFE( "Message receive error, errno=%s.", strerror(errno) );
            return( SM_FAILED );

        } else if( 0 == length ) {
            DPRINTFD( "No messages received." );
            return( SM_NO_MSG );
        } 

        DPRINTFD( "Netlink message received." );

        for( payload = (struct nlmsghdr*) _rx_buffer;
             NLMSG_OK( payload, length );
             payload = NLMSG_NEXT( payload, length )
           )
        {
            if( NLMSG_ERROR == payload->nlmsg_type  )
            {
                struct nlmsgerr* err_msg;
                err_msg = (struct nlmsgerr*) NLMSG_DATA( payload );

                if( 0 == err_msg->error )
                {
                    if( payload->nlmsg_flags & NLM_F_MULTI )
                    {
                        // Multi part message need to continue.
                        continue;
                    }

                    DPRINTFD( "Netlink ack received." );
                    return( SM_OKAY );
                }

                if(( -EEXIST == err_msg->error )&&
                    (( RTM_NEWROUTE == err_msg->msg.nlmsg_type )||
                     ( RTM_NEWADDR  == err_msg->msg.nlmsg_type )))
                {
                    DPRINTFD( "Netlink error message received, msg_pid=%i, "
                              "msg_type=%i, msg_seq_num=%i, errno=%s.", 
                              err_msg->msg.nlmsg_pid, err_msg->msg.nlmsg_type,
                              err_msg->msg.nlmsg_seq, 
                              strerror( err_msg->error ) );
                    return( SM_OKAY );
                }
                
                DPRINTFE( "Netlink error message received, msg_pid=%i, "
                          "msg_type=%i, msg_seq_num=%i, errno=%s.", 
                          err_msg->msg.nlmsg_pid, err_msg->msg.nlmsg_type,
                          err_msg->msg.nlmsg_seq, strerror( err_msg->error ) );
                return( SM_FAILED );
            }

            if( NLMSG_OVERRUN == payload->nlmsg_type  )
            {
                DPRINTFE( "Netlink overrun message received." );
                return( SM_FAILED );
            }

            if( NLMSG_NOOP == payload->nlmsg_type  )
            {
                DPRINTFE( "Netlink no-op message received." );
                return( SM_NO_MSG );
            }

            if( NLMSG_DONE == payload->nlmsg_type  )
            {
                DPRINTFD( "Netlink done message received." );
                return( SM_OKAY );
            }

            if( NULL != callback )
            {
                callback( socket_fd, &address, payload, invocation_data );
            }
        }
    }

    return( SM_OKAY );
}
// ***************************************************************************

// ***************************************************************************
// Netlink - Command
// =================
SmErrorT sm_netlink_command( int socket_fd, struct nlmsghdr* payload,
    SmNetlinkCallbackT callback, void* invocation_data )
{
    struct sockaddr_nl address;
    struct iovec io_vec;
    struct msghdr msg_hdr;
    SmErrorT error, error2;

    memset( &address, 0, sizeof(address) );
    address.nl_family = AF_NETLINK;
    address.nl_pad    = 0; // unused
    address.nl_pid    = 0; // kernel 
    address.nl_groups = 0; // kernel notifications

    memset( &io_vec, 0, sizeof(io_vec) );
    io_vec.iov_base = (void*) payload;
    io_vec.iov_len  = payload->nlmsg_len;

    memset( &msg_hdr, 0, sizeof(msg_hdr) );
    msg_hdr.msg_name       = &address;
    msg_hdr.msg_namelen    = sizeof(address);
    msg_hdr.msg_iov        = &io_vec;
    msg_hdr.msg_iovlen     = 1;
    msg_hdr.msg_control    = NULL;
    msg_hdr.msg_controllen = 0;
    msg_hdr.msg_flags      = 0;

	payload->nlmsg_flags |= NLM_F_ACK;

    if( 0 > sendmsg( socket_fd, &msg_hdr, 0 ) )
    {
        DPRINTFE( "Send failed, error=%s.", strerror( errno ) );
        return( SM_FAILED );
    }
    
    error = sm_netlink_set_blocking( socket_fd, true );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to set socket to blocking." );
        return( error );
    }

    error = sm_netlink_receive( socket_fd, callback, invocation_data );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to receive response, error=%s.",
                  sm_error_str( error ) );
    }

    error2 = sm_netlink_set_blocking( socket_fd, false );
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to set socket to non-blocking, error=%s.",
                  sm_error_str( error2 ) );
    }

    return( error );
}
// ***************************************************************************

// ****************************************************************************
// Netlink - Open 
// ==============
SmErrorT sm_netlink_open( int* socket_fd, unsigned long groups )
{
    int fd;
    int socket_size = SM_NETLINK_MAX_SOCKET_SIZE;
    SmErrorT error;

    *socket_fd = -1;

    // Open a netlink socket for sending messages.
    fd = socket( AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE );
    if( 0 > fd )
    {
        DPRINTFE( "Unable to open socket, errno=%s.", strerror( errno ) );
        return( SM_FAILED );
    }

    // Make sure the netlink socket is non-blocking.
    error = sm_netlink_set_blocking( fd, false );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to set socket to non-blocking." );
        close( fd );
        return( SM_FAILED );
    }

    // Set netlink socket buffer size.
    if( 0 > setsockopt( fd, SOL_SOCKET, SO_RCVBUF, &socket_size,
                        sizeof(socket_size) ) )
    {
        DPRINTFE( "Cannot set buffer size: error=%s.", strerror( errno ) );
        close( fd );
        return( SM_FAILED );
    }

    // Bind the netlink socket to our address information.
    if( 0 != groups )
    {
        struct sockaddr_nl address;

        memset( &address, 0, sizeof(address) );

        address.nl_family = AF_NETLINK;
        address.nl_pad    = 0;
        address.nl_pid    = getpid();
        address.nl_groups = groups;

        if( 0 > bind( fd, (struct sockaddr*) &address, sizeof(address) ) )
        {
            DPRINTFE( "Unable to bind address, errno=%s.", strerror( errno ) );
            close( fd );
            return( SM_FAILED );
        }
    }

    *socket_fd = fd;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Netlink - Close
// ===============
SmErrorT sm_netlink_close( int socket_fd )
{
    if( -1 < socket_fd )
    {
        close( socket_fd );
    }

    return( SM_OKAY );
}
// ****************************************************************************
