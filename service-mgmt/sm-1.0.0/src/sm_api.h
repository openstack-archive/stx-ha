//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_API_H__
#define __SM_API_H__

#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SM_NODE_SET_ACTION_UNKNOWN,
    SM_NODE_SET_ACTION_LOCK,
    SM_NODE_SET_ACTION_UNLOCK,
    SM_NODE_SET_ACTION_SWACT,
    SM_NODE_SET_ACTION_SWACT_FORCE,
    SM_NODE_SET_ACTION_EVENT,
    SM_NODE_SET_ACTION_MAX
} SmNodeSetActionT;

typedef void (*SmApiNodeSetCallbackT) ( char node_name[],
        SmNodeSetActionT action, SmNodeAdminStateT admin_state,
        SmNodeOperationalStateT oper_state, SmNodeAvailStatusT avail_status,
        int seqno );

typedef void (*SmApiServiceRestartCallbackT) ( char service_name[],
        int seqno, int flag );

typedef struct
{
    SmApiNodeSetCallbackT node_set;
    SmApiServiceRestartCallbackT service_restart;
} SmApiCallbacksT;

// ****************************************************************************
// API - Register Callbacks
// ========================
extern SmErrorT sm_api_register_callbacks( SmApiCallbacksT* callbacks );
// ****************************************************************************

// ****************************************************************************
// API - Deregister Callbacks
// ==========================
extern SmErrorT sm_api_deregister_callbacks( SmApiCallbacksT* callbacks );
// ****************************************************************************

// ****************************************************************************
// API - Send Node Set
// ===================
extern SmErrorT sm_api_send_node_set( char node_name[],
    SmNodeSetActionT action, SmNodeAdminStateT admin_state,
    SmNodeOperationalStateT oper_state, SmNodeAvailStatusT avail_status );
// ****************************************************************************

// ****************************************************************************
// API - Send Node Set Ack
// =======================
extern SmErrorT sm_api_send_node_set_ack( char node_name[],
    SmNodeSetActionT action, SmNodeAdminStateT admin_state,
    SmNodeOperationalStateT oper_state, SmNodeAvailStatusT avail_status,
    int seqno );
// ****************************************************************************

// ***************************************************************************
// API - Initialize
// ================
extern SmErrorT sm_api_initialize( void );
// ***************************************************************************

// ***************************************************************************
// API - Finalize
// ==============
extern SmErrorT sm_api_finalize( void );
// ***************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_API_H__
