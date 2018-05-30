//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_NEIGHBOR_TABLE_H__
#define __SM_SERVICE_DOMAIN_NEIGHBOR_TABLE_H__

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
    bool exchange_master;
    int exchange_seq;
    int64_t exchange_last_sent_id;
    int64_t exchange_last_recvd_id;
    SmTimerIdT exchange_timer_id;
    SmTimerIdT dead_timer_id;
    SmTimerIdT pause_timer_id;
    struct timespec last_exchange_start;
} SmServiceDomainNeighborT;

typedef void (*SmServiceDomainNeighborTableForEachCallbackT)
    (void* user_data[], SmServiceDomainNeighborT* neighbor);

// ****************************************************************************
// Service Domain Neighbor Table - Read
// ====================================
extern SmServiceDomainNeighborT* sm_service_domain_neighbor_table_read(
    char name[], char service_domain_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Read By Identifier
// ==================================================
extern SmServiceDomainNeighborT* sm_service_domain_neighbor_table_read_by_id(
    int64_t id );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - For Each
// ========================================
extern void sm_service_domain_neighbor_table_foreach( char service_domain_name[],
    void* user_data[], SmServiceDomainNeighborTableForEachCallbackT callback );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Load
// ====================================
extern SmErrorT sm_service_domain_neighbor_table_load( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Insert
// ======================================
extern SmErrorT sm_service_domain_neighbor_table_insert( char name[],
    char service_domain_name[], char orchestration[], char designation[],
    int generation, int priority, int hello_interval, int dead_interval,
    int wait_interval, int exchange_interval );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Persist
// =======================================
extern SmErrorT sm_service_domain_neighbor_table_persist(
    SmServiceDomainNeighborT* neighbor );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Initialize
// ==========================================
extern SmErrorT sm_service_domain_neighbor_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain Neighbor Table - Finalize
// ========================================
extern SmErrorT sm_service_domain_neighbor_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_NEIGHBOR_TABLE_H__
