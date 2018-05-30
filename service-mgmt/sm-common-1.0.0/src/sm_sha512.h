//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SHA512_H__
#define __SM_SHA512_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint64_t length;
    uint64_t state[8];
    uint32_t current_length;
    uint8_t buffer[128];
} SmSha512ContextT;

#define SM_SHA512_HASH_SIZE 64
#define SM_SHA512_HASH_STR_SIZE ((SM_SHA512_HASH_SIZE*2)+1)

typedef struct
{
    uint8_t bytes[SM_SHA512_HASH_SIZE];
} SmSha512HashT;

// ****************************************************************************
// SHA512 - Initialize 
// ===================
extern void sm_sha512_initialize( SmSha512ContextT* context );
// ****************************************************************************

// ****************************************************************************
// SHA512 - Update 
// ===============
extern void sm_sha512_update( SmSha512ContextT* context, void* buffer,
    uint32_t buffer_size );
// ****************************************************************************

// ****************************************************************************
// SHA512 - Finalize 
// =================
extern void sm_sha512_finalize( SmSha512ContextT* context, SmSha512HashT* digest );
// ****************************************************************************

// ****************************************************************************
// SHA512 - Hash String
// ====================
extern void sm_sha512_hash_str( char hash_str[], SmSha512HashT* hash );
// ****************************************************************************

// ****************************************************************************
// SHA512 - HMAC
// =============
extern void sm_sha512_hmac( void* buffer, uint32_t buffer_size, 
    void* key, uint32_t key_size, SmSha512HashT* digest );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SHA512_H__
