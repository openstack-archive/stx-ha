//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_DEPENDENCY_H__
#define __SM_DB_SERVICE_DEPENDENCY_H__

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_SERVICE_DEPENDENCY_TABLE_NAME                    "SERVICE_DEPENDENCY"
#define SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENCY_TYPE  "DEPENDENCY_TYPE"
#define SM_SERVICE_DEPENDENCY_TABLE_COLUMN_SERVICE_NAME     "SERVICE_NAME"
#define SM_SERVICE_DEPENDENCY_TABLE_COLUMN_STATE            "STATE"
#define SM_SERVICE_DEPENDENCY_TABLE_COLUMN_ACTION           "ACTION"
#define SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT        "DEPENDENT"
#define SM_SERVICE_DEPENDENCY_TABLE_COLUMN_DEPENDENT_STATE  "DEPENDENT_STATE"

typedef struct
{
    SmServiceDependencyTypeT type;
    char service_name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    SmServiceStateT state;
    SmServiceActionT action;
    char dependent[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    SmServiceStateT dependent_state;
} SmDbServiceDependencyT;

// ****************************************************************************
// Database Service Dependency - Convert
// =====================================
extern SmErrorT sm_db_service_dependency_convert( const char* col_name,
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Dependency - Read
// ==================================
extern SmErrorT sm_db_service_dependency_read( SmDbHandleT* sm_db_handle,
    SmServiceDependencyTypeT type, char service_name[], SmServiceStateT state,
    SmServiceActionT action, char dependent[], SmDbServiceDependencyT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Dependency - Insert
// ====================================
extern SmErrorT sm_db_service_dependency_insert( SmDbHandleT* sm_db_handle,
    SmDbServiceDependencyT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Dependency - Update
// ====================================
extern SmErrorT sm_db_service_dependency_update( SmDbHandleT* sm_db_handle,
        SmDbServiceDependencyT* record );
// ****************************************************************************

// ****************************************************************************
// Database Service Dependency - Delete
// ====================================
extern SmErrorT sm_db_service_dependency_delete( SmDbHandleT* sm_db_handle,
    SmServiceDependencyTypeT type, char service_name[], SmServiceStateT state,
    SmServiceActionT action, char dependent[] );
// ****************************************************************************

// ****************************************************************************
// Database Service Dependency - Create Table
// ==========================================
extern SmErrorT sm_db_service_dependency_create_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Dependency - Delete Table
// ==========================================
extern SmErrorT sm_db_service_dependency_delete_table( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database Service Dependency - Initialize
// ========================================
extern SmErrorT sm_db_service_dependency_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Dependency - Finalize
// ======================================
extern SmErrorT sm_db_service_dependency_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_DEPENDENCY_H__
