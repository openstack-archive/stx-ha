//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_MSG_H__
#define __SM_MSG_H__

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_uuid.h"
#include "sm_service_domain_interface_table.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SmMsgNodeHelloCallbackT) (SmNetworkAddressT* network_address,
        int network_port, int version, int revision, char node_name[],
        SmNodeAdminStateT admin_state, SmNodeOperationalStateT oper_state,
        SmNodeAvailStatusT avail_status, SmNodeReadyStateT ready_state,
        SmUuidT state_uuid, long uptime);

typedef void (*SmMsgNodeUpdateCallbackT) (SmNetworkAddressT* network_address,
        int network_port, int version, int revision, char node_name[],
        SmNodeAdminStateT admin_state, SmNodeOperationalStateT oper_state,
        SmNodeAvailStatusT avail_status, SmNodeReadyStateT ready_state,
        SmUuidT old_state_uuid, SmUuidT state_uuid, long uptime, bool force);

typedef void (*SmMsgNodeSwactCallbackT) (
        SmNetworkAddressT* network_address, int network_port, int version,
        int revision, char node_name[], bool force, SmUuidT request_uuid );

typedef void (*SmMsgNodeSwactAckCallbackT) (
        SmNetworkAddressT* network_address, int network_port, int version,
        int revision, char node_name[], bool force, SmUuidT request_uuid );

typedef void (*SmMsgHelloCallbackT) (SmNetworkAddressT* network_address, 
        int network_port, int version, int revision, char service_domain[],
        char node_name[], char orchestration[], char designation[], 
        int generation, int priority, int hello_interval, int dead_interval,
        int wait_interval, int exchange_interval, char leader[]);

typedef void (*SmMsgPauseCallbackT) (SmNetworkAddressT* network_address, 
        int network_port, int version, int revision, char service_domain[],
        char node_name[], int pause_interval);

typedef void (*SmMsgExchangeStartCallbackT) (SmNetworkAddressT* network_address,
        int network_port, int version, int revision, char service_domain[],
        char node_name[], int exchange_seq);

typedef void (*SmMsgExchangeCallbackT) (SmNetworkAddressT* network_address,
        int network_port, int version, int revision, char service_domain[],
        char node_name[], int exchange_seq, int64_t member_id, 
        char member_name[], SmServiceGroupStateT member_desired_state,
        SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
        SmServiceGroupConditionT member_condition, int64_t member_health,
        char reason_text[], bool more_members, int64_t last_received_member_id );

typedef void (*SmMsgMemberRequestCallbackT) (SmNetworkAddressT* network_address,
        int network_port, int version, int revision, char service_domain[],
        char node_name[], char member_node_name[], int64_t member_id,
        char member_name[], SmServiceGroupActionT member_action,
        SmServiceGroupActionFlagsT member_action_flags );

typedef void (*SmMsgMemberUpdateCallbackT) (SmNetworkAddressT* network_address,
        int network_port, int version, int revision, char service_domain[],
        char node_name[], char member_node_name[], int64_t member_id,
        char member_name[], SmServiceGroupStateT member_desired_state,
        SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
        SmServiceGroupConditionT condition, int64_t member_health,
        char reason_text[] );

typedef struct
{
    // Node Message Callbacks
    SmMsgNodeHelloCallbackT node_hello;
    SmMsgNodeUpdateCallbackT node_update;
    SmMsgNodeSwactCallbackT node_swact;
    SmMsgNodeSwactCallbackT node_swact_ack;

    // Service Domain Message Callbacks
    SmMsgHelloCallbackT hello;
    SmMsgPauseCallbackT pause;
    SmMsgExchangeStartCallbackT exchange_start;
    SmMsgExchangeCallbackT exchange;
    SmMsgMemberRequestCallbackT member_request;
    SmMsgMemberUpdateCallbackT member_update;
} SmMsgCallbacksT;

// ****************************************************************************
// Messaging - Enable
// ==================
extern void sm_msg_enable( void );
// ****************************************************************************

// ****************************************************************************
// Messaging - Disable
// ===================
extern void sm_msg_disable( void );
// ****************************************************************************

// ****************************************************************************
// Messaging - Add Peer Interface 
// ==============================
extern SmErrorT sm_msg_add_peer_interface( char interface_name[],
    SmNetworkAddressT* network_address, int network_port,
    SmAuthTypeT auth_type, char auth_key[] );
// ****************************************************************************

// ****************************************************************************
// Messaging - Delete Peer Interface 
// =================================
extern SmErrorT sm_msg_delete_peer_interface( char interface_name[],
    SmNetworkAddressT* network_address, int network_port );
// ****************************************************************************

// ****************************************************************************
// Messaging - Register Callbacks
// ==============================
extern SmErrorT sm_msg_register_callbacks( SmMsgCallbacksT* callbacks );
// ****************************************************************************

// ****************************************************************************
// Messaging - Deregister Callbacks
// ================================
extern SmErrorT sm_msg_deregister_callbacks( SmMsgCallbacksT* callbacks );
// ****************************************************************************

// ****************************************************************************
// Messaging - Increment Sequence Number
// =====================================
extern void sm_msg_increment_seq_num( void );
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Node Hello
// ===========================
extern SmErrorT sm_msg_send_node_hello( char node_name[], 
    SmNodeAdminStateT admin_state, SmNodeOperationalStateT oper_state,
    SmNodeAvailStatusT avail_status, SmNodeReadyStateT ready_state,
    SmUuidT state_uuid, long uptime, SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Node Update
// ============================
extern SmErrorT sm_msg_send_node_update( char node_name[], 
    SmNodeAdminStateT admin_state, SmNodeOperationalStateT oper_state,
    SmNodeAvailStatusT avail_status, SmNodeReadyStateT ready_state,
    SmUuidT old_state_uuid, SmUuidT state_uuid, long uptime, bool force,
    SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Node Swact
// ===========================
extern SmErrorT sm_msg_send_node_swact( char node_name[], bool force,
    SmUuidT request_uuid, SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Node Swact Ack
// ===============================
extern SmErrorT sm_msg_send_node_swact_ack( char node_name[], bool force,
    SmUuidT request_uuid, SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Hello
// =====================================
extern SmErrorT sm_msg_send_service_domain_hello( char node_name[], 
    SmOrchestrationTypeT orchestration, SmDesignationTypeT designation,
    int generation, int priority, int hello_interval, int dead_interval,
    int wait_interval, int exchange_interval, char leader[],
    SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Pause
// =====================================
extern SmErrorT sm_msg_send_service_domain_pause( char node_name[], 
    int pause_interval, SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Exchange Start
// ==============================================
extern SmErrorT sm_msg_send_service_domain_exchange_start( char node_name[],
    char exchange_node_name[], int exchange_seq,
    SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Exchange
// ========================================
extern SmErrorT sm_msg_send_service_domain_exchange( char node_name[],
    char exchange_node_name[], int exchange_seq,  int64_t member_id,
    char member_name[], SmServiceGroupStateT member_desired_state,
    SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
    SmServiceGroupConditionT member_condition, int64_t member_health,
    char reason_text[], bool more_members, int64_t last_received_member_id,
    SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Member Request
// ==============================================
extern SmErrorT sm_msg_send_service_domain_member_request( char node_name[],
    char member_node_name[], int64_t member_id, char member_name[],
    SmServiceGroupActionT member_action,
    SmServiceGroupActionFlagsT member_action_flags,
    SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Member Update
// =============================================
extern SmErrorT sm_msg_send_service_domain_member_update( char node_name[],
    char member_node_name[], int64_t member_id, char member_name[], 
    SmServiceGroupStateT member_desired_state,
    SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
    SmServiceGroupConditionT member_condition, int64_t member_health,
    const char reason_text[], SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Open Sockets
// ========================
extern SmErrorT sm_msg_open_sockets( SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Close Sockets
// =========================
extern SmErrorT sm_msg_close_sockets( SmServiceDomainInterfaceT* interface );
// ****************************************************************************

// ****************************************************************************
// Messaging - Dump Data
// =====================
extern void sm_msg_dump_data( FILE* log );
// ****************************************************************************

// ****************************************************************************
// Messaging - Initialize
// ======================
extern SmErrorT sm_msg_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Messaging - Finalize
// ====================
extern SmErrorT sm_msg_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_MSG_H__
