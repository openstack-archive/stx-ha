//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_DISABLE_H__
#define __SM_SERVICE_GROUP_DISABLE_H__

#include "sm_types.h"
#include "sm_service_group_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Group Disable
// =====================
extern SmErrorT sm_service_group_disable( SmServiceGroupT* service_group );
// ****************************************************************************

// ****************************************************************************
// Service Group Disable - Complete
// ================================
extern SmErrorT sm_service_group_disable_complete( 
    SmServiceGroupT* service_group, bool* complete );
// ****************************************************************************

// ****************************************************************************
// Service Group Disable - Initialize
// ==================================
extern SmErrorT sm_service_group_disable_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Disable - Finalize
// ================================
extern SmErrorT sm_service_group_disable_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_DISABLE_H__
