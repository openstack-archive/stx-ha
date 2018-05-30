//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_TROUBLESHOOT_H__
#define __SM_TROUBLESHOOT_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Troubleshoot - Dump Data
// ========================
extern SmErrorT sm_troubleshoot_dump_data( const char reason[] );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_TROUBLESHOOT_H__

