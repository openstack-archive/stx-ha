//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_troubleshoot.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h> 
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/prctl.h>

#include "sm_debug.h"
#include "sm_timer.h"
#include "sm_msg.h"
#include "sm_failover.h"
#include "sm_service_domain_neighbor_fsm.h"
#include "sm_service_domain_fsm.h"
#include "sm_cluster_hbs_info_msg.h"

#define SM_TROUBLESHOOT_NAME                                "sm_troubleshoot"

extern void sm_service_domain_interface_dump_state(FILE* fp);
// ****************************************************************************
// Troubleshoot - Dump Data
// ========================
SmErrorT sm_troubleshoot_dump_data( const char reason[] )
{
    pid_t pid;

    pid = fork();
    if( 0 > pid )
    {
        DPRINTFE( "Failed to fork process for troubleshoot data dump,"
                  "error=%s.", strerror( errno ) );
        return( SM_FAILED );

    } else if( 0 == pid ) {
        // Child process.
        FILE* log = NULL;
        struct rlimit file_limits;
        int result;

        result = prctl( PR_SET_NAME, SM_TROUBLESHOOT_NAME );
        if( 0 > result )
        {
            printf( "Failed to set troubleshoot process name, error=%s.\n",
                    strerror(errno) );
            exit( EXIT_FAILURE );
        }

        result = prctl( PR_SET_PDEATHSIG, SIGTERM );
        if( 0 > result )
        {
            printf( "Failed to set troubleshoot parent death signal, "
                    "error=%s.\n", strerror(errno) );
            exit( EXIT_FAILURE );
        }

        if( 0 > getrlimit( RLIMIT_NOFILE, &file_limits ) )
        {
            printf( "Failed to get file limits, error=%s.",
                    strerror( errno ) );
            exit( EXIT_FAILURE );
        }

        unsigned int fd_i;
        for( fd_i=0; fd_i < file_limits.rlim_cur; ++fd_i )
        {
            close( fd_i );
        }
        
        if( 0 > open( "/dev/null", O_RDONLY ) )
        {
            printf( "Failed to open stdin to /dev/null, error=%s.\n",
                    strerror( errno ) );
        }

        if( 0 > open( "/dev/null", O_WRONLY ) )
        {
            printf( "Failed to open stdout to /dev/null, error=%s.\n",
                    strerror( errno ) );
        }

        if( 0 > open( "/dev/null", O_WRONLY ) )
        {
            printf( "Failed to open stderr to /dev/null, error=%s.\n",
                    strerror( errno ) );
        }

        log = fopen( SM_TROUBLESHOOT_LOG_FILE, "a" );
        if( NULL != log )
        {
            char time_str[80];
            char date_str[32];
            struct tm t_real;
            struct timespec ts_real;

            clock_gettime( CLOCK_REALTIME, &(ts_real) );

            if( NULL == localtime_r( &(ts_real.tv_sec), &t_real ) )
            {
                snprintf( time_str, sizeof(time_str), "YYYY:MM:DD HH:MM:SS.xxx" );
            } else {
                strftime( date_str, sizeof(date_str), "%FT%T", &t_real );
                snprintf( time_str, sizeof(time_str), "%s.%03ld", date_str,
                          ts_real.tv_nsec/1000000 );
            }

            fprintf( log, "====================================================================\n" );
            fprintf( log, "dump-reason:     %s\n", reason );
            fprintf( log, "dump-started-at: %s\n\n", time_str );

            sm_failover_dump_state( log ); fprintf( log, "\n" );
            sm_service_domain_dump_state( log ); fprintf( log, "\n" );
            sm_service_domain_interface_dump_state( log ); fprintf( log, "\n" );
            sm_domain_neighbor_fsm_dump( log );
            SmClusterHbsInfoMsg::dump_hbs_record(log);
            sm_timer_dump_data( log ); fprintf( log, "\n" );
            sm_msg_dump_data( log );   fprintf( log, "\n" );

            fflush( log );
            fclose( log );
        }

        if( 0 == access( SM_TROUBLESHOOT_SCRIPT,  F_OK | X_OK ) )
        {
            char cmd[80];

            snprintf( cmd, sizeof(cmd), "%s %s", SM_TROUBLESHOOT_SCRIPT,
                      SM_TROUBLESHOOT_LOG_FILE );
            system( cmd );
        }

        exit( EXIT_SUCCESS );

    } else {
        // Parent process.
        DPRINTFI( "Troubleshoot process (%i) created.", (int) pid );
    }

    return( SM_OKAY );
}
// ****************************************************************************
