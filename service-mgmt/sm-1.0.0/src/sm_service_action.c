//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_action.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "sm_types.h"
#include "sm_utils.h"
#include "sm_debug.h"
#include "sm_sha512.h"
#include "sm_service_action_table.h"
#include "sm_service_action_result_table.h"

#define SM_SERVICE_ACTION_MAX_DELAY_IN_SECS                     4
#define SM_SERVICE_ACTION_TIMER_SKEW_IN_MS                  60000
#define SM_SERVICE_ACTION_VALIDATE_TIMER_IN_MS              60000

// ****************************************************************************
// Service Action - Validate
// =========================
static void sm_service_action_validate( SmServiceActionDataT* action_data )
{
    int bytes;
    FILE* fp;
    long ms_expired;
    unsigned char data[2048];
    SmSha512HashT hash;
    SmSha512ContextT context;
    char plugin_exec[SM_SERVICE_ACTION_PLUGIN_EXEC_MAX_CHAR];

    ms_expired = sm_time_get_elapsed_ms( &(action_data->last_hash_validate) );
    if( ms_expired < SM_SERVICE_ACTION_VALIDATE_TIMER_IN_MS  )
    {
        DPRINTFD( "Not enough time has elapsed, since last validate." );
        return;
    }

    sm_time_get( &(action_data->last_hash_validate) );

    if( 0 == strcmp( SM_SERVICE_ACTION_PLUGIN_TYPE_LSB_SCRIPT,
                     action_data->plugin_type ) )
    {
        snprintf( plugin_exec, sizeof(plugin_exec), "%s/%s",
                  SM_SERVICE_ACTION_PLUGIN_TYPE_LSB_DIR,
                  action_data->plugin_name );

    } else if( 0 == strcmp( SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_SCRIPT,
                            action_data->plugin_type ) )
    {
        snprintf( plugin_exec, sizeof(plugin_exec), "%s/%s/%s",
                  SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_PLUGIN_DIR,
                  action_data->plugin_class, action_data->plugin_name );
    } else {
        snprintf( plugin_exec, sizeof(plugin_exec), "%s",
                  action_data->plugin_name );
    }

    fp = fopen( plugin_exec, "rb" );
    if( NULL == fp )
    {
        DPRINTFE( "Failed to open file (%s).", plugin_exec );
        return;
    }

    sm_sha512_initialize( &context );

    while( 0 != (bytes = fread( data, 1, 2048, fp )) )
    {
        sm_sha512_update( &context, data, bytes );
    }

    sm_sha512_finalize( &context, &hash );

    if( 0 != memcmp( &(action_data->hash.bytes[0]), &(hash.bytes[0]),
                     SM_SHA512_HASH_SIZE ) )
    {
        char was_hash_str[SM_SHA512_HASH_STR_SIZE];
        char now_hash_str[SM_SHA512_HASH_STR_SIZE];

        sm_sha512_hash_str( was_hash_str, &(action_data->hash) );
        sm_sha512_hash_str( now_hash_str, &hash );

        DPRINTFI( "Plugin (%s) has been changed, was=%s, now=%s.",
                  plugin_exec, was_hash_str, now_hash_str );

        memcpy( &(action_data->hash), &hash, sizeof(hash) );
    }

    fclose( fp );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Exist
// ======================
SmErrorT sm_service_action_exist( char service_name[], SmServiceActionT action )
{
    SmServiceActionDataT* action_data;

    action_data = sm_service_action_table_read( service_name, action );
    if( NULL == action_data )
    {
        DPRINTFD( "Service (%s) action (%s) does not exist.", service_name,
                  sm_service_action_str( action ) );
        return( SM_NOT_FOUND );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Interval
// =========================
SmErrorT sm_service_action_interval( char service_name[],
    SmServiceActionT action, int* interval_in_ms )
{
    SmServiceActionDataT* action_data;

    *interval_in_ms = 0;

    action_data = sm_service_action_table_read( service_name, action );
    if( NULL == action_data )
    {
        DPRINTFD( "Service (%s) action (%s) does not exist.", service_name,
                  sm_service_action_str( action ) );
        return( SM_NOT_FOUND );
    }

    *interval_in_ms = action_data->interval_in_secs * 1000;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Maximum Retries
// ================================
SmErrorT sm_service_action_max_retries( char service_name[],
    SmServiceActionT action, int* max_failure_retries,
    int* max_timeout_retries, int* max_total_retries )
{
    SmServiceActionDataT* action_data;

    *max_failure_retries = 0;
    *max_timeout_retries = 0;
    *max_total_retries = 0;

    action_data = sm_service_action_table_read( service_name, action );
    if( NULL == action_data )
    {
        DPRINTFD( "Service (%s) action (%s) does not exist.", service_name,
                  sm_service_action_str( action ) );
        return( SM_NOT_FOUND );
    }

    *max_failure_retries = action_data->max_failure_retries;
    *max_timeout_retries = action_data->max_timeout_retries;
    *max_total_retries   = action_data->max_total_retries;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Plugin Result
// ==============================
static SmErrorT sm_service_action_plugin_result( char service_name[],
    SmServiceActionT action, char plugin_type[], char plugin_name[],
    char plugin_command[], char plugin_exit_code[],
    SmServiceActionResultDataT** action_result )
{
    char plugin_command_default[] = "default";
    char plugin_exit_code_other[] = "other";

    // Map the plugin result to an action result with the given data.
    *action_result = sm_service_action_result_table_read( plugin_type,
                        plugin_name, plugin_command, plugin_exit_code );
    if( NULL != *action_result )
    {
        DPRINTFD( "Read service (%s) action (%s) plugin (%s, %s, %s, %s) data.",
                  service_name, sm_service_action_str( action ), plugin_type,
                  plugin_name, plugin_command, plugin_exit_code );
        return( SM_OKAY );
    }

    // Map the plugin result to an action result with the plugin exit
    // code set to "other".
    *action_result = sm_service_action_result_table_read( plugin_type,
                        plugin_name, plugin_command, plugin_exit_code_other );
    if( NULL != *action_result )
    {
        DPRINTFD( "Read service (%s) action (%s) plugin (%s, %s, %s, other) "
                  "data.", service_name, sm_service_action_str( action ),
                  plugin_type, plugin_name, plugin_command );
        return( SM_OKAY );
    }

    // Map the plugin result to an action result with the plugin command
    // set to "default".
    *action_result = sm_service_action_result_table_read( plugin_type,
                        plugin_name, plugin_command_default, plugin_exit_code );
    if( NULL != *action_result )
    {
        DPRINTFD( "Read service (%s) action (%s) plugin (%s, %s, default, %s) "
                  "data.", service_name, sm_service_action_str( action ),
                  plugin_type, plugin_name, plugin_exit_code );
        return( SM_OKAY );
    }

    // Map the plugin result to an action result with the plugin
    // command set to "default" and the plugin exit code set to "other".
    *action_result = sm_service_action_result_table_read( plugin_type,
                        plugin_name, plugin_command_default, 
                        plugin_exit_code_other );
    if( NULL != *action_result )
    {
        DPRINTFD( "Read service (%s) action (%s) plugin "
                  "(%s, %s, default, other) data.", service_name,
                  sm_service_action_str( action ), plugin_type, plugin_name );
        return( SM_OKAY );
    }

    return( SM_NOT_FOUND );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Result
// =======================
SmErrorT sm_service_action_result( int exit_code, char service_name[],
    SmServiceActionT action, SmServiceActionResultT* action_result,
    SmServiceStateT* service_state, SmServiceStatusT* service_status,
    SmServiceConditionT* service_condition, char reason_text[] )
{
    char plugin_name_default[] = "default";
    char plugin_exit_code[SM_SERVICE_ACTION_PLUGIN_EXIT_CODE_MAX_CHAR];
    SmServiceActionDataT* action_data;
    SmServiceActionResultDataT* result;
    SmErrorT error;

    reason_text[0] = '\0';

    if( SM_SERVICE_ACTION_PLUGIN_FAILURE == exit_code )
    {
        snprintf( plugin_exit_code, sizeof(plugin_exit_code), 
                  "plugin-failure" );

    } else if( SM_SERVICE_ACTION_PLUGIN_TIMEOUT == exit_code ) {
        snprintf( plugin_exit_code, sizeof(plugin_exit_code), 
                  "plugin-timeout" );
    } else {
        snprintf( plugin_exit_code, sizeof(plugin_exit_code),
                  "%i", exit_code );
    }

    *action_result = SM_SERVICE_ACTION_RESULT_UNKNOWN;
    *service_state = SM_SERVICE_STATE_UNKNOWN;
    *service_status = SM_SERVICE_STATUS_UNKNOWN;
    *service_condition = SM_SERVICE_CONDITION_UNKNOWN;

    action_data = sm_service_action_table_read( service_name, action );
    if( NULL == action_data )
    {
        DPRINTFD( "Service (%s) action (%s) does not exist.", service_name,
                  sm_service_action_str( action ) );
        return( SM_NOT_FOUND );
    }

    error = sm_service_action_plugin_result( service_name, action,
                action_data->plugin_type, action_data->plugin_name,
                action_data->plugin_command, plugin_exit_code, &result );
    if( SM_OKAY == error )
    {
        *action_result = result->action_result;
        *service_state = result->service_state;
        *service_status = result->service_status;
        *service_condition = result->service_condition;
        return( SM_OKAY );

    } else if(( SM_OKAY != error )&&( SM_NOT_FOUND != error )) {
        DPRINTFE( "Failed to read service (%s) action (%s) plugin result "
                  "data, error=%s.", service_name, 
                  sm_service_action_str( action ), sm_error_str( error ) );
        return( error );
    }

    error = sm_service_action_plugin_result( service_name, action,
                action_data->plugin_type, plugin_name_default,
                action_data->plugin_command, plugin_exit_code, &result );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to read service (%s) action (%s) plugin result "
                  "data, error=%s.", service_name, 
                  sm_service_action_str( action ), sm_error_str( error ) );
        return( error );
    }

    *action_result = result->action_result;
    *service_state = result->service_state;
    *service_status = result->service_status;
    *service_condition = result->service_condition;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Abort
// ======================
SmErrorT sm_service_action_abort( char service_name[], int process_id )
{
//
//TODO: Verify the process we are about to abort is the process we expect.
//
    if( -1 == process_id )
    {
        DPRINTFE( "Trying to abort a process (%i) for service (%s), "
                  "but pid is invalid.", process_id, service_name );
        return( SM_FAILED );
    }

    if( process_id == (int) getpid() )
    {
        DPRINTFE( "Trying to abort a process (%i) for service (%s), "
                  "but pid is self.", process_id, service_name );
        return( SM_FAILED );
    }

    DPRINTFI( "Aborting service (%s) with kill signal, pid=%i.",
              service_name, process_id );

    if( 0 > kill( process_id, SIGKILL ) )
    {
        if( ESRCH == errno )
        {
            DPRINTFD( "Service (%s) action not running.", service_name );
            return( SM_OKAY );

        } else {
            DPRINTFE( "Failed to send kill signal to service (%s), "
                      "error=%s.", service_name, strerror( errno ) );
            return( SM_FAILED );
        }
    }

    DPRINTFD( "Kill signal sent to service (%s).", service_name );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Setup Instance Environment Variables
// =====================================================
static SmErrorT sm_service_action_setup_instance_env_vars( char instance_name[],
    char instance_params[], SmServiceActionDataT* action_data )
{
    char looking_for = '=';
    char key_storage[SM_SERVICE_INSTANCE_PARAMS_MAX_CHAR+32];
    char value_storage[SM_SERVICE_INSTANCE_PARAMS_MAX_CHAR+32];
    char* key = NULL;
    char* value = NULL;
    char* param = NULL;

    if( 0 == strcmp( SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_SCRIPT,
                     action_data->plugin_type ) )
    {
        if( 0 > setenv( "OCF_RA_VERSION_MAJOR", 
                        SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_VERSION, 1 ) )
        {
            DPRINTFE( "Failed to set environment variable "
                      "(OCF_RA_VERSION_MAJOR), error=%s.", strerror( errno ) );
            goto ERROR;
        }

        if( 0 > setenv( "OCF_RA_VERSION_MINOR", 
                        SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_REVISION, 1 ) )
        {
            DPRINTFE( "Failed to set environment variable "
                      "(OCF_RA_VERSION_MINOR), error=%s.", strerror( errno ) );
            goto ERROR;
        }

        if( 0 > setenv( "OCF_ROOT",
                        SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_DIR, 1 ) )
        {
            DPRINTFE( "Failed to set environment variable "
                      "(OCF_ROOT), error=%s.", strerror( errno ) );
            goto ERROR;
        }

        if( 0 > setenv( "OCF_RESOURCE_INSTANCE", instance_name, 1 ) )
        {
            DPRINTFE( "Failed to set environment variable "
                      "(OCF_RESOURCE_INSTANCE), error=%s.", strerror( errno ) );
            goto ERROR;
        }

        if( 0 > setenv( "OCF_RESOURCE_TYPE",
                        action_data->plugin_name, 1 ) )
        {
            DPRINTFE( "Failed to set environment variable "
                      "(OCF_RESOURCE_TYPE), error=%s.", strerror( errno ) );
            goto ERROR;
        }
    }

    param = &(value_storage[sizeof(value_storage)-1]);
    *param = '\0'; --param;

    int char_i;
    for( char_i=strlen(instance_params)-1; 0 <= char_i; --char_i )
    {
        if( looking_for == instance_params[char_i] )
        {
            if( ',' == looking_for )
            {
                key = param+1;

                if( 0 == strcmp( SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_SCRIPT,
                                 action_data->plugin_type ) )
                {
                    key -= strlen("OCF_RESKEY_");
                    memcpy( key, "OCF_RESKEY_", strlen("OCF_RESKEY_") );
                }

                DPRINTFD( "%s set environment: %s=%s", instance_name, key,
                          value );

                if( 0 > setenv( key, value, 1 ) )
                {
                    DPRINTFE( "Failed to set environment variable (%s=%s), "
                              "error=%s.", key, value, strerror( errno ) );
                    goto ERROR;
                }
           
                key = NULL;
                value = NULL;
                param = &(value_storage[sizeof(value_storage)-1]);
                *param = '\0'; --param;
                looking_for = '=';

            } else {
                key = NULL;
                value = param+1;
                param = &(key_storage[sizeof(key_storage)-1]);
                *param = '\0'; --param;
                looking_for = ',';
            }
        } else {
            *param = instance_params[char_i];
            --param;
        }
    }

    if( NULL != value )
    {
        key = param+1;

        if( 0 == strcmp( SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_SCRIPT,
                         action_data->plugin_type ) )
        {
            key -= strlen("OCF_RESKEY_");
            memcpy( key, "OCF_RESKEY_", strlen("OCF_RESKEY_") );
        }

        DPRINTFD( "%s set environment: %s=%s", instance_name, key,
                  value );

        if( 0 > setenv( key, value, 1 ) )
        {
            DPRINTFE( "Failed to set environment variable (%s=%s), "
                      "error=%s.", key, value, strerror( errno ) );
            goto ERROR;
        }
    }

    return( SM_OKAY );

ERROR:
    return( SM_FAILED );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Setup Plugin Environment Variables
// ===================================================
static SmErrorT sm_service_action_setup_plugin_env_vars(
    SmServiceActionDataT* action_data )
{
    char looking_for = '=';
    char key_storage[SM_SERVICE_ACTION_PLUGIN_PARAMS_MAX_CHAR+32];
    char value_storage[SM_SERVICE_ACTION_PLUGIN_PARAMS_MAX_CHAR+32];
    char* key = NULL;
    char* value = NULL;
    char* param = NULL;

    if( 0 == strcmp( SM_SERVICE_ACTION_PLUGIN_TYPE_LSB_SCRIPT,
                     action_data->plugin_type ) )
    {
        if( 0 > setenv( "SYSTEMCTL_SKIP_REDIRECT", "1", 1 ) )
        {
            DPRINTFE( "Failed to set environment variable "
                      "(SYSTEMCTL_SKIP_REDIRECT), error=%s.",
                      strerror( errno ) );
            goto ERROR;
        }

    } else if( 0 == strcmp( SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_SCRIPT,
                            action_data->plugin_type ) )
    {
        if( 0 > setenv( "HA_LOGFACILITY", "daemon", 1 ) )
        {
            DPRINTFE( "Failed to set environment variable (HA_LOGFACILITY), "
                      "error=%s.", strerror( errno ) );
            goto ERROR;
        }
    }

    param = &(value_storage[sizeof(value_storage)-1]);
    *param = '\0'; --param;

    int char_i;
    for( char_i=strlen(action_data->plugin_params)-1; 0 <= char_i; --char_i )
    {
        if( looking_for == action_data->plugin_params[char_i] )
        {
            if( ',' == looking_for )
            {
                key = param+1;

                if( 0 == strcmp( SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_SCRIPT,
                                 action_data->plugin_type ) )
                {
                    key -= strlen("OCF_RESKEY_CRM_meta_");
                    memcpy( key, "OCF_RESKEY_CRM_meta_",
                            strlen("OCF_RESKEY_CRM_meta_") );
                }

                DPRINTFD( "%s set environment: %s=%s", 
                          action_data->service_name, key, value );

                if( 0 > setenv( key, value, 1 ) )
                {
                    DPRINTFE( "Failed to set environment variable (%s=%s), "
                              "error=%s.", key, value, strerror( errno ) );
                    goto ERROR;
                }

                param = &(value_storage[sizeof(value_storage)-1]);
                *param = '\0'; --param;
                looking_for = '=';

            } else {
                value = param+1;
                param = &(key_storage[sizeof(key_storage)-1]);
                *param = '\0'; --param;
                looking_for = ',';
            }
        } else {
            *param = action_data->plugin_params[char_i];
            --param;
        }
    }

    if( NULL != value )
    {
        key = param+1;

        if( 0 == strcmp( SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_SCRIPT,
                         action_data->plugin_type ) )
        {
            key -= strlen("OCF_RESKEY_CRM_meta_");
            memcpy( key, "OCF_RESKEY_CRM_meta_", strlen("OCF_RESKEY_CRM_meta_") );
        }

        DPRINTFD( "%s set environment: %s=%s", action_data->service_name,
                  key, value );

        if( 0 > setenv( key, value, 1 ) )
        {
            DPRINTFE( "Failed to set environment variable (%s=%s), "
                      "error=%s.", key, value, strerror( errno ) );
            goto ERROR;
        }
    }

    return( SM_OKAY );

ERROR:
    return( SM_FAILED );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Setup Environment
// ==================================
static SmErrorT sm_service_action_setup_env( char instance_name[],
    char instance_params[], SmServiceActionDataT* action_data )
{
    SmErrorT error;

    error = sm_service_action_setup_instance_env_vars( instance_name,
                                            instance_params, action_data );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to set instance environment variables for service "
                  "(%s) action (%s), error=%s.", action_data->service_name,
                  sm_service_action_str( action_data->action ),
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_action_setup_plugin_env_vars( action_data );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to set plugin environment variables for service "
                  "(%s) action (%s), error=%s.", action_data->service_name,
                  sm_service_action_str( action_data->action ),
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Run
// ====================
SmErrorT sm_service_action_run( char service_name[], char instance_name[],
    char instance_params[], SmServiceActionT action, int* process_id,
    int* timeout_in_ms )
{
    pid_t pid;
    bool service_managed = true;
    struct stat stat_data;
    char plugin_exec[SM_SERVICE_ACTION_PLUGIN_EXEC_MAX_CHAR];
    int result;
    SmServiceActionDataT* action_data;
    SmErrorT error;

    *process_id = -1;
    *timeout_in_ms = 0;

    action_data = sm_service_action_table_read( service_name, action );
    if( NULL == action_data )
    {
        DPRINTFD( "Service (%s) action (%s) does not exist.", service_name,
                  sm_service_action_str( action ) );
        return( SM_NOT_FOUND );
    }

    snprintf( plugin_exec, sizeof(plugin_exec), "%s/%s.unmanaged",
              SM_RUN_SERVICES_DIRECTORY, service_name );
    if( 0 == access( plugin_exec,  F_OK ) )
    {
        DPRINTFI( "Service (%s) is unmanaged, actions are force-passed.",
                  service_name );
        service_managed = false;
    }

    if( service_managed )
    {
        sm_service_action_validate( action_data );

        if( 0 == strcmp( SM_SERVICE_ACTION_PLUGIN_TYPE_LSB_SCRIPT,
                         action_data->plugin_type ) )
        {
            snprintf( plugin_exec, sizeof(plugin_exec), "%s/%s",
                      SM_SERVICE_ACTION_PLUGIN_TYPE_LSB_DIR,
                      action_data->plugin_name );

        } else if( 0 == strcmp( SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_SCRIPT,
                                action_data->plugin_type ) )
        {
            snprintf( plugin_exec, sizeof(plugin_exec), "%s/%s/%s",
                      SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_PLUGIN_DIR,
                      action_data->plugin_class, action_data->plugin_name );
        } else {
            snprintf( plugin_exec, sizeof(plugin_exec), "%s",
                      action_data->plugin_name );
        }

        DPRINTFD( "Plugin executable (%s) for service (%s).", plugin_exec,
                  action_data->service_name );

        if( 0 > access( plugin_exec,  F_OK | X_OK ) )
        {
            DPRINTFE( "Service (%s) plugin (%s) access failed, error=%s.",
                      action_data->service_name, plugin_exec, 
                      strerror( errno ) );
            return( SM_FAILED );
        }

        if( 0 > stat( plugin_exec, &stat_data ) )
        {
            DPRINTFE( "Service (%s) plugin (%s) stat failed, error=%s.",
                      action_data->service_name, plugin_exec, 
                      strerror( errno ) );
            return( SM_FAILED );
        }

        if( 0 >= stat_data.st_size )
        {
            DPRINTFE( "Service (%s) plugin (%s) has zero size.",
                      action_data->service_name, action_data->plugin_name );
            return( SM_FAILED );
        }
    }

    pid = fork();
    if( 0 > pid )
    {
        DPRINTFE( "Failed to fork process for service (%s), error=%s.",
                  action_data->service_name, strerror( errno ) );
        return( SM_FAILED );

    } else if( 0 == pid ) {
        // Child process.
        struct rlimit file_limits;

        DPRINTFD( "Child process created for service (%s).",
                  action_data->service_name );

        if( 0 > setpgid( 0, 0 ) )
        {
            DPRINTFE( "Failed to set process group id for service (%s), "
                      "error=%s.", action_data->service_name,
                      strerror( errno ) );
            exit( SM_SERVICE_ACTION_PLUGIN_FAILURE );
        }

        if( 0 > getrlimit( RLIMIT_NOFILE, &file_limits ) )
        {
            DPRINTFE( "Failed to get file limits for service (%s), "
                      "error=%s.", action_data->service_name,
                      strerror( errno ) );
            exit( SM_SERVICE_ACTION_PLUGIN_FAILURE );
        }

        unsigned int fd_i;
        for( fd_i=0; fd_i < file_limits.rlim_cur; ++fd_i )
        {
            close( fd_i );
        }

        if( 0 > open( "/dev/null", O_RDONLY ) )
        {
            DPRINTFE( "Failed to open stdin to /dev/null for service (%s), "
                      "error=%s.", action_data->service_name,
                      strerror( errno ) );
        }

        if( 0 > open( "/dev/null", O_WRONLY ) )
        {
            DPRINTFE( "Failed to open stdout to /dev/null for service (%s), "
                      "error=%s.", action_data->service_name, 
                      strerror( errno ) );
        }

        if( 0 > open( "/dev/null", O_WRONLY ) )
        {
            DPRINTFE( "Failed to open stderr to /dev/null for service (%s), "
                      "error=%s.", action_data->service_name,
                      strerror( errno ) );
        }

        result = setpriority( PRIO_PROCESS, getpid(), -1 );
        if( 0 > result )
        {
            DPRINTFE( "Failed to set priority of process, error=%s.",
                      strerror( errno ) );
            exit( SM_SERVICE_ACTION_PLUGIN_FAILURE );
        }

        if( !service_managed )
        {
            exit( SM_SERVICE_ACTION_PLUGIN_FORCE_SUCCESS );
        }

        error = sm_service_action_setup_env( instance_name, instance_params,
                                             action_data );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to setup environment for service (%s), "
                      "error=%s.", action_data->service_name,
                      sm_error_str( error ) );
            exit( SM_SERVICE_ACTION_PLUGIN_FAILURE );
        }

#ifdef SM_SERVICE_ACTION_TIME_PLUGIN
        char time_exec[] = "/usr/bin/time";
        char time_append[] = "-a";
        char time_output[] = "-o";
        char time_file[80];
        char* argv[] = {time_exec, time_append, time_output, time_file,
                        plugin_exec, action_data->plugin_command, NULL};

        snprintf( time_file, sizeof(time_file), "/tmp/%s_%s-timing.txt",
                  action_data->service_name, action_data->plugin_command );

        if( 0 > execv( time_exec, argv ) )
        {
            DPRINTFE( "Failed to exec command for service (%s), "
                      "error=%s.", action_data->service_name,
                      strerror( errno ) );
        }
#else
        char* argv[] = {plugin_exec, action_data->plugin_command, NULL};

        if( 0 > execv( plugin_exec, argv ) )
        {
            DPRINTFE( "Failed to exec command for service (%s), "
                      "error=%s.", action_data->service_name,
                      strerror( errno ) );
        }
#endif // SM_SERVICE_ACTION_TIME_PLUGIN

        exit( SM_SERVICE_ACTION_PLUGIN_FAILURE );

    } else {
        // Parent process.
        *process_id = (int) pid;
        *timeout_in_ms = action_data->timeout_in_secs * 1000;

        if( sm_utils_watchdog_delayed( SM_SERVICE_ACTION_MAX_DELAY_IN_SECS ) )
        {
            DPRINTFI( "Service (%s) timeout %d secs increased by %d ms, "
                      "sm-watchdog delayed.", action_data->service_name,
                      action_data->timeout_in_secs,
                      SM_SERVICE_ACTION_TIMER_SKEW_IN_MS );
            *timeout_in_ms += SM_SERVICE_ACTION_TIMER_SKEW_IN_MS;
        }

        DPRINTFD( "Child process (%i) created for service (%s).", *process_id,
                  action_data->service_name );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Initialize
// ===========================
SmErrorT sm_service_action_initialize( void )
{
    SmErrorT error;

    error = sm_service_action_table_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service action table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_service_action_result_table_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service action result table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Action - Finalize
// =========================
SmErrorT sm_service_action_finalize( void )
{
    SmErrorT error;

    error = sm_service_action_result_table_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service action result table, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_service_action_table_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service action table, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
