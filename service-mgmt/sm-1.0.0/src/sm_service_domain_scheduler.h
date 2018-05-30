//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_SCHEDULER_H__
#define __SM_SERVICE_DOMAIN_SCHEDULER_H__

#include <stdbool.h>

#include "sm_types.h"
#include "sm_uuid.h"
#include "sm_db.h"
#include "sm_service_domain_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Domain Scheduler - Swact Node
// =====================================
extern SmErrorT sm_service_domain_scheduler_swact_node( char node_name[],
    bool force, SmUuidT request_uuid );
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Schedule Service Domain
// ==================================================
extern void sm_service_domain_scheduler_schedule_service_domain(
    SmServiceDomainT* domain );
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Initialize
// =====================================
extern SmErrorT sm_service_domain_scheduler_initialize(
    SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Service Domain Scheduler - Finalize
// ===================================
extern SmErrorT sm_service_domain_scheduler_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_SCHEDULER_H__
