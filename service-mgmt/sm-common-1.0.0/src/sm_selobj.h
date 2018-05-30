//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
// The following module is thread safe.
//
#ifndef __SM_SELOBJ_H__
#define __SM_SELOBJ_H__

#include <stdint.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmSelObjCallbackT) (int selobj, int64_t user_data);

// ****************************************************************************
// Selection Object - Register
// ===========================
extern SmErrorT sm_selobj_register( int selobj, SmSelObjCallbackT callback,
    int64_t user_data );
// ****************************************************************************

// ****************************************************************************
// Selection Object - Deregister
// =============================
extern SmErrorT sm_selobj_deregister( int selobj );
// ****************************************************************************

// ****************************************************************************
// Selection Object - Dispatch
// ===========================
extern SmErrorT sm_selobj_dispatch( unsigned int timeout_in_ms );
// ****************************************************************************

// ****************************************************************************
// Selection Object - Initialize
// =============================
extern SmErrorT sm_selobj_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Selection Object - Finalize
// ===========================
extern SmErrorT sm_selobj_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SELOBJ_H__
