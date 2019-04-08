//
// Copyright (c) 2017 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_FAILOVER_H__
#define __SM_FAILOVER_H__
#include <stdio.h>
#include "sm_types.h"
#include "sm_service_domain_interface_table.h"
#include "sm_db_nodes.h"
#include "sm_failover_ss.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    char* service_domain;
    char* service_domain_interface;
    char* interface_name;
    SmInterfaceStateT interface_state;
}SmFailoverInterfaceT;


typedef enum
{
    SM_FAILOVER_DEGRADE_SOURCE_SERVICE_GROUP = 1,
    SM_FAILOVER_DEGRADE_SOURCE_IF_DOWN = 2
}SmFailoverDegradeSourceT;

typedef enum
{
    SM_FAILOVER_CLUSTER_HOST_DOWN = 1,
    SM_FAILOVER_MGMT_DOWN = 2,
    SM_FAILOVER_OAM_DOWN = 4,
    SM_FAILOVER_HEARTBEAT_ALIVE = 8,
    SM_FAILOVER_HELLO_MSG_ALIVE = 16,
    SM_FAILOVER_PEER_DISABLED = 0x4000,
}SmFailoverCommFaultBitFlagT;

// ****************************************************************************
// Failover - check
// ==================
extern void sm_failover_check();
// ****************************************************************************

// ****************************************************************************
// Failover - audit
// ==================
extern void sm_failover_audit(bool recheck);
// ****************************************************************************

// ****************************************************************************
// Failover - lost hello msg
// ==================
extern void sm_failover_lost_hello_msg();
// ****************************************************************************


// ****************************************************************************
// Failover - hello msg restore
// ==================
extern void sm_failover_hello_msg_restore();
// ****************************************************************************

// ****************************************************************************
// Failover - lost heartbeat
// ==================
extern void sm_failover_lost_heartbeat( SmFailoverInterfaceT* interface );
// ****************************************************************************


// ****************************************************************************
// Failover - heartbeat restore
// ==================
extern void sm_failover_heartbeat_restore( SmFailoverInterfaceT* interface );
// ****************************************************************************


// ****************************************************************************
// Failover - interface down
// ==================
extern void sm_failover_interface_down( const char* const interface_name );
// ****************************************************************************


// ****************************************************************************
// Failover - interface down
// ==================
extern void sm_failover_interface_up( const char* const interface_name );
// ****************************************************************************

// ****************************************************************************
// Failover - interface state update for peer
extern void sm_failover_if_state_update(const char node_name[],
        SmHeartbeatMsgIfStateT if_state);
// ****************************************************************************

// ****************************************************************************
// Failover - get local interface state
extern SmHeartbeatMsgIfStateT sm_failover_if_state_get();
// ****************************************************************************

// ****************************************************************************
// Failover - action
// ==================
SmErrorT sm_failover_action( void );
// ****************************************************************************

// ****************************************************************************
// Failover - degrade
// ==================
SmErrorT sm_failover_degrade(SmFailoverDegradeSourceT source);
// ****************************************************************************

// ****************************************************************************
// Failover - degraded clear
// ==================
SmErrorT sm_failover_degrade_clear(SmFailoverDegradeSourceT source);
// ****************************************************************************

// ****************************************************************************
// Failover - get node
// ==================
SmErrorT sm_failover_get_node(char* node_name, SmDbNodeT& node);
// ****************************************************************************

// ****************************************************************************
// Failover - get node interface info
// ==================
SmFailoverInterfaceStateT sm_failover_get_interface_info(SmInterfaceTypeT interface_type);
// ****************************************************************************


// ****************************************************************************
// Failover - get peer node interface state
// ==================
SmHeartbeatMsgIfStateT sm_failover_get_peer_if_state();
// ****************************************************************************


// ****************************************************************************
// Failover - set system to scheduled status
// ==================
SmErrorT sm_failover_set_system(const SmSystemFailoverStatus& failover_status);
// ****************************************************************************

// ****************************************************************************
// Failover - active self
// =======================
SmErrorT sm_failover_activate_self();
// ****************************************************************************

// ****************************************************************************
// Failover - disable peer node
// =======================
SmErrorT sm_failover_disable_peer();
// ****************************************************************************

// ****************************************************************************
// Failover - is active controller
// ==================
bool sm_is_active_controller();
// ****************************************************************************

// ****************************************************************************
// Failover - get interface state
// ==================
int sm_failover_get_if_state();
// ****************************************************************************

// ****************************************************************************
// Failover - dump state
// ==================
extern void sm_failover_dump_state(FILE* fp);
// ****************************************************************************

// ****************************************************************************
// Failover - Initialize
// ======================
extern SmErrorT sm_failover_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Failover - Finalize
// ====================
extern SmErrorT sm_failover_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif

