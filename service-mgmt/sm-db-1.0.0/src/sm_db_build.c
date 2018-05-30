//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db.h"

// ****************************************************************************
// Main - Database Builder
// =======================
int main( int argc, char *argv[], char *envp[] )
{
    char main_db_name[] = "sm.db.main";
    char heartbeat_db_name[] = "sm.db.hb";
    SmErrorT error;

    error = sm_debug_initialize();
    if( SM_OKAY != error )
    {
        printf( "Debug initialization failed, error=%s.", 
                sm_error_str( error ) );
        return( EXIT_FAILURE );
    }

    error = sm_db_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize database module, error=%s.",
                  sm_error_str( error ) );
        return( SM_FAILED );
    }

    DPRINTFI( "Building Databases." );

    error = sm_db_build( main_db_name, SM_DB_TYPE_MAIN );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed build database, error=%s.", 
                  sm_error_str(error) );
        return( error );
    }

    error = sm_db_build( heartbeat_db_name, SM_DB_TYPE_HEARTBEAT );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed build heartbeat database, error=%s.",
                  sm_error_str(error) );
        return( error );
    }

    DPRINTFI( "Building Databases Complete." );

    error = sm_db_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finialize database module, error=%s.",
                  sm_error_str( error ) );
    }

    error = sm_debug_finalize();
    if( SM_OKAY != error )
    {
        printf( "Debug finalization failed, error=%s.", 
                sm_error_str( error ) );
    }

   return( EXIT_SUCCESS );
}
// ****************************************************************************
