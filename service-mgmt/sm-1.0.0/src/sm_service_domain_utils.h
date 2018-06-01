//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_DOMAIN_UTILS_H__
#define __SM_SERVICE_DOMAIN_UTILS_H__

#include <stdbool.h>

#include "sm_types.h"
#include "sm_service_domain_neighbor_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Domain Utilities - Service Domain Enabled
// =================================================
extern SmErrorT sm_service_domain_utils_service_domain_enabled( char name[],
    bool* enabled  );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Hello
// =====================================
extern SmErrorT sm_service_domain_utils_send_hello( char name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Pause
// =====================================
extern SmErrorT sm_service_domain_utils_send_pause( char name[],
    int pause_interval );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Exchange Start throttled
// ==============================================
extern SmErrorT sm_service_domain_utils_send_exchange_start_throttle( char name[],
    SmServiceDomainNeighborT* neighbor, int exchange_seq, int min_interval_ms );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Exchange Start
// ==============================================
extern SmErrorT sm_service_domain_utils_send_exchange_start( char name[],
    SmServiceDomainNeighborT* neighbor, int exchange_seq );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Exchange
// ========================================
extern SmErrorT sm_service_domain_utils_send_exchange( char name[],
    SmServiceDomainNeighborT* neighbor, int exchange_seq, int64_t member_id,
    char member_name[], SmServiceGroupStateT member_desired_state,
    SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
    SmServiceGroupConditionT member_condition, int64_t member_health,
    const char reason_text[], bool more_members,
    int64_t last_received_member_id );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Member Request
// ==============================================
extern SmErrorT sm_service_domain_utils_send_member_request( char name[],
    char member_node_name[], int64_t member_id, char member_name[],
    SmServiceGroupActionT member_action,
    SmServiceGroupActionFlagsT member_action_flags );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Member Update
// =============================================
extern SmErrorT sm_service_domain_utils_send_member_update( char name[],
    char member_node_name[], int64_t member_id, char member_name[],
    SmServiceGroupStateT member_desired_state,
    SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
    SmServiceGroupConditionT member_condition, int64_t health,
    const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Hold Election
// ========================================
extern SmErrorT sm_service_domain_utils_hold_election( char name[],
    char leader[], bool* elected );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Service Domain Neighbor InSync
// =========================================================
extern SmErrorT sm_service_domain_utils_service_domain_neighbor_insync(
    char name[], char node_name[], bool* insync  );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Service Domain Neighbors InSync
// ==========================================================
extern SmErrorT sm_service_domain_utils_service_domain_neighbors_insync(
    char name[], bool* insync  );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Service Domain Neighbors Syncing
// ===========================================================
extern SmErrorT sm_service_domain_utils_service_domain_neighbors_syncing(
    char name[], bool* syncing );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Update Assignment
// ============================================
extern SmErrorT sm_service_domain_utils_update_assignment( char name[],
    char node_name[], char service_group_name[],
    SmServiceGroupStateT desired_state, SmServiceGroupStateT state,
    SmServiceGroupStatusT status, SmServiceGroupConditionT condition,
    int64_t health, const char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Update Own Assignments
// =================================================
extern SmErrorT sm_service_domain_utils_update_own_assignments( char name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Disable Self
// =======================================
extern SmErrorT sm_service_domain_utils_service_domain_disable_self( char name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - active Self
// =======================================
extern SmErrorT sm_service_domain_utils_service_domain_active_self( char name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Neighbor Cleanup
// ===========================================
extern SmErrorT sm_service_domain_utils_service_domain_neighbor_cleanup(
    char name[], char node_name[] );
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities is aa service group
// ==============================
extern bool sm_is_aa_service_group(char* service_group_name);
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Initialize
// =====================================
extern SmErrorT sm_service_domain_utils_initialize( void );

// ****************************************************************************
// Service Domain Utilities - Finalize
// ===================================
extern SmErrorT sm_service_domain_utils_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_DOMAIN_UTILS_H__
