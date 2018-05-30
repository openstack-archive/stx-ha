//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_HEALTH_H__
#define __SM_SERVICE_GROUP_HEALTH_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Group Health - Calculate
// ================================
extern int64_t sm_service_group_health_calculate( int failed, int degraded,
    int warn );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_HEALTH_H__
