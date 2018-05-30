//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_HW_H__
#define __SM_HW_H__

#include <stdbool.h>
#include <stdint.h>

#include "sm_limits.h"
#include "sm_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SM_HW_QDISC_TYPE_MAX_CHAR       32
#define SM_HW_QDISC_HANDLE_MAX_CHAR     32

typedef struct
{
    char interface_name[SM_INTERFACE_NAME_MAX_CHAR];
    char queue_type[SM_HW_QDISC_TYPE_MAX_CHAR];
    char handle[SM_HW_QDISC_HANDLE_MAX_CHAR];
    uint64_t bytes;
    uint64_t packets;
    uint64_t q_length;
    uint64_t backlog;
    uint64_t drops;
    uint64_t requeues;
    uint64_t overlimits;
} SmHwQdiscInfoT;

typedef enum
{
    SM_HW_INTERFACE_CHANGE_TYPE_UNKNOWN,
    SM_HW_INTERFACE_CHANGE_TYPE_ADD,
    SM_HW_INTERFACE_CHANGE_TYPE_DELETE,
} SmHwInterfaceChangeTypeT;

typedef struct
{
    SmHwInterfaceChangeTypeT type;
    char interface_name[SM_INTERFACE_NAME_MAX_CHAR];
    SmInterfaceStateT interface_state;
} SmHwInterfaceChangeDataT;

typedef enum
{
    SM_HW_IP_CHANGE_TYPE_UNKNOWN,
    SM_HW_IP_CHANGE_TYPE_ADD,
    SM_HW_IP_CHANGE_TYPE_DELETE,
} SmHwIpChangeTypeT;

typedef enum
{
    SM_HW_ADDRESS_TYPE_UNKNOWN,
    SM_HW_ADDRESS_TYPE_ADDRESS,
    SM_HW_ADDRESS_TYPE_LOCAL,
    SM_HW_ADDRESS_TYPE_BROADCAST,
    SM_HW_ADDRESS_TYPE_ANYCAST,
} SmHwAddressTypeT;

typedef struct
{
    SmHwIpChangeTypeT type;
    char interface_name[SM_INTERFACE_NAME_MAX_CHAR];
    SmHwAddressTypeT address_type;
    SmNetworkAddressT address;
    int prefix_len;
} SmHwIpChangeDataT;

typedef void (*SmHwQdiscInfoCallbackT) (SmHwQdiscInfoT* data);
typedef void (*SmHwInterfaceChangeCallbackT) (SmHwInterfaceChangeDataT* data );
typedef void (*SmHwIpChangeCallbackT) (SmHwIpChangeDataT* data);

typedef struct
{
    SmHwQdiscInfoCallbackT qdisc_info;
    SmHwInterfaceChangeCallbackT interface_change;
    SmHwIpChangeCallbackT ip_change;
} SmHwCallbacksT;

// ***************************************************************************
// Hardware - Get Interface By Network Address
// ===========================================
extern SmErrorT sm_hw_get_if_by_network_address(
    SmNetworkAddressT* network_address, char if_name[] );
// ***************************************************************************

// ***************************************************************************
// Hardware - Get Interface Index
// ==============================
extern SmErrorT sm_hw_get_if_index( const char if_name[], int* if_index );
// ***************************************************************************

// ***************************************************************************
// Hardware - Get Interface Name
// =============================
extern SmErrorT sm_hw_get_if_name( int if_index, char if_name[] );
// ***************************************************************************

// ***************************************************************************
// Sm Hardware - Get Interface State
// =================================
extern SmErrorT sm_hw_get_if_state( const char if_name[], bool* enabled );
// ***************************************************************************

// ***************************************************************************
// Sm Hardware - Get All QDisc Asynchronous
// ========================================
extern SmErrorT sm_hw_get_all_qdisc_async( void );
// ***************************************************************************

// ***************************************************************************
// Hardware - Initialize
// =====================
extern SmErrorT sm_hw_initialize( SmHwCallbacksT* callbacks );
// ***************************************************************************

// ***************************************************************************
// Hardware - Finalize
// ===================
extern SmErrorT sm_hw_finalize( void );
// ***************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_HW_H__
