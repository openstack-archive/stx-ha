//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_DOMAIN_NEIGHBORS_H__
#define __SM_DB_SERVICE_DOMAIN_NEIGHBORS_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_timer.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_NAME                           "SERVICE_DOMAIN_NEIGHBORS"
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_ID                      "ID"
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_NAME                    "NAME"
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_SERVICE_DOMAIN          "SERVICE_DOMAIN"
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_ORCHESTRATION           "ORCHESTRATION"
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_DESIGNATION             "DESIGNATION"
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_GENERATION              "GENERATION"
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_PRIORITY                "PRIORITY"
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_HELLO_INTERVAL          "HELLO_INTERVAL"
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_DEAD_INTERVAL           "DEAD_INTERVAL"
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_WAIT_INTERVAL           "WAIT_INTERVAL" 
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_EXCHANGE_INTERVAL       "EXCHANGE_INTERVAL"
#define SM_SERVICE_DOMAIN_NEIGHBORS_TABLE_COLUMN_STATE                   "STATE"

typedef struct
{
    int64_t id;
    char name[SM_NODE_NAME_MAX_CHAR];
    char service_domain[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char orchestration[SM_ORCHESTRATION_MAX_CHAR];
    char designation[SM_DESIGNATION_MAX_CHAR];
    int generation;
    int priority;
    int hello_interval;
    int dead_interval;
    int wait_interval;
    int exchange_interval;
    SmServiceDomainNeighborStateT state;
} SmDbServiceDomainNeighborT;

// ****************************************************************************
// Database Service Domain Neighbors - Convert
// ===========================================
extern SmErrorT sm_db_service_domain_neighbors_convert( const char* col_name,
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Read
// ========================================
extern SmErrorT sm_db_service_domain_neighbors_read( SmDbHandleT* sm_db_handle,
    char name[], char service_domain[], SmDbServiceDomainNeighborT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Read By Identifier
// ======================================================
extern SmErrorT sm_db_service_domain_neighbors_read_by_id( 
    SmDbHandleT* sm_db_handle, int64_t id, SmDbServiceDomainNeighborT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Query
// =========================================
extern SmErrorT sm_db_service_domain_neighbors_query(
    SmDbHandleT* sm_db_handle, const char* db_query, 
    SmDbServiceDomainNeighborT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Insert
// ==========================================
extern SmErrorT sm_db_service_domain_neighbors_insert(
    SmDbHandleT* sm_db_handle, SmDbServiceDomainNeighborT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Update
// ==========================================
extern SmErrorT sm_db_service_domain_neighbors_update(
    SmDbHandleT* sm_db_handle, SmDbServiceDomainNeighborT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Delete
// ==========================================
extern SmErrorT sm_db_service_domain_neighbors_delete(
    SmDbHandleT* sm_db_handle, char name[], char service_domain[] );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Create Table
// ================================================
extern SmErrorT sm_db_service_domain_neighbors_create_table(
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Delete Table
// ================================================
extern SmErrorT sm_db_service_domain_neighbors_delete_table(
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Cleanup Table
// =================================================
extern SmErrorT sm_db_service_domain_neighbors_cleanup_table( 
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Initialize
// ==============================================
extern SmErrorT sm_db_service_domain_neighbors_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Neighbors - Finalize
// ============================================
extern SmErrorT sm_db_service_domain_neighbors_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_DOMAIN_NEIGHBORS_H__
