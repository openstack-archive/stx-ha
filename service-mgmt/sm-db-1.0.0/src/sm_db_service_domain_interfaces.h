//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_DOMAIN_INTERFACES_H__
#define __SM_DB_SERVICE_DOMAIN_INTERFACES_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_timer.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_NAME                                 "SERVICE_DOMAIN_INTERFACES"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_ID                            "ID"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_PROVISIONED                   "PROVISIONED"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN                "SERVICE_DOMAIN"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_SERVICE_DOMAIN_INTERFACE      "SERVICE_DOMAIN_INTERFACE"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_PATH_TYPE                     "PATH_TYPE"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_AUTH_TYPE                     "AUTH_TYPE"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_AUTH_KEY                      "AUTH_KEY"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_NAME                "INTERFACE_NAME"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_STATE               "INTERFACE_STATE"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_TYPE                  "NETWORK_TYPE"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_MULTICAST             "NETWORK_MULTICAST"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_ADDRESS               "NETWORK_ADDRESS"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PORT                  "NETWORK_PORT"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_HEARTBEAT_PORT        "NETWORK_HEARTBEAT_PORT"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_ADDRESS          "NETWORK_PEER_ADDRESS"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_PORT             "NETWORK_PEER_PORT"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_NETWORK_PEER_HEARTBEAT_PORT   "NETWORK_PEER_HEARTBEAT_PORT"
#define SM_SERVICE_DOMAIN_INTERFACES_TABLE_COLUMN_INTERFACE_CONNECT_TYPE        "INTERFACE_CONNECT_TYPE"


#define SM_INTERFACE_CONNECT_TYPE_DC "dc"
#define SM_INTERFACE_CONNECT_TYPE_TOR "tor"

typedef struct
{
    int64_t id;
    bool provisioned;
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
    SmServiceDomainInterfaceConnectTypeT interface_connect_type;
} SmDbServiceDomainInterfaceT;

// ****************************************************************************
// Database Service Domain Interfaces - Convert
// ============================================
extern SmErrorT sm_db_service_domain_interfaces_convert( const char* col_name,
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Read
// =========================================
extern SmErrorT sm_db_service_domain_interfaces_read( 
    SmDbHandleT* sm_db_handle, char service_domain[],
    char service_domain_interface[], SmDbServiceDomainInterfaceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Query
// ==========================================
extern SmErrorT sm_db_service_domain_interfaces_query(
    SmDbHandleT* sm_db_handle, const char* db_query,
    SmDbServiceDomainInterfaceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Insert
// ===========================================
extern SmErrorT sm_db_service_domain_interfaces_insert(
    SmDbHandleT* sm_db_handle, SmDbServiceDomainInterfaceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Update
// ===========================================
extern SmErrorT sm_db_service_domain_interfaces_update(
    SmDbHandleT* sm_db_handle, SmDbServiceDomainInterfaceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Delete
// ===========================================
extern SmErrorT sm_db_service_domain_interfaces_delete(
    SmDbHandleT* sm_db_handle, char service_domain[],
    char service_domain_interface[] );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Create Table
// =================================================
extern SmErrorT sm_db_service_domain_interfaces_create_table(
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Delete Table
// =================================================
extern SmErrorT sm_db_service_domain_interfaces_delete_table(
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Cleanup Table
// ==================================================
extern SmErrorT sm_db_service_domain_interfaces_cleanup_table(
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Initialize
// ===============================================
extern SmErrorT sm_db_service_domain_interfaces_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Interfaces - Finalize
// =============================================
extern SmErrorT sm_db_service_domain_interfaces_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_DOMAIN_INTERFACES_H__
