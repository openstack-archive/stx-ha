//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_H__
#define __SM_DB_H__

#include <stdbool.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void SmDbHandleT;
typedef void SmDbStatementT;

// ****************************************************************************
// Database - Statement Result Reset
// =================================
extern SmErrorT sm_db_statement_result_reset( SmDbStatementT* sm_db_statement );
// ****************************************************************************

// ****************************************************************************
// Database - Statement Result Step
// =================================
extern SmErrorT sm_db_statement_result_step( SmDbStatementT* sm_db_statement,
    bool* done );
// ****************************************************************************

// ****************************************************************************
// Database - Transaction Start
// ============================
extern SmErrorT sm_db_transaction_start( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database - Transaction End
// ==========================
extern SmErrorT sm_db_transaction_end( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database - Statement Initialize
// ===============================
extern SmErrorT sm_db_statement_initialize( SmDbHandleT* sm_db_handle,
    const char* sql_statement, SmDbStatementT** sm_db_statement );
// ****************************************************************************

// ****************************************************************************
// Database - Statement Finalize
// =============================
extern SmErrorT sm_db_statement_finalize( SmDbStatementT* sm_db_statement );
// ****************************************************************************

// ****************************************************************************
// Database - Connect
// ==================
extern SmErrorT sm_db_connect( const char* sm_db_name,
    SmDbHandleT** sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database - Disconnect
// =====================
extern SmErrorT sm_db_disconnect( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Database - Configure
// ====================
extern SmErrorT sm_db_configure( const char* sm_db_name, SmDbTypeT sm_db_type );
// ****************************************************************************

// ****************************************************************************
// Database - Build
// ================
extern SmErrorT sm_db_build( const char* sm_db_name, SmDbTypeT sm_db_type );
// ****************************************************************************

// ****************************************************************************
// Database - Integrity Check
// ==========================
extern SmErrorT sm_db_integrity_check( const char* sm_db_name,
    bool* check_passed );
// ****************************************************************************

// ****************************************************************************
// Database - Checkpoint
// =====================
extern SmErrorT sm_db_checkpoint( const char* sm_db_name );
// ****************************************************************************

// ****************************************************************************
// Database - Initialize
// =====================
extern SmErrorT sm_db_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Database - Finalize
// ===================
extern SmErrorT sm_db_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_H__
