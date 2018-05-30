//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_trap.h"
#include "sm_process.h"

// ****************************************************************************
// Main - Thread
// =============
int main( int argc, char *argv[], char *envp[] )
{
    SmErrorT error;

    error = sm_trap_initialize( "sm" );
    if( SM_OKAY != error )
    {
        printf( "Trap initialization handling failed, error=%s.\n",
                sm_error_str( error ) );
        return( EXIT_FAILURE );
    }

    error = sm_debug_initialize();
    if( SM_OKAY != error )
    {
        printf( "Debug initialization failed, error=%s.\n", 
                sm_error_str( error ) );
        return( EXIT_FAILURE );
    }

    error = sm_process_main( argc, argv, envp );
    if( SM_OKAY != error )
    {
        printf( "Process failure, error=%s.\n", sm_error_str( error ) );
        return( EXIT_FAILURE );
    }

    error = sm_debug_finalize();
    if( SM_OKAY != error )
    {
        printf( "Debug finalization failed, error=%s.\n",
                sm_error_str( error ) );
    }

    error = sm_trap_finalize();
    if( SM_OKAY != error )
    {
        printf( "Trap handling finalization failed, error=%s.\n",
                sm_error_str( error ) );
    }

    return( EXIT_SUCCESS );
}
// ****************************************************************************
