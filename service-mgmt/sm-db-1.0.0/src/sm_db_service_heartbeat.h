
//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_HEARTBEAT_H__
#define __SM_DB_SERVICE_HEARTBEAT_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_timer.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_HEARTBEAT_TABLE_NAME                      "SERVICE_HEARTBEAT"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_ID                 "ID"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_PROVISIONED        "PROVISIONED"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_NAME               "NAME"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_TYPE               "TYPE"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_SRC_ADDRESS        "SRC_ADDRESS"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_SRC_PORT           "SRC_PORT"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_DST_ADDRESS        "DST_ADDRESS"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_DST_PORT           "DST_PORT"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MESSAGE            "MESSAGE"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_INTERVAL_IN_MS     "INTERVAL_IN_MS"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_WARN        "MISSED_WARN"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_DEGRADE     "MISSED_DEGRADE"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED_FAIL        "MISSED_FAIL"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_STATE              "STATE"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_MISSED             "MISSED"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_TIMER_ID "HEARTBEAT_TIMER_ID"
#define SM_SERVICE_HEARTBEAT_TABLE_COLUMN_HEARTBEAT_SOCKET   "HEARTBEAT_SOCKET"

typedef struct
{
    int64_t id;
    bool provisioned;
    char name[SM_SERVICE_HEARTBEAT_NAME_MAX_CHAR];
    SmServiceHeartbeatTypeT type;
    char src_address[SM_SERVICE_HEARTBEAT_ADDRESS_MAX_CHAR];
    int src_port;
    char dst_address[SM_SERVICE_HEARTBEAT_ADDRESS_MAX_CHAR];
    int dst_port;
    char message[SM_SERVICE_HEARTBEAT_MESSAGE_MAX_CHAR];
    int interval_in_ms;
    int missed_warn;
    int missed_degrade;
    int missed_fail;
    SmServiceHeartbeatStateT state;
    int missed;
    SmTimerIdT heartbeat_timer_id;
    int heartbeat_socket;
} SmDbServiceHeartbeatT;

// ****************************************************************************
// Database Service Heartbeat - Convert
// ====================================
extern SmErrorT sm_db_service_heartbeat_convert( const char* col_name, 
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Read
// =================================
extern SmErrorT sm_db_service_heartbeat_read( SmDbHandleT* sm_db_handle,
    char name[], SmDbServiceHeartbeatT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Read By Identifier
// ===============================================
extern SmErrorT sm_db_service_heartbeat_read_by_id( SmDbHandleT* sm_db_handle,
    int64_t id, SmDbServiceHeartbeatT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Query
// ==================================
extern SmErrorT sm_db_service_heartbeat_query( SmDbHandleT* sm_db_handle,
    const char* db_query, SmDbServiceHeartbeatT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Insert
// ===================================
extern SmErrorT sm_db_service_heartbeat_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceHeartbeatT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Update
// ===================================
extern SmErrorT sm_db_service_heartbeat_update( SmDbHandleT* sm_db_handle,
    SmDbServiceHeartbeatT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Delete
// ===================================
extern SmErrorT sm_db_service_heartbeat_delete( SmDbHandleT* sm_db_handle,
    char name[] );
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Create Table
// =========================================
extern SmErrorT sm_db_service_heartbeat_create_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Delete Table
// =========================================
extern SmErrorT sm_db_service_heartbeat_delete_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Cleanup Table
// ==========================================
extern SmErrorT sm_db_service_heartbeat_cleanup_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Initialize
// =======================================
extern SmErrorT sm_db_service_heartbeat_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Heartbeat - Finalize
// =====================================
extern SmErrorT sm_db_service_heartbeat_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_HEARTBEAT_H__
