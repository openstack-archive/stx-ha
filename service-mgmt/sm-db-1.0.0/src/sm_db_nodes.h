//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_NODES_H__
#define __SM_DB_NODES_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_uuid.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_NODES_TABLE_NAME                     "NODES"
#define SM_NODES_TABLE_COLUMN_ID                "ID"
#define SM_NODES_TABLE_COLUMN_NAME              "NAME"
#define SM_NODES_TABLE_COLUMN_ADMIN_STATE       "ADMINISTRATIVE_STATE"
#define SM_NODES_TABLE_COLUMN_OPER_STATE        "OPERATIONAL_STATE"
#define SM_NODES_TABLE_COLUMN_AVAIL_STATUS      "AVAILABILITY_STATUS"
#define SM_NODES_TABLE_COLUMN_READY_STATE       "READY_STATE"
#define SM_NODES_TABLE_COLUMN_STATE_UUID        "STATE_UUID"

typedef struct
{
    int64_t id;
    char name[SM_NODE_NAME_MAX_CHAR];
    SmNodeAdminStateT admin_state;
    SmNodeOperationalStateT oper_state;
    SmNodeAvailStatusT avail_status;
    SmNodeReadyStateT ready_state;
    SmUuidT state_uuid;
} SmDbNodeT;

// ****************************************************************************
// Database Nodes - Convert
// ========================
extern SmErrorT sm_db_nodes_convert( const char* col_name, const char* col_data,
    void* data );
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Read
// =====================
extern SmErrorT sm_db_nodes_read( SmDbHandleT* sm_db_handle, char name[],
    SmDbNodeT* record );
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Query
// ======================
extern SmErrorT sm_db_nodes_query( SmDbHandleT* sm_db_handle,
    const char* db_query, SmDbNodeT* record );
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Insert
// =======================
extern SmErrorT sm_db_nodes_insert( SmDbHandleT* sm_db_handle,
    SmDbNodeT* record );
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Update
// =======================
extern SmErrorT sm_db_nodes_update( SmDbHandleT* sm_db_handle,
    SmDbNodeT* record );
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Delete
// =======================
extern SmErrorT sm_db_nodes_delete( SmDbHandleT* sm_db_handle, char name[] );
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Create Table
// =============================
extern SmErrorT sm_db_nodes_create_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Delete Table
// =============================
extern SmErrorT sm_db_nodes_delete_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Cleanup Table
// ==============================
extern SmErrorT sm_db_nodes_cleanup_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Initialize
// ===========================
extern SmErrorT sm_db_nodes_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Nodes - Finalize
// =========================
extern SmErrorT sm_db_nodes_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_NODES_H__
