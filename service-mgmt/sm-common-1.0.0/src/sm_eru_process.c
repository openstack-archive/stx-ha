//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_eru_process.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h> 
#include <sched.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <getopt.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_utils.h"
#include "sm_selobj.h"
#include "sm_time.h"
#include "sm_timer.h"
#include "sm_hw.h"
#include "sm_thread_health.h"
#include "sm_node_utils.h"
#include "sm_node_stats.h"
#include "sm_eru_db.h"

#define SM_ERU_PROCESS_TICK_INTERVAL_IN_MS                             1000
#define SM_ERU_PROCESS_STATS_TIMER_IN_MS                              30000

static sig_atomic_t _stay_on = 1;
static SmTimerIdT _stats_timer_id = SM_TIMER_ID_INVALID;
static SmEruDatabaseEntryT _cpu_prev_entry;
static char _interfaces[SM_INTERFACE_MAX][SM_INTERFACE_NAME_MAX_CHAR];

// ****************************************************************************
// Event Recorder Unit Process - Find Interface
// ============================================
static int sm_eru_process_find_interface( char interface_name[] )
{
    int if_i;
    for( if_i=0; SM_INTERFACE_MAX > if_i; ++if_i )
    {
        if( 0 == strcmp( &(_interfaces[if_i][0]), interface_name ) )
        {
            break;
        }
    }

    return( if_i );
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Process - Add Interface
// ===========================================
void sm_eru_process_add_interface( char interface_name[] )
{
    if( SM_INTERFACE_MAX <= sm_eru_process_find_interface( interface_name ) )
    {
        int if_i;
        for( if_i=0; SM_INTERFACE_MAX > if_i; ++if_i )
        {
            if( '\0' == _interfaces[if_i][0] )
            {
                snprintf( &(_interfaces[if_i][0]), SM_INTERFACE_NAME_MAX_CHAR,
                          "%s", interface_name );
                break;
            }
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Process - Load Interfaces
// =============================================
void sm_eru_process_load_interfaces( void )
{
    SmErrorT error;

    memset( _interfaces, 0, sizeof(_interfaces) );

    error = sm_node_utils_get_mgmt_interface( &(_interfaces[0][0]) );
    if(( SM_OKAY != error )&&( SM_NOT_FOUND != error ))
    {
        DPRINTFE( "Failed to look up management interface, error=%s.",
                  sm_error_str(error) );
    }

    error = sm_node_utils_get_oam_interface( &(_interfaces[1][0]) );
    if(( SM_OKAY != error )&&( SM_NOT_FOUND != error ))
    {
        DPRINTFE( "Failed to look up oam interface, error=%s.",
                  sm_error_str(error) );
    }

    error = sm_node_utils_get_cluster_host_interface( &(_interfaces[2][0]) );
    if(( SM_OKAY != error )&&( SM_NOT_FOUND != error ))
    {
        DPRINTFE( "Failed to look up cluster-host interface, error=%s.",
                  sm_error_str(error) );
    }
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Process - Qdisc Information Callback 
// ========================================================
static void sm_eru_process_qdisc_info_callback( SmHwQdiscInfoT* qdisc )
{
    int if_i;

    if_i = sm_eru_process_find_interface( qdisc->interface_name );
    if( SM_INTERFACE_MAX > if_i )
    {
        SmEruDatabaseEntryT entry;

        memset( &entry, 0, sizeof(SmEruDatabaseEntryT) );    
        entry.type = SM_ERU_DATABASE_ENTRY_TYPE_TC_STATS;
        memcpy( &(entry.u.tc_stats), qdisc, sizeof(SmHwQdiscInfoT) );
        sm_eru_db_write( &entry );
    }

    // Ensure all the records are written to disk after this callback
    sm_eru_db_sync();
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Process - Interface Change Callback 
// =======================================================
static void sm_eru_process_interface_change_callback(
    SmHwInterfaceChangeDataT* if_change )
{
    int if_i;

    if_i = sm_eru_process_find_interface( if_change->interface_name );
    if( SM_INTERFACE_MAX > if_i )
    {
        SmEruDatabaseEntryT entry;

        memset( &entry, 0, sizeof(SmEruDatabaseEntryT) );    
        entry.type = SM_ERU_DATABASE_ENTRY_TYPE_IF_CHANGE;
        memcpy( &(entry.u.if_change), if_change,
                sizeof(SmHwInterfaceChangeDataT) );
        sm_eru_db_write( &entry );
    }

    // Ensure all the records are written to disk after this callback
    sm_eru_db_sync();
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Process - IP Change Callback 
// ================================================
static void sm_eru_process_ip_change_callback( SmHwIpChangeDataT* ip_change )
{
    int if_i;

    if_i = sm_eru_process_find_interface( ip_change->interface_name );
    if( SM_INTERFACE_MAX > if_i )
    {
        SmEruDatabaseEntryT entry;

        memset( &entry, 0, sizeof(SmEruDatabaseEntryT) );    
        entry.type = SM_ERU_DATABASE_ENTRY_TYPE_IP_CHANGE;
        memcpy( &(entry.u.ip_change), ip_change,
                sizeof(SmHwIpChangeDataT) );
        sm_eru_db_write( &entry );
    }

    // Ensure all the records are written to disk after this callback
    sm_eru_db_sync();
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Process - Statistics 
// ========================================
static bool sm_eru_process_stats( SmTimerIdT timer_id, int64_t user_data )
{
    SmEruDatabaseEntryT entry;
    SmNodeDiskStatsT* disk_stats = &(entry.u.disk_stats);
    SmErrorT error;

    sm_eru_process_load_interfaces();

    // CPU Statistics.
    memset( &entry, 0, sizeof(SmEruDatabaseEntryT) );    
    entry.type = SM_ERU_DATABASE_ENTRY_TYPE_CPU_STATS;
    error = sm_node_stats_get_cpu( "", &(entry.u.cpu_stats) );
    if( SM_OKAY == error )
    {
        if( -1 != _cpu_prev_entry.index )
        {
            SmEruDatabaseEntryT delta_entry;

            memcpy( &delta_entry, &entry, sizeof(SmEruDatabaseEntryT) );

            delta_entry.u.cpu_stats.user_cpu_usage
                = entry.u.cpu_stats.user_cpu_usage
                - _cpu_prev_entry.u.cpu_stats.user_cpu_usage;
            delta_entry.u.cpu_stats.nice_cpu_usage
                = entry.u.cpu_stats.nice_cpu_usage
                - _cpu_prev_entry.u.cpu_stats.nice_cpu_usage;
            delta_entry.u.cpu_stats.system_cpu_usage
                = entry.u.cpu_stats.system_cpu_usage
                - _cpu_prev_entry.u.cpu_stats.system_cpu_usage;
            delta_entry.u.cpu_stats.idle_task_cpu_usage
                = entry.u.cpu_stats.idle_task_cpu_usage
                - _cpu_prev_entry.u.cpu_stats.idle_task_cpu_usage;
            delta_entry.u.cpu_stats.iowait_cpu_usage
                = entry.u.cpu_stats.iowait_cpu_usage
                - _cpu_prev_entry.u.cpu_stats.iowait_cpu_usage;
            delta_entry.u.cpu_stats.irq_cpu_usage
                = entry.u.cpu_stats.irq_cpu_usage
                - _cpu_prev_entry.u.cpu_stats.irq_cpu_usage;
            delta_entry.u.cpu_stats.soft_irq_cpu_usage
                = entry.u.cpu_stats.soft_irq_cpu_usage
                - _cpu_prev_entry.u.cpu_stats.soft_irq_cpu_usage;
            delta_entry.u.cpu_stats.steal_cpu_usage
                = entry.u.cpu_stats.steal_cpu_usage
                - _cpu_prev_entry.u.cpu_stats.steal_cpu_usage;
            delta_entry.u.cpu_stats.guest_cpu_usage
                = entry.u.cpu_stats.guest_cpu_usage
                - _cpu_prev_entry.u.cpu_stats.guest_cpu_usage;
            delta_entry.u.cpu_stats.guest_nice_cpu_usage
                = entry.u.cpu_stats.guest_nice_cpu_usage
                - _cpu_prev_entry.u.cpu_stats.guest_nice_cpu_usage;

            sm_eru_db_write( &delta_entry );
        }
        memcpy( &_cpu_prev_entry, &entry, sizeof(_cpu_prev_entry) );
    }

    // Memory Statistics.
    memset( &entry, 0, sizeof(SmEruDatabaseEntryT) );    
    entry.type = SM_ERU_DATABASE_ENTRY_TYPE_MEM_STATS;
    error = sm_node_stats_get_memory( &(entry.u.mem_stats) );
    if( SM_OKAY == error )
    {
        sm_eru_db_write( &entry );
    }

    // Disk Statistics
    int disk_i;
    for( disk_i=0; SM_DISK_MAX > disk_i; ++disk_i )
    {
        memset( &entry, 0, sizeof(SmEruDatabaseEntryT) );
        entry.type = SM_ERU_DATABASE_ENTRY_TYPE_DISK_STATS;
        error = sm_node_stats_get_disk_by_index( disk_i, disk_stats );
        if( SM_OKAY == error )
        {
            if(( 0 != disk_stats->reads_completed )||
               ( 0 != disk_stats->reads_merged )||
               ( 0 != disk_stats->sectors_read )||
               ( 0 != disk_stats->ms_spent_reading )||
               ( 0 != disk_stats->writes_completed )||
               ( 0 != disk_stats->writes_merged )||
               ( 0 != disk_stats->sectors_written )||
               ( 0 != disk_stats->ms_spent_writing )||
               ( 0 != disk_stats->io_inprogress )||
               ( 0 != disk_stats->ms_spent_io )||
               ( 0 != disk_stats->weighted_ms_spent_io ))
            {
                sm_eru_db_write( &entry );
            }
        }
    }

    // Net Device Statistics.
    int if_i;
    for( if_i=0; SM_INTERFACE_MAX > if_i; ++if_i )
    {
        if( '\0' != _interfaces[if_i][0] )
        {
            memset( &entry, 0, sizeof(SmEruDatabaseEntryT) );    
            entry.type = SM_ERU_DATABASE_ENTRY_TYPE_NET_STATS;
            error = sm_node_stats_get_netdev( _interfaces[if_i],
                                              &(entry.u.net_stats) );
            if( SM_OKAY == error )
            {
                sm_eru_db_write( &entry );
            }
        }
    }

    // Traffic Class Statistics.
    sm_hw_get_all_qdisc_async();

    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Process - Signal Handler
// ============================================
static void sm_eru_process_signal_handler( int signum )
{
    switch( signum )
    {
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
            _stay_on = 0;
        break;

        case SIGCONT:
            DPRINTFD( "Ignoring signal SIGCONT (%i).", signum );
        break;

        default:
            DPRINTFD( "Signal (%i) ignored.", signum );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Process - Setup Signal Handler
// ==================================================
static void sm_eru_process_setup_signal_handler( void )
{
    struct sigaction sa;

    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = sm_eru_process_signal_handler;

    sigaction( SIGINT,  &sa, NULL );
    sigaction( SIGTERM, &sa, NULL );
    sigaction( SIGQUIT, &sa, NULL );
    sigaction( SIGCONT, &sa, NULL );

    signal( SIGCHLD, SIG_IGN );
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Process - Initialize
// ========================================
static SmErrorT sm_eru_process_initialize( void )
{
    SmHwCallbacksT callbacks;
    SmErrorT error;

    memset( &_cpu_prev_entry, 0, sizeof(_cpu_prev_entry) );
    _cpu_prev_entry.index = -1;

    error = sm_selobj_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize selection object module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_timer_initialize( SM_ERU_PROCESS_TICK_INTERVAL_IN_MS );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize timer module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    memset( &callbacks, 0, sizeof(callbacks) );
    callbacks.qdisc_info = sm_eru_process_qdisc_info_callback;
    callbacks.interface_change = sm_eru_process_interface_change_callback;
    callbacks.ip_change = sm_eru_process_ip_change_callback;

    error = sm_hw_initialize( &callbacks );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize hardware module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_node_stats_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize node stats module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_thread_health_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize thread health module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_eru_db_initialize( NULL, false );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize eru database module, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }
    
    error = sm_timer_register( "eru stats",
                               SM_ERU_PROCESS_STATS_TIMER_IN_MS,
                               sm_eru_process_stats, 0, &_stats_timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create stats collection timer, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Process - Finalize
// ======================================
static SmErrorT sm_eru_process_finalize( void )
{
    SmErrorT error;

    if( SM_TIMER_ID_INVALID != _stats_timer_id )
    {
        error = sm_timer_deregister( _stats_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to cancel stats collection timer, error=%s.",
                      sm_error_str( error ) );
        }

        _stats_timer_id = SM_TIMER_ID_INVALID;
    }

    error = sm_eru_db_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize eru database module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_thread_health_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize thread health module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_node_stats_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize node stats module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_hw_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize hardware module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_timer_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize timer module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_selobj_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize selection object module, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Process - Main
// ==================================
SmErrorT sm_eru_process_main( int argc, char *argv[], char *envp[] )
{
    bool thread_health;
    SmErrorT error;

    sm_eru_process_setup_signal_handler();

    DPRINTFI( "Starting" );

    if( sm_utils_process_running( SM_ERU_PROCESS_PID_FILENAME ) )
    {
        DPRINTFI( "Already running an instance of sm-eru." );
        return( SM_OKAY );
    }

    if( !sm_utils_set_pid_file( SM_ERU_PROCESS_PID_FILENAME ) )
    {
        DPRINTFE( "Failed to write pid file for sm-eru, error=%s.",
                  strerror(errno) );
        return( SM_FAILED );
    }

    error = sm_eru_process_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed initialize process, error=%s.", 
                  sm_error_str(error) );
        return( error );
    }

    DPRINTFI( "Started." );

    while( _stay_on )
    {
        error = sm_selobj_dispatch( SM_ERU_PROCESS_TICK_INTERVAL_IN_MS );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Selection object dispatch failed, error=%s.",
                      sm_error_str(error) );
            break;
        }

        error = sm_thread_health_check( &thread_health );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to check thread health, error=%s.",
                      sm_error_str(error) );
            break;
        }

        if( !thread_health )
        {
            DPRINTFE( "Thread health check failed." );
            break;
        }
    }

    DPRINTFI( "Shutting down." );

    error = sm_eru_process_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize process, error=%s.",
                  sm_error_str( error ) );
    }

    DPRINTFI( "Shutdown complete." );

    return( SM_OKAY );
}
// ****************************************************************************
