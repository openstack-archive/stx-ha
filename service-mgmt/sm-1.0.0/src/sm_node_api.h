//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_NODE_API_H__
#define __SM_NODE_API_H__

#include <stdbool.h>

#include "sm_types.h"
#include "sm_uuid.h"
#include "sm_limits.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Node API - Get Host Name
// ========================
extern SmErrorT sm_node_api_get_hostname( char node_name[] );
// ****************************************************************************

// ****************************************************************************
// Node API - Get Peer Name
// ========================
extern SmErrorT sm_node_api_get_peername(char peer_name[SM_NODE_NAME_MAX_CHAR]);
// ****************************************************************************

// ****************************************************************************
// Node API - Configuration Complete
// =================================
extern SmErrorT sm_node_api_config_complete( char node_name[], bool* complete );
// ****************************************************************************

// ****************************************************************************
// Node API - Add Node
// ===================
extern SmErrorT sm_node_api_add_node( char node_name[] );
// ****************************************************************************

// ****************************************************************************
// Node API - Update Node
// ======================
extern SmErrorT sm_node_api_update_node( char node_name[], 
    SmNodeAdminStateT admin_state, SmNodeOperationalStateT oper_state,
    SmNodeAvailStatusT avail_status );
// ****************************************************************************

// ****************************************************************************
// Node API - Fail Node
// ======================
SmErrorT sm_node_api_fail_node( char node_name[] );
// ****************************************************************************

// ****************************************************************************
// Node API - Delete Node
// ======================
extern SmErrorT sm_node_api_delete_node( char node_name[] );
// ****************************************************************************

// ****************************************************************************
// Node API - Swact
// ================
extern SmErrorT sm_node_api_swact( char node_name[], bool force );
// ****************************************************************************

// ****************************************************************************
// Node API - Reboot
// =================
extern SmErrorT sm_node_api_reboot( char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Node API - Audit
// ================
extern SmErrorT sm_node_api_audit( void );
// ****************************************************************************

// ****************************************************************************
// Node API - Initialize 
// =====================
extern SmErrorT sm_node_api_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Node API - Finalize 
// ===================
extern SmErrorT sm_node_api_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_NODE_API_H__
