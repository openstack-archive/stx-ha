//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_health.h"

#include <stdio.h>
#include <stdint.h>

#include "sm_types.h"
#include "sm_debug.h"

// ****************************************************************************
// Service Group Health - Calculate
// ================================
int64_t sm_service_group_health_calculate( int failed, int degraded, int warn )
{
    int64_t health;

    health = ((failed & 0xFFFFL) << 32) | ((degraded & 0xFFFFL) << 16)
           | (warn & 0xFFFFL);

    health = ~health & 0x0000FFFFFFFFFFFFL;

    return( health );
}
// ****************************************************************************
