//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_AUDIT_H__
#define __SM_SERVICE_GROUP_AUDIT_H__

#include <stdbool.h>
#include <stdint.h>

#include "sm_types.h"
#include "sm_service_group_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Group Audit - Status
// ============================
extern SmErrorT sm_service_group_audit_status( SmServiceGroupT* service_group );
// ****************************************************************************

// ****************************************************************************
// Service Group Audit - Initialize
// ================================
extern SmErrorT sm_service_group_audit_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Audit - Finalize
// ==============================
extern SmErrorT sm_service_group_audit_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_AUDIT_H__
