//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_DB_SERVICE_GROUP_ENABLE_H__
#define __SM_DB_SERVICE_GROUP_ENABLE_H__

#include "sm_types.h"
#include "sm_service_group_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Group Enable
// ====================
extern SmErrorT sm_service_group_enable( SmServiceGroupT* service_group );
// ****************************************************************************

// ****************************************************************************
// Service Group Enable - Complete
// ===============================
extern SmErrorT sm_service_group_enable_complete(
    SmServiceGroupT* service_group, bool* complete );
// ****************************************************************************

// ****************************************************************************
// Service Group Enable - Initialize
// =================================
extern SmErrorT sm_service_group_enable_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Enable - Finalize
// ===============================
extern SmErrorT sm_service_group_enable_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_DB_SERVICE_GROUP_ENABLE_H__
