//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_watchdog_nfs.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "sm_types.h"
#include "sm_time.h"
#include "sm_debug.h"
#include "sm_node_utils.h"
#include "sm_node_stats.h"

#define SM_WATCHDOG_NFS_THREAD_NAME                                 "(nfsd)"
#define SM_WATCHDOG_NFS_REBOOT_INPROGRESS                         0xA5A5A5A5
#define SM_WATCHDOG_NFS_MAX_BLOCKED_THREADS                               32
#define SM_WATCHDOG_NFS_CHECK_IN_MS                                    10000
#define SM_WATCHDOG_NFS_MAX_UNINTERRUPTIBLE_SLEEP                      60000
#define SM_WATCHDOG_NFS_DELAY_REBOOT_IN_MS                             60000 
#define SM_WATCHDOG_NFS_DELAY_REBOOT_FORCE_IN_MS                      480000
#define SM_WATCHDOG_NFS_DEBUG_FILE                      "/var/log/nfs.debug"

typedef struct
{
    bool inuse;
    bool stale;
    int pid;
    SmTimeT timestamp;
    SmNodeProcessStatusT status;
} SmWatchDogNfsBlockedInfoT;

static uint32_t _nfs_reboot_inprogress;

static SmWatchDogNfsBlockedInfoT
    _nfs_blocked_threads[SM_WATCHDOG_NFS_MAX_BLOCKED_THREADS];

// ****************************************************************************
// Watchdog NFS - Find Blocked Thread
// ==================================
static SmWatchDogNfsBlockedInfoT* sm_watchdog_nfs_find_blocked_thread( int pid )
{
    SmWatchDogNfsBlockedInfoT* entry;

    int thread_i;
    for( thread_i=0; SM_WATCHDOG_NFS_MAX_BLOCKED_THREADS > thread_i;
         ++thread_i )
    {
        entry = &(_nfs_blocked_threads[thread_i]);

        if( entry->inuse )
        {
            if( pid == entry->pid )
            {
                return( entry );
            }
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Watchdog NFS - Add Blocked Thread
// =================================
static void sm_watchdog_nfs_add_blocked_thread( int pid, 
    SmNodeProcessStatusT* status )
{
    SmWatchDogNfsBlockedInfoT* entry;

    int thread_i;
    for( thread_i=0; SM_WATCHDOG_NFS_MAX_BLOCKED_THREADS > thread_i;
         ++thread_i )
    {
        entry = &(_nfs_blocked_threads[thread_i]);

        if( !(entry->inuse) )
        {
            entry->inuse = true;
            entry->stale = false;
            entry->pid = pid;
            sm_time_get( &(entry->timestamp) );
            memcpy( &(entry->status), status, sizeof(SmNodeProcessStatusT) );
            return;
        }
    }

    DPRINTFE( "Not enough room for all the NFS blocked threads." );
}
// ****************************************************************************

// ****************************************************************************
// Watchdog NFS - Delete Blocked Thread
// ====================================
static void sm_watchdog_nfs_delete_blocked_thread( int pid )
{
    SmWatchDogNfsBlockedInfoT* entry;
    
    entry = sm_watchdog_nfs_find_blocked_thread( pid );
    if( NULL != entry )
    {
        memset( entry, 0, sizeof(SmWatchDogNfsBlockedInfoT) );
        entry->inuse = false;
    }
}
// ****************************************************************************

// ****************************************************************************
// Watchdog NFS - Do Reboot
// ========================
static void sm_watchdog_nfs_do_reboot( void )
{
    char cmd[2048];
    pid_t reboot_pid;
    pid_t reboot_force_pid;
    pid_t sm_troubleshoot_pid;
    pid_t collect_pid;
    SmWatchDogNfsBlockedInfoT* entry;
    SmErrorT error;

    if( SM_WATCHDOG_NFS_REBOOT_INPROGRESS == _nfs_reboot_inprogress )
    {
        DPRINTFD( "Reboot already inprogress." );
        return;
    }

    // Fork child to do the reboot.
    reboot_pid = fork();
    if( 0 > reboot_pid )
    {
        DPRINTFE( "Failed to fork process for reboot, error=%s.",
                  strerror( errno ) );
        return;

    } else if( 0 == reboot_pid ) {
        // Child process.
        long ms_expired;
        char reboot_cmd[] = "reboot";
        char* reboot_argv[] = {reboot_cmd, NULL};
        char* reboot_env[] = {NULL};
        struct rlimit file_limits;
        SmTimeT timestamp;

        setpgid( 0, 0 );

        if( 0 == getrlimit( RLIMIT_NOFILE, &file_limits ) )
        {
            unsigned int fd_i;
            for( fd_i=0; fd_i < file_limits.rlim_cur; ++fd_i )
            {
                close( fd_i );
            }

            open( "/dev/null", O_RDONLY ); // stdin
            open( "/dev/null", O_WRONLY ); // stdout
            open( "/dev/null", O_WRONLY ); // stderr
        }

        sm_time_get( &timestamp );

        while( true )
        {
            ms_expired = sm_time_get_elapsed_ms( &timestamp );
            if( SM_WATCHDOG_NFS_DELAY_REBOOT_IN_MS < ms_expired )
            {
                break;
            }

            sleep( 10 ); // 10 seconds
        }

        execve( "/sbin/reboot", reboot_argv, reboot_env );
        
        // Shouldn't get this far, else there was an error.
        exit(-1);
    }

    // Fork child to do reboot force.
    reboot_force_pid = fork();
    if( 0 > reboot_force_pid )
    {
        DPRINTFE( "Failed to fork process for reboot escalation, "
                  "error=%s.", strerror( errno ) );
        return;

    } else if( 0 == reboot_force_pid ) {
        // Child process.
        long ms_expired;
        int sysrq_handler_fd;
        int sysrq_tigger_fd;
        struct rlimit file_limits;
        SmTimeT timestamp;

        setpgid( 0, 0 );

        if( 0 == getrlimit( RLIMIT_NOFILE, &file_limits ) )
        {
            unsigned int fd_i;
            for( fd_i=0; fd_i < file_limits.rlim_cur; ++fd_i )
            {
                close( fd_i );
            }

            open( "/dev/null", O_RDONLY ); // stdin
            open( "/dev/null", O_WRONLY ); // stdout
            open( "/dev/null", O_WRONLY ); // stderr
        }

        sm_time_get( &timestamp );

        while( true )
        {
            ms_expired = sm_time_get_elapsed_ms( &timestamp );
            if( SM_WATCHDOG_NFS_DELAY_REBOOT_FORCE_IN_MS < ms_expired )
            {
                break;
            }

            sleep( 10 ); // 10 seconds
        }

        // Enable sysrq handling.
        sysrq_handler_fd = open( "/proc/sys/kernel/sysrq", O_RDWR | O_CLOEXEC );
        if( 0 > sysrq_handler_fd )
        {
            return;
        }

        write( sysrq_handler_fd, "1", 1 );
        close( sysrq_handler_fd );

        // Trigger sysrq command.
        sysrq_tigger_fd = open( "/proc/sysrq-trigger", O_RDWR | O_CLOEXEC );
        if( 0 > sysrq_tigger_fd )
        {
            return;
        }

        write( sysrq_tigger_fd, "b", 1 ); 
        close( sysrq_tigger_fd );

        exit( EXIT_SUCCESS );
    }

    _nfs_reboot_inprogress = SM_WATCHDOG_NFS_REBOOT_INPROGRESS;

    // Fork child to do the sm-troubleshoot.
    sm_troubleshoot_pid = fork();
    if( 0 > sm_troubleshoot_pid )
    {
        DPRINTFE( "Failed to fork process for sm-trouble, error=%s.",
                  strerror( errno ) );

    } else if( 0 == sm_troubleshoot_pid ) {
        // Child process.
        char cmd[] = "sm-troubleshoot";
        char log_file[] = SM_TROUBLESHOOT_LOG_FILE;
        char* argv[] = {cmd, log_file, NULL};
        char* env[] = {NULL};
        struct rlimit file_limits;

        setpgid( 0, 0 );

        if( 0 == getrlimit( RLIMIT_NOFILE, &file_limits ) )
        {
            unsigned int fd_i;
            for( fd_i=0; fd_i < file_limits.rlim_cur; ++fd_i )
            {
                close( fd_i );
            }

            open( "/dev/null", O_RDONLY ); // stdin
            open( "/dev/null", O_WRONLY ); // stdout
            open( "/dev/null", O_WRONLY ); // stderr
        }

        execve( SM_TROUBLESHOOT_SCRIPT, argv, env );
        
        // Shouldn't get this far, else there was an error.
        exit(-1);
    }

    // Fork child to run collect.
    collect_pid = fork();
    if( 0 > collect_pid )
    {
        DPRINTFE( "Failed to fork process for collect, error=%s.",
                  strerror( errno ) );

    } else if( 0 == collect_pid ) {
        // Child process.
        char cmd[] = "collect";
        char* argv[] = {cmd, NULL};
        char* env[] = {NULL};
        struct rlimit file_limits;

        setpgid( 0, 0 );

        if( 0 == getrlimit( RLIMIT_NOFILE, &file_limits ) )
        {
            unsigned int fd_i;
            for( fd_i=0; fd_i < file_limits.rlim_cur; ++fd_i )
            {
                close( fd_i );
            }

            open( "/dev/null", O_RDONLY ); // stdin
            open( "/dev/null", O_WRONLY ); // stdout
            open( "/dev/null", O_WRONLY ); // stderr
        }

        execve( "/usr/local/sbin/collect", argv, env );

        // Shouldn't get this far, else there was an error.
        exit(-1);
    }

    error = sm_node_utils_set_unhealthy();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to set node unhealthy, error=%s.",
                  sm_error_str(error) );
    }

    DPRINTFI( "*******************************************************" );
    DPRINTFI( "** Issuing a reboot of the system, NFS hang detected **" );
    DPRINTFI( "*******************************************************" );

    DPRINTFI( "Reboot (%i) process created.", (int) reboot_pid );
    DPRINTFI( "Reboot force (%i) process created.", (int) reboot_force_pid );
    DPRINTFI( "SM troubleshoot (%i) process created.", (int) sm_troubleshoot_pid );
    DPRINTFI( "Collect (%i) process created.", (int) collect_pid );

    snprintf( cmd, sizeof(cmd),
              "date >> %s; "
              "echo \"*******************************************\" >> %s; "
              "echo \"NFS HANG DETECTED\" >> %s", SM_WATCHDOG_NFS_DEBUG_FILE,
              SM_WATCHDOG_NFS_DEBUG_FILE, SM_WATCHDOG_NFS_DEBUG_FILE );
    system( cmd );

    int thread_i;
    for( thread_i=0; SM_WATCHDOG_NFS_MAX_BLOCKED_THREADS > thread_i;
         ++thread_i )
    {
        entry = &(_nfs_blocked_threads[thread_i]);

        if( entry->inuse )
        {
            snprintf( cmd, sizeof(cmd),
                      "date >> %s; "
                      "echo \"cat /proc/%i/sched\" >> %s; "
                      "cat /proc/%i/sched >> %s", SM_WATCHDOG_NFS_DEBUG_FILE,
                      entry->pid, SM_WATCHDOG_NFS_DEBUG_FILE, entry->pid,
                      SM_WATCHDOG_NFS_DEBUG_FILE );
            system( cmd );

            snprintf( cmd, sizeof(cmd),
                      "date >> %s; "
                      "echo \"cat /proc/%i/stack\" >> %s; "
                      "cat /proc/%i/stack >> %s", SM_WATCHDOG_NFS_DEBUG_FILE,
                      entry->pid, SM_WATCHDOG_NFS_DEBUG_FILE, entry->pid,
                      SM_WATCHDOG_NFS_DEBUG_FILE );
            system( cmd );
        }
    }

    snprintf( cmd, sizeof(cmd),
              "echo \"*******************************************\" >> %s",
              SM_WATCHDOG_NFS_DEBUG_FILE );
    system( cmd );
}
// ****************************************************************************

// ****************************************************************************
// Watchdog NFS - Search
// =====================
static void sm_watchdog_nfs_search( const char dir_name[] )
{
    bool is_dir;
    DIR* dir;
    char path[PATH_MAX];
    int path_len;
    SmNodeProcessStatusT status;
    SmErrorT error;

    dir = opendir( dir_name );
    if( NULL == dir )
    {
        DPRINTFE( "Failed to open directory (%s), error=%s.", dir_name,
                  strerror( errno ) );
        return;
    }

    struct dirent* entry;
    for( entry = readdir( dir ); NULL != entry; entry = readdir( dir ) )
    {
        is_dir = false;

        path_len = snprintf( path, sizeof(path), "%s/%s", dir_name,
                             entry->d_name );
        if( PATH_MAX <= path_len )
        {
            DPRINTFE( "Path (%s/%s) is too long, max_len=%i.",
                      dir_name, entry->d_name, path_len );
            break;
        }

        if( 0 != (DT_REG & entry->d_type) )
        {
            if( '.' != entry->d_name[0] )
            {
                struct stat stat_data;

                if( 0 > lstat( path, &stat_data ) )
                {
                    DPRINTFE( "Stat on (%s) failed, error=%s.", entry->d_name,
                              strerror( errno ) );
                    continue;
                }
    
                is_dir = S_ISDIR( stat_data.st_mode );
            }
        } else if( 0 != (DT_DIR & entry->d_type) ) {
            if(( 0 != strcmp( ".",  entry->d_name ) )&&
               ( 0 != strcmp( "..", entry->d_name ) ))
            {
                is_dir = true;
            }
        }

        if( is_dir ) 
        {
            long val;
            char* end;

            val = strtol( entry->d_name, &end, 10 );
            if(( ERANGE == errno )&&
               (( LONG_MIN == val ) ||( LONG_MAX == val )))
            {
                DPRINTFD( "Directory (%s) name out of range.",
                          entry->d_name );
                continue;
            }
            
            if( end == entry->d_name ) 
            {
                DPRINTFD( "Directory (%s) is not a pid directory.",
                          entry->d_name );
                continue;
            }

            error = sm_node_stats_get_process_status( val, &status );
            if( SM_OKAY != error )
            {
                if( SM_NOT_FOUND == error )
                {
                    DPRINTFD( "Failed to get %ld pid status, error=%s.",
                              val, sm_error_str(error) );
                } else {
                    DPRINTFE( "Failed to get %ld pid status, error=%s.",
                              val, sm_error_str(error) );
                }
                continue;
            }

            DPRINTFD( "Looking at pid=%i, name=%s", status.pid, status.name );

            if( 0 != strcmp( SM_WATCHDOG_NFS_THREAD_NAME, status.name ) )
            {
                DPRINTFD( "Process (%s) not an nfs thread, pid=%i.",
                          status.name, status.pid );
                continue;
            }

            DPRINTFD( "NFS thread, pid=%i, state=%c, block_start_ns=%lld.",
                      status.pid, status.state, status.block_start_ns );

            if(( 0 != status.block_start_ns )&&( 'D' == status.state ))
            {
                SmWatchDogNfsBlockedInfoT* entry;

                entry = sm_watchdog_nfs_find_blocked_thread( (int) val );
                if( NULL == entry )
                {
                    sm_watchdog_nfs_add_blocked_thread( (int) val, &status );

                } else if( status.block_start_ns == entry->status.block_start_ns ) {
                    long ms_expired;

                    entry->stale = false;
                    ms_expired = sm_time_get_elapsed_ms( &(entry->timestamp) );
                    if( SM_WATCHDOG_NFS_MAX_UNINTERRUPTIBLE_SLEEP < ms_expired )
                    {
                        sm_watchdog_nfs_do_reboot();
                        DPRINTFI( "Rebooting stuck nfs thread (%i).",
                                  (int) val );
                        break;
                    } else {
                        if( (SM_WATCHDOG_NFS_MAX_UNINTERRUPTIBLE_SLEEP/2)
                                < ms_expired )
                        {
                            DPRINTFI( "WARNING: NFS thread, pid=%i, state=%c, "
                                      "block_start_ns=%lld, elapsed_ms=%ld.",
                                      status.pid, status.state,
                                      status.block_start_ns, ms_expired );
                        }
                    }
                } else {
                    sm_watchdog_nfs_delete_blocked_thread( (int) val );
                    sm_watchdog_nfs_add_blocked_thread( (int) val, &status );
                }
            } else {
                sm_watchdog_nfs_delete_blocked_thread( (int) val );
            }
        }
    }

    closedir( dir );
}
// ****************************************************************************

// ****************************************************************************
// Watchdog NFS - Do Check
// =======================
void sm_watchdog_module_do_check( void )
{
    DPRINTFD( "NFS do check called." );

    if( SM_WATCHDOG_NFS_REBOOT_INPROGRESS != _nfs_reboot_inprogress )
    {
        int thread_i;
        SmWatchDogNfsBlockedInfoT* entry;

        // Mark entries as stale.
        for( thread_i=0; SM_WATCHDOG_NFS_MAX_BLOCKED_THREADS > thread_i;
             ++thread_i )
        {
            entry = &(_nfs_blocked_threads[thread_i]);

            if( entry->inuse )
            {
                entry->stale = true;
            }
        }

        // Audit NFS threads.
        sm_watchdog_nfs_search( "/proc" );

        // Cleanup stale entries.
        for( thread_i=0; SM_WATCHDOG_NFS_MAX_BLOCKED_THREADS > thread_i;
             ++thread_i )
        {
            entry = &(_nfs_blocked_threads[thread_i]);

            if(( entry->inuse )&&( entry->stale ))
            {
                memset( entry, 0, sizeof(SmWatchDogNfsBlockedInfoT) );
                entry->inuse = false;
            }
        }
    } else {
        DPRINTFD( "Reboot inprogress." );
    }
}
// ****************************************************************************

// ****************************************************************************
// Watchdog NFS - Initialize
// =========================
bool sm_watchdog_module_initialize( int* do_check_in_ms )
{
    *do_check_in_ms = SM_WATCHDOG_NFS_CHECK_IN_MS;
    _nfs_reboot_inprogress = 0;
    memset( &_nfs_blocked_threads, 0, sizeof(_nfs_blocked_threads) );
    return( true );
}
// ****************************************************************************

// ****************************************************************************
// Watchdog NFS - Finalize
// =======================
bool sm_watchdog_module_finalize( void )
{
    _nfs_reboot_inprogress = 0;
    memset( &_nfs_blocked_threads, 0, sizeof(_nfs_blocked_threads) );
    return( true );
}
// ****************************************************************************
