//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_NODE_HISTORY_H__
#define __SM_DB_NODE_HISTORY_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_uuid.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_NODE_HISTORY_TABLE_NAME                  "NODE_HISTORY"
#define SM_NODE_HISTORY_TABLE_COLUMN_ID             "ID"
#define SM_NODE_HISTORY_TABLE_COLUMN_NAME           "NAME"
#define SM_NODE_HISTORY_TABLE_COLUMN_ADMIN_STATE    "ADMINISTRATIVE_STATE"
#define SM_NODE_HISTORY_TABLE_COLUMN_OPER_STATE     "OPERATIONAL_STATE"
#define SM_NODE_HISTORY_TABLE_COLUMN_AVAIL_STATUS   "AVAILABILITY_STATUS"
#define SM_NODE_HISTORY_TABLE_COLUMN_READY_STATE    "READY_STATE"
#define SM_NODE_HISTORY_TABLE_COLUMN_STATE_UUID     "STATE_UUID"

typedef struct
{
    int64_t id;
    char name[SM_NODE_NAME_MAX_CHAR];
    SmNodeAdminStateT admin_state;
    SmNodeOperationalStateT oper_state;
    SmNodeAvailStatusT avail_status;
    SmNodeReadyStateT ready_state;
    SmUuidT state_uuid;
} SmDbNodeHistoryT;

// ****************************************************************************
// Database Node History - Convert
// ===============================
extern SmErrorT sm_db_node_history_convert( const char* col_name,
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Node History - Read
// ============================
extern SmErrorT sm_db_node_history_read( SmDbHandleT* sm_db_handle,
    char name[], SmDbNodeHistoryT* record );
// ****************************************************************************

// ****************************************************************************
// Database Node History - Query
// =============================
extern SmErrorT sm_db_node_history_query( SmDbHandleT* sm_db_handle,
    const char* db_query, SmDbNodeHistoryT* record );
// ****************************************************************************

// ****************************************************************************
// Database Node History - Insert
// ==============================
extern SmErrorT sm_db_node_history_insert( SmDbHandleT* sm_db_handle,
    SmDbNodeHistoryT* record );
// ****************************************************************************

// ****************************************************************************
// Database Node History - Delete
// ==============================
extern SmErrorT sm_db_node_history_delete( SmDbHandleT* sm_db_handle,
    char name[] );
// ****************************************************************************

// ****************************************************************************
// Database Node History - Create Table
// ====================================
extern SmErrorT sm_db_node_history_create_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Node History - Delete Table
// ====================================
extern SmErrorT sm_db_node_history_delete_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Node History - Initialize
// ==================================
extern SmErrorT sm_db_node_history_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Node History - Finalize
// ================================
extern SmErrorT sm_db_node_history_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_NODE_HISTORY_H__
