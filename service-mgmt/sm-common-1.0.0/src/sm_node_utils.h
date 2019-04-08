//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_NODE_UTILS_H__
#define __SM_NODE_UTILS_H__

#include <stdbool.h>

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Node Utilities - Node Type Is Controller
// ========================================
extern SmErrorT sm_node_utils_node_type_is_controller( bool* is_controller );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - System Is AIO
// ========================================
extern SmErrorT sm_node_utils_is_aio( bool* is_aio );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Sub-Functions Has Compute
// ==========================================
extern SmErrorT sm_node_utils_sub_function_has_worker( bool* has_worker );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get Management Interface
// =========================================
extern SmErrorT sm_node_utils_get_mgmt_interface( char interface_name[] );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get OAM Interface
// ==================================
extern SmErrorT sm_node_utils_get_oam_interface( char interface_name[] );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get Cluster Host Interface
// ===========================================
extern SmErrorT sm_node_utils_get_cluster_host_interface( char interface_name[] );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get system mode
// =============================================
SmSystemModeT sm_node_utils_get_system_mode();
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get Uptime
// ===========================
extern SmErrorT sm_node_utils_get_uptime( long* uptime );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Get Host Name
// ==============================
extern SmErrorT sm_node_utils_get_hostname( char node_name[] );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Configuration Complete 
// =======================================
extern SmErrorT sm_node_utils_config_complete( bool* complete );
// ****************************************************************************
//
// ****************************************************************************
// Node Utilities - Enabled
// ========================
extern SmErrorT sm_node_utils_enabled( bool* enabled, char reason_text[] );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Set Unhealthy
// ==============================
extern SmErrorT sm_node_utils_set_unhealthy( void );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - System Is AIO Simplex
// =======================================
extern SmErrorT sm_node_utils_is_aio_simplex( bool* is_aio_simplex );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - System is AIO Duplex
// =====================================
extern SmErrorT sm_node_utils_is_aio_duplex( bool* is_aio_duplex );
// ****************************************************************************

// ****************************************************************************
// Node Utilities - Set Failover
// ==============================
extern bool sm_node_utils_set_failover( bool to_disable );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_NODE_UTILS_H__
