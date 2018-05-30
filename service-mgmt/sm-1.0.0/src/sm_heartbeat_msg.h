//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_HEARTBEAT_MSG_H__
#define __SM_HEARTBEAT_MSG_H__

#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*SmHeartbeatMsgAuthCallbackT) (char interface_name[],
    SmNetworkAddressT* network_address, int network_port,
    void* msg, int msg_size, uint8_t auth_vector[]);

typedef void (*SmHeartbeatMsgAliveCallbackT) (char node_name[],
        SmNetworkAddressT* network_address, int network_port, int version,
        int revision, char interface_name[]);

typedef void (*SmHeartbeatMsgIfStateCallbackT) (const char node_name[],
        SmHeartbeatMsgIfStateT if_state);

typedef struct
{
    SmHeartbeatMsgAuthCallbackT auth;
    SmHeartbeatMsgAliveCallbackT alive;
    SmHeartbeatMsgIfStateCallbackT if_state;
} SmHeartbeatMsgCallbacksT;

// ****************************************************************************
// Heartbeat Messaging - Register Callbacks
// ========================================
extern SmErrorT sm_heartbeat_msg_register_callbacks(
    SmHeartbeatMsgCallbacksT* callbacks );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Deregister Callbacks
// ==========================================
extern SmErrorT sm_heartbeat_msg_deregister_callbacks(
    SmHeartbeatMsgCallbacksT* callbacks );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Send Alive
// ================================
extern SmErrorT sm_heartbeat_msg_send_alive( SmNetworkTypeT network_type,
    char node_name[], SmNetworkAddressT* network_address,
    SmNetworkAddressT* network_multicast, int network_port,
    char interface_name[], SmAuthTypeT auth_type, char auth_key[],
    int multicast_socket );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Open Sockets
// ==================================
extern SmErrorT sm_heartbeat_msg_open_sockets( SmNetworkTypeT network_type,
    SmNetworkAddressT* network_address, SmNetworkAddressT* network_multicast,
    int network_port, char interface_name[], int* multicast_socket, int* unicast_socket );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Messaging - Close Sockets
// ===================================
extern SmErrorT sm_heartbeat_msg_close_sockets( int* multicast_socket );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Message - Initialize
// ==============================
extern SmErrorT sm_heartbeat_msg_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Heartbeart Message - Finalize
// =============================
extern SmErrorT sm_heartbeat_msg_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_HEARTBEAT_MSG_H__
