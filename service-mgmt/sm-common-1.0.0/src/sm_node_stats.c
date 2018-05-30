//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_node_stats.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"

#define SM_NODE_STATS_PROC_STAT         "/proc/stat"
#define SM_NODE_STATS_PROC_MEMINFO      "/proc/meminfo"
#define SM_NODE_STATS_PROC_DISKSTATS    "/proc/diskstats"
#define SM_NODE_STATS_PROC_NET_DEV      "/proc/net/dev"

static long _user_hz = 0;

// ****************************************************************************
// Node Statistics - Get CPU
// =========================
SmErrorT sm_node_stats_get_cpu( const char cpu_name[], SmNodeCpuStatsT* stats )
{
    char line[512];
    FILE* fp;

    memset( stats, 0, sizeof(SmNodeCpuStatsT) );

    if( 0 == _user_hz )
    {
        DPRINTFE( "User hertz is not defined." );
        return( SM_FAILED );
    }

    if( '\0' == cpu_name[0] )
    {
        snprintf( stats->cpu_name, sizeof(stats->cpu_name), "all" );
    } else {
        snprintf( stats->cpu_name, sizeof(stats->cpu_name), "%s", cpu_name );
    }

    fp = fopen( SM_NODE_STATS_PROC_STAT, "r" );
    if( NULL == fp )
    {
        DPRINTFE( "Failed to get cpu statistics from %s.",
                  SM_NODE_STATS_PROC_STAT );
        return( SM_NOT_FOUND );
    }

    while( NULL != fgets( line, sizeof(line), fp ) )
    {
        if((( '\0' == cpu_name[0] )&&( 0 == strncmp( line, "cpu ", 4 ) ))||
           ( 0 == strncmp( line, cpu_name, 4 ) )) 
        {
            sscanf( line+5, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                    &(stats->user_cpu_usage),
                    &(stats->nice_cpu_usage),
                    &(stats->system_cpu_usage),
                    &(stats->idle_task_cpu_usage),
                    &(stats->iowait_cpu_usage),
                    &(stats->irq_cpu_usage),
                    &(stats->soft_irq_cpu_usage),
                    &(stats->steal_cpu_usage),
                    &(stats->guest_cpu_usage),
                    &(stats->guest_nice_cpu_usage) );

        } else if( 0 == strncmp( line, "intr ", 5 ) ) {
            sscanf( line+5, "%llu", &(stats->total_interrupts) );

        } else if( 0 == strncmp( line, "ctxt ", 5 ) ) {
            sscanf( line+5, "%llu", &(stats->total_context_switches) );

        } else if( 0 == strncmp( line, "processes ", 10 ) ) {
            sscanf( line+10, "%llu", &(stats->total_processes) );

        } else if( 0 == strncmp( line, "procs_running ", 14 ) ) {
            sscanf( line+14, "%llu", &(stats->processes_running) );

        } else if( 0 == strncmp( line, "procs_blocked ", 14 ) ) {
            sscanf( line+14, "%llu", &(stats->processes_blocked) );

        } else if( 0 == strncmp( line, "btime ", 6 ) ) {
            sscanf( line+6, "%llu", &(stats->boot_time_in_secs) );
        }
    }

    fclose( fp );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Get Memory
// ============================
SmErrorT sm_node_stats_get_memory( SmNodeMemStatsT* stats )
{
    char line[512];
    FILE* fp;

    memset( stats, 0, sizeof(SmNodeMemStatsT) );

    fp = fopen( SM_NODE_STATS_PROC_MEMINFO, "r" );
    if( NULL == fp )
    {
        DPRINTFE( "Failed to get memory statistics from %s.",
                  SM_NODE_STATS_PROC_MEMINFO );
        return( SM_NOT_FOUND );
    }

    while( NULL != fgets( line, sizeof(line), fp ) )
    {
        if( 0 == strncmp( line, "MemTotal:", 9 ) )
        {
            sscanf( line+9, "%lu", &(stats->total_memory_kB) );

        } else if( 0 == strncmp( line, "MemFree:", 8 ) ) {
            sscanf( line+8, "%lu", &(stats->free_memory_kB) );

        } else if( 0 == strncmp( line, "Buffers:", 8 ) ) {
            sscanf( line+8, "%lu", &(stats->buffers_kB) );

        } else if( 0 == strncmp( line, "Cached:", 7 ) ) {
            sscanf( line+7, "%lu", &(stats->cached_kB) );

        } else if( 0 == strncmp( line, "SwapCached:", 11 ) ) {
            sscanf( line+11, "%lu", &(stats->swap_cached_kB) );

        } else if( 0 == strncmp( line, "SwapTotal:", 10 ) ) {
            sscanf( line+10, "%lu", &(stats->swap_total_kB) );

        } else if( 0 == strncmp( line, "SwapFree:", 9 ) ) {
            sscanf( line+9, "%lu", &(stats->swap_free_kB) );

        } else if( 0 == strncmp( line, "Active:", 7 ) ) {
            sscanf( line+7, "%lu", &(stats->active_kB) );

        } else if( 0 == strncmp( line, "Inactive:", 9 ) ) {
            sscanf( line+9, "%lu", &(stats->inactive_kB) );

        } else if( 0 == strncmp( line, "Dirty:", 6 ) ) {
            sscanf( line+6, "%lu", &(stats->dirty_kB) );

        } else if( 0 == strncmp( line, "HugePages_Total:", 16 ) ) {
            sscanf( line+16, "%lu", &(stats->hugepages_total) );

        } else if( 0 == strncmp( line, "HugePages_Free:", 15 ) ) {
            sscanf( line+15, "%lu", &(stats->hugepages_free) );

        } else if( 0 == strncmp( line, "Hugepagesize:", 13 ) ) {
            sscanf( line+13, "%lu", &(stats->hugepage_size_kB) );

        } else if( 0 == strncmp( line, "NFS_Unstable:", 13 ) ) {
            sscanf( line+13, "%lu", &(stats->nfs_uncommited_kB) );

        } else if( 0 == strncmp( line, "Committed_AS:", 13 ) ) {
            sscanf( line+13, "%lu", &(stats->commited_memory_kB) );
        }
    }

    fclose( fp );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Get Disk
// ==========================
SmErrorT sm_node_stats_get_disk( const char dev_name[],
    SmNodeDiskStatsT* stats )
{
    int n_scanned;
    char line[512];
    FILE* fp;

    memset( stats, 0, sizeof(SmNodeDiskStatsT) );

    fp = fopen( SM_NODE_STATS_PROC_DISKSTATS, "r" );
    if( NULL == fp )
    {
        DPRINTFE( "Failed to get disk statistics for %s from %s.",
                  dev_name, SM_NODE_STATS_PROC_DISKSTATS );
        return( SM_NOT_FOUND );
    }

    while( NULL != fgets( line, sizeof(line), fp ) )
    {
        n_scanned = sscanf( line, "%u %u %s "
                            "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                            &(stats->major_num), &(stats->minor_num),
                            stats->dev_name,
                            &(stats->reads_completed),
                            &(stats->reads_merged),
                            &(stats->sectors_read),
                            &(stats->ms_spent_reading),
                            &(stats->writes_completed),
                            &(stats->writes_merged),
                            &(stats->sectors_written),
                            &(stats->ms_spent_writing),
                            &(stats->io_inprogress),
                            &(stats->ms_spent_io),
                            &(stats->weighted_ms_spent_io) );
        if( 14 == n_scanned )
        {
            if( 0 == strcmp( dev_name, stats->dev_name ) )
            {
                DPRINTFD( "Disk (%s) stats read.", dev_name );
                break;
            }
        }

        memset( stats, 0, sizeof(SmNodeDiskStatsT) );
    }

    fclose( fp );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Get Disk By Index
// ===================================
SmErrorT sm_node_stats_get_disk_by_index( unsigned int index,
    SmNodeDiskStatsT* stats )
{
    unsigned int current_index = 0;
    int n_scanned;
    char line[512];
    FILE* fp;

    memset( stats, 0, sizeof(SmNodeDiskStatsT) );

    fp = fopen( SM_NODE_STATS_PROC_DISKSTATS, "r" );
    if( NULL == fp )
    {
        DPRINTFE( "Failed to get disk statistics from %s.",
                  SM_NODE_STATS_PROC_DISKSTATS );
        return( SM_NOT_FOUND );
    }

    while( NULL != fgets( line, sizeof(line), fp ) )
    {
        if( current_index == index )
        {
            n_scanned = sscanf( line, "%u %u %s "
                                "%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu",
                                &(stats->major_num), &(stats->minor_num),
                                stats->dev_name,
                                &(stats->reads_completed),
                                &(stats->reads_merged),
                                &(stats->sectors_read),
                                &(stats->ms_spent_reading),
                                &(stats->writes_completed),
                                &(stats->writes_merged),
                                &(stats->sectors_written),
                                &(stats->ms_spent_writing),
                                &(stats->io_inprogress),
                                &(stats->ms_spent_io),
                                &(stats->weighted_ms_spent_io) );
            if( 14 == n_scanned )
            {
                break;
            }

            memset( stats, 0, sizeof(SmNodeDiskStatsT) );
        }

        ++current_index;
    }

    fclose( fp );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Get Network Device
// ====================================
SmErrorT sm_node_stats_get_netdev( const char dev_name[],
    SmNodeNetDevStatsT* stats )
{
    int stats_start = 0;
    char line[512];
    char if_name[512];
    char dev_name_with_colon[512];
    FILE* fp;

    memset( stats, 0, sizeof(SmNodeNetDevStatsT) );

    snprintf( stats->dev_name, sizeof(stats->dev_name), "%s", dev_name );

    snprintf( dev_name_with_colon, sizeof(dev_name_with_colon), "%s:",
              dev_name );

    fp = fopen( SM_NODE_STATS_PROC_NET_DEV, "r" );
    if( NULL == fp )
    {
        DPRINTFE( "Failed to get network device statistics for %s from %s.",
                  dev_name, SM_NODE_STATS_PROC_NET_DEV );
        return( SM_NOT_FOUND );
    }

    while( NULL != fgets( line, sizeof(line), fp ) )
    {
		stats_start = strcspn( line, ":" );
        sscanf( line, "%s:", if_name );
        if( 0 == strncmp( if_name, dev_name_with_colon, strlen(if_name) ) )
        {
            sscanf( line+stats_start+2,
                    "%llu %llu %llu %llu %llu %llu %llu %llu "
                    "%llu %llu %llu %llu %llu %llu %llu %llu",
                    &(stats->rx_bytes),
                    &(stats->rx_packets),
                    &(stats->rx_errors),
                    &(stats->rx_dropped),
                    &(stats->rx_fifo_errors),
                    &(stats->rx_frame_errors),
                    &(stats->rx_compressed_packets),
                    &(stats->rx_multicast_frames),
                    &(stats->tx_bytes),
                    &(stats->tx_packets),
                    &(stats->tx_errors),
                    &(stats->tx_dropped),
                    &(stats->tx_fifo_errors),
                    &(stats->tx_collisions),
                    &(stats->tx_carrier_loss),
                    &(stats->tx_compressed_packets) );
        }
    }

    fclose( fp );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Get Process Status
// ====================================
SmErrorT sm_node_stats_get_process_status( int pid,
    SmNodeProcessStatusT* status )
{
    unsigned long long ms;
    unsigned long long ns;
    char filename[80];
    char line[512];
    FILE* fp;

    memset( status, 0, sizeof(SmNodeProcessStatusT) );

    // Process Stat Information
    snprintf( filename, sizeof(filename), "/proc/%i/stat", pid );

    fp = fopen( filename, "r" );
    if( NULL == fp )
    {
        // Could be a short lived process.
        DPRINTFD( "Failed to get process (%i) status.", pid );
        return( SM_NOT_FOUND );
    }

    while( NULL != fgets( line, sizeof(line), fp ) )
    {
        sscanf( line, "%d %s %c %d %d",
                &(status->pid), status->name, &(status->state),
                &(status->ppid), &(status->pgrp) );
    }

    fclose( fp );

    // Process Scheduler Information
    snprintf( filename, sizeof(filename), "/proc/%i/sched", pid );

    fp = fopen( filename, "r" );
    if( NULL == fp )
    {
        // Could be a short lived process.
        DPRINTFD( "Failed to get process (%i) status.", pid );
        return( SM_NOT_FOUND );
    }

    while( NULL != fgets( line, sizeof(line), fp ) )
    {
        if( 0 == strncmp( "se.statistics.block_start", line, 25 ) )
        {
            sscanf( line, "se.statistics.block_start : %llu.%llu",
                    &ms, &ns );
            status->block_start_ns = (ms * 1000000) + ns;
            continue;
        }

        if( 0 == strncmp( "nr_switches", line, 11 ) )
        {
            sscanf( line, "nr_switches : %llu", &(status->nr_switches) );
            continue;
        }

        if( 0 == strncmp( "nr_voluntary_switches", line, 21 ) )
        {
            sscanf( line, "nr_voluntary_switches : %llu", 
                    &(status->nr_voluntary_switches) );
            continue;
        }

        if( 0 == strncmp( "nr_involuntary_switches", line, 23 ) )
        {
            sscanf( line, "nr_involuntary_switches : %llu", 
                    &(status->nr_involuntary_switches) );
            continue;
        }
    }

    fclose( fp );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Initialize
// ============================
SmErrorT sm_node_stats_initialize( void )
{
    _user_hz = sysconf(_SC_CLK_TCK);
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Node Statistics - Finalize
// ==========================
SmErrorT sm_node_stats_finalize( void )
{
    _user_hz = 0;
    return( SM_OKAY );
}
// ****************************************************************************
