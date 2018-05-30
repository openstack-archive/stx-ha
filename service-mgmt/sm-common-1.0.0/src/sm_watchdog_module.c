//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_watchdog_module.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gmodule.h>

#include "sm_types.h"
#include "sm_list.h"
#include "sm_timer.h"
#include "sm_debug.h"

#define SM_WATCHDOG_MODULE_FILENAME_MAX_SIZE    128
#define SM_WATCHDOG_MODULE_PATH                 "/var/lib/sm/watchdog/modules"
#define SM_WATCHDOG_MODULE_DO_CHECK_FUNC        "sm_watchdog_module_do_check"
#define SM_WATCHDOG_MODULE_INITIALIZE_FUNC      "sm_watchdog_module_initialize"
#define SM_WATCHDOG_MODULE_FINALIZE_FUNC        "sm_watchdog_module_finalize"

typedef void (*SmWatchdogModuleDoCheckT)    (void);
typedef bool (*SmWatchdogModuleInitializeT) (int* do_check_in_ms);
typedef bool (*SmWatchdogModuleFinalizeT)   (void);

typedef struct
{
    gchar filename[SM_WATCHDOG_MODULE_FILENAME_MAX_SIZE];
    GModule* glibmod;
    int do_check_in_ms;
    SmTimerIdT do_check_timer_id;    
    SmWatchdogModuleDoCheckT do_check;
    SmWatchdogModuleInitializeT initialize;
    SmWatchdogModuleFinalizeT finalize;
} SmWatchdogModuleT;

static SmListT* _modules = NULL;

// ****************************************************************************
// Watchdog Module - Do Check Timer
// ================================
static bool sm_watchdog_module_do_check_timer( SmTimerIdT timer_id,
    int64_t user_data )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmWatchdogModuleT* module = NULL;

    SM_LIST_FOREACH( _modules, entry, entry_data )
    {
        module = (SmWatchdogModuleT*) entry_data;
        if( NULL == module )
        {
            continue;
        }
        
        if( timer_id == module->do_check_timer_id )
        {
            DPRINTFD( "Found do-check timer for module (%s).",
                      g_module_name(module->glibmod) );
            break;
        }
    }
    
    if( NULL != module )
    {
        if( NULL != module->do_check )
        {
            DPRINTFD( "Calling do-check for module (%s).",
                      g_module_name(module->glibmod) );
            module->do_check();
            return( true );
        }
    } else {
        DPRINTFE( "Module not found for do-check timer." );
    }

    return( false );
}
// ****************************************************************************

// ***************************************************************************
// Watchdog Module - Load
// ======================
static SmErrorT sm_watchdog_module_load( const gchar* filename )
{
    gchar* filepath;
    SmWatchdogModuleT* module;

    module = (SmWatchdogModuleT*) malloc( sizeof(SmWatchdogModuleT) );
    if( NULL == module )
    {
        DPRINTFE( "Failed to allocate watchdog module." );
        return( SM_FAILED );
    }

    memset( module, 0, sizeof(SmWatchdogModuleT) );

    g_snprintf(module->filename, SM_WATCHDOG_MODULE_FILENAME_MAX_SIZE,
               "%s", filename);

    filepath = g_module_build_path( SM_WATCHDOG_MODULE_PATH, filename );
        
    module->glibmod = g_module_open( filepath, G_MODULE_BIND_LAZY );
    if( NULL == module->glibmod )
    {
        DPRINTFE( "Failed to open module (%s).", filepath );
        free( module );
        g_free( filepath );
        return( SM_FAILED );
    }

    g_free( filepath );

    g_module_symbol( module->glibmod, SM_WATCHDOG_MODULE_INITIALIZE_FUNC,
                     (gpointer*) &(module->initialize) );

    g_module_symbol( module->glibmod, SM_WATCHDOG_MODULE_FINALIZE_FUNC,
                     (gpointer*) &(module->finalize) );

    g_module_symbol( module->glibmod, SM_WATCHDOG_MODULE_DO_CHECK_FUNC,
                     (gpointer*) &(module->do_check) );

    SM_LIST_PREPEND( _modules, (SmListEntryDataPtrT) module );

    return( SM_OKAY );
}
// ***************************************************************************

// ***************************************************************************
// Watchdog Module - Load All
// ==========================
SmErrorT sm_watchdog_module_load_all( void )
{
    const gchar* file;
    GDir* directory;
    GError* g_error;
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmWatchdogModuleT* module;
    SmErrorT error;

    directory = g_dir_open( SM_WATCHDOG_MODULE_PATH, 0, &g_error );
    if( NULL == directory )
    {
        DPRINTFE( "Failed to open directory( %s), error=%s",
                  SM_WATCHDOG_MODULE_PATH, g_error->message );
        g_error_free( g_error );
        return( SM_FAILED );
    }

    file = g_dir_read_name( directory );
    while( NULL != file )
    {
        DPRINTFI( "Loading module (%s).", file );

        error = sm_watchdog_module_load( file );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to load module (%s), error=%s.",
                      file, sm_error_str(error) );
        }

        file = g_dir_read_name( directory );
    }

    g_dir_close( directory );

    SM_LIST_FOREACH( _modules, entry, entry_data )
    {
        module = (SmWatchdogModuleT*) entry_data;
        if( NULL == module )
        {
            continue;
        }

        if( NULL != module->initialize  )
        {
            DPRINTFI( "Initializing module (%s).",
                      g_module_name(module->glibmod) );

            if( !(module->initialize( &(module->do_check_in_ms) )) )
            {
                DPRINTFE( "Failed to initialize %s.",
                          g_module_name(module->glibmod) );
                return( SM_FAILED );
            }

            error = sm_timer_register( module->filename,
                                       module->do_check_in_ms,
                                       sm_watchdog_module_do_check_timer,
                                       0, &(module->do_check_timer_id) );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to create module (%s) do-check timer, "
                          "error=%s.", g_module_name(module->glibmod),
                          sm_error_str( error ) );
                return( error );
            }
        }
    }

    return( SM_OKAY );
}
// ***************************************************************************

// ***************************************************************************
// Watchdog Module - Unload All
// ============================
SmErrorT sm_watchdog_module_unload_all( void )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmWatchdogModuleT* module;

    SM_LIST_FOREACH( _modules, entry, entry_data )
    {
        module = (SmWatchdogModuleT*) entry_data;
        if( NULL == module )
        {
            continue;
        }

        if( NULL != module->finalize  )
        {
            DPRINTFI( "Finalizing module (%s).",
                      g_module_name(module->glibmod) );

            if( !(module->finalize()) )
            {
                DPRINTFE( "Failed to finalize %s.",
                          g_module_name(module->glibmod) );
            }
        }

        g_module_close( module->glibmod );
    }

    SM_LIST_CLEANUP_ALL( _modules );

    return( SM_OKAY );
}
// ***************************************************************************
