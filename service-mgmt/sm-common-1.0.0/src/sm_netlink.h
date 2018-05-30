//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_NETLINK_H__
#define __SM_NETLINK_H__

#include <sys/socket.h>
#include <linux/netlink.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmNetlinkCallbackT) (int socket_fd, struct sockaddr_nl* address,
        struct nlmsghdr* payload, void* invocation_data );

// ***************************************************************************
// Netlink - Get Tx Buffer
// =======================
extern void* sm_netlink_get_tx_buffer( void );
// ***************************************************************************

// ***************************************************************************
// Netlink - Add Attribute 
// =======================
extern void sm_netlink_add_attribute( struct nlmsghdr* payload, int type,
    void* data, int data_length );
// ***************************************************************************

// ***************************************************************************
// Netlink - Send 
// ==============
extern SmErrorT sm_netlink_send( int socket_fd, struct nlmsghdr* payload );
// ***************************************************************************

// ***************************************************************************
// Netlink - Send Request
// ======================
extern SmErrorT sm_netlink_send_request( int socket_fd,
    struct nlmsghdr* payload, void* request, int request_len );
// ***************************************************************************

// ***************************************************************************
// Netlink - Receive 
// =================
extern SmErrorT sm_netlink_receive( int socket_fd, SmNetlinkCallbackT callback,
    void* invocation_data );
// ***************************************************************************

// ***************************************************************************
// Netlink - Command
// =================
extern SmErrorT sm_netlink_command( int socket_fd, struct nlmsghdr* payload,
    SmNetlinkCallbackT callback, void* invocation_data );
// ***************************************************************************

// ****************************************************************************
// Netlink - Open 
// ==============
extern SmErrorT sm_netlink_open( int* socket_fd, unsigned long groups );
// ****************************************************************************

// ****************************************************************************
// Netlink - Close
// ===============
extern SmErrorT sm_netlink_close( int socket_fd );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_NETLINK_H__
