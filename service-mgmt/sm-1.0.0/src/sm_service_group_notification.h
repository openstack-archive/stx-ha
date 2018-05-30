//
// Copyright (c) 2014-2016 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_SERVICE_GROUP_NOTIFICATION_H__
#define __SM_SERVICE_GROUP_NOTIFICATION_H__

#include "sm_types.h"
#include "sm_service_group_table.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Service Group Notification - Notify
// ===================================
extern SmErrorT sm_service_group_notification_notify(
    SmServiceGroupT* service_group, SmServiceGroupNotificationT notification );
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Abort
// ==================================
extern SmErrorT sm_service_group_notification_abort(
    SmServiceGroupT* service_group );
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Initialize
// =======================================
extern SmErrorT sm_service_group_notification_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Service Group Notification - Finalize
// =====================================
extern SmErrorT sm_service_group_notification_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_SERVICE_GROUP_NOTIFICATION_H__

