//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_LOG_H__
#define __SM_LOG_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Log - Node Reboot
// =================
extern void sm_log_node_reboot( char entity_name[], const char reason_text[],
    bool forced );
// ****************************************************************************

// ****************************************************************************
// Log - Node State Change
// =======================
extern void sm_log_node_state_change( char entity_name[], 
    const char prev_state[], const char state[], const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Log - Interface State Change
// ============================
extern void sm_log_interface_state_change( char entity_name[],
    const char prev_state[], const char state[], const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Log - Comunication State Change
// ===============================
extern void sm_log_communication_state_change( char entity_name[],
    const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Log - Neighbor State Change
// ===========================
extern void sm_log_neighbor_state_change( char entity_name[],
    const char prev_state[], const char state[], const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Log - Service Domain State Change
// =================================
extern void sm_log_service_domain_state_change( char entity_name[],
    const char prev_state[], const char state[], const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Log - Service Group Redundancy Change
// =====================================
extern void sm_log_service_group_redundancy_change( char entity_name[],
    const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Log - Service Group State Change
// ================================
extern void sm_log_service_group_state_change( char entity_name[],
    const char prev_state[], const char prev_status[], const char state[],
    const char status[], const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Log - Service State Change
// ==========================
extern void sm_log_service_state_change( char entity_name[],
    const char prev_state[], const char prev_status[], const char state[],
    const char status[], const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Log - Initialize
// ================
extern SmErrorT sm_log_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Log - Finalize
// ==============
extern SmErrorT sm_log_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_LOG_H__
