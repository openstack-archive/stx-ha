//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_FOREACH_H__
#define __SM_DB_FOREACH_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef SmErrorT (SmDbForEachRecordConverterT) 
    ( const char* col_name, const char* col_data, void* record );

typedef SmErrorT (*SmDbForEachRecordCallbackT) 
    (void* user_data[], void* record);

// ****************************************************************************
// Database For-Each
// =================
extern SmErrorT sm_db_foreach( const char* db_name, const char* db_table,
        const char* db_query, void* record_storage,
        SmDbForEachRecordConverterT record_converter,
        SmDbForEachRecordCallbackT record_callback, void* user_data[] );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_FOREACH_H__
