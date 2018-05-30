//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_ERU_DB_H__
#define __SM_ERU_DB_H__

#include <stdbool.h>

#include "sm_types.h"
#include "sm_hw.h"
#include "sm_node_stats.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SM_ERU_DATABASE_ENTRY_TYPE_CPU_STATS,
    SM_ERU_DATABASE_ENTRY_TYPE_MEM_STATS,
    SM_ERU_DATABASE_ENTRY_TYPE_DISK_STATS,
    SM_ERU_DATABASE_ENTRY_TYPE_NET_STATS,
    SM_ERU_DATABASE_ENTRY_TYPE_TC_STATS,
    SM_ERU_DATABASE_ENTRY_TYPE_IF_CHANGE,
    SM_ERU_DATABASE_ENTRY_TYPE_IP_CHANGE,
    SM_ERU_DATABASE_ENTRY_TYPE_MAX
} SmEruDatabaseEntryType;

typedef struct
{
    int index;               // Filled in on write.
    struct timespec ts_real; // Filled in on write.

    SmEruDatabaseEntryType type;
    union
    {
        SmNodeCpuStatsT cpu_stats;
        SmNodeMemStatsT mem_stats;
        SmNodeDiskStatsT disk_stats;
        SmNodeNetDevStatsT net_stats;
        SmHwQdiscInfoT tc_stats;
        SmHwInterfaceChangeDataT if_change;
        SmHwIpChangeDataT ip_change;
    } u;
} SmEruDatabaseEntryT;

// ****************************************************************************
// Event Recorder Unit Database - Total
// ====================================
extern int sm_eru_db_total( void );
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Get Write Index
// ==============================================
extern int sm_eru_db_get_write_index( void );
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Flush the database
// =================================================
extern void sm_eru_db_sync( void );
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Read
// ===================================
extern SmErrorT sm_eru_db_read( int read_index, SmEruDatabaseEntryT* entry );
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Write
// ====================================
extern SmErrorT sm_eru_db_write( SmEruDatabaseEntryT* entry );
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Display
// ======================================
extern void sm_eru_db_display( SmEruDatabaseEntryT* entry, bool want_raw );
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Initialize
// =========================================
extern SmErrorT sm_eru_db_initialize( char filename[], bool read_only );
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Finalize
// =======================================
extern SmErrorT sm_eru_db_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_ERU_DB_H__
