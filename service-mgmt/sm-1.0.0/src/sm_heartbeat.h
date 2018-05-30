//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_HEARTBEAT_H__
#define __SM_HEARTBEAT_H__

#include <stdbool.h>

#include "sm_types.h"
#include "sm_service_domain_interface_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Heartbeat - Enable
// ==================
extern void sm_heartbeat_enable( void );
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Disable
// ===================
extern void sm_heartbeat_disable( void );
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Add Interface
// =========================
extern SmErrorT sm_heartbeat_add_interface(
    SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Delete Interface
// ============================
extern SmErrorT sm_heartbeat_delete_interface( 
    SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Add Peer Interface 
// ==============================
extern SmErrorT sm_heartbeat_add_peer_interface( int64_t id,
    char interface_name[], SmNetworkAddressT* network_address,
    int network_port, int dead_interval );
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Delete Peer Interface 
// =================================
extern SmErrorT sm_heartbeat_delete_peer_interface( char interface_name[],
    SmNetworkAddressT* network_address, int network_port );
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Peer Alive In Period
// ================================
extern bool sm_heartbeat_peer_alive_in_period( char node_name[],
    unsigned int period_in_ms );
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Initialize
// ======================
extern SmErrorT sm_heartbeat_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Finalize
// ====================
extern SmErrorT sm_heartbeat_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_HEARTBEAT_H__
