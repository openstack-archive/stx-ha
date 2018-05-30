//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_INTERFACE_TABLE_H__
#define __SM_SERVICE_DOMAIN_INTERFACE_TABLE_H__

#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"

#ifdef __cplusplus
extern "C" { 
#endif

typedef struct
{
    int64_t id;
    char service_domain[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char service_domain_interface[SM_SERVICE_DOMAIN_INTERFACE_NAME_MAX_CHAR];
    SmPathTypeT path_type;
    SmAuthTypeT auth_type;
    char auth_key[SM_AUTHENTICATION_KEY_MAX_CHAR];
    char interface_name[SM_INTERFACE_NAME_MAX_CHAR];
    SmInterfaceStateT interface_state;
    SmNetworkTypeT network_type;
    SmNetworkAddressT network_multicast;
    SmNetworkAddressT network_address;
    int network_port;
    int network_heartbeat_port;
    SmNetworkAddressT network_peer_address;
    int network_peer_port;
    int network_peer_heartbeat_port;
    int unicast_socket;
    int multicast_socket;
    SmServiceDomainInterfaceConnectTypeT interface_connect_type;
    SmInterfaceTypeT interface_type;
} SmServiceDomainInterfaceT;

typedef void (*SmServiceDomainInterfaceTableForEachCallbackT)
    (void* user_data[], SmServiceDomainInterfaceT* interface);

// ****************************************************************************
// Service Domain Interface Table - Read
// =====================================
extern SmServiceDomainInterfaceT*
sm_service_domain_interface_table_read( char service_domain_name[], 
    char service_domain_interface_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - Read By Identifier
// ===================================================
extern SmServiceDomainInterfaceT* 
sm_service_domain_interface_table_read_by_id( int64_t id );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - For Each
// =========================================
extern void sm_service_domain_interface_table_foreach( void* user_data[],
    SmServiceDomainInterfaceTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - For Each Service Domain
// ========================================================
extern void sm_service_domain_interface_table_foreach_service_domain(
    char service_domain_name[], void* user_data[],
    SmServiceDomainInterfaceTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - Load
// =====================================
extern SmErrorT sm_service_domain_interface_table_load( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - Persist
// ========================================
extern SmErrorT sm_service_domain_interface_table_persist(
    SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - Initialize
// ===========================================
extern SmErrorT sm_service_domain_interface_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Table - Finalize
// =========================================
extern SmErrorT sm_service_domain_interface_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_INTERFACE_TABLE_H__
