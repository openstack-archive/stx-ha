//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_GO_ACTIVE_H__
#define __SM_SERVICE_GROUP_GO_ACTIVE_H__

#include "sm_types.h"
#include "sm_service_group_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Group Go-Active
// =======================
extern SmErrorT sm_service_group_go_active( SmServiceGroupT* service_group );
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active - Complete
// ==================================
extern SmErrorT sm_service_group_go_active_complete(
    SmServiceGroupT* service_group, bool* complete );
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active - Initialize
// ====================================
extern SmErrorT sm_service_group_go_active_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Active - Finalize
// ==================================
extern SmErrorT sm_service_group_go_active_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_GO_ACTIVE_H__
