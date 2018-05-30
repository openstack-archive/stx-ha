//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_FILTER_H__
#define __SM_SERVICE_DOMAIN_FILTER_H__

#include "sm_types.h"
#include "sm_db.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int active_members;
    int go_active_members;
    int go_standby_members;
    int standby_members;
    int disabling_members;
    int disabled_members;
    int failed_members;
    int fatal_members;
    int unavailable_members;
    int total_members;
} SmServiceDomainFilterCountsT;

// ****************************************************************************
// Service Domain Filter - Preselect Apply
// =======================================
extern SmErrorT sm_service_domain_filter_preselect_apply(
    char service_domain_name[], SmServiceDomainFilterCountsT* counts );
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Post Select Apply
// =========================================
extern SmErrorT sm_service_domain_filter_post_select_apply(
    char service_domain_name[], SmServiceDomainFilterCountsT* counts );
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Initialize
// ==================================
extern SmErrorT sm_service_domain_filter_initialize( SmDbHandleT* sm_db_handle );
// ****************************************************************************

// ****************************************************************************
// Service Domain Filter - Finalize
// ================================
extern SmErrorT sm_service_domain_filter_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_FILTER_H__
