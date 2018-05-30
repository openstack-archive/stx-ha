//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_eru_db.h"

static struct option _sm_eru_long_options[] =
{
    { "file",       required_argument, NULL, 'f'},
    { "type",       required_argument, NULL, 't'},
    { "start-time", required_argument, NULL, 's'},
    { "end-time",   required_argument, NULL, 'e'},
    { "last",       required_argument, NULL, 'l'},
    { "raw",        no_argument,       NULL, 'r'},
    {0, 0, 0, 0}
};

// ****************************************************************************
// Main - Event Recorder Unit Dump
// ===============================
static void usage( void )
{
    printf( " usage:\n"
            "   sm-eru-dump [--file <file name of the eru database>]\n"
            "               [--type <cpu|mem|disk|net|tc|if|ip>]\n"
            "               [--start-time <\"YEAR-MONTH-DAY HH:MM:SS\">]\n"
            "               [--end-time   <\"YEAR-MONTH-DAY HH:MM:SS\">]\n"
            "               [--last <number>]\n"
            "               [--raw]\n"
            "               [--help]\n"
            "       --type      : type of record to filter on\n"
            "       --start-time: start time for filter (inclusive)\n"
            "       --end-time  : end time for filter (inclusive)\n"
            "       --last      : display last so many records\n"
            "       --raw       : display raw record data\n"
            "       --help      : print out this help message\n"
            "\n"
            "   eg. sm-eru-dump --start-time \"2014-10-17 22:15:10\"\n"
            "\n" );
}
// ****************************************************************************

// ****************************************************************************
// Main - Event Recorder Unit Dump
// ===============================
int main( int argc, char *argv[], char *envp[] )
{
    int c;
    int last = -1;
    int record_i, record_count, total_records;
    char buf[128];
    bool want_raw = false;
    bool filter_record_type = false;
    bool filter_start_time = false;
    bool filter_end_time = false;
    struct tm start_tm;
    struct tm end_tm;
    time_t start_time = 0;
    time_t end_time = 0;
    int num_record_types = 0;
    char* filename = NULL;
    char* tz = NULL;

    SmEruDatabaseEntryType record_types[SM_ERU_DATABASE_ENTRY_TYPE_MAX];
    SmEruDatabaseEntryT entry;
    SmErrorT error;

    memset( record_types, 0, sizeof(record_types) );

    tz = getenv( "TZ" );
    setenv( "TZ", "", 1 );
    tzset();

    while( true )
    {
        c = getopt_long( argc, argv, "", _sm_eru_long_options, NULL );
        if( -1 == c )
        {
            break;
        }

        switch( c )
        {
            case 'f':
                if( optarg )
                {
                    filename = (char*) optarg;
                }
                break;

            case 't':
                if( optarg )
                {
                    if( 0 == strcmp( optarg, "cpu" ) )
                    {
                        filter_record_type = true;
                        record_types[num_record_types]
                            = SM_ERU_DATABASE_ENTRY_TYPE_CPU_STATS;
                        ++num_record_types;
                        printf( "filter-record-match : cpu-stats\n" );
                        
                    } else if( 0 == strcmp( optarg, "mem" ) ) {
                        filter_record_type = true;
                        record_types[num_record_types]
                            = SM_ERU_DATABASE_ENTRY_TYPE_MEM_STATS;
                        ++num_record_types;
                        printf( "filter-record-match : memory-stats\n" );

                    } else if( 0 == strcmp( optarg, "disk" ) ) {
                        filter_record_type = true;
                        record_types[num_record_types]
                            = SM_ERU_DATABASE_ENTRY_TYPE_DISK_STATS;
                        ++num_record_types;
                        printf( "filter-record-match : disk-stats\n" );

                    } else if( 0 == strcmp( optarg, "net" ) ) {
                        filter_record_type = true;
                        record_types[num_record_types]
                            = SM_ERU_DATABASE_ENTRY_TYPE_NET_STATS;
                        ++num_record_types;
                        printf( "filter-record-match : net-stats\n" );

                    } else if( 0 == strcmp( optarg, "tc" ) ) {
                        filter_record_type = true;
                        record_types[num_record_types]
                            = SM_ERU_DATABASE_ENTRY_TYPE_TC_STATS;
                        ++num_record_types;
                        printf( "filter-record-match : tc-stats\n" );

                    } else if( 0 == strcmp( optarg, "if" ) ) {
                        filter_record_type = true;
                        record_types[num_record_types]
                            = SM_ERU_DATABASE_ENTRY_TYPE_IF_CHANGE;
                        ++num_record_types;
                        printf( "filter-record-match : if-change\n" );
                
                    } else if( 0 == strcmp( optarg, "ip" ) ) {
                        filter_record_type = true;
                        record_types[num_record_types]
                            = SM_ERU_DATABASE_ENTRY_TYPE_IP_CHANGE;
                        ++num_record_types;
                        printf( "filter-record-match : ip-change\n" );
                    }
                }
            break;

            case 's':
                if( optarg )
                {
                    filter_start_time = true;
                    if( NULL == strptime( optarg, "%FT%T", &start_tm ) )
                    {
                        strptime( optarg, "%F %T", &start_tm );
                    }
                    start_time = mktime( &start_tm );
                    strftime( buf, sizeof(buf), "%FT%T", &start_tm );
                    printf( "filter-start-time   : %s <= record-time\n", buf );
                }
            break;

            case 'e':
                if( optarg )
                {
                    filter_end_time = true;
                    if( NULL == strptime( optarg, "%FT%T", &end_tm ) )
                    {
                        strptime( optarg, "%F %T", &end_tm );
                    }
                    end_time = mktime( &end_tm );
                    strftime( buf, sizeof(buf), "%FT%T", &end_tm );
                    printf( "filter-end-time     : %s >= record-time\n", buf );
                }
            break;

            case 'l':
                if( optarg )
                {
                    last = atoi( optarg );
                    printf( "display-last        : %i records\n", last );
                }
            break;

            case 'r':
                printf( "dump-raw-data       : true\n" );
                want_raw = true;
            break;

            case 'h':
            case '?':
                usage();
                exit(0);
            break;
        }
    }

    if( tz )
    {
        setenv( "TZ", tz, 1 );
    } else {
        unsetenv( "TZ" );
    }
    tzset();

    error = sm_eru_db_initialize( filename, true );
    if( SM_OKAY != error )
    {
        DPRINTFE( "ERU database initialize failed, error=%s.",
                  sm_error_str( error ) );
        return( EXIT_FAILURE );
    }

    total_records = sm_eru_db_total();
    record_count = total_records;

    printf( "total-records: %i (entry-size=%i bytes)\n", total_records,
            (int) sizeof(SmEruDatabaseEntryT) );

    if(( -1 == last )||( last >= total_records ))
    {
        record_i = 0;
    } else {
        int last_record = sm_eru_db_get_write_index();
        if( 0 >= last_record )
        {
            record_i = 0;
        } else {
            record_i = last_record;
            record_count = last;
            for( ; 0 < last; --last )
            {
                --record_i;
                if( 0 > record_i )
                {
                    record_i = total_records-1;
                }
            }
        }
    }

    for( ; 0 < record_count; --record_count )
    {
        error = sm_eru_db_read( record_i, &entry );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to read record at index (%i), error=%s.",
                      record_i, sm_error_str( error ) );
            break;
        }

        if( filter_record_type )
        {
            bool found = false;

            int type_i;
            for( type_i=0; num_record_types > type_i; ++type_i )
            {
                if( record_types[type_i] == entry.type )
                {
                    found = true;
                    break;
                }
            }

            if( !found )
            {
                goto INCREMENT;
            }
        }

        if( filter_start_time )
        {
            double diff = difftime( start_time, entry.ts_real.tv_sec ); 
            if( diff > 0.0 )
            {
                goto INCREMENT;
            }
        }

        if( filter_end_time )
        {
            double diff = difftime( end_time, entry.ts_real.tv_sec ); 
            if( diff < 0.0 )
            {
                goto INCREMENT;
            }
        }

        sm_eru_db_display( &entry, want_raw );

INCREMENT:
        ++record_i;
        if( total_records <= record_i )
        {
            record_i = 0;
        }
    }
    
    printf( "total-records: %i (entry-size=%i bytes)\n", total_records,
            (int) sizeof(SmEruDatabaseEntryT) );

    error = sm_eru_db_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "ERU database finalize failed, error=%s.",
                  sm_error_str( error ) );
    }

    return( EXIT_SUCCESS );
}
// ****************************************************************************
