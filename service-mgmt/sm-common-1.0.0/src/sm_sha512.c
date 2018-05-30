//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_sha512.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROR64(value, bits) (((value) >> (bits)) | ((value) << (64 - (bits))))

#define MIN(x, y) (((x)<(y))?(x):(y))

#define LOAD64H(x, y)                                                        \
   { x = (((uint64_t)((y)[0] & 255))<<56)|(((uint64_t)((y)[1] & 255))<<48) | \
         (((uint64_t)((y)[2] & 255))<<40)|(((uint64_t)((y)[3] & 255))<<32) | \
         (((uint64_t)((y)[4] & 255))<<24)|(((uint64_t)((y)[5] & 255))<<16) | \
         (((uint64_t)((y)[6] & 255))<<8)|(((uint64_t)((y)[7] & 255))); }

#define STORE64H(x, y)                                                       \
   { (y)[0] = (uint8_t)(((x)>>56)&255); (y)[1] = (uint8_t)(((x)>>48)&255);   \
     (y)[2] = (uint8_t)(((x)>>40)&255); (y)[3] = (uint8_t)(((x)>>32)&255);   \
     (y)[4] = (uint8_t)(((x)>>24)&255); (y)[5] = (uint8_t)(((x)>>16)&255);   \
     (y)[6] = (uint8_t)(((x)>>8)&255); (y)[7] = (uint8_t)((x)&255); }

// Various logical functions
#define CH( x, y, z )     (z ^ (x & (y ^ z)))
#define MAJ(x, y, z )     (((x | y) & z) | (x & y))
#define S( x, n )         ROR64( x, n )
#define R( x, n )         (((x)&0xFFFFFFFFFFFFFFFFULL)>>((uint64_t)n))
#define SIGMA0( x )       (S(x, 28) ^ S(x, 34) ^ S(x, 39))
#define SIGMA1( x )       (S(x, 14) ^ S(x, 18) ^ S(x, 41))
#define GAMMA0( x )       (S(x, 1) ^ S(x, 8) ^ R(x, 7))
#define GAMMA1( x )       (S(x, 19) ^ S(x, 61) ^ R(x, 6))

#define SHA512_ROUND( a, b, c, d, e, f, g, h, i )      \
     t0 = h + SIGMA1(e) + CH(e, f, g) + K[i] + W[i];   \
     t1 = SIGMA0(a) + MAJ(a, b, c);                    \
     d += t0;                                          \
     h  = t0 + t1;

#define BLOCK_SIZE          128

// The K array
static const uint64_t K[80] = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL
};

// ****************************************************************************
// SHA512 - Transform Function
// ===========================
static void sm_sha512_transform_function( SmSha512ContextT* context,
    uint8_t* buffer )
{
    uint64_t S[8];
    uint64_t W[80];
    uint64_t t0;
    uint64_t t1;
    int i;

    // Copy state into S.
    for( i=0; 8 > i; ++i )
    {
        S[i] = context->state[i];
    }

    // Copy the state into 1024-bits into W[0..15].
    for( i=0; 16 > i; ++i )
    {
        LOAD64H(W[i], buffer + (8*i));
    }

    // Fill W[16..79].
    for( i=16; 80 > i; ++i )
    {
        W[i] = GAMMA1(W[i - 2]) + W[i - 7] + GAMMA0(W[i - 15]) + W[i - 16];
    }

    // Compress.
     for( i=0; 80 > i; i+=8 )
     {
         SHA512_ROUND(S[0],S[1],S[2],S[3],S[4],S[5],S[6],S[7],i+0);
         SHA512_ROUND(S[7],S[0],S[1],S[2],S[3],S[4],S[5],S[6],i+1);
         SHA512_ROUND(S[6],S[7],S[0],S[1],S[2],S[3],S[4],S[5],i+2);
         SHA512_ROUND(S[5],S[6],S[7],S[0],S[1],S[2],S[3],S[4],i+3);
         SHA512_ROUND(S[4],S[5],S[6],S[7],S[0],S[1],S[2],S[3],i+4);
         SHA512_ROUND(S[3],S[4],S[5],S[6],S[7],S[0],S[1],S[2],i+5);
         SHA512_ROUND(S[2],S[3],S[4],S[5],S[6],S[7],S[0],S[1],i+6);
         SHA512_ROUND(S[1],S[2],S[3],S[4],S[5],S[6],S[7],S[0],i+7);
     }

    // Feedback.
    for( i=0; 8 > i; ++i )
    {
        context->state[i] = context->state[i] + S[i];
    }
}
// ****************************************************************************

// ****************************************************************************
// SHA512 - Initialize 
// ===================
void sm_sha512_initialize( SmSha512ContextT* context )
{
    context->current_length = 0;
    context->length = 0;
    context->state[0] = 0x6a09e667f3bcc908ULL;
    context->state[1] = 0xbb67ae8584caa73bULL;
    context->state[2] = 0x3c6ef372fe94f82bULL;
    context->state[3] = 0xa54ff53a5f1d36f1ULL;
    context->state[4] = 0x510e527fade682d1ULL;
    context->state[5] = 0x9b05688c2b3e6c1fULL;
    context->state[6] = 0x1f83d9abfb41bd6bULL;
    context->state[7] = 0x5be0cd19137e2179ULL;
}
// ****************************************************************************

// ****************************************************************************
// SHA512 - Update 
// ===============
void sm_sha512_update( SmSha512ContextT* context, void* buffer,
    uint32_t buffer_size )
{
    uint32_t n;

    if( context->current_length > sizeof(context->buffer) )
    {
       return;
    }

    while( 0 < buffer_size )
    {
        if(( 0 == context->current_length )&&( BLOCK_SIZE <= buffer_size ))
        {
           sm_sha512_transform_function( context, (uint8_t*) buffer );
           context->length += BLOCK_SIZE * 8;
           buffer = (uint8_t*) buffer + BLOCK_SIZE;
           buffer_size -= BLOCK_SIZE;
        } else {
           n = MIN( buffer_size, (BLOCK_SIZE - context->current_length) );
           memcpy( context->buffer + context->current_length, buffer,
                   (size_t) n );
           context->current_length += n;
           buffer = (uint8_t*) buffer + n;
           buffer_size -= n;
           if( context->current_length == BLOCK_SIZE )
           {
              sm_sha512_transform_function( context, context->buffer );
              context->length += (8 * BLOCK_SIZE);
              context->current_length = 0;
           }
       }
    }
}
// ****************************************************************************

// ****************************************************************************
// SHA512 - Finalize 
// =================
void sm_sha512_finalize( SmSha512ContextT* context, SmSha512HashT* hash )
{
    if( context->current_length >= sizeof(context->buffer) )
    {
       return;
    }

    // Increase the length of the message.
    context->length += context->current_length * 8ULL;

    // Append the '1' bit.
    context->buffer[(context->current_length)++] = (uint8_t) 0x80;

    // If the length is currently above 112 bytes we append zeros
    // then compress.  Then we can fall back to padding zeros and length
    // encoding like normal.
    if( 112 < context->current_length )
    {
        while( 128 > context->current_length )
        {
            context->buffer[(context->current_length)++] = (uint8_t) 0;
        }
        sm_sha512_transform_function( context, context->buffer );
        context->current_length = 0;
    }

    // Pad up to 120 bytes of zeros.
    // Note: that from 112 to 120 is the 64 MSB of the length.  
    // Assume that you won't hash > 2^64 bits of data...
    while( 120 > context->current_length )
    {
        context->buffer[(context->current_length)++] = (uint8_t) 0;
    }

    // Store length.
    STORE64H( context->length, context->buffer+120 );
    sm_sha512_transform_function( context, context->buffer );

    // Copy output.
    int i;
    for( i=0; 8 > i; ++i )
    {
        STORE64H( context->state[i], hash->bytes+(8*i) );
    }
}
// ****************************************************************************

// ****************************************************************************
// SHA512 - Hash String
// ====================
void sm_sha512_hash_str( char hash_str[], SmSha512HashT* hash )
{
    int i;
    for( i=0; SM_SHA512_HASH_SIZE > i; ++i )
    {
        snprintf( hash_str+(i*2), SM_SHA512_HASH_STR_SIZE-(i*2), "%02x",
                  hash->bytes[i] );
    }
}
// ****************************************************************************

// ****************************************************************************
// SHA512 - HMAC
// =============
void sm_sha512_hmac( void* buffer, uint32_t buffer_size, void* key,
    uint32_t key_size, SmSha512HashT* hash )
{
    int byte_i;
    SmSha512ContextT context;
    uint8_t key_ipad[BLOCK_SIZE];
    uint8_t key_opad[BLOCK_SIZE];
    SmSha512HashT key_hash;

    if( BLOCK_SIZE < key_size )
    {
        SmSha512ContextT key_context;
        sm_sha512_initialize( &key_context );
        sm_sha512_update( &key_context, key, key_size );
        sm_sha512_finalize( &key_context, &key_hash );
        key = &(key_hash.bytes[0]);
        key_size = SM_SHA512_HASH_SIZE;
    }

    // First Pass (inner hash).
    memset( key_ipad, 0, sizeof(key_ipad) );
    memcpy( key_ipad, key, key_size ); 

    for( byte_i=0; BLOCK_SIZE > byte_i; ++byte_i )
        key_ipad[byte_i] ^= 0x36;

    sm_sha512_initialize( &context );
    sm_sha512_update( &context, key_ipad, sizeof(key_ipad) );
    sm_sha512_update( &context, buffer, buffer_size );
    sm_sha512_finalize( &context, hash );

    // Second Pass (outer hash).
    memset( key_opad, 0, sizeof(key_opad) );
    memcpy( key_opad, key, key_size ); 

    for( byte_i=0; BLOCK_SIZE > byte_i; ++byte_i )
        key_opad[byte_i] ^= 0x5C;

    sm_sha512_initialize( &context );
    sm_sha512_update( &context, key_opad, sizeof(key_opad) );
    sm_sha512_update( &context,  &(hash->bytes[0]), SM_SHA512_HASH_SIZE );
    sm_sha512_finalize( &context, hash );
}
// ****************************************************************************
