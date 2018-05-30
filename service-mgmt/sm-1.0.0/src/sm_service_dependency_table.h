//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DEPENDENCY_TABLE_H__
#define __SM_SERVICE_DEPENDENCY_TABLE_H__

#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    SmServiceDependencyTypeT type;
    char service_name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    SmServiceStateT state;
    SmServiceActionT action;
    char dependent[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    SmServiceStateT dependent_state;
} SmServiceDependencyT;

typedef void (*SmServiceDependencyTableForEachCallbackT) 
    (void* user_data[], SmServiceDependencyT* service_dependency);

// ****************************************************************************
// Service Dependency Table - Read
// ===============================
extern SmServiceDependencyT* sm_service_dependency_table_read( 
    SmServiceDependencyTypeT type, char service_name[], SmServiceStateT state,
    SmServiceActionT action, char dependent[] );
// ****************************************************************************

// ****************************************************************************
// Service Dependency Table - For Each 
// ===================================
extern void sm_service_dependency_table_foreach( SmServiceDependencyTypeT type,
    char service_name[], SmServiceStateT state, SmServiceActionT action,  
    void* user_data[], SmServiceDependencyTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Dependency Table - For Each Dependent
// =============================================
extern void sm_service_dependency_table_foreach_dependent(
    SmServiceDependencyTypeT type, char dependent[],
    SmServiceStateT dependent_state, void* user_data[],
    SmServiceDependencyTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Dependency Table - Load
// ===============================
extern SmErrorT sm_service_dependency_table_load( void );
// ****************************************************************************

// ****************************************************************************
// Service Dependency Table - Initialize
// =====================================
extern SmErrorT sm_service_dependency_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Dependency Table - Finalize
// ===================================
extern SmErrorT sm_service_dependency_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DEPENDENCY_TABLE_H__
