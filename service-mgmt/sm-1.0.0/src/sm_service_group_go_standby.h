//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_GO_STANDBY_H__
#define __SM_SERVICE_GROUP_GO_STANDBY_H__

#include "sm_types.h"
#include "sm_service_group_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Group Go-Standby
// ========================
extern SmErrorT sm_service_group_go_standby( SmServiceGroupT* service_group );
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Standby - Complete
// ===================================
extern SmErrorT sm_service_group_go_standby_complete(
    SmServiceGroupT* service_group, bool* complete );
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Standby - Initialize
// =====================================
extern SmErrorT sm_service_group_go_standby_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Go-Standby - Finalize
// =====================================
extern SmErrorT sm_service_group_go_standby_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_GO_STANDBY_H__
