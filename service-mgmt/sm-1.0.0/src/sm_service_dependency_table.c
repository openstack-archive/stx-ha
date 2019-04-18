//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_dependency_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_db.h"
#include "sm_db_foreach.h"
#include "sm_db_services.h"
#include "sm_db_service_dependency.h"

static SmListT* _service_dependency = NULL;
static SmDbHandleT* _sm_db_handle = NULL;

// ****************************************************************************
// Service Dependency Table - Read
// ===============================
SmServiceDependencyT* sm_service_dependency_table_read( 
    SmServiceDependencyTypeT type, char service_name[], SmServiceStateT state,
    SmServiceActionT action, char dependent[] )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDependencyT* service_dependency;

    SM_LIST_FOREACH( _service_dependency, entry, entry_data )
    {
        service_dependency = (SmServiceDependencyT*) entry_data;

        if(( type == service_dependency->type )&&
           ( 0 == strcmp( service_name, service_dependency->service_name ) )&&
           ( state == service_dependency->state )&&
           ( action == service_dependency->action )&&
           ( 0 == strcmp( dependent, service_dependency->dependent ) ))
        {
            return( service_dependency );
        }
    }

    return( NULL );    
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency Table - For Each
// ===================================
void sm_service_dependency_table_foreach( SmServiceDependencyTypeT type,
    char service_name[], SmServiceStateT state, SmServiceActionT action,
    void* user_data[], SmServiceDependencyTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDependencyT* service_dependency;

    SM_LIST_FOREACH( _service_dependency, entry, entry_data )
    {
        service_dependency = (SmServiceDependencyT*) entry_data;

        if(( type == service_dependency->type )&&
           ( 0 == strcmp( service_name, service_dependency->service_name ) )&&
           ( state == service_dependency->state )&&
           ( action == service_dependency->action ))
        {
            callback( user_data, service_dependency );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency Table - For Each Dependent
// =============================================
void sm_service_dependency_table_foreach_dependent(
    SmServiceDependencyTypeT type, char dependent[],
    SmServiceStateT dependent_state, void* user_data[],
    SmServiceDependencyTableForEachCallbackT callback )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDependencyT* service_dependency;

    SM_LIST_FOREACH( _service_dependency, entry, entry_data )
    {
        service_dependency = (SmServiceDependencyT*) entry_data;

        if(( type == service_dependency->type )&&
           ( 0 == strcmp( dependent, service_dependency->dependent ) )&&
           ( dependent_state == service_dependency->dependent_state ))
        {
            callback( user_data, service_dependency );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency Table - Add
// ==============================
static SmErrorT sm_service_dependency_table_add( void* user_data[], 
    void* record )
{
    SmDbServiceT db_service;
    SmDbServiceT db_dependent_service;
    SmServiceDependencyT* service_dependency;
    SmDbServiceDependencyT* db_service_dependency;
    SmErrorT error;

    db_service_dependency = (SmDbServiceDependencyT*) record;

    error = sm_db_services_read( _sm_db_handle,
                                 db_service_dependency->service_name,
                                 &db_service );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  db_service_dependency->service_name, sm_error_str( error ) );
        return( SM_FAILED );
    }

    error = sm_db_services_read( _sm_db_handle,
                                 db_service_dependency->dependent,
                                 &db_dependent_service );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to read dependent service (%s), error=%s.",
                  db_service_dependency->service_name, sm_error_str( error ) );
        return( SM_FAILED );
    }

    if( !(db_service.provisioned) || (!db_dependent_service.provisioned) )
    {
        return( SM_OKAY );
    }

    service_dependency = sm_service_dependency_table_read( 
                            db_service_dependency->type,
                            db_service_dependency->service_name,
                            db_service_dependency->state,
                            db_service_dependency->action,
                            db_service_dependency->dependent );
    if( NULL == service_dependency )
    {
        service_dependency 
            = (SmServiceDependencyT*) malloc( sizeof(SmServiceDependencyT) );
        if( NULL == service_dependency )
        {
            DPRINTFE( "Failed to allocate service dependency table entry." );
            return( SM_FAILED );
        }

        memset( service_dependency, 0, sizeof(SmServiceDependencyT) );

        service_dependency->type = db_service_dependency->type;
        snprintf( service_dependency->service_name, 
                  sizeof(service_dependency->service_name),
                  "%s", db_service_dependency->service_name );

        service_dependency->state = db_service_dependency->state;
        service_dependency->action = db_service_dependency->action;
        snprintf( service_dependency->dependent, 
                  sizeof(service_dependency->dependent),
                  "%s", db_service_dependency->dependent );
        service_dependency->dependent_state
            = db_service_dependency->dependent_state;

        SM_LIST_PREPEND( _service_dependency,
                         (SmListEntryDataPtrT) service_dependency );

    } else {
        service_dependency->dependent_state
            = db_service_dependency->dependent_state;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency Table - Load
// ===============================
SmErrorT sm_service_dependency_table_load( void )
{
    SmDbServiceDependencyT service_dependency;
    SmErrorT error;

    if( NULL != _service_dependency )
    {
        SM_LIST_CLEANUP_ALL( _service_dependency );
        _service_dependency = NULL;
    }

    error = sm_db_foreach( SM_DATABASE_NAME, SM_SERVICE_DEPENDENCY_TABLE_NAME,
                           NULL, &service_dependency,
                           sm_db_service_dependency_convert,
                           sm_service_dependency_table_add, NULL );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over service dependencies in database, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency Table - Initialize
// =====================================
SmErrorT sm_service_dependency_table_initialize( void )
{
    SmErrorT error;

    error = sm_db_connect( SM_DATABASE_NAME, &_sm_db_handle );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to connect to database (%s), error=%s.",
                  SM_DATABASE_NAME, sm_error_str( error ) );
        return( error );
    }

    error = sm_service_dependency_table_load();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to load service dependencies from database, "
                  "error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency Table - Finalize
// ===================================
SmErrorT sm_service_dependency_table_finalize( void )
{
    SmErrorT error;

    SM_LIST_CLEANUP_ALL( _service_dependency );

    if( NULL != _sm_db_handle )
    {
        error = sm_db_disconnect( _sm_db_handle );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to disconnect from database (%s), error=%s.",
                      SM_DATABASE_NAME, sm_error_str( error ) );
        }

        _sm_db_handle = NULL;
    }

    return( SM_OKAY );
}
// ****************************************************************************
