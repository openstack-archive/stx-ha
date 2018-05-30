//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_heartbeat.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_heartbeat_thread.h"

// ****************************************************************************
// Heartbeat - Enable
// ==================
void sm_heartbeat_enable( void )
{
    sm_heartbeat_thread_enable();
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Disable
// ===================
void sm_heartbeat_disable( void )
{
    sm_heartbeat_thread_disable();
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Add Interface
// =========================
SmErrorT sm_heartbeat_add_interface( SmServiceDomainInterfaceT* interface )
{
    if ( NULL == interface )
    {
        DPRINTFE( "Domain interface is NULL" );
        return SM_FAILED;
    }

    return( sm_heartbeat_thread_add_interface( *interface ) );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Delete Interface
// ============================
SmErrorT sm_heartbeat_delete_interface( SmServiceDomainInterfaceT* interface )
{
    return( sm_heartbeat_thread_delete_interface( interface->id ) );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Add Peer Interface 
// ==============================
SmErrorT sm_heartbeat_add_peer_interface( int64_t id, char interface_name[],
    SmNetworkAddressT* network_address, int network_port, int dead_interval )
{
    return( sm_heartbeat_thread_add_peer_interface( id, interface_name,
                network_address, network_port, dead_interval ) );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Delete Peer Interface 
// =================================
SmErrorT sm_heartbeat_delete_peer_interface( char interface_name[],
    SmNetworkAddressT* network_address, int network_port )
{
    return( sm_heartbeat_thread_delete_peer_interface( interface_name,
                network_address, network_port ) );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Peer Alive In Period
// ================================
bool sm_heartbeat_peer_alive_in_period( char node_name[],
    unsigned int period_in_ms )
{
    return( sm_heartbeat_thread_peer_alive_in_period( node_name,
                                                      period_in_ms ) );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Initialize
// ======================
SmErrorT sm_heartbeat_initialize( void )
{
    SmErrorT error;

    error = sm_heartbeat_thread_start();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to start heartbeat thread, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Heartbeat - Finalize
// ====================
SmErrorT sm_heartbeat_finalize( void )
{
    SmErrorT error;

    error = sm_heartbeat_thread_stop();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to stop heartbeat thread, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
