//
// Copyright (c) 2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_CONFIGURATION_TABLE_H__
#define __SM_CONFIGURATION_TABLE_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_limits.h"
#include "sm_types.h"

#ifdef __cplusplus
extern "C" { 
#endif

// ****************************************************************************
// Configuration Table - Get
// ==================================
extern SmErrorT sm_configuration_table_get( const char* key, char* buf, unsigned int buf_size );
// ****************************************************************************

// ****************************************************************************
// Configuration Table - Initialize
// ========================================
extern SmErrorT sm_configuration_table_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Configuration Table - Finalize
// ======================================
extern SmErrorT sm_configuration_table_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_CONFIGURATION_TABLE_H__
