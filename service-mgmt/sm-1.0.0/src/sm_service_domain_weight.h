//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_WEIGHT_H__
#define __SM_SERVICE_DOMAIN_WEIGHT_H__

#include "sm_types.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Domain Weight - Apply
// =============================
extern SmErrorT sm_service_domain_weight_apply( char service_domain_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Weight - Initialize
// ==================================
extern SmErrorT sm_service_domain_weight_initialize( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Service Domain Weight - Finalize
// ================================
extern SmErrorT sm_service_domain_weight_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_WEIGHT_H__
