//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_db_configuration.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_db.h"

// ****************************************************************************
// Database Configuration - Convert
// =========================================
SmErrorT sm_db_configuration_convert( const char* col_name,
    const char* col_data, void* data )
{
    SmDbConfigurationT* record = (SmDbConfigurationT*) data;

    DPRINTFD( "%s = %s", col_name, col_data ? col_data : "NULL" );

    if( 0 ==  strcmp( SM_CONFIGURATION_TABLE_COLUMN_KEY,
                      col_name ) )
    {
        strncpy( record->key, col_data, SM_CONFIGURATION_KEY_MAX_CHAR );
    }
    else if( 0 == strcmp( SM_CONFIGURATION_TABLE_COLUMN_VALUE,
                          col_name ) )
    {
        strncpy( record->value, col_data, SM_CONFIGURATION_VALUE_MAX_CHAR );
    }
    else if( 0 == strcmp( SM_CONFIGURATION_TABLE_COLUMN_ID,
                          col_name ) )
    {
        //don't care
    }
    else {
        DPRINTFE( "Unknown column (%s).", col_name );
    }

    return( SM_OKAY );
}

// ****************************************************************************

// ****************************************************************************
// Database Configuration - Initialize
// ============================================
SmErrorT sm_db_configuration_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Database Configuration - Finalize
// ==========================================
SmErrorT sm_db_configuration_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
