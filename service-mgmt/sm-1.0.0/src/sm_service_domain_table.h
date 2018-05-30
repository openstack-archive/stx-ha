//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_TABLE_H__
#define __SM_SERVICE_DOMAIN_TABLE_H__

#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_timer.h"

#ifdef __cplusplus
extern "C" { 
#endif

typedef struct
{
    int64_t id;
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
    SmTimerIdT hello_timer_id;
    SmTimerIdT wait_timer_id;
    char leader[SM_NODE_NAME_MAX_CHAR];
} SmServiceDomainT;

typedef void (*SmServiceDomainTableForEachCallbackT)
    (void* user_data[], SmServiceDomainT* domain);

// ****************************************************************************
// Service Domain Table - Read
// ===========================
extern SmServiceDomainT* sm_service_domain_table_read( char name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - Read By Identifier
// =========================================
extern SmServiceDomainT* sm_service_domain_table_read_by_id( int64_t id );
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - For Each
// ===============================
extern void sm_service_domain_table_foreach( void* user_data[],
    SmServiceDomainTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - Load
// ===========================
extern SmErrorT sm_service_domain_table_load( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - Persist
// ==============================
extern SmErrorT sm_service_domain_table_persist( SmServiceDomainT* domain );
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - Initialize
// =================================
extern SmErrorT sm_service_domain_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Table - Finalize
// ===============================
extern SmErrorT sm_service_domain_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_TABLE_H__
