//
// Copyright (c) 2014-2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_eru_db.h"

#include <stddef.h>
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
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/mman.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"

#define SM_ERU_DB_FILENAME                          "/var/lib/sm/sm.eru.v1"
#define SM_ERU_DB_MMAP_INITIALIZED                               0xA5A5A5A5
#define SM_ERU_DB_MAX_RECORDS                                       4194304
#define SM_ERU_DB_HEADER_SIZE                                             8

typedef struct
{
    bool initialized;
    bool read_only;
    int fd;
    FILE* fp;
    off_t file_size;
} SmEruDBFileInfoT;

typedef struct
{
    int write_index;
    bool wrapped;
} SmEruDatabaseHeaderT;

static SmEruDBFileInfoT _file_info;
static SmEruDatabaseHeaderT _file_header;

static uint32_t _1MB_data_block[1048576/sizeof(uint32_t)];

// ****************************************************************************
// Event Recorder Unit Database - Read Lock
// ========================================
static SmErrorT sm_eru_db_read_lock( void )
{
    struct flock lock;
    int result;

    memset( &lock, 0, sizeof(lock) );
    lock.l_type = F_RDLCK;

    int retry_i;
    for( retry_i=0; 10 > retry_i; ++retry_i )
    {
        result = fcntl( _file_info.fd, F_SETLK, &lock );
        if( 0 == result )
        {
            return( SM_OKAY );
        }

        usleep( 50000 ); // 50 milliseconds.
    }

    return( SM_FAILED );
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Write Lock
// =========================================
static SmErrorT sm_eru_db_write_lock( void )
{
    struct flock lock;
    int result;

    memset( &lock, 0, sizeof(lock) );
    lock.l_type = F_WRLCK;

    int retry_i;
    for( retry_i=0; 10 > retry_i; ++retry_i )
    {
        result = fcntl( _file_info.fd, F_SETLK, &lock );
        if( 0 == result )
        {
            return( SM_OKAY );
        }

        usleep( 50000 ); // 50 milliseconds.
    }

    return( SM_FAILED );
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Unlock
// =====================================
static void sm_eru_db_unlock( void )
{
    struct flock lock;
    int result;

    memset( &lock, 0, sizeof(lock) );
    lock.l_type = F_UNLCK;

    int retry_i;
    for( retry_i=0; 10 > retry_i; ++retry_i )
    {
        result = fcntl( _file_info.fd, F_SETLK, &lock );
        if( 0 == result )
        {
            return;
        }

        usleep( 50000 ); // 50 milliseconds.
    }

    abort();
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Total
// ====================================
int sm_eru_db_total( void )
{
    if( !_file_info.initialized )
    {
        DPRINTFE( "Database is not initialized." );
        return( 0 );
    }

    if( _file_header.wrapped )
    {
        return( SM_ERU_DB_MAX_RECORDS );
    }
    return( _file_header.write_index );
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Get Write Index
// ==============================================
int sm_eru_db_get_write_index( void )
{
    if( !_file_info.initialized )
    {
        DPRINTFE( "Database is not initialized." );
        return( -1 );
    }

    return( _file_header.write_index );
}

// ****************************************************************************
// Event Recorder Unit Database - Flush the database
// =================================================
void sm_eru_db_sync( void )
{
    if( _file_info.fp != NULL )
    {
       fflush( _file_info.fp );
       fsync(fileno(_file_info.fp));
    }
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Read
// ===================================
SmErrorT sm_eru_db_read( int read_index, SmEruDatabaseEntryT* entry )
{
    SmErrorT error;

    if( !_file_info.initialized )
    {
        DPRINTFE( "Database is not initialized." );
        return( SM_NOT_FOUND );
    }

    if(( 0 > read_index )&&( SM_ERU_DB_MAX_RECORDS <= read_index ))
    {
        DPRINTFE( "Database read index (%i) is invalid.", read_index );
        return( SM_FAILED );
    }
    
    error = sm_eru_db_read_lock();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get eru database read lock, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    fseek(_file_info.fp, (read_index*sizeof(SmEruDatabaseEntryT))+SM_ERU_DB_HEADER_SIZE, SEEK_SET);
    fread(entry, 1, sizeof(SmEruDatabaseEntryT), _file_info.fp);

    sm_eru_db_unlock();

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Write
// ====================================
SmErrorT sm_eru_db_write( SmEruDatabaseEntryT* entry )
{
    SmErrorT error;

    if( !_file_info.initialized )
    {
        DPRINTFE( "Database is not initialized." );
        return( SM_FAILED );
    }

    error = sm_eru_db_write_lock();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get eru database write lock, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    clock_gettime( CLOCK_REALTIME, &(entry->ts_real) );
    entry->index = _file_header.write_index;

    fseek(_file_info.fp, (_file_header.write_index*sizeof(SmEruDatabaseEntryT))+SM_ERU_DB_HEADER_SIZE, SEEK_SET);
    size_t written = fwrite(entry, 1, sizeof(SmEruDatabaseEntryT), _file_info.fp);

    if( 0 > written )
    {
        DPRINTFE( "Failed to write , error=%s.", strerror(errno) );
        sm_eru_db_unlock();
        return( SM_FAILED );
    }
    if( written != sizeof(SmEruDatabaseEntryT))
    {
        DPRINTFE( "Failed to write, %lu bytes should of been written instead of %lu", sizeof(SmEruDatabaseEntryT), written );
        sm_eru_db_unlock();
        return( SM_FAILED );
    }

    ++(_file_header.write_index);

    if( SM_ERU_DB_MAX_RECORDS <= _file_header.write_index )
    {
        DPRINTFD( "Database wrapped." );
        _file_header.wrapped = true;
        _file_header.write_index = 0;
    }

    // The first 8 bytes of the database contains the write index and the wrapped flag
    unsigned char buffer[SM_ERU_DB_HEADER_SIZE];
    memset( &buffer, 0, SM_ERU_DB_HEADER_SIZE);

    memcpy( buffer, &_file_header.write_index, sizeof(int));
    memcpy( buffer+sizeof(int), &_file_header.wrapped, sizeof(bool));

    fseek(_file_info.fp, 0, SEEK_SET);
    fwrite(buffer, 1, SM_ERU_DB_HEADER_SIZE, _file_info.fp);

    sm_eru_db_unlock();

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Display
// ======================================
void sm_eru_db_display( SmEruDatabaseEntryT* entry, bool want_raw )
{
    char time_str[80];
    char date_str[32];
    char addr_type_str[32];
    struct tm t_real;
    unsigned long long user_time;
    unsigned long long nice_time;
    unsigned long long total_time;
    long double user, nice, sys, idle, iowait, irq, soft_irq, steal;
    long double guest, guest_nice;
    char ip_str[SM_NETWORK_ADDRESS_MAX_CHAR];
    SmNodeCpuStatsT* cpu = &(entry->u.cpu_stats);
    SmNodeMemStatsT* mem = &(entry->u.mem_stats);
    SmNodeDiskStatsT* disk = &(entry->u.disk_stats);
    SmNodeNetDevStatsT* net = &(entry->u.net_stats);
    SmHwQdiscInfoT* qdisc = &(entry->u.tc_stats);
    SmHwInterfaceChangeDataT* if_change = &(entry->u.if_change);
    SmHwIpChangeDataT* ip_change = &(entry->u.ip_change);

    if( NULL == localtime_r( &(entry->ts_real.tv_sec), &t_real ) )
    {
        snprintf( time_str, sizeof(time_str), "YYYY:MM:DD HH:MM:SS.xxx" );
    } else {
        strftime( date_str, sizeof(date_str), "%FT%T", &t_real );
        snprintf( time_str, sizeof(time_str), "%s.%03ld", date_str,
                  entry->ts_real.tv_nsec/1000000 );
    }

    printf( "%10i. %s " , entry->index, time_str );

    switch( entry->type )
    {
        case SM_ERU_DATABASE_ENTRY_TYPE_CPU_STATS:
            user_time  = cpu->user_cpu_usage - cpu->guest_cpu_usage;
            nice_time  = cpu->nice_cpu_usage - cpu->guest_nice_cpu_usage;
            total_time = user_time + nice_time + cpu->idle_task_cpu_usage
                       + cpu->iowait_cpu_usage + cpu->system_cpu_usage
                       + cpu->irq_cpu_usage + cpu->soft_irq_cpu_usage
                       + cpu->steal_cpu_usage + cpu->guest_cpu_usage
                       + cpu->guest_nice_cpu_usage;

            if( 0 != total_time )
            {
                user       = (long double)user_time
                           /(long double)total_time * 100.0;
                nice       = (long double)nice_time
                           /(long double)total_time * 100.0;
                sys        = (long double)cpu->system_cpu_usage
                           /(long double)total_time * 100.0;
                idle       = (long double)cpu->idle_task_cpu_usage
                           /(long double)total_time * 100.0;
                iowait     = (long double)cpu->iowait_cpu_usage
                           /(long double)total_time * 100.0;
                irq        = (long double)cpu->irq_cpu_usage
                           /(long double)total_time * 100.0;
                soft_irq   = (long double)cpu->soft_irq_cpu_usage
                           /(long double)total_time * 100.0;
                steal      = (long double)cpu->steal_cpu_usage
                           /(long double)total_time * 100.0;
                guest      = (long double)cpu->guest_cpu_usage
                           /(long double)total_time * 100.0;
                guest_nice = (long double)cpu->guest_nice_cpu_usage
                           /(long double)total_time * 100.0;

                printf( "cpu-stats: %s user: %5.1Lf nice: %5.1Lf sys: %5.1Lf"
                        " idle: %5.1Lf iowait: %5.1Lf irq: %5.1Lf"
                        " soft-irq: %5.1Lf steal: %5.1Lf guest-cpu: %5.1Lf"
                        " guest-nice: %5.1Lf total-interrupts: %llu"
                        " total-context-switches: %llu"
                        " total-processes: %llu processes_running: %llu"
                        " processes_blocked: %llu boot_time_in_secs: %llu\n",
                        cpu->cpu_name, user, nice, sys, idle, iowait, irq,
                        soft_irq, steal, guest, guest_nice,
                        cpu->total_interrupts, cpu->total_context_switches,
                        cpu->total_processes, cpu->processes_running,
                        cpu->processes_blocked, cpu->boot_time_in_secs );
            } else {
                want_raw = true;
            }

            if( want_raw )
            {
                printf( "cpu-raw-stats: %s user: %llu nice: %llu sys: %llu"
                        " idle: %llu iowait: %llu irq: %llu soft-irq: %llu"
                        " steal: %llu guest-cpu: %llu guest-nice: %llu"
                        " total-interrupts: %llu total-context-switches: %llu"
                        " total-processes: %llu processes_running: %llu"
                        " processes_blocked: %llu boot_time_in_secs: %llu\n",
                        cpu->cpu_name, cpu->user_cpu_usage, cpu->nice_cpu_usage,
                        cpu->system_cpu_usage, cpu->idle_task_cpu_usage,
                        cpu->iowait_cpu_usage, cpu->irq_cpu_usage,
                        cpu->soft_irq_cpu_usage, cpu->steal_cpu_usage,
                        cpu->guest_cpu_usage, cpu->guest_nice_cpu_usage,
                        cpu->total_interrupts, cpu->total_context_switches,
                        cpu->total_processes, cpu->processes_running,
                        cpu->processes_blocked, cpu->boot_time_in_secs );
            }
        break;

        case SM_ERU_DATABASE_ENTRY_TYPE_MEM_STATS:
            printf( "mem-stats: total: %lu free: %lu buffers: %lu"
                    " cached: %lu swap-cached: %lu swap-total: %lu"
                    " swap-free: %lu active: %lu inactive: %lu"
                    " dirty: %lu hugepages-total: %lu"
                    " hugepages-free: %lu hugepage-size: %lu"
                    " nfs-uncommited: %lu commited: %lu\n",
                    mem->total_memory_kB, mem->free_memory_kB,
                    mem->buffers_kB, mem->cached_kB, mem->swap_cached_kB,
                    mem->swap_total_kB, mem->swap_free_kB, mem->active_kB,
                    mem->inactive_kB, mem->dirty_kB, mem->hugepages_total,
                    mem->hugepages_free, mem->hugepage_size_kB,
                    mem->nfs_uncommited_kB, mem->commited_memory_kB );
        break;

        case SM_ERU_DATABASE_ENTRY_TYPE_DISK_STATS:
            printf( "disk-stats: %s major: %i minor: %i"
                    " reads-completed: %lu reads-merged: %lu"
                    " sectors-read: %lu ms-spent-reading: %lu"
                    " writes-completed: %lu writes-merged: %lu"
                    " sectors-written: %lu ms-spent-writing: %lu"
                    " io-inprogress: %lu ms-spent-io: %lu"
                    " weighted-ms-spent-io: %lu\n", disk->dev_name,
                    disk->major_num, disk->minor_num, disk->reads_completed,
                    disk->reads_merged, disk->sectors_read,
                    disk->ms_spent_reading, disk->writes_completed,
                    disk->writes_merged, disk->sectors_written, 
                    disk->ms_spent_writing, disk->io_inprogress,
                    disk->ms_spent_io, disk->weighted_ms_spent_io );
        break;

        case SM_ERU_DATABASE_ENTRY_TYPE_NET_STATS:
            printf( "net-stats: %s rx-bytes: %llu rx-packets: %llu"
                    " rx-errors: %llu rx-dropped: %llu rx-fifo-errors: %llu"
                    " rx-frame-errors: %llu rx-compressed-packets: %llu"
                    " rx-multicast-frames: %llu tx-bytes: %llu"
                    " tx-packets: %llu tx-errors: %llu tx-dropped: %llu"
                    " tx-fifo-errors: %llu tx-collisions: %llu"
                    " tx-carrier-loss: %llu tx-compressed-packets: %llu\n",
                    net->dev_name, net->rx_bytes, net->rx_packets, 
                    net->rx_errors, net->rx_dropped, net->rx_fifo_errors,
                    net->rx_frame_errors, net->rx_compressed_packets,
                    net->rx_multicast_frames, net->tx_bytes, net->tx_packets,
                    net->tx_errors, net->tx_dropped, net->tx_fifo_errors,
                    net->tx_collisions, net->tx_carrier_loss,
                    net->tx_compressed_packets );
        break;

        case SM_ERU_DATABASE_ENTRY_TYPE_TC_STATS:
            printf( "tc-stats: %s %s %s bytes: %" PRIu64 " packets: %" PRIu64
                    " qlen: %" PRIu64 " backlog: %" PRIu64 " drops: %" PRIu64
                    " requeues: %" PRIu64 " overlimits: %" PRIu64 "\n",
                    qdisc->interface_name, qdisc->queue_type,
                    qdisc->handle, qdisc->bytes, qdisc->packets,
                    qdisc->q_length, qdisc->backlog, qdisc->drops,
                    qdisc->requeues, qdisc->overlimits );
        break;

        case SM_ERU_DATABASE_ENTRY_TYPE_IF_CHANGE:
            printf( "if-change: %s type: %s state: %s\n",
                    if_change->interface_name, 
                    (SM_HW_INTERFACE_CHANGE_TYPE_ADD == if_change->type)
                    ? "add" : "delete",
                    sm_interface_state_str(if_change->interface_state) );
        break;

        case SM_ERU_DATABASE_ENTRY_TYPE_IP_CHANGE:
            sm_network_address_str( &(ip_change->address), ip_str );

            if( SM_HW_ADDRESS_TYPE_ADDRESS == ip_change->address_type )
            {
                snprintf( addr_type_str, sizeof(addr_type_str), "address" );
            } else if( SM_HW_ADDRESS_TYPE_LOCAL == ip_change->address_type ){
                snprintf( addr_type_str, sizeof(addr_type_str), "local" );
            } else if( SM_HW_ADDRESS_TYPE_BROADCAST == ip_change->address_type ){
                snprintf( addr_type_str, sizeof(addr_type_str), "broadcast" );
            } else if( SM_HW_ADDRESS_TYPE_ANYCAST == ip_change->address_type ){
                snprintf( addr_type_str, sizeof(addr_type_str), "anycast" );
            } else {
                snprintf( addr_type_str, sizeof(addr_type_str), "unknown" );
            }

            printf( "ip-change: %s type: %s address: %s/%i address-type=%s\n",
                    ip_change->interface_name,
                    (SM_HW_IP_CHANGE_TYPE_ADD == ip_change->type)
                    ? "add" : "delete", ip_str, ip_change->prefix_len,
                    addr_type_str );
        break;

        default:
            DPRINTFE( "Unknown entry type (%i).", entry->type );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Initialize
// =========================================
SmErrorT sm_eru_db_initialize( char* filename, bool read_only )
{
    char db_filename[512] = "";
    struct stat stat_info;
    int result;
    _file_info.initialized = false;
    _file_info.fp = NULL;
    _file_header.wrapped = false;
    _file_header.write_index = 0;

    if( NULL == filename )
    {
        snprintf( db_filename, sizeof(db_filename), "%s", SM_ERU_DB_FILENAME );
    } else {
        snprintf( db_filename, sizeof(db_filename), "%s", filename );

    }

    DPRINTFD( "ERU Database Entry size is %i, total_records=%i.",
              (int) sizeof(SmEruDatabaseEntryT), SM_ERU_DB_MAX_RECORDS );

    if( read_only )
    {
        _file_info.fp = fopen (db_filename, "r");
    } else {
       _file_info.fp = fopen (db_filename, "r+");
       if (_file_info.fp == NULL)
       {
          // File does not exist, open for write with file creation
          _file_info.fp = fopen (db_filename, "w+");
       }
    }

    if( _file_info.fp == NULL )
    {
        DPRINTFE( "Failed to open database file (%s), error=%s.", db_filename,
                  strerror( errno ) );
        return( SM_FAILED );
    }

    _file_info.fd = fileno(_file_info.fp);

    result = fstat( _file_info.fd, &stat_info );
    if( 0 > result )
    {
        DPRINTFE( "Failed to get database file stats, error=%s.",
                  strerror(errno) );
        sm_eru_db_finalize();
        return( SM_FAILED );
    }

    int max_db_size = (sizeof(SmEruDatabaseEntryT) * SM_ERU_DB_MAX_RECORDS ) + SM_ERU_DB_HEADER_SIZE;

    if( (int) stat_info.st_size < max_db_size )
    {
        if( read_only )
        {
            DPRINTFE( "Database is not sized correctly, expected=%i, "
                      "actual=%i.", max_db_size, (int) stat_info.st_size );
            sm_eru_db_finalize();
            return( SM_FAILED );

        } else {
            int num_blocks;

            DPRINTFI( "Initializing database." );

            num_blocks = max_db_size / sizeof(_1MB_data_block) + 1;
            memset( _1MB_data_block, 0, sizeof(_1MB_data_block) );

            int block_i;
            for( block_i=0; block_i < num_blocks; ++block_i )
            {
                write( _file_info.fd, (void*) _1MB_data_block,
                       sizeof(_1MB_data_block) );
            }

            result = fstat( _file_info.fd, &stat_info );
            if( 0 > result )
            {
                DPRINTFE( "Failed to get database file statistics, error=%s.",
                          strerror(errno) );
                sm_eru_db_finalize();
                return( SM_FAILED );
            }

            if( (int) stat_info.st_size < (max_db_size) )
            {
                DPRINTFE( "Failed to create database, size=%d.",
                          (int) stat_info.st_size );
                sm_eru_db_finalize();
                return( SM_FAILED );
            }

            _file_info.file_size = stat_info.st_size;
        }
    } else {
        _file_info.file_size = stat_info.st_size;
    }

    _file_info.read_only = read_only;
    _file_info.initialized = true;

    // Read write index and wrapper flag
    unsigned char buffer[SM_ERU_DB_HEADER_SIZE];
    int bytes_read = fread(buffer, 1, SM_ERU_DB_HEADER_SIZE, _file_info.fp);

    if (bytes_read == SM_ERU_DB_HEADER_SIZE)
    {
       memcpy(&_file_header.write_index, buffer, sizeof(int));
       memcpy(&_file_header.wrapped, buffer+sizeof(int), sizeof(bool));
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Event Recorder Unit Database - Finalize
// =======================================
SmErrorT sm_eru_db_finalize( void )
{
    _file_info.initialized = false;

    if( _file_info.fp != NULL )
    {
        sm_eru_db_sync();
        fclose(_file_info.fp);
    }

    return( SM_OKAY );
}
// ****************************************************************************
