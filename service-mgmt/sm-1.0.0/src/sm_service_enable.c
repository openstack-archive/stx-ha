//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_enable.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_process_death.h"
#include "sm_timer.h"
#include "sm_service_dependency.h"
#include "sm_service_fsm.h"
#include "sm_service_action.h"
#include "sm_configuration_table.h"
#include "sm_node_utils.h"
#include "sm_util_types.h"

#define SERVICE_ENABLING_THROTTLE_DEFAULT_SIZE 2
#define EXTRA_CORES_STORAGE "/etc/platform/.task_affining_incomplete"
#define PLATFORM_CORES_FILE "/var/run/sm/.platform_cores"

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t sem;
static int num_initial_cores = 0;
static sem_t sem_more_cores;
static bool check_more_cores_available = false;
static bool more_cores_available = false;
static int  num_extra_cores = 0;


// ****************************************************************************
// add more cores for enabling service
// ==============
void sm_service_enable_throttle_add_cores( bool additional_cores )
{
    mutex_holder holder(&_mutex);

    check_more_cores_available = additional_cores;
    if (!additional_cores)
    {
        if( more_cores_available )
        {
            sem_destroy(&sem);
            more_cores_available = false;
        }
    }else
    {
        DPRINTFI("Additional cores check enabled");
    }
}
// ****************************************************************************

// ****************************************************************************
// handling special upgrade case.
// THIS IS A HACK TO TEMPORALLY OVERCOME ISCSI ENABLE TIMEOUT AFTER UPGRADE
// THIS CODE SHOULD BE REMOVED in Release 17.xx
// ===============================
static int handle_special_upgrade_case(SmServiceT* service, int* timeout_ms)
{
    FILE* fp;
    char flag_file[] = "/tmp/.extend_sm_iscsi_enable_timeout";
    int reading_sec = 0;
    if (0 == strcmp(service->name, "iscsi"))
    {
        fp = fopen( flag_file, "r" );
        if ( NULL == fp)
            return 0;

        fscanf( fp, "%d", &reading_sec );
        fclose(fp);

        if ( reading_sec <= 0 )
        {
            reading_sec = 960; // make it 16 minutes
        }

        DPRINTFI( "Extending enable timeout for iscsi from %d ms to %d ms",
            *timeout_ms, reading_sec * 1000);
        *timeout_ms = reading_sec * 1000;

        if( 0 != remove(flag_file) )
        {
            DPRINTFE( "Failed to remove flag file (%s).", flag_file );
        }
        return 1;
    }
    return 0;
}

// ****************************************************************************
// Service Enable - Result Handler
// ===============================
static SmErrorT service_enable_result_handler( SmServiceT* service,
    SmServiceActionT action_running, SmServiceActionResultT action_result,
    SmServiceStateT service_state, SmServiceStatusT service_status,
    SmServiceConditionT service_condition, const char reason_text[] )
{
    bool retries_exhausted = true;
    int max_failure_retries;
    int max_timeout_retries;
    int max_total_retries;
    SmErrorT error;

    if( SM_SERVICE_ACTION_RESULT_SUCCESS == action_result )
    {
        DPRINTFD( "Action (%s) of service (%s) completed with success.",
                  sm_service_action_str( action_running ), service->name );
        goto REPORT;
    }

    error = sm_service_action_max_retries( service->name, action_running,
                                           &max_failure_retries,
                                           &max_timeout_retries,
                                           &max_total_retries );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get max retries for action (%s) of service (%s), "
                  "error=%s.", sm_service_action_str( action_running ),
                  service->name, sm_error_str( error ) );
        goto REPORT;
    }

    if( SM_SERVICE_ACTION_RESULT_FAILED == action_result )
    {
        retries_exhausted =
            (( max_failure_retries <= service->action_attempts )||
             ( max_total_retries   <= service->action_attempts ));

    } else if( SM_SERVICE_ACTION_RESULT_TIMEOUT == action_result ) {
        retries_exhausted =
            (( max_timeout_retries <= service->action_attempts )||
             ( max_total_retries   <= service->action_attempts ));
    }

    if( retries_exhausted )
    {
        DPRINTFI( "Max retires met for action (%s) of service (%s), "
                  "attempts=%i.", sm_service_action_str( action_running ),
                  service->name, service->action_attempts );
        goto REPORT;
    } else {
        DPRINTFI( "Max retires not met for action (%s) of service (%s), "
                  "attempts=%i.", sm_service_action_str( action_running ),
                  service->name, service->action_attempts );

        error = sm_service_enable( service );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to enable service (%s), error=%s.",
                      service->name, sm_error_str( error ) );
            goto REPORT;
        }
    }

    return( SM_OKAY );

REPORT:
    service->action_attempts = 0;

    error = sm_service_fsm_action_complete_handler( service->name,
                        action_running, action_result, service_state,
                        service_status, service_condition, reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to notify service (%s) fsm action complete, "
                  "error=%s.", service->name, sm_error_str( error ) );
        abort();
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enable - Timeout
// ========================
static bool sm_service_enable_timeout( SmTimerIdT timer_id, int64_t user_data )
{
    int64_t id = user_data;
    SmServiceT* service;    
    SmServiceActionT action_running;
    SmServiceActionResultT action_result;
    SmServiceStateT service_state;
    SmServiceStatusT service_status;
    SmServiceConditionT service_condition;
    char reason_text[SM_SERVICE_ACTION_RESULT_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    service = sm_service_table_read_by_id( id );
    if( NULL == service )
    {
        DPRINTFE( "Failed to read service, error=%s.",
                  sm_error_str(SM_NOT_FOUND) );
        return( false );
    }

    sm_service_enable_throttle_uncheck();

    if( SM_SERVICE_ACTION_ENABLE != service->action_running )
    {
        DPRINTFE( "Service (%s) not running action (%s).", service->name,
                  sm_service_action_str( service->action_running ) );
        return( false );
    }

    action_running = service->action_running;

    error = sm_service_action_result( SM_SERVICE_ACTION_PLUGIN_TIMEOUT,
                                      service->name, service->action_running,
                                      &action_result, &service_state,
                                      &service_status, &service_condition,
                                      reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get action result for service (%s) defaulting "
                  "to failed, exit_code=%i, error=%s.", service->name,
                  SM_SERVICE_ACTION_PLUGIN_TIMEOUT, sm_error_str( error ) );
        action_result = SM_SERVICE_ACTION_RESULT_FAILED;
        service_state = SM_SERVICE_STATE_UNKNOWN;
        service_status = SM_SERVICE_STATUS_UNKNOWN;
        service_condition = SM_SERVICE_CONDITION_UNKNOWN;
    }

    DPRINTFI( "Action (%s) timeout with result (%s), state (%s), "
              "status (%s), and condition (%s) for service (%s), "
              "reason_text=%s, exit_code=%i.", 
              sm_service_action_str( action_running ),
              sm_service_action_result_str( action_result ),
              sm_service_state_str( service_state ),
              sm_service_status_str( service_status ),
              sm_service_condition_str( service_condition ),
              service->name, reason_text, SM_SERVICE_ACTION_PLUGIN_TIMEOUT );

    error = sm_service_action_abort( service->name, service->action_pid );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to abort action (%s) of service (%s), error=%s.",
                  sm_service_action_str( service->action_running ),
                  service->name, sm_error_str( error ) );
    }

    service->action_running = SM_SERVICE_ACTION_NONE;
    service->action_pid = -1;
    service->action_timer_id = SM_TIMER_ID_INVALID;

    error = service_enable_result_handler( service, action_running,
                                           action_result, service_state,
                                           service_status, service_condition,
                                           reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to handle enable result for service (%s), "
                  "error=%s.", service->name, sm_error_str( error ) );
        abort();
    }

    return( false );
}
// ****************************************************************************

// ****************************************************************************
// Service Enable - Complete
// =========================
static void sm_service_enable_complete( pid_t pid, int exit_code,
    int64_t user_data )
{
    SmServiceT* service;    
    SmServiceActionT action_running;
    SmServiceActionResultT action_result;
    SmServiceStateT service_state;
    SmServiceStatusT service_status;
    SmServiceConditionT service_condition;
    char reason_text[SM_SERVICE_ACTION_RESULT_REASON_TEXT_MAX_CHAR] = "";
    SmErrorT error;

    if( SM_PROCESS_FAILED == exit_code )
    {
        exit_code = SM_SERVICE_ACTION_PLUGIN_FAILURE;
    }

    service = sm_service_table_read_by_action_pid( (int) pid );
    if( NULL == service )
    {
        DPRINTFE( "Failed to query service based on pid (%i), error=%s.",
                  (int) pid, sm_error_str(SM_NOT_FOUND) );
        return;
    }
    
    sm_service_enable_throttle_uncheck();

    if( SM_SERVICE_ACTION_ENABLE != service->action_running )
    {
        DPRINTFE( "Service (%s) not running enable action.", service->name );
        return;
    }

    if( SM_TIMER_ID_INVALID != service->action_timer_id )
    {
        error = sm_timer_deregister( service->action_timer_id );
        if( SM_OKAY == error )
        {
            DPRINTFD( "Timer stopped for action (%s) of service (%s).",
                      sm_service_action_str( service->action_running ),
                      service->name );
        } else {
            DPRINTFE( "Failed to stop timer for action (%s) of "
                      "service (%s), error=%s.",
                      sm_service_action_str( service->action_running ),
                      service->name, sm_error_str( error ) );
        } 
    }

    action_running = service->action_running;

    if( SM_SERVICE_ACTION_PLUGIN_FORCE_SUCCESS == exit_code )
    {
        DPRINTFI( "Action (%s) for service (%s) force-passed.",
                  sm_service_action_str( action_running ), service->name );

        action_result = SM_SERVICE_ACTION_RESULT_SUCCESS;
        service_state = SM_SERVICE_STATE_UNKNOWN;
        service_status = SM_SERVICE_STATUS_UNKNOWN;
        service_condition = SM_SERVICE_CONDITION_UNKNOWN;

    } else {
        error = sm_service_action_result( exit_code, service->name, 
                                          service->action_running,
                                          &action_result, &service_state,
                                          &service_status, &service_condition,
                                          reason_text );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to get action result for service (%s) "
                      "defaulting to failed, exit_code=%i, error=%s.",
                      service->name, exit_code, sm_error_str( error ) );
            action_result = SM_SERVICE_ACTION_RESULT_FAILED;
            service_state = SM_SERVICE_STATE_UNKNOWN;
            service_status = SM_SERVICE_STATUS_UNKNOWN;
            service_condition = SM_SERVICE_CONDITION_UNKNOWN;
        }
    }

    DPRINTFI( "Action (%s) completed with result (%s), state (%s), "
              "status (%s), and condition (%s) for service (%s), "
              "reason_text=%s, exit_code=%i.", 
              sm_service_action_str( action_running ),
              sm_service_action_result_str( action_result ),
              sm_service_state_str( service_state ),
              sm_service_status_str( service_status ),
              sm_service_condition_str( service_condition ),
              service->name, reason_text, exit_code );

    service->action_running = SM_SERVICE_ACTION_NONE;
    service->action_pid = -1;
    service->action_timer_id = SM_TIMER_ID_INVALID;

    error = service_enable_result_handler( service, action_running,
                                           action_result, service_state,
                                           service_status, service_condition,
                                           reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to handle enable result for service (%s), "
                  "error=%s.", service->name, sm_error_str( error ) );
        abort();
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Enable
// ==============
SmErrorT sm_service_enable( SmServiceT* service )
{
    char timer_name[80] = "";
    int process_id = -1;
    int timeout = 0;
    bool dependency_met;
    SmTimerIdT timer_id = SM_TIMER_ID_INVALID;
    SmErrorT error;

    // Are enable dependencies met?
    error =  sm_service_dependency_enable_met( service, &dependency_met );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to check service (%s) dependencies, error=%s.",
                  service->name, sm_error_str( error ) );
        return( error );
    }

    if( !dependency_met )
    {
        DPRINTFD( "Some dependencies for service (%s) were not met.",
                  service->name );
        return( SM_OKAY );
    }

    DPRINTFD( "All dependencies for service (%s) were met.", service->name );

    // Run action.
    error = sm_service_action_run( service->name,
                                   service->instance_name,
                                   service->instance_params,            
                                   SM_SERVICE_ACTION_ENABLE,
                                   &process_id, &timeout );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to run enable action for service (%s), "
                  "error=%s.", service->name, sm_error_str( error ) );
        return( error );
    }
   
    // Register for action script exit notification.
    error = sm_process_death_register( process_id, true,
                                       sm_service_enable_complete, 0 );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to register for enable action completion for "
                  "service (%s), error=%s.", service->name,
                  sm_error_str( error ) );
        abort();
    }

    // Create timer for action completion.
    snprintf( timer_name, sizeof(timer_name), "%s %s action",
              service->name, sm_service_action_str( SM_SERVICE_ACTION_ENABLE ) );

    if ( 0 != handle_special_upgrade_case(service, &timeout) )
    {
        DPRINTFI( "Extend service (%s) enable timeout to (%d) ms", service->name, timeout);
    }

    error = sm_timer_register( timer_name, timeout, sm_service_enable_timeout,
                               service->id, &timer_id );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create a timer for action (%s) for service (%s), "
                  "error=%s.", sm_service_action_str( SM_SERVICE_ACTION_ENABLE ),
                  service->name, sm_error_str( error ) );
        abort();
    }

    service->action_running = SM_SERVICE_ACTION_ENABLE;
    service->action_pid = process_id;
    service->action_timer_id = timer_id;
    service->action_attempts += 1;

    DPRINTFI( "Started enable action (%i) for service (%s).", process_id,
              service->name );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enable - Abort
// ======================
SmErrorT sm_service_enable_abort( SmServiceT* service )
{
    SmErrorT error;
    sm_service_enable_throttle_uncheck();

    DPRINTFI( "Aborting action (%s) of service (%s).",
              sm_service_action_str( service->action_running ),
              service->name );

    error = sm_service_action_abort( service->name, service->action_pid );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to abort action (%s) of service (%s), error=%s.",
                  sm_service_action_str( service->action_running ),
                  service->name, sm_error_str( error ) );
        return( error );
    }

    if( SM_TIMER_ID_INVALID != service->action_timer_id )
    {
        error = sm_timer_deregister( service->action_timer_id );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Cancel timer for action (%s) of service (%s) failed, "
                      "error=%s.", service->name,
                      sm_service_action_str( service->action_running ),
                      sm_error_str( error ) );
            return( error );
        }
    }

    service->action_running = SM_SERVICE_ACTION_NONE;
    service->action_pid = -1;
    service->action_timer_id = SM_TIMER_ID_INVALID;
    service->action_attempts = 0;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enable - Exists
// =======================
SmErrorT sm_service_enable_exists( SmServiceT* service, bool* exists )
{
    SmErrorT error;

    error = sm_service_action_exist( service->name, SM_SERVICE_ACTION_ENABLE );
    if(( SM_OKAY != error )&&( SM_NOT_FOUND != error ))
    {
        DPRINTFE( "Failed to determine if action (%s) exists for service (%s), "
                  "error=%s.", sm_service_action_str( SM_SERVICE_ACTION_ENABLE ),
                  service->name, sm_error_str( error ) );
        return( error );
    }

    if( SM_OKAY == error )
    {
        *exists = true;
    } else {
        *exists = false;
    }

    return( SM_OKAY );
}
// ****************************************************************************


// ****************************************************************************
// Service Enable - get number int items in a comma separated file
// =========================
static int _get_num_csv_file(const char* filename)
{
    if( 0 != access( filename, F_OK ) )
    {
        return -1;
    }

    FILE* f = fopen(filename, "r");
    if( NULL == f )
    {
        DPRINTFE("Failed open file %s, error %d", filename, errno);
        return 0;
    }

    int count = 0;
    int n, res;
    do
    {
        res = fscanf(f, "%d,", &n);
        if( 1 == res )
        {
            count ++;
        }
    }while(EOF != res);

    fclose(f);
    return count;
}
// ****************************************************************************

// ****************************************************************************
// Service Enable - get initial throttle size
// ===========================
static int get_initial_throttle()
{
    bool is_aio;
    SmErrorT error;
    error = sm_node_utils_is_aio(&is_aio);
    if(SM_OKAY != error)
    {
        DPRINTFE( "Failed to determine if it is AIO, "
                  "error=%s.", sm_error_str(error) );
        // prepare for the worst
        is_aio = true;
    }

    #define MAX_SERVICE_EXPECTED 1000
    if( !is_aio )
    {
        return MAX_SERVICE_EXPECTED;
    }

    char buf[SM_CONFIGURATION_KEY_MAX_CHAR + 1];
    int size;
    if( SM_OKAY == sm_configuration_table_get("ENABLING_THROTTLE", buf, sizeof(buf) - 1) )
    {
        size = atoi(buf);
    }else
    {
        size = 0;
    }

    int num_platform_cores = _get_num_csv_file(PLATFORM_CORES_FILE);
    if( num_platform_cores > size )
    {
        size = num_platform_cores;
    }

    if( SERVICE_ENABLING_THROTTLE_DEFAULT_SIZE > size )
        size = SERVICE_ENABLING_THROTTLE_DEFAULT_SIZE;

    return size;
}
// ****************************************************************************

// ****************************************************************************
// Service Enable - Initialize
// ===========================
SmErrorT sm_service_enable_initialize( void )
{
    num_initial_cores = get_initial_throttle();
    DPRINTFI("Enable services initial throttle size %d ", num_initial_cores);

    if( 0 != sem_init(&sem, 0, num_initial_cores) )
    {
        DPRINTFE( "Could not initialize sempaphore. Error (%d)", errno );
        return SM_FAILED;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enable - Finalize
// =========================
SmErrorT sm_service_enable_finalize( void )
{
    sem_destroy(&sem);
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Enable - get number of extra cores
// =========================
static int get_extra_cores()
{
    return _get_num_csv_file(EXTRA_CORES_STORAGE);
}
// ****************************************************************************

// ****************************************************************************
// get a ticket to enable service
// ==============
bool sm_service_enable_throttle_check( void )
{
    int wait_res = sem_trywait(&sem);
    if ( 0 == wait_res )
    {
        return true;
    } else if( EAGAIN == errno )
    {
        mutex_holder holder(&_mutex);
        if(check_more_cores_available)
        {
            int new_num_platform_cores = get_extra_cores();
            if( -1 == new_num_platform_cores )
            {
                //not ready, will try again next attempt
            }
            else
            {
                if (num_initial_cores < new_num_platform_cores)
                {
                    num_extra_cores = new_num_platform_cores - num_initial_cores;
                    DPRINTFI("Enable services additional throttle size %d ", num_extra_cores);

                    if( 0 != sem_init(&sem_more_cores, 0, num_extra_cores) )
                    {
                        DPRINTFE( "Could not initialize sempaphore for additional cores. Error (%d)", errno );
                        // cannot use additional cores
                        num_extra_cores = 0;
                    }else
                    {
                        more_cores_available = true;
                    }
                }else
                {
                    DPRINTFI("Enable services no more additional core available");
                }
                check_more_cores_available = false;
            }

        }else if (more_cores_available)
        {
            wait_res = sem_trywait(&sem_more_cores);
            if ( 0 == wait_res )
            {
                return true;
            }else if( EAGAIN != errno )
            {
                DPRINTFE( "Wait for additional cores failed error (%d) ", errno );
            }
        }
    }else
    {
        DPRINTFE( "Wait failed error (%d) ", errno );
    }
    return false;
}
// ****************************************************************************

// ****************************************************************************
// return a ticket after enabling service completed
// ==============
void sm_service_enable_throttle_uncheck( void )
{
    int res;
    res = sem_post(&sem);
    if( -1 == res )
    {
        if(EOVERFLOW == errno)
        {
            res = sem_post(&sem_more_cores);
        }
    }

    if( 0 != res )
    {
        DPRINTFE( "Error uncheck enable throttle. Error (%d)", errno );
    }
}
