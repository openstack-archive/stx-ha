//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_DOMAINS_H__
#define __SM_DB_SERVICE_DOMAINS_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_timer.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_DOMAINS_TABLE_NAME                        "SERVICE_DOMAINS"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_ID                   "ID"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_PROVISIONED          "PROVISIONED"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_NAME                 "NAME"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_ORCHESTRATION        "ORCHESTRATION"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_DESIGNATION          "DESIGNATION"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_PREEMPT              "PREEMPT"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_GENERATION           "GENERATION"    
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_PRIORITY             "PRIORITY"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_HELLO_INTERVAL       "HELLO_INTERVAL"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_DEAD_INTERVAL        "DEAD_INTERVAL"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_WAIT_INTERVAL        "WAIT_INTERVAL"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_EXCHANGE_INTERVAL    "EXCHANGE_INTERVAL"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_STATE                "STATE"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_SPLIT_BRAIN_RECOVERY "SPLIT_BRAIN_RECOVERY"
#define SM_SERVICE_DOMAINS_TABLE_COLUMN_LEADER               "LEADER"

typedef struct
{
    int64_t id;
    bool provisioned;
    char name[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    SmOrchestrationTypeT orchestration;
    SmDesignationTypeT designation;
    bool preempt;
    int generation;    
    int priority;
    int hello_interval;
    int dead_interval;
    int wait_interval;
    int exchange_interval;
    SmServiceDomainStateT state;
    SmServiceDomainSplitBrainRecoveryT split_brain_recovery;
    char leader[SM_NODE_NAME_MAX_CHAR];
} SmDbServiceDomainT;

// ****************************************************************************
// Database Service Domains - Convert
// ==================================
extern SmErrorT sm_db_service_domains_convert( const char* col_name,
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Read
// ===============================
extern SmErrorT sm_db_service_domains_read( SmDbHandleT* sm_db_handle, 
    char name[], SmDbServiceDomainT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Read By Identifier
// =============================================
extern SmErrorT sm_db_service_domains_read_by_id( SmDbHandleT* sm_db_handle,
    int64_t id, SmDbServiceDomainT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Query
// ================================
extern SmErrorT sm_db_service_domains_query( SmDbHandleT* sm_db_handle, 
    const char* db_query, SmDbServiceDomainT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Insert
// =================================
extern SmErrorT sm_db_service_domains_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceDomainT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Update
// =================================
extern SmErrorT sm_db_service_domains_update( SmDbHandleT* sm_db_handle,
    SmDbServiceDomainT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Delete
// =================================
extern SmErrorT sm_db_service_domains_delete( SmDbHandleT* sm_db_handle,
    char name[] );
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Create Table
// =======================================
extern SmErrorT sm_db_service_domains_create_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Delete Table
// =======================================
extern SmErrorT sm_db_service_domains_delete_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Cleanup Table
// ========================================
extern SmErrorT sm_db_service_domains_cleanup_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Initialize
// =====================================
extern SmErrorT sm_db_service_domains_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Domains - Finalize
// ===================================
extern SmErrorT sm_db_service_domains_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_DOMAINS_H__
