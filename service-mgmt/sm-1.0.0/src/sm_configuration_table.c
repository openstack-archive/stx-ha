//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_configuration_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_db_foreach.h"
#include "sm_db_configuration.h"
#include "sm_debug.h"


// ****************************************************************************
// count number of records found
// ==================================
static SmErrorT sm_configuration_table_add(void* user_data[], void* record)
{
    int& count = *(int*)user_data[0];
    count ++;
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Configuration Table - Get
// ==================================
SmErrorT sm_configuration_table_get( const char* key, char* buf, unsigned int buf_size )
{
    char db_query[SM_DB_QUERY_STATEMENT_MAX_CHAR];
    SmDbConfigurationT value;
    SmErrorT error;
    int count = 0;
    void* user_data[1] = {&count};
    unsigned int cx;

    cx = snprintf( db_query, sizeof(db_query), "%s = '%s'",
              SM_CONFIGURATION_TABLE_COLUMN_KEY, key );

    if( sizeof(db_query) <= cx )
    {
        DPRINTFE( "Unexpected query too long %s ='%s', length %d",
            SM_CONFIGURATION_TABLE_COLUMN_KEY, key, cx
         );
         buf[0] = '\0';
         return SM_FAILED;
    }

    error = sm_db_foreach( SM_DATABASE_NAME,
                           SM_CONFIGURATION_TABLE_NAME, db_query,
                           &value, sm_db_configuration_convert,
                           sm_configuration_table_add, user_data );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to loop over configuration in "
                  "database, error=%s.", sm_error_str( error ) );
        return( error );
    }

    if( 0 != count )
    {
        snprintf( buf, buf_size, "%s", value.value );
    }else
    {
        buf[0] = '\0';
    }

    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Configuration Table - Initialize
// ========================================
SmErrorT sm_configuration_table_initialize( void )
{
    return SM_OKAY;
}
// ****************************************************************************

// ****************************************************************************
// Configuration Table - Finalize
// ======================================
SmErrorT sm_configuration_table_finalize( void )
{
    return SM_OKAY;
}
// ****************************************************************************


