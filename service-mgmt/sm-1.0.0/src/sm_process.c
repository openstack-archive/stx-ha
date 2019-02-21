//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_process.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/select.h>
#include <getopt.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_time.h"
#include "sm_utils.h"
#include "sm_selobj.h"
#include "sm_timer.h"
#include "sm_heartbeat.h"
#include "sm_log.h"
#include "sm_alarm.h"
#include "sm_thread_health.h"
#include "sm_process_death.h"
#include "sm_hw.h"
#include "sm_msg.h"
#include "sm_db.h"
#include "sm_node_utils.h"
#include "sm_node_stats.h"
#include "sm_node_api.h"
#include "sm_service_domain_api.h"
#include "sm_service_domain_interface_api.h"
#include "sm_service_group_api.h"
#include "sm_service_api.h"
#include "sm_service_action.h"
#include "sm_service_heartbeat_api.h"
#include "sm_service_heartbeat_thread.h"
#include "sm_service_domain_scheduler.h"
#include "sm_main_event_handler.h"
#include "sm_troubleshoot.h"
#include "sm_service_action_table.h"
#include "sm_heartbeat_thread.h"
#include "sm_failover.h"
#include "sm_task_affining_thread.h"
#include "sm_worker_thread.h"
#include "sm_configuration_table.h"

#define SM_PROCESS_DB_CHECKPOINT_INTERVAL_IN_MS         30000
#define SM_PROCESS_TICK_INTERVAL_IN_MS                    200
#define SM_PROCESS_PAUSE_IN_MS                          30000

static sig_atomic_t _stay_on = 1;
static sig_atomic_t _reap_children = 0;
static sig_atomic_t _do_reload_data = 0;
static sig_atomic_t _do_dump_data = 0;
static sig_atomic_t _about_to_patch = 0;
static int _last_signum = 0;
static bool _is_aio = false;
static bool _is_aio_simplex = false;
static bool _is_aio_duplex = false;

// ****************************************************************************
// Process - Reap Children
// =======================
static void sm_process_reap_children( void )
{
    if( _reap_children )
    {
        pid_t pid;
        int status;

        while( 0 < (pid = waitpid( -1, &status, WNOHANG | WUNTRACED )) )
        {
            if( WIFEXITED( status ) )
            {
                sm_process_death_save( pid, WEXITSTATUS( status ) );
            } else {
                sm_process_death_save( pid, SM_PROCESS_FAILED );
            }
        }

        _reap_children = 0;
    }
}
// ****************************************************************************

// ****************************************************************************
// Process - Signal Handler
// ========================
static void sm_process_signal_handler( int signum )
{
    switch( signum )
    {
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
            _stay_on = 0;
        break;

        case SIGCHLD:
            _reap_children = 1;
        break;

        case SIGHUP:
            _do_reload_data = 1;
        break;

        case SIGUSR1:
            _do_dump_data = 1;
        break;

        case SIGUSR2:
            _about_to_patch = 1;
        break;

        case SIGCONT:
            DPRINTFD( "Ignoring signal SIGCONT (%i).", signum );
        break;

        case SIGPIPE:
            DPRINTFD( "Ignoring signal SIGPIPE (%i).", signum );
        break;

        default:
            DPRINTFD( "Signal (%i) ignored.", signum );
        break;
    }

    _last_signum = signum;
}
// ****************************************************************************

// ****************************************************************************
// Process - Setup Signal Handler
// ==============================
static void sm_process_setup_signal_handler( void )
{
    struct sigaction sa;

    memset( &sa, 0, sizeof(sa) );
    sa.sa_handler = sm_process_signal_handler;

    sigaction( SIGINT,  &sa, NULL );
    sigaction( SIGTERM, &sa, NULL );
    sigaction( SIGQUIT, &sa, NULL );
    sigaction( SIGCHLD, &sa, NULL );
    sigaction( SIGUSR1,  &sa, NULL );
    sigaction( SIGUSR2,  &sa, NULL );
    sigaction( SIGCONT, &sa, NULL );
    sigaction( SIGPIPE, &sa, NULL );
    sigaction( SIGHUP, &sa, NULL );
}
// ****************************************************************************

// ****************************************************************************
// Process - Initialize
// ====================
static SmErrorT sm_process_initialize( void )
{
    SmErrorT error;

    error = sm_selobj_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize selection object module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_timer_initialize( SM_PROCESS_TICK_INTERVAL_IN_MS );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize timer module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = SmWorkerThread::initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize worker thread, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_msg_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize messaging module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_node_stats_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize node stats module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_thread_health_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize thread health module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_alarm_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize alarm module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_log_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize log module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    if (_is_aio_simplex)
    {
    	sm_heartbeat_thread_disable_heartbeat();
    }
    else
    {
    	error = sm_heartbeat_initialize();
    	if( SM_OKAY != error )
    	{
    	    DPRINTFE( "Failed to initialize heartbeat module, error=%s.",
    	              sm_error_str( error ) );
   	    return( SM_FAILED );
    	}
    }

    error = sm_process_death_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize process death module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_db_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize database module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_node_api_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize node api module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_service_domain_api_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain api module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_service_domain_interface_api_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain interface api module, "
                  "error=%s.", sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_service_group_api_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group api module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_service_action_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service action module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_service_api_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service api module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_service_heartbeat_api_initialize( true );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service heartbeat api module, "
                  "error=%s.", sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_failover_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize failover handler module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_service_heartbeat_thread_start();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed start service heartbeat thread, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    error = sm_main_event_handler_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize main event handler module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    // Start a task affining thread for AIO duplex system
    if(_is_aio_duplex)
    {
        error = sm_task_affining_thread_start();
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to start the task affining thread, error=%s.",
                      sm_error_str( error ) );
            return( SM_FAILED );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Process - Finalize
// ==================
static SmErrorT sm_process_finalize( void )
{
    SmErrorT error;

    // Stop the task affining thread if it is AIO duplex
    if(_is_aio_duplex)
    {
        error = sm_task_affining_thread_stop();
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to stop task affining thread, error=%s.",
                      sm_error_str( error ) );
        }
    }

    error = sm_main_event_handler_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize main event handler module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_heartbeat_thread_stop();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed start service heartbeat thread, error=%s.",
                  sm_error_str(error) );
    }

    error = sm_failover_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize failover handler module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_heartbeat_api_finalize( true );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service heartbeat api module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_api_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service api module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_action_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service action module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_group_api_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service group api module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_domain_interface_api_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain interface api module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_api_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain api module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_node_api_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize interface api module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_db_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize database module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_process_death_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize process death module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_heartbeat_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize heartbeat module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_alarm_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize alarm module, error=%s.",
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

    error = sm_msg_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize messaging module, error=%s.",
                  sm_error_str( error ) );
    }

    error = SmWorkerThread::finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize worker thread, error=%s.",
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

    error = sm_log_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize log module, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Process - Wait For Node Configuration
// =====================================
static SmErrorT sm_process_wait_node_configuration( void )
{
    bool config_complete;
    SmErrorT error;

    while( _stay_on )
    {
        error = sm_node_utils_config_complete( &config_complete );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to determine if node configuration "
                      "completed, error=%s.", sm_error_str(error) );
            sleep( 10 );
            continue;
        }

        if( config_complete )
        {
            DPRINTFI( "Node configuration completed." );
            break;

        } else {
            DPRINTFI( "Waiting for node configuration to complete." );
            sleep( 10 );
        }
    }

    if( _stay_on )
    {
        error = SM_OKAY;
    } else {
        DPRINTFI( "Shutdown signalled, last-signal=%i.", _last_signum );
        error = SM_FAILED;
    }
    return error;
}
// ****************************************************************************

// ****************************************************************************
// Process - get process priority setting
// ==============
static int get_process_nice_val()
{
    char buf[SM_CONFIGURATION_VALUE_MAX_CHAR + 1];
    int nice_val = -2;
    if( SM_OKAY == sm_configuration_table_get("sm_process_priority", buf, sizeof(buf) - 1) )
    {
        if(buf[0] != '\0')
        {
            nice_val = atoi(buf);
        }
    }

    if(nice_val > -2 || nice_val < -20)
    {
        DPRINTFE("Invalid sm_process_priority value %d, reset to default (-2)", nice_val);
        nice_val = -2;
    }
    else
    {
        DPRINTFI("sm_process_priority value is set to %d", nice_val);
    }
    return nice_val;
}
// ****************************************************************************

// ****************************************************************************
// Process - Main
// ==============
SmErrorT sm_process_main( int argc, char *argv[], char *envp[] )
{
    int result;
    long ms_expired;
    bool thread_health;
    bool do_patch = false;
    SmTimeT db_checkpoint_time_prev;
    SmTimeT patch_time_prev;
    SmErrorT error;

    int opt = 0;
    static struct option long_options[] = {
        {"interval-extension", 1, 0, 'i'},
        {"timeout-extension",  1, 0, 't'},
        {0, 0, 0, 0}
    };
    int long_index = 0;

    sm_process_setup_signal_handler();

    DPRINTFI( "Starting. SW built at %s %s", __DATE__, __TIME__ );

    if( sm_utils_process_running( SM_PROCESS_PID_FILENAME ) )
    {
        DPRINTFI( "Already running an instance of sm." );
        return( SM_OKAY );
    }

    if( !sm_utils_set_pid_file( SM_PROCESS_PID_FILENAME ) )
    {
        DPRINTFE( "Failed to write pid file for sm, error=%s.",
                  strerror(errno) );
        return( SM_FAILED );
    }

    if( 0 > mkdir( SM_RUN_DIRECTORY, 0700 ) )
    {
        if( EEXIST == errno )
        {
            DPRINTFI( "Run directory (%s) exists.", SM_RUN_DIRECTORY );
        } else {
            DPRINTFE( "Run directory (%s) creation failed, error=%s.",
                      SM_RUN_DIRECTORY, strerror(errno) );
            return( SM_FAILED );
        }
    }

    // Check for cmdline args
    while ((opt = getopt_long(argc, argv, "i:t:",
                              long_options,
                              &long_index)) != -1)
    {
        switch (opt)
        {
            case 'i':
            {
                sm_service_action_table_set_interval_extension( atoi(optarg) );
                break;
            }
            case 't':
            {
                sm_service_action_table_set_timeout_extension( atoi(optarg) );
                break;
            }
            default:
            {
                DPRINTFE( "Failed to process cmdline arg." );
                return( SM_FAILED );
            }
        }
    }

    error = sm_process_wait_node_configuration();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to wait for node configuration, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    DPRINTFI( "Configuring Databases" );

    error = sm_db_configure( SM_DATABASE_NAME, SM_DB_TYPE_MAIN );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed configuring database, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    error = sm_db_configure( SM_HEARTBEAT_DATABASE_NAME, SM_DB_TYPE_HEARTBEAT );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed configuring heartbeat database, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    error = sm_node_utils_is_aio(&_is_aio);
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to check for AIO system, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    error = sm_node_utils_is_aio_simplex(&_is_aio_simplex);
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to check for AIO simplex system, error=%s.",
                  sm_error_str(error) );
    	return( error );
    }

    error = sm_node_utils_is_aio_duplex(&_is_aio_duplex);
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to check for AIO duplex system, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    error = sm_process_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed initialize process, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    int process_nice_val = get_process_nice_val();

    result = setpriority( PRIO_PROCESS, getpid(), process_nice_val);
    if( 0 > result )
    {
        DPRINTFE( "Failed to set priority of process, error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    error = sm_utils_set_boot_complete();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to set boot complete, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    DPRINTFI( "Started." );

    sm_time_get( &db_checkpoint_time_prev );

    while( _stay_on )
    {
        error = sm_selobj_dispatch( SM_PROCESS_TICK_INTERVAL_IN_MS );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Selection object dispatch failed, error=%s.",
                      sm_error_str(error) );
            break;
        }

        sm_process_reap_children();

        ms_expired = sm_time_get_elapsed_ms( &db_checkpoint_time_prev );
        if( SM_PROCESS_DB_CHECKPOINT_INTERVAL_IN_MS <= ms_expired )
        {
            error = sm_db_checkpoint( SM_DATABASE_NAME );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Database (%s) checkpoint failed, error=%s.",
                          SM_DATABASE_NAME, sm_error_str(error) );
            }

            error = sm_db_checkpoint( SM_HEARTBEAT_DATABASE_NAME );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Database (%s) checkpoint failed, error=%s.",
                          SM_HEARTBEAT_DATABASE_NAME, sm_error_str(error) );
            }

            sm_time_get( &db_checkpoint_time_prev );
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
            sm_troubleshoot_dump_data( "thread health check failed" );
            break;
        }

        if( _do_reload_data )
        {
            DPRINTFI( "Reload data signalled." );
            sm_main_event_handler_reload_data();
            _do_reload_data = 0;
        }

        if( _do_dump_data )
        {
            DPRINTFI( "Dump data signalled." );
            sm_troubleshoot_dump_data( "user request" );
            _do_dump_data = 0;
        }

        if( _about_to_patch )
        {
            do_patch = true;
            sm_time_get( &patch_time_prev );
            _about_to_patch = 0;
            DPRINTFI( "About to patch signalled." );
        }

        if( do_patch )
        {
            ms_expired = sm_time_get_elapsed_ms( &patch_time_prev );
            if( SM_PROCESS_PAUSE_IN_MS < ms_expired )
            {
                do_patch = false;
                DPRINTFI( "Too much time elapsed between patch signal and "
                          "shutdown, ms_expired=%li, max=%i.", ms_expired,
                          SM_PROCESS_PAUSE_IN_MS );
            }
        }
    }

    if( do_patch )
    {
        ms_expired = sm_time_get_elapsed_ms( &patch_time_prev );
        if( SM_PROCESS_PAUSE_IN_MS < ms_expired )
        {
            DPRINTFI( "Too much time elapsed between patch signal and "
                      "shutdown, ms_expired=%li, max=%i.", ms_expired,
                      SM_PROCESS_PAUSE_IN_MS );
        } else {
            DPRINTFI( "Sending pause signal." );

            int retry_i;
            for( retry_i=0; 5 > retry_i; ++retry_i )
            {
                sm_service_domain_api_pause_all( SM_PROCESS_PAUSE_IN_MS );
            }
        }
    }

    DPRINTFI( "Shutting down." );

    error = sm_process_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed finalize process, error=%s.",
                  sm_error_str(error) );
    }

    DPRINTFI( "Shutdown complete." );

    return( SM_OKAY );
}
// ****************************************************************************
