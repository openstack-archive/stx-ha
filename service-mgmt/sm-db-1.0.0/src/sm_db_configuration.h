//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_CONFIGURATION_H__
#define __SM_DB_CONFIGURATION_H__

#include <stdbool.h>

#include "sm_types.h"
#include "sm_limits.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_CONFIGURATION_TABLE_NAME                            "CONFIGURATION"
#define SM_CONFIGURATION_TABLE_COLUMN_ID                       "ID"
#define SM_CONFIGURATION_TABLE_COLUMN_KEY                      "KEY"
#define SM_CONFIGURATION_TABLE_COLUMN_VALUE                    "VALUE"

typedef struct
{
    char key[SM_CONFIGURATION_KEY_MAX_CHAR];
    char value[SM_CONFIGURATION_VALUE_MAX_CHAR];
} SmDbConfigurationT;

// ****************************************************************************
// Database Configuration - Convert
// =========================================
extern SmErrorT sm_db_configuration_convert( const char* col_name,
    const char* col_data, void* data );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Initialize
// ============================================
extern SmErrorT sm_db_service_domain_members_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database Service Domain Members - Finalize
// ==========================================
extern SmErrorT sm_db_service_domain_members_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_CONFIGURATION_H__
