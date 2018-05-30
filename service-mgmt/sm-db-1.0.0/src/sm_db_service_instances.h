//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_INSTANCES_H__
#define __SM_DB_SERVICE_INSTANCES_H__

#include <stdint.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_INSTANCES_TABLE_NAME                        "SERVICE_INSTANCES"
#define SM_SERVICE_INSTANCES_TABLE_COLUMN_ID                   "ID"
#define SM_SERVICE_INSTANCES_TABLE_COLUMN_SERVICE_NAME         "SERVICE_NAME"
#define SM_SERVICE_INSTANCES_TABLE_COLUMN_INSTANCE_NAME        "INSTANCE_NAME"
#define SM_SERVICE_INSTANCES_TABLE_COLUMN_INSTANCE_PARAMETERS  "INSTANCE_PARAMETERS"

typedef struct
{
    int64_t id;
    char service_name[SM_SERVICE_NAME_MAX_CHAR];
    char instance_name[SM_SERVICE_INSTANCE_NAME_MAX_CHAR];
    char instance_params[SM_SERVICE_INSTANCE_PARAMS_MAX_CHAR];
} SmDbServiceInstanceT;

// ****************************************************************************
// Database Service Instances - Convert
// ====================================
extern SmErrorT sm_db_service_instances_convert( const char* col_name, 
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Instances - Read
// =================================
extern SmErrorT sm_db_service_instances_read( SmDbHandleT* sm_db_handle,
    char service_name[], SmDbServiceInstanceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Instances - Insert
// ===================================
extern SmErrorT sm_db_service_instances_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceInstanceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Instances - Update
// ===================================
extern SmErrorT sm_db_service_instances_update( SmDbHandleT* sm_db_handle,
    SmDbServiceInstanceT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Instances - Delete
// ===================================
extern SmErrorT sm_db_service_instances_delete( SmDbHandleT* sm_db_handle,
    char service_name[] );
// ****************************************************************************

// ****************************************************************************
// Database Service Instances - Create Table
// =========================================
extern SmErrorT sm_db_service_instances_create_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Instances - Delete Table
// =========================================
extern SmErrorT sm_db_service_instances_delete_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Instances - Initialize
// =======================================
extern SmErrorT sm_db_service_instances_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Instances - Finalize
// =====================================
extern SmErrorT sm_db_service_instances_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_INSTANCES_H__
