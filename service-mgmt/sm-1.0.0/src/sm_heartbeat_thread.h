//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_HEARTBEAT_THREAD_H__
#define __SM_HEARTBEAT_THREAD_H__

#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_service_domain_interface_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Heartbeat Thread - Disable heartbeat
// =========================
extern void sm_heartbeat_thread_disable_heartbeat( void );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Enable
// =========================
extern void sm_heartbeat_thread_enable( void );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Disable
// ==========================
extern void sm_heartbeat_thread_disable( void );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Add Interface
// ================================
extern SmErrorT sm_heartbeat_thread_add_interface(
    const SmServiceDomainInterfaceT& domain_interface );
// ****************************************************************************



// ****************************************************************************
// Heartbeat Thread - Delete Interface
// ===================================
extern SmErrorT sm_heartbeat_thread_delete_interface( int64_t id );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Add Peer Interface 
// =====================================
extern SmErrorT sm_heartbeat_thread_add_peer_interface( int64_t id,
    char interface_name[], SmNetworkAddressT* network_address,
    int network_port, int dead_interval );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Delete Peer Interface 
// ========================================
extern SmErrorT sm_heartbeat_thread_delete_peer_interface(
    char interface_name[], SmNetworkAddressT* network_address,
    int network_port );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Peer Alive In Period
// =======================================
extern bool sm_heartbeat_thread_peer_alive_in_period( char node_name[],
    unsigned int period_in_ms );
// ****************************************************************************

// ****************************************************************************
// Heartbeat Thread - Start
// ========================
extern SmErrorT sm_heartbeat_thread_start( void );
// ****************************************************************************

// ****************************************************************************
// Heartbeart Thread - Stop
// ========================
extern SmErrorT sm_heartbeat_thread_stop( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_HEARTBEAT_THREAD_H__
