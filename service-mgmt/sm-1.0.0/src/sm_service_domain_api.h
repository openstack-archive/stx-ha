//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_API_H__
#define __SM_SERVICE_DOMAIN_API_H__

#include <stdbool.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Domain API - Interface Enabled
// ======================================
extern SmErrorT sm_service_domain_api_interface_enabled( 
    char service_domain_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain API - Interface Disabled
// =======================================
extern SmErrorT sm_service_domain_api_interface_disabled( 
    char service_domain_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain API - Pause All
// ==============================
extern void sm_service_domain_api_pause_all( int pause_interval );
// ****************************************************************************

// ****************************************************************************
// Service Domain API - Initialize
// ===============================
extern SmErrorT sm_service_domain_api_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Domain API - Finalize 
// =============================
extern SmErrorT sm_service_domain_api_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_API_H__
