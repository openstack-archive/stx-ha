//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_UUID_H__
#define __SM_UUID_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_UUID_MAX_CHAR    42

typedef char SmUuidT[SM_UUID_MAX_CHAR];
typedef char* SmUuidPtrT;

// ****************************************************************************
// Universally Unique Identifier - Create
// ======================================
extern void sm_uuid_create( SmUuidT uuid );
// ****************************************************************************

// ****************************************************************************
// Universally Unique Identifier - Initialize
// ==========================================
extern SmErrorT sm_uuid_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Universally Unique Identifier - Finalize
// ========================================
extern SmErrorT sm_uuid_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_UUID_H__
