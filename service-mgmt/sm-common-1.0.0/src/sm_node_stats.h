//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_NODE_STATS_H__
#define __SM_NODE_STATS_H__

#include <stdbool.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_NODE_STATS_CPU_NAME_MAX_LEN          128
#define SM_NODE_STATS_DEV_NAME_MAX_LEN          128
#define SM_NODE_STATS_NET_DEV_NAME_MAX_LEN      128
#define SM_NODE_STATS_PROCESS_NAME_MAX_LEN      128
#define SM_NODE_STATS_PROCESS_STATE_MAX_LEN      32

typedef struct
{
    char cpu_name[SM_NODE_STATS_CPU_NAME_MAX_LEN];
    unsigned long long user_cpu_usage;
    unsigned long long nice_cpu_usage;
    unsigned long long system_cpu_usage;
    unsigned long long idle_task_cpu_usage;
    unsigned long long iowait_cpu_usage;
    unsigned long long irq_cpu_usage;
    unsigned long long soft_irq_cpu_usage;
    unsigned long long steal_cpu_usage;
    unsigned long long guest_cpu_usage;
    unsigned long long guest_nice_cpu_usage;
    unsigned long long total_interrupts;       // across all cpus   
    unsigned long long total_context_switches; // across all cpus
    unsigned long long total_processes;        // across all cpus
    unsigned long long processes_running;      // across all cpus
    unsigned long long processes_blocked;      // across all cpus
    unsigned long long boot_time_in_secs;      // since unix epoch
} SmNodeCpuStatsT;

typedef struct
{
    unsigned long total_memory_kB;
    unsigned long free_memory_kB;
    unsigned long buffers_kB;
    unsigned long cached_kB;
    unsigned long swap_cached_kB;
    unsigned long swap_total_kB;
    unsigned long swap_free_kB;
    unsigned long active_kB;
    unsigned long inactive_kB;
    unsigned long dirty_kB;
    unsigned long hugepages_total;
    unsigned long hugepages_free;
    unsigned long hugepage_size_kB;
    unsigned long nfs_uncommited_kB;
    unsigned long commited_memory_kB;
} SmNodeMemStatsT;

typedef struct
{
    unsigned int major_num;
    unsigned int minor_num;
    char dev_name[SM_NODE_STATS_DEV_NAME_MAX_LEN];
    unsigned long reads_completed;
    unsigned long reads_merged;
    unsigned long sectors_read;
    unsigned long ms_spent_reading;
    unsigned long writes_completed;
    unsigned long writes_merged;
    unsigned long sectors_written;
    unsigned long ms_spent_writing;
    unsigned long io_inprogress;
    unsigned long ms_spent_io;
    unsigned long weighted_ms_spent_io;
} SmNodeDiskStatsT;

typedef struct
{
    char dev_name[SM_NODE_STATS_NET_DEV_NAME_MAX_LEN];
    unsigned long long rx_bytes;
    unsigned long long rx_packets;
    unsigned long long rx_errors;
    unsigned long long rx_dropped;
    unsigned long long rx_fifo_errors;
    unsigned long long rx_frame_errors;
    unsigned long long rx_compressed_packets;
    unsigned long long rx_multicast_frames;
    unsigned long long tx_bytes;
    unsigned long long tx_packets;
    unsigned long long tx_errors;
    unsigned long long tx_dropped;
    unsigned long long tx_fifo_errors;
    unsigned long long tx_collisions;
    unsigned long long tx_carrier_loss;
    unsigned long long tx_compressed_packets;
} SmNodeNetDevStatsT;

typedef struct
{
    int pid;
    char name[SM_NODE_STATS_PROCESS_NAME_MAX_LEN];
    // RSDZTW
    // R is running,
    // S is sleeping in an interruptible wait,
    // D is waiting in uninterruptible disk sleep,
    // Z is zombie,
    // T is traced or stopped (on a signal),
    // W is paging.
    char state; // RSDZTW
    int ppid;
    int pgrp;

    unsigned long long block_start_ns;
    unsigned long long nr_switches;
    unsigned long long nr_voluntary_switches;
    unsigned long long nr_involuntary_switches;
} SmNodeProcessStatusT;

// ****************************************************************************
// Node Statistics - Get CPU
// =========================
extern SmErrorT sm_node_stats_get_cpu( const char cpu_name[],
    SmNodeCpuStatsT* stats );
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Get Memory
// ============================
extern SmErrorT sm_node_stats_get_memory( SmNodeMemStatsT* stats );
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Get Disk
// ==========================
extern SmErrorT sm_node_stats_get_disk( const char dev_name[],
    SmNodeDiskStatsT* stats );
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Get Disk By Index
// ===================================
extern SmErrorT sm_node_stats_get_disk_by_index( unsigned int index,
    SmNodeDiskStatsT* stats );
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Get Network Device
// ====================================
extern SmErrorT sm_node_stats_get_netdev( const char dev_name[],
    SmNodeNetDevStatsT* stats );
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Get Process Status
// ====================================
extern SmErrorT sm_node_stats_get_process_status( int pid,
    SmNodeProcessStatusT* status );
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Initialize
// ============================
extern SmErrorT sm_node_stats_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Finalize
// ==========================
extern SmErrorT sm_node_stats_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_NODE_STATS_H__
