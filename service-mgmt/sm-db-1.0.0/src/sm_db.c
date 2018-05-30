//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sqlite3.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db_nodes.h"
#include "sm_db_node_history.h"
#include "sm_db_service_domains.h"
#include "sm_db_service_domain_interfaces.h"
#include "sm_db_service_domain_members.h"
#include "sm_db_service_domain_neighbors.h"
#include "sm_db_service_domain_assignments.h"
#include "sm_db_service_groups.h"
#include "sm_db_service_group_members.h"
#include "sm_db_services.h"
#include "sm_db_service_heartbeat.h"
#include "sm_db_service_dependency.h"
#include "sm_db_service_instances.h"
#include "sm_db_service_actions.h"
#include "sm_db_service_action_results.h"

SmErrorT sm_db_patch(const char* sm_db_name);
// ****************************************************************************
// Database - Transaction Start
// ============================
SmErrorT sm_db_transaction_start( SmDbHandleT* sm_db_handle )
{
    char* error = NULL;
    int rc;

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, "BEGIN TRANSACTION;", 0, 0,
                       &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to start a transaction, rc=%i, error=%s.", rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database - Transaction End
// ==========================
SmErrorT sm_db_transaction_end( SmDbHandleT* sm_db_handle )
{
    char* error = NULL;
    int rc;

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, "END TRANSACTION;", 0, 0,
                       &error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to end a transaction, rc=%i, error=%s.", rc,
                  error );
        sqlite3_free( error );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database - Statement Result Reset
// =================================
SmErrorT sm_db_statement_result_reset( SmDbStatementT* sm_db_statement )
{
    int rc;

    rc = sqlite3_reset( (sqlite3_stmt*) sm_db_statement );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to reset statement results, rc=%i.", rc );
        return( SM_FAILED );
    }

    return( SM_OKAY );    
}
// ****************************************************************************

// ****************************************************************************
// Database - Statement Result Step
// =================================
SmErrorT sm_db_statement_result_step( SmDbStatementT* sm_db_statement,
    bool* done )
{
    int rc;

    *done = false;

    rc = sqlite3_step(  (sqlite3_stmt*) sm_db_statement );
    if( SQLITE_ROW != rc )
    {
        *done = true;

        if( SQLITE_DONE != rc )
        {
            DPRINTFE( "Failed to step through statement results, rc=%i.", rc );
            return( SM_FAILED );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database - Statement Initialize
// ===============================
SmErrorT sm_db_statement_initialize( SmDbHandleT* sm_db_handle,
    const char* sql_statement, SmDbStatementT** sm_db_statement )
{
    int rc;

    rc = sqlite3_prepare_v2( (sqlite3*) sm_db_handle, sql_statement, -1, 
                             (sqlite3_stmt **) sm_db_statement, NULL );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to initialize statement (%s), rc=%i.", sql_statement,
                  rc );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database - Statement Finalize
// =============================
SmErrorT sm_db_statement_finalize( SmDbStatementT* sm_db_statement )
{
    int rc;

    rc = sqlite3_finalize( (sqlite3_stmt*) sm_db_statement );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to finalize statement, rc=%i.", rc );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database - Connect
// ==================
SmErrorT sm_db_connect( const char* sm_db_name, SmDbHandleT** sm_db_handle )
{
    int rc;

    rc = sqlite3_open_v2( sm_db_name, (sqlite3**) sm_db_handle,
                          SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
                          SQLITE_OPEN_NOMUTEX, NULL );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to connect to database (%s), rc=%i.", sm_db_name, rc );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database - Disconnect
// =====================
SmErrorT sm_db_disconnect( SmDbHandleT* sm_db_handle )
{
    int rc;

    rc = sqlite3_close( (sqlite3*) sm_db_handle );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to disconnect from database, rc=%i.", rc );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database - Build Main
// =====================
static SmErrorT sm_db_build_main( SmDbHandleT* sm_db_handle )
{
    SmErrorT error;

    // Create Database Tables.
    error = sm_db_nodes_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create node table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_node_history_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create node history table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_interfaces_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service domain interfaces table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domains_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service domains table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_members_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service domain members table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_neighbors_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service domain neighbors table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_assignments_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service domain assignments table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_groups_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service groups table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_group_members_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service group members table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_services_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create services table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_heartbeat_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service heartbeat table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_dependency_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service dependency table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_instances_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service instances table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }
    
    error = sm_db_service_actions_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service actions table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }
    
    error = sm_db_service_action_results_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service action results table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    // Cleanup Database Tables.
    error = sm_db_nodes_cleanup_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to cleanup nodes table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_interfaces_cleanup_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to cleanup service domain interfaces table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }
    
    error = sm_db_service_domains_cleanup_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to cleanup service domains table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_neighbors_cleanup_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to cleanup service domain neighbors table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_assignments_cleanup_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to cleanup service domain assignments table, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_groups_cleanup_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to cleanup service groups table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    error = sm_db_services_cleanup_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to cleanup services table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database - Build Heartbeat
// ==========================
static SmErrorT sm_db_build_heartbeat( SmDbHandleT* sm_db_handle )
{
    SmErrorT error;

    // Create Database Tables.
    error = sm_db_service_heartbeat_create_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to create service heartbeat table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    // Cleanup Database Tables.
    error = sm_db_service_heartbeat_cleanup_table( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to cleanup service heartbeat table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database - Build
// ================
SmErrorT sm_db_build( const char* sm_db_name, SmDbTypeT sm_db_type )
{
    int rc;
    char* sqlite3_error = NULL;
    SmDbHandleT* sm_db_handle;
    SmErrorT error;

    error = sm_db_connect( sm_db_name, &sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to open database (%s), error=%s.", sm_db_name,
                  sm_error_str( error ) );
        return( error );
    }

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, "PRAGMA journal_mode=WAL;",
                       NULL, NULL, &sqlite3_error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to set pragma journal mode to WAL, rc=%i, error=%s.",
                  rc, sqlite3_error );
        sqlite3_free( sqlite3_error );
        goto ERROR;
    }

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, "PRAGMA auto_vacuum = FULL;",
                       NULL, NULL, &sqlite3_error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to set pragma auto-vacumm to FULL, rc=%i, error=%s.",
                  rc, sqlite3_error );
        sqlite3_free( sqlite3_error );
        goto ERROR;
    }

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, "PRAGMA synchronous = FULL;",
                       NULL, NULL, &sqlite3_error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to set pragma synchronous to FULL, rc=%i, error=%s.",
                  rc, sqlite3_error );
        sqlite3_free( sqlite3_error );
        goto ERROR;
    }

    if( SM_DB_TYPE_MAIN == sm_db_type )
    {
        error = sm_db_build_main( sm_db_handle );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to build main database, error=%s.",
                      sm_error_str(error) );
            goto ERROR;
        }
    } else if( SM_DB_TYPE_HEARTBEAT == sm_db_type ) {
        error = sm_db_build_heartbeat( sm_db_handle );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to build heartbeat database, error=%s.",
                      sm_error_str(error) );
            goto ERROR;
        }
    } else {
        DPRINTFE( "Unknown database name (%s) given.", sm_db_name );
        goto ERROR;
    }

    error = sm_db_disconnect( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to close database (%s), error=%s.", sm_db_name,
                  sm_error_str( error ) );
    }

    return( SM_OKAY );

ERROR:
    error = sm_db_disconnect( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to close database (%s), error=%s.", sm_db_name,
                  sm_error_str( error ) );
    }

    return( SM_FAILED );
}
// ****************************************************************************

// ****************************************************************************
// Database - clean failed db copy
// ====================
SmErrorT _clean_failed_db( const char* sm_db_name )
{
    int errno;
    errno = unlink(sm_db_name );
    if ( 0 == errno )
    {
        DPRINTFD( "%s is removed.", sm_db_name );
        return SM_OKAY;
    }

    DPRINTFE( "Failed to remove %s. Error %d", sm_db_name, errno );
    return SM_FAILED;
}
// ****************************************************************************

// ****************************************************************************
// Database - Configure
// ====================
SmErrorT sm_db_configure( const char* sm_db_name, SmDbTypeT sm_db_type )
{
    bool copy_master_db;
    bool check_passed;
    SmErrorT error;

    copy_master_db = false;

    if( 0 > access( sm_db_name, F_OK ) )
    {
        if( ENOENT == errno )
        {
            copy_master_db = true;
            DPRINTFI( "Database file (%s) not available.", sm_db_name );

        } else {
            DPRINTFE( "Database file (%s) access failed, error=%s.",
                      sm_db_name, strerror( errno ) );
            return( SM_FAILED );
        }
    }

    if( !copy_master_db )
    {
        error = sm_db_integrity_check( sm_db_name, &check_passed );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to do integrity check on database (%s), "
                      "error=%s.", sm_db_name, sm_error_str( error ) );
            return( error );
        }

        copy_master_db = !check_passed;
    }

    if( copy_master_db )
    {
        FILE* in = NULL;
        FILE* out= NULL;
        char buf[8192] = {0};
        ssize_t rlen = 0;
        ssize_t wlen = 0;

        if( 0 == strcmp( SM_DATABASE_NAME, sm_db_name ) )
        {
            in = fopen( SM_MASTER_DATABASE_NAME, "r" );
            if( NULL == in )
            {
                DPRINTFE( "Failed to open master database (%s).",
                          SM_MASTER_DATABASE_NAME );
                return( SM_FAILED );
            }
        }
        else if( 0 == strcmp( SM_HEARTBEAT_DATABASE_NAME, sm_db_name ) )
        {
            in = fopen( SM_MASTER_HEARTBEAT_DATABASE_NAME, "r" );
            if( NULL == in )
            {
                DPRINTFE( "Failed to open master database (%s).",
                          SM_MASTER_HEARTBEAT_DATABASE_NAME );
                return( SM_FAILED );
            }
        } else {
            DPRINTFE( "Unknown database (%s) requested to configure.",
                      sm_db_name );
            return( SM_FAILED );
        }

        out = fopen( sm_db_name, "w" );
        if( NULL == out )
        {
            DPRINTFE( "Failed to open master database (%s).", sm_db_name );
            fclose( in );
            return( SM_FAILED );
        }

        while( true )
        {
            rlen = read( fileno(in), buf, sizeof(buf) );
            if( 0 == rlen )
            {
                break;

            } else if ( 0 > rlen ) {
                DPRINTFE( "Failed to read for database (%s), error=%s.",
                          sm_db_name, strerror(errno) );
                fclose( in );
                fclose( out );
                _clean_failed_db( sm_db_name );
                return( SM_FAILED );
            }

            wlen = write( fileno(out), buf, rlen );
            if( 0 > wlen )
            {
                DPRINTFE( "Failed to write to database (%s), error=%s.",
                          sm_db_name, strerror(errno) );
                fclose( in );
                fclose( out );
                _clean_failed_db( sm_db_name );
                return( SM_FAILED );
            }

            if( rlen != wlen )
            {
                DPRINTFE( "Failed to write all data to database (%s), "
                          "rlen=%i, wlen=%i.", sm_db_name,
                          (int) rlen, (int) wlen );
                fclose( in );
                fclose( out );
                _clean_failed_db( sm_db_name );
                return( SM_FAILED );
            }
        }

        DPRINTFI( "Master database copy completed." );

        error = sm_db_integrity_check( sm_db_name, &check_passed );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to do integrity check on database (%s), "
                      "error=%s.", sm_db_name, sm_error_str( error ) );
            fclose( in );
            fclose( out );
            _clean_failed_db( sm_db_name );
            return( error );
        }

        if( !check_passed )
        {
            DPRINTFE( "Database (%s) integrity check failed, after master "
                      "database copy.", sm_db_name );
            fclose( in );
            fclose( out );
            _clean_failed_db( sm_db_name );
            return( SM_FAILED );
        }

        error = sm_db_patch(sm_db_name);
        if ( SM_OKAY != error )
        {
            _clean_failed_db( sm_db_name );
            return( error );
        }
    }

    error = sm_db_build( sm_db_name, sm_db_type );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to do build database (%s), error=%s.",
                  sm_db_name, sm_error_str( error ) );
        _clean_failed_db( sm_db_name );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database - Integrity Check Callback
// ===================================
static int sm_db_integrity_check_callback( void* user_data, int num_cols,
    char **col_data, char **col_name )
{
    bool* check_passed = (bool*) user_data;

    int col_i;
    for( col_i=0; num_cols > col_i; ++col_i )
    {
        if( 0 == strcmp( col_data[col_i], "ok" ) )
        {
            *check_passed = true;

        } else {
            DPRINTFE( "Database integrity error detected, error=%s.",
                      col_data[col_i] );
        }
    }

    return( SQLITE_OK );
}
// ****************************************************************************

// ****************************************************************************
// Database - Integrity Check
// ==========================
SmErrorT sm_db_integrity_check( const char* sm_db_name, bool* check_passed )
{
    int rc;
    char* sqlite3_error = NULL;
    SmDbHandleT* sm_db_handle;
    SmErrorT error;

    *check_passed = false;

    error = sm_db_connect( sm_db_name, &sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to open database (%s), error=%s.", sm_db_name,
                  sm_error_str( error ) );
        return( error );
    }

    rc = sqlite3_exec( (sqlite3*) sm_db_handle, "PRAGMA integrity_check;",
                       sm_db_integrity_check_callback, check_passed,
                       &sqlite3_error );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to execute database (%s) integrity check, rc=%i, "
                  "error=%s.", sm_db_name, rc, sqlite3_error );
        sqlite3_free( sqlite3_error );
        goto ERROR;
    }
    
    error = sm_db_disconnect( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to close database (%s), error=%s.", sm_db_name,
                  sm_error_str( error ) );
    }

    return( SM_OKAY );

ERROR:
    error = sm_db_disconnect( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to close database (%s), error=%s.", sm_db_name,
                  sm_error_str( error ) );
    }

    return( SM_FAILED );
}
// ****************************************************************************

// ****************************************************************************
// Database - Checkpoint
// =====================
SmErrorT sm_db_checkpoint( const char* sm_db_name )
{
    int wal_num_frames, checkpointed_frames;
    int rc;
    SmDbHandleT* sm_db_handle;
    SmErrorT error;

    error = sm_db_connect( sm_db_name, &sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to open database (%s), error=%s.", sm_db_name,
                  sm_error_str( error ) );
        return( error );
    }

    rc = sqlite3_wal_checkpoint_v2( (sqlite3*) sm_db_handle, NULL,
                                    SQLITE_CHECKPOINT_PASSIVE,
                                    &wal_num_frames, &checkpointed_frames );
    if( SQLITE_OK != rc )
    {
        DPRINTFE( "Failed to checkpoint database (%s), rc=%i.", sm_db_name, 
                  rc );
        goto ERROR;
    }

    DPRINTFD( "Database (%s) checkpoint complete, wal_num_frames=%i, "
              "checkpointed_frames=%i.", sm_db_name, wal_num_frames,
              checkpointed_frames );

    error = sm_db_disconnect( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to close database (%s), error=%s.", sm_db_name,
                  sm_error_str( error ) );
    }

    return( SM_OKAY );

ERROR:
    error = sm_db_disconnect( sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to close database (%s), error=%s.", sm_db_name,
                  sm_error_str( error ) );
    }

    return( SM_FAILED );
}
// ****************************************************************************

// ****************************************************************************
// Database - patch
// ===================
SmErrorT sm_db_patch(const char* sm_db_name)
{
    SmErrorT result = SM_OKAY;
    if( 0 == strcmp(sm_db_name, SM_DATABASE_NAME) )
    {
        FILE *fp;
        char cmd_line[255];
        char line[1024];
        int ret;

        struct stat stat_o = {0};
        struct stat stat_n = {0};
        bool changed = true;

        if( 0 != stat(SM_DATABASE_NAME, &stat_o) )
        {
            DPRINTFE("Failed to get stat: %s", SM_DATABASE_NAME);
        }
        snprintf(cmd_line, sizeof(cmd_line), "cat %s | sqlite3 %s 2>&1",
                 SM_PATCH_SCRIPT,
                 SM_DATABASE_NAME
        );

        fp = popen(cmd_line, "r");
        if (fp == NULL) {
            DPRINTFE("Failed to execute patch script.");
            return SM_FAILED;
        }

        while (fgets(line, sizeof(line) - 1, fp) != NULL) {
            DPRINTFI("%s", line);
        }
        ret = pclose(fp);

        if( 0 != stat_o.st_mtime)
        {
            if(0 != stat(SM_DATABASE_NAME, &stat_n))
            {
                DPRINTFE("Failed to get stat: %s", SM_DATABASE_NAME);
            }else
            {
                changed = (stat_n.st_mtime != stat_o.st_mtime ||
                    stat_n.st_mtim.tv_nsec != stat_o.st_mtim.tv_nsec);
            }
        }

        if (0 == ret)
        {
            if(changed)
            {
                DPRINTFI("Patch script completed successfully.");
            }
        }else
        {
            result = SM_FAILED;
            DPRINTFE("Patch script failed.");
        }
    }
    return result;
}
// ****************************************************************************

// ****************************************************************************
// Database - Initialize
// =====================
SmErrorT sm_db_initialize( void )
{
    SmErrorT error;

    error = sm_db_nodes_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize node database module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_node_history_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize node history database module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_interfaces_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain interfaces database "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domains_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domains database "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_members_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain members database "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_neighbors_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain neighbors database "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_domain_assignments_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain assignments database "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_groups_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service groups database module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_group_members_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service group members database "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_services_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize services database module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_heartbeat_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service heartbeat database module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_dependency_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service dependency database module, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_instances_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service instances database "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_actions_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service actions database "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_db_service_action_results_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service action results database "
                  "module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database - Finalize
// ===================
SmErrorT sm_db_finalize( void )
{
    SmErrorT error;

    error = sm_db_service_action_results_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service action results database "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_db_service_actions_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service actions database "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_db_service_instances_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service instances database "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_db_service_heartbeat_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service heartbeat database module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_db_service_dependency_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service dependency database module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_db_services_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize services database module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_db_service_group_members_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service group members database "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_db_service_groups_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service groups database module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_db_service_domain_assignments_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service domain assignments database "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_db_service_domain_neighbors_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service domain neighbors database "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_db_service_domain_members_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service domain members database "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_db_service_domains_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service domains database "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_db_service_domain_interfaces_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize service domain interfaces database "
                  "module, error=%s.", sm_error_str( error ) );
    }

    error = sm_db_node_history_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize node database module, "
                  "error=%s.", sm_error_str( error ) );
    }

    error = sm_db_nodes_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize node database module, "
                  "error=%s.", sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************