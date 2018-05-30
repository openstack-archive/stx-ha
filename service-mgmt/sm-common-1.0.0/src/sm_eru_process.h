//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_ERU_PROCESS_H__
#define __SM_ERU_PROCESS_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Event Recorder Unit Process - Main
// ==================================
extern SmErrorT sm_eru_process_main( int argc, char *argv[], char *envp[] );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_ERU_PROCESS_H__
