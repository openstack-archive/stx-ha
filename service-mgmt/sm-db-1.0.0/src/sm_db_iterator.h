//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_ITERATOR_H__
#define __SM_DB_ITERATOR_H__

#include <stdint.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint32_t valid;
    bool done;
    SmDbHandleT* sm_db_handle;
    SmDbStatementT* sm_db_statement;
} SmDbIteratorT;

typedef SmErrorT (SmDbIteratorRecordConverterT) ( const char* col_name, 
                                                  const char* col_data,
                                                  void* record );

// ****************************************************************************
// Database Iterator - First
// =========================
extern SmErrorT sm_db_iterator_first( SmDbIteratorT* it );
// ****************************************************************************

// ****************************************************************************
// Database Iterator - Get
// =======================
extern SmErrorT sm_db_iterator_get( SmDbIteratorT* it, void* record,
    SmDbIteratorRecordConverterT converter );
// ****************************************************************************

// ****************************************************************************
// Database Iterator - Next
// ========================
extern SmErrorT sm_db_iterator_next( SmDbIteratorT* it );
// ****************************************************************************

// ****************************************************************************
// Database Iterator - Initialize
// ==============================
extern SmErrorT sm_db_iterator_initialize( const char* db_name, 
    const char* db_table, const char* db_query, SmDbIteratorT* it );
// ****************************************************************************

// ****************************************************************************
// Database Iterator - Finalize
// ============================
extern SmErrorT sm_db_iterator_finalize( SmDbIteratorT* it );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_ITERATOR_H__
