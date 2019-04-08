//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_TYPES_H__
#define __SM_TYPES_H__

#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SM_CONFIG_OPTION_STANDBY_ALL_SERVICES_ON_A_LOCKED_NODE

#define STRINGIZE(str) #str
#define MAKE_STRING(str)  STRINGIZE(str)

#define SM_VERSION                                   1
#define SM_REVISION                                  2
#define SM_RUN_DIRECTORY                             "/var/run/sm"
#define SM_RUN_SERVICES_DIRECTORY                    "/var/run/sm/services"
#define SM_DATABASE_NAME                             "/var/run/sm/sm.db"
#define SM_HEARTBEAT_DATABASE_NAME                   "/var/run/sm/sm.hb.db"
#define SM_MASTER_DATABASE_NAME                      "/var/lib/sm/sm.db"
#define SM_MASTER_HEARTBEAT_DATABASE_NAME            "/var/lib/sm/sm.hb.db"
#define SM_PATCH_SCRIPT                              "/var/lib/sm/patches/sm-patch.sql"

#define SM_SERVICE_ACTION_PLUGIN_TYPE_LSB_SCRIPT     "lsb-script"
#ifdef __LSB_DIR
#define SM_SERVICE_ACTION_PLUGIN_TYPE_LSB_DIR        MAKE_STRING(__LSB_DIR)
#else
#define SM_SERVICE_ACTION_PLUGIN_TYPE_LSB_DIR        "/etc/init.d"
#endif

#define SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_VERSION    "1"
#define SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_REVISION   "1"
#define SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_SCRIPT     "ocf-script"

#ifdef __OCF_DIR
#define SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_DIR          MAKE_STRING(__OCF_DIR)
#else
#define SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_DIR          "/usr/lib/ocf"
#endif
#ifdef __OCF_PLUGIN_DIR
#define SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_PLUGIN_DIR   MAKE_STRING(__OCF_PLUGIN_DIR)
#else
#define SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_PLUGIN_DIR   "/usr/lib/ocf/resource.d"
#endif

#ifdef __OCF_DIR64
#define SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_DIR64        MAKE_STRING(__OCF_DIR64)
#else
#define SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_DIR64        "/usr/lib64/ocf"
#endif
#ifdef __OCF_PLUGIN_DIR64
#define SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_PLUGIN_DIR64 MAKE_STRING(__OCF_PLUGIN_DIR64)
#else
#define SM_SERVICE_ACTION_PLUGIN_TYPE_OCF_PLUGIN_DIR64 "/usr/lib64/ocf/resource.d"
#endif

#define SM_SERVICE_ACTION_PLUGIN_TYPE_PYTHON_SCRIPT  "python-script"
#define SM_SERVICE_ACTION_PLUGIN_TYPE_CUSTOM_SCRIPT  "custom-script"
#define SM_SERVICE_ACTION_PLUGIN_TYPE_SHARED_LIBRARY "shared-library"

#define SM_PROCESS_FAILED                            -65533
#define SM_SERVICE_ACTION_PLUGIN_TIMEOUT             -65534
#define SM_SERVICE_ACTION_PLUGIN_FAILURE             -65535
#define SM_SERVICE_ACTION_PLUGIN_FORCE_SUCCESS       -65536

#define SM_INVALID_INDEX                             -1

#define SM_PROCESS_PID_FILENAME                      "/var/run/sm.pid"
#define SM_TRAP_PROCESS_PID_FILENAME                 "/var/run/sm-trap.pid"
#define SM_WATCHDOG_PROCESS_PID_FILENAME             "/var/run/sm-watchdog.pid"
#define SM_ERU_PROCESS_PID_FILENAME                  "/var/run/sm-eru.pid"

#define SM_BOOT_COMPLETE_FILENAME                    "/var/run/sm_boot_complete"

#define SM_INDICATE_DEGRADED_FILENAME                "/var/run/.sm_degraded"

#define SM_WATCHDOG_HEARTBEAT_FILENAME               "/var/run/.sm_watchdog_heartbeat"

#define SM_DUMP_DATA_FILE                            "/tmp/sm_data_dump.txt"

#define SM_TROUBLESHOOT_LOG_FILE                     "/var/log/sm-troubleshoot.log"
#define SM_TROUBLESHOOT_SCRIPT                       "/usr/local/sbin/sm-troubleshoot"

#define SM_NOTIFICATION_SCRIPT                       "/usr/local/sbin/sm-notification"

#define SM_SERVICE_DOMAIN_WEIGHT_MINIMUM              0
#define SM_SERVICE_DOMAIN_WEIGHT_UNSELECTABLE_ACTIVE -1

#define SM_SERVICE_DOMAIN_MGMT_INTERFACE             "management-interface"
#define SM_SERVICE_DOMAIN_OAM_INTERFACE              "oam-interface"
#define SM_SERVICE_DOMAIN_CLUSTER_HOST_INTERFACE     "cluster-host-interface"

#define SM_MGMT_INTERFACE_NAME                       "mgmt"
#define SM_OAM_INTERFACE_NAME                        "oam"
#define SM_CLUSTER_HOST_INTERFACE_NAME               "cluster-host"
#define SM_MAX_IF_NAME_LEN                           5

#define SM_NODE_CONTROLLER_0_NAME                    "controller-0"
#define SM_NODE_CONTROLLER_1_NAME                    "controller-1"

#define SM_CPE_MODE_SIMPLEX                           "simplex"
#define SM_CPE_MODE_DUPLEX_DIRECT                     "duplex-direct"
#define SM_CPE_MODE_DUPLEX                            "duplex"

typedef enum
{
    SM_DB_TYPE_MAIN,
    SM_DB_TYPE_HEARTBEAT,
    SM_DB_TYPE_MAX
} SmDbTypeT;

typedef enum
{
    SM_COMPARE_OPERATOR_GT,
    SM_COMPARE_OPERATOR_GE,
    SM_COMPARE_OPERATOR_LT,
    SM_COMPARE_OPERATOR_LE,
    SM_COMPARE_OPERATOR_EQ,
    SM_COMPARE_OPERATOR_NE,
    SM_COMPARE_OPERATOR_MAX
} SmCompareOperatorT;

typedef enum
{
    SM_NODE_ADMIN_STATE_NIL,
    SM_NODE_ADMIN_STATE_UNKNOWN,
    SM_NODE_ADMIN_STATE_LOCKED,
    SM_NODE_ADMIN_STATE_UNLOCKED,
    SM_NODE_ADMIN_STATE_MAX
} SmNodeAdminStateT;

typedef enum
{
    SM_NODE_OPERATIONAL_STATE_NIL,
    SM_NODE_OPERATIONAL_STATE_UNKNOWN,
    SM_NODE_OPERATIONAL_STATE_ENABLED,
    SM_NODE_OPERATIONAL_STATE_DISABLED,
    SM_NODE_OPERATIONAL_STATE_MAX
} SmNodeOperationalStateT;

typedef enum
{
    SM_NODE_AVAIL_STATUS_NIL,
    SM_NODE_AVAIL_STATUS_UNKNOWN,
    SM_NODE_AVAIL_STATUS_NONE,
    SM_NODE_AVAIL_STATUS_AVAILABLE,
    SM_NODE_AVAIL_STATUS_DEGRADED,
    SM_NODE_AVAIL_STATUS_FAILED,
    SM_NODE_AVAIL_STATUS_MAX
} SmNodeAvailStatusT;

typedef enum
{
    SM_NODE_READY_STATE_NIL,
    SM_NODE_READY_STATE_UNKNOWN,
    SM_NODE_READY_STATE_ENABLED,
    SM_NODE_READY_STATE_DISABLED,
    SM_NODE_READY_STATE_MAX
} SmNodeReadyStateT;

typedef enum
{
    SM_NODE_EVENT_NIL,
    SM_NODE_EVENT_UNKNOWN,
    SM_NODE_EVENT_ENABLED,
    SM_NODE_EVENT_DISABLED,
    SM_NODE_EVENT_AUDIT,
    SM_NODE_EVENT_MAX
} SmNodeEventT;

typedef enum
{
    SM_INTERFACE_STATE_NIL,
    SM_INTERFACE_STATE_UNKNOWN,
    SM_INTERFACE_STATE_ENABLED,
    SM_INTERFACE_STATE_DISABLED,
    SM_INTERFACE_STATE_NOT_IN_USE,
    SM_INTERFACE_STATE_MAX
} SmInterfaceStateT;

typedef enum
{
    SM_NETWORK_TYPE_NIL,
    SM_NETWORK_TYPE_UNKNOWN,
    SM_NETWORK_TYPE_IPV4,
    SM_NETWORK_TYPE_IPV6,
    SM_NETWORK_TYPE_IPV4_UDP,
    SM_NETWORK_TYPE_IPV6_UDP,
    SM_NETWORK_TYPE_MAX
} SmNetworkTypeT;

typedef enum
{
    SM_INTERFACE_UNKNOWN,
    SM_INTERFACE_MGMT,
    SM_INTERFACE_CLUSTER_HOST,
    SM_INTERFACE_OAM
}SmInterfaceTypeT;

typedef enum
{
    SM_PATH_TYPE_NIL,
    SM_PATH_TYPE_UNKNOWN,
    SM_PATH_TYPE_PRIMARY,
    SM_PATH_TYPE_SECONDARY,
    SM_PATH_TYPE_STATUS_ONLY,
    SM_PATH_TYPE_MAX
} SmPathTypeT;

typedef enum
{
    SM_AUTH_TYPE_NIL,
    SM_AUTH_TYPE_UNKNOWN,
    SM_AUTH_TYPE_NONE,
    SM_AUTH_TYPE_HMAC_SHA512,
    SM_AUTH_TYPE_MAX
} SmAuthTypeT;

typedef enum
{
    SM_ORCHESTRATION_TYPE_NIL,
    SM_ORCHESTRATION_TYPE_UNKNOWN,
    SM_ORCHESTRATION_TYPE_GEOGRAPHICAL,
    SM_ORCHESTRATION_TYPE_REGIONAL,
    SM_ORCHESTRATION_TYPE_HYBRID,
    SM_ORCHESTRATION_TYPE_MAX
} SmOrchestrationTypeT;

typedef enum
{
    SM_DESIGNATION_TYPE_NIL,
    SM_DESIGNATION_TYPE_UNKNOWN,
    SM_DESIGNATION_TYPE_LEADER,
    SM_DESIGNATION_TYPE_BACKUP,
    SM_DESIGNATION_TYPE_OTHER,
    SM_DESIGNATION_TYPE_MAX
} SmDesignationTypeT;

typedef enum
{
    SM_SERVICE_DOMAIN_STATE_NIL,
    SM_SERVICE_DOMAIN_STATE_UNKNOWN,
    SM_SERVICE_DOMAIN_STATE_INITIAL,
    SM_SERVICE_DOMAIN_STATE_WAITING,
    SM_SERVICE_DOMAIN_STATE_LEADER,
    SM_SERVICE_DOMAIN_STATE_BACKUP,
    SM_SERVICE_DOMAIN_STATE_OTHER,
    SM_SERVICE_DOMAIN_STATE_MAX
} SmServiceDomainStateT;

typedef enum
{
    SM_SERVICE_DOMAIN_EVENT_NIL,
    SM_SERVICE_DOMAIN_EVENT_UNKNOWN,
    SM_SERVICE_DOMAIN_EVENT_HELLO_MSG,
    SM_SERVICE_DOMAIN_EVENT_NEIGHBOR_AGEOUT,
    SM_SERVICE_DOMAIN_EVENT_INTERFACE_ENABLED,
    SM_SERVICE_DOMAIN_EVENT_INTERFACE_DISABLED,
    SM_SERVICE_DOMAIN_EVENT_WAIT_EXPIRED,
    SM_SERVICE_DOMAIN_EVENT_CHANGING_LEADER,
    SM_SERVICE_DOMAIN_EVENT_MAX
} SmServiceDomainEventT;

typedef enum 
{
    SM_SERVICE_DOMAIN_EVENT_DATA_MSG_NODE_NAME,
    SM_SERVICE_DOMAIN_EVENT_DATA_MSG_GENERATION,
    SM_SERVICE_DOMAIN_EVENT_DATA_MSG_PRIORITY,
    SM_SERVICE_DOMAIN_EVENT_DATA_MSG_LEADER,
    SM_SERVICE_DOMAIN_EVENT_DATA_MAX
} SmServiceDomainEventDataT;

typedef enum{
    SM_FAILOVER_STATE_INITIAL,
    SM_FAILOVER_STATE_NORMAL,
    SM_FAILOVER_STATE_FAIL_PENDING,
    SM_FAILOVER_STATE_FAILED,
    SM_FAILOVER_STATE_SURVIVED,
    SM_FAILOVER_STATE_MAX
}SmFailoverStateT;

typedef enum{
    SM_FAILOVER_EVENT_HEARTBEAT_ENABLED,
    SM_FAILOVER_EVENT_IF_STATE_CHANGED,
    SM_FAILOVER_EVENT_FAIL_PENDING_TIMEOUT,
    SM_FAILOVER_EVENT_NODE_ENABLED,
    SM_FAILOVER_EVENT_MAX
}SmFailoverEventT;

typedef enum
{
    SM_SERVICE_DOMAIN_INTERFACE_EVENT_NIL,
    SM_SERVICE_DOMAIN_INTERFACE_EVENT_UNKNOWN,
    SM_SERVICE_DOMAIN_INTERFACE_EVENT_NODE_ENABLED,
    SM_SERVICE_DOMAIN_INTERFACE_EVENT_NODE_DISABLED,
    SM_SERVICE_DOMAIN_INTERFACE_EVENT_ENABLED,
    SM_SERVICE_DOMAIN_INTERFACE_EVENT_DISABLED,
    SM_SERVICE_DOMAIN_INTERFACE_EVENT_AUDIT,
    SM_SERVICE_DOMAIN_INTERFACE_EVENT_NOT_IN_USE,
    SM_SERVICE_DOMAIN_INTERFACE_EVENT_MAX
} SmServiceDomainInterfaceEventT;

typedef enum
{
    SM_SERVICE_DOMAIN_INTERFACE_CONNECT_TYPE_TOR,
    SM_SERVICE_DOMAIN_INTERFACE_CONNECT_TYPE_DC
} SmServiceDomainInterfaceConnectTypeT;

typedef enum
{
    SM_SERVICE_DOMAIN_NEIGHBOR_STATE_NIL,
    SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN,
    SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE_START,
    SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE,
    SM_SERVICE_DOMAIN_NEIGHBOR_STATE_FULL,
    SM_SERVICE_DOMAIN_NEIGHBOR_STATE_MAX
} SmServiceDomainNeighborStateT;

typedef enum
{
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_NIL,
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_UNKNOWN,
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_HELLO_MSG,
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_MSG,
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_TIMER,
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_MSG,
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_TIMEOUT,
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DOWN,
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_MAX
} SmServiceDomainNeighborEventT;

typedef enum 
{
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DATA_MSG_EXCHANGE_SEQ,
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DATA_MSG_MORE_MEMBERS,
    SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DATA_MAX
} SmServiceDomainNeighborEventDataT;

typedef enum
{
    SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_NIL,
    SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_UNKNOWN,
    SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_NONE,
    SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N,
    SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_PLUS_M,
    SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_TO_1,
    SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_TO_N,
    SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_MAX
} SmServiceDomainMemberRedundancyModelT;

typedef enum
{
    SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_NIL,
    SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_UNKNOWN,
    SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_DISABLE_ALL_ACTIVE,
    SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_SELECT_BEST_ACTIVE,
    SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_MAX
} SmServiceDomainSplitBrainRecoveryT;

typedef enum
{
    SM_SERVICE_GROUP_ACTION_NIL,
    SM_SERVICE_GROUP_ACTION_UNKNOWN,
    SM_SERVICE_GROUP_ACTION_DISABLE,
    SM_SERVICE_GROUP_ACTION_GO_ACTIVE,
    SM_SERVICE_GROUP_ACTION_GO_STANDBY,
    SM_SERVICE_GROUP_ACTION_AUDIT,
    SM_SERVICE_GROUP_ACTION_RECOVER,
    SM_SERVICE_GROUP_ACTION_MAX
} SmServiceGroupActionT;

#define SM_SERVICE_GROUP_ACTION_FLAG_RECOVER                1
#define SM_SERVICE_GROUP_ACTION_FLAG_ESCALATE_RECOVERY      2
#define SM_SERVICE_GROUP_ACTION_FLAG_CLEAR_FATAL_CONDITION  3

#define SM_FLAG_SET(flags, flag)    (flags |= (1 << flag))
#define SM_FLAG_CLEAR(flags, flag)  (flags &= ~(1 << flag))
#define SM_FLAG_TOGGLE(flags, flag) (flags ^= (1 << flag))
#define SM_FLAG_IS_SET(flags, flag) (flags & (1 << flag))

// bit flag for sm service action options
// restart a service without restarting its dependency
#define SM_SVC_RESTART_NO_DEP 0x1

typedef uint64_t SmServiceGroupActionFlagsT;

typedef enum
{
    SM_SERVICE_GROUP_STATE_NIL,
    SM_SERVICE_GROUP_STATE_NA,
    SM_SERVICE_GROUP_STATE_INITIAL,
    SM_SERVICE_GROUP_STATE_UNKNOWN,
    SM_SERVICE_GROUP_STATE_STANDBY,
    SM_SERVICE_GROUP_STATE_GO_STANDBY,
    SM_SERVICE_GROUP_STATE_GO_ACTIVE,
    SM_SERVICE_GROUP_STATE_ACTIVE,
    SM_SERVICE_GROUP_STATE_DISABLING,
    SM_SERVICE_GROUP_STATE_DISABLED,
    SM_SERVICE_GROUP_STATE_SHUTDOWN,
    SM_SERVICE_GROUP_STATE_MAX
} SmServiceGroupStateT;

typedef enum
{
    SM_SERVICE_GROUP_EVENT_NIL,
    SM_SERVICE_GROUP_EVENT_UNKNOWN,
    SM_SERVICE_GROUP_EVENT_GO_ACTIVE,
    SM_SERVICE_GROUP_EVENT_GO_STANDBY,
    SM_SERVICE_GROUP_EVENT_DISABLE,
    SM_SERVICE_GROUP_EVENT_AUDIT,
    SM_SERVICE_GROUP_EVENT_SERVICE_SCN,
    SM_SERVICE_GROUP_EVENT_SHUTDOWN,
    SM_SERVICE_GROUP_EVENT_NOTIFICATION_SUCCESS,
    SM_SERVICE_GROUP_EVENT_NOTIFICATION_FAILED,
    SM_SERVICE_GROUP_EVENT_NOTIFICATION_TIMEOUT,
    SM_SERVICE_GROUP_EVENT_MAX,
} SmServiceGroupEventT;

typedef enum 
{
    SM_SERVICE_GROUP_EVENT_DATA_SERVICE_NAME,
    SM_SERVICE_GROUP_EVENT_DATA_SERVICE_STATE,
    SM_SERVICE_GROUP_EVENT_DATA_MAX
} SmServiceGroupEventDataT;

typedef enum
{
    SM_SERVICE_GROUP_STATUS_NIL,
    SM_SERVICE_GROUP_STATUS_UNKNOWN,
    SM_SERVICE_GROUP_STATUS_NONE,
    SM_SERVICE_GROUP_STATUS_WARN,
    SM_SERVICE_GROUP_STATUS_DEGRADED,
    SM_SERVICE_GROUP_STATUS_FAILED,
    SM_SERVICE_GROUP_STATUS_MAX
} SmServiceGroupStatusT;

typedef enum
{
    SM_SERVICE_GROUP_CONDITION_NIL,
    SM_SERVICE_GROUP_CONDITION_UNKNOWN,
    SM_SERVICE_GROUP_CONDITION_NONE,
    SM_SERVICE_GROUP_CONDITION_DATA_INCONSISTENT,
    SM_SERVICE_GROUP_CONDITION_DATA_OUTDATED,
    SM_SERVICE_GROUP_CONDITION_DATA_CONSISTENT,
    SM_SERVICE_GROUP_CONDITION_DATA_SYNC,
    SM_SERVICE_GROUP_CONDITION_DATA_STANDALONE,
    SM_SERVICE_GROUP_CONDITION_RECOVERY_FAILURE,
    SM_SERVICE_GROUP_CONDITION_ACTION_FAILURE,
    SM_SERVICE_GROUP_CONDITION_FATAL_FAILURE,
    SM_SERVICE_GROUP_CONDITION_MAX
} SmServiceGroupConditionT;

typedef enum
{
    SM_SERVICE_GROUP_NOTIFICATION_NIL,
    SM_SERVICE_GROUP_NOTIFICATION_UNKNOWN,
    SM_SERVICE_GROUP_NOTIFICATION_STANDBY,
    SM_SERVICE_GROUP_NOTIFICATION_GO_STANDBY,
    SM_SERVICE_GROUP_NOTIFICATION_GO_ACTIVE,
    SM_SERVICE_GROUP_NOTIFICATION_ACTIVE,
    SM_SERVICE_GROUP_NOTIFICATION_DISABLING,
    SM_SERVICE_GROUP_NOTIFICATION_DISABLED,
    SM_SERVICE_GROUP_NOTIFICATION_SHUTDOWN,
    SM_SERVICE_GROUP_NOTIFICATION_MAX
} SmServiceGroupNotificationT;

typedef enum
{
    SM_SERVICE_ADMIN_STATE_NIL,
    SM_SERVICE_ADMIN_STATE_NA,
    SM_SERVICE_ADMIN_STATE_LOCKED,
    SM_SERVICE_ADMIN_STATE_UNLOCKED,
    SM_SERVICE_ADMIN_STATE_MAX
} SmServiceAdminStateT;

typedef enum
{
    SM_SERVICE_STATE_NIL,
    SM_SERVICE_STATE_NA,
    SM_SERVICE_STATE_INITIAL,
    SM_SERVICE_STATE_UNKNOWN,
    SM_SERVICE_STATE_ENABLED_STANDBY,
    SM_SERVICE_STATE_ENABLED_GO_STANDBY,
    SM_SERVICE_STATE_ENABLED_GO_ACTIVE,
    SM_SERVICE_STATE_ENABLED_ACTIVE,
    SM_SERVICE_STATE_ENABLING_THROTTLE,
    SM_SERVICE_STATE_ENABLING,
    SM_SERVICE_STATE_DISABLING,
    SM_SERVICE_STATE_DISABLED,
    SM_SERVICE_STATE_SHUTDOWN,
    SM_SERVICE_STATE_MAX
} SmServiceStateT;

typedef enum
{
    SM_SERVICE_EVENT_NIL,
    SM_SERVICE_EVENT_UNKNOWN,
    SM_SERVICE_EVENT_ENABLE_THROTTLE,
    SM_SERVICE_EVENT_ENABLE,
    SM_SERVICE_EVENT_ENABLE_SUCCESS,
    SM_SERVICE_EVENT_ENABLE_FAILED,
    SM_SERVICE_EVENT_ENABLE_TIMEOUT,
    SM_SERVICE_EVENT_GO_ACTIVE,
    SM_SERVICE_EVENT_GO_ACTIVE_SUCCESS,
    SM_SERVICE_EVENT_GO_ACTIVE_FAILED,
    SM_SERVICE_EVENT_GO_ACTIVE_TIMEOUT,
    SM_SERVICE_EVENT_GO_STANDBY,
    SM_SERVICE_EVENT_GO_STANDBY_SUCCESS,
    SM_SERVICE_EVENT_GO_STANDBY_FAILED,
    SM_SERVICE_EVENT_GO_STANDBY_TIMEOUT,
    SM_SERVICE_EVENT_DISABLE,
    SM_SERVICE_EVENT_DISABLE_SUCCESS,
    SM_SERVICE_EVENT_DISABLE_FAILED,
    SM_SERVICE_EVENT_DISABLE_TIMEOUT,
    SM_SERVICE_EVENT_AUDIT,
    SM_SERVICE_EVENT_AUDIT_SUCCESS,
    SM_SERVICE_EVENT_AUDIT_FAILED,
    SM_SERVICE_EVENT_AUDIT_TIMEOUT,
    SM_SERVICE_EVENT_AUDIT_MISMATCH,
    SM_SERVICE_EVENT_HEARTBEAT_OKAY,
    SM_SERVICE_EVENT_HEARTBEAT_WARN,
    SM_SERVICE_EVENT_HEARTBEAT_DEGRADE,
    SM_SERVICE_EVENT_HEARTBEAT_FAIL,
    SM_SERVICE_EVENT_PROCESS_FAILURE,
    SM_SERVICE_EVENT_SHUTDOWN,
    SM_SERVICE_EVENT_MAX,
} SmServiceEventT;

typedef enum
{
    SM_SERVICE_EVENT_DATA_IS_ACTION,
    SM_SERVICE_EVENT_DATA_STATE,
    SM_SERVICE_EVENT_DATA_STATUS,
    SM_SERVICE_EVENT_DATA_CONDITION,
    SM_SERVICE_EVENT_DATA_MAX
} SmServiceEventDataT;

typedef enum
{
    SM_SERVICE_STATUS_NIL,
    SM_SERVICE_STATUS_UNKNOWN,
    SM_SERVICE_STATUS_NONE,
    SM_SERVICE_STATUS_WARN,
    SM_SERVICE_STATUS_DEGRADED,
    SM_SERVICE_STATUS_FAILED,
    SM_SERVICE_STATUS_MAX
} SmServiceStatusT;

// Service Condition Meanings:
//   data-inconsistent: data is not useable.
//   data-outdated:     data is consistent, but not the latest.
//   data-consistent:   data is consistent, but might not be the latest (unknown).
//   data-standalone:   data is consistent, but will not sync.
//   recovery-failure:  recovery of the service has failed.
//   action-failure:    a service action has failed.
//   fatal-failure:     a fatal failure has occured.
typedef enum 
{
    SM_SERVICE_CONDITION_NIL,
    SM_SERVICE_CONDITION_UNKNOWN,
    SM_SERVICE_CONDITION_NONE,
    SM_SERVICE_CONDITION_DATA_INCONSISTENT,
    SM_SERVICE_CONDITION_DATA_OUTDATED,
    SM_SERVICE_CONDITION_DATA_CONSISTENT,
    SM_SERVICE_CONDITION_DATA_SYNC,
    SM_SERVICE_CONDITION_DATA_STANDALONE,
    SM_SERVICE_CONDITION_RECOVERY_FAILURE,
    SM_SERVICE_CONDITION_ACTION_FAILURE,
    SM_SERVICE_CONDITION_FATAL_FAILURE,
    SM_SERVICE_CONDITION_MAX
} SmServiceConditionT;

typedef enum
{
    SM_SERVICE_SEVERITY_NIL,
    SM_SERVICE_SEVERITY_UNKNOWN,
    SM_SERVICE_SEVERITY_NONE,
    SM_SERVICE_SEVERITY_MINOR,
    SM_SERVICE_SEVERITY_MAJOR,
    SM_SERVICE_SEVERITY_CRITICAL,
    SM_SERVICE_SEVERITY_MAX
} SmServiceSeverityT;

typedef enum
{
    SM_SERVICE_HEARTBEAT_TYPE_NIL,
    SM_SERVICE_HEARTBEAT_TYPE_UNKNOWN,
    SM_SERVICE_HEARTBEAT_TYPE_UNIX,
    SM_SERVICE_HEARTBEAT_TYPE_UDP,
    SM_SERVICE_HEARTBEAT_TYPE_MAX,
} SmServiceHeartbeatTypeT;

typedef enum
{
    SM_SERVICE_HEARTBEAT_STATE_NIL,
    SM_SERVICE_HEARTBEAT_STATE_UNKNOWN,
    SM_SERVICE_HEARTBEAT_STATE_STARTED,
    SM_SERVICE_HEARTBEAT_STATE_STOPPED,
    SM_SERVICE_HEARTBEAT_STATE_MAX,
} SmServiceHeartbeatStateT;

typedef enum
{
    SM_SERVICE_DEPENDENCY_TYPE_NIL,
    SM_SERVICE_DEPENDENCY_TYPE_UNKNOWN,
    SM_SERVICE_DEPENDENCY_TYPE_ACTION,
    SM_SERVICE_DEPENDENCY_TYPE_STATE,
    SM_SERVICE_DEPENDENCY_TYPE_MAX,
} SmServiceDependencyTypeT;

typedef enum
{
    SM_SERVICE_ACTION_NIL,
    SM_SERVICE_ACTION_NA,
    SM_SERVICE_ACTION_UNKNOWN,
    SM_SERVICE_ACTION_NONE,
    SM_SERVICE_ACTION_ENABLE,
    SM_SERVICE_ACTION_DISABLE,
    SM_SERVICE_ACTION_GO_ACTIVE,
    SM_SERVICE_ACTION_GO_STANDBY,
    SM_SERVICE_ACTION_AUDIT_ENABLED,
    SM_SERVICE_ACTION_AUDIT_DISABLED,
    SM_SERVICE_ACTION_MAX
} SmServiceActionT;

typedef enum
{
    SM_SERVICE_ACTION_RESULT_NIL,
    SM_SERVICE_ACTION_RESULT_UNKNOWN,
    SM_SERVICE_ACTION_RESULT_SUCCESS,
    SM_SERVICE_ACTION_RESULT_FATAL,
    SM_SERVICE_ACTION_RESULT_FAILED,
    SM_SERVICE_ACTION_RESULT_TIMEOUT,
    SM_SERVICE_ACTION_RESULT_MAX
} SmServiceActionResultT;

typedef enum
{
    SM_SERVICE_DOMAIN_SCHEDULING_STATE_NIL,
    SM_SERVICE_DOMAIN_SCHEDULING_STATE_UNKNOWN,
    SM_SERVICE_DOMAIN_SCHEDULING_STATE_NONE,
    SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT,
    SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT_FORCE,
    SM_SERVICE_DOMAIN_SCHEDULING_STATE_DISABLE,
    SM_SERVICE_DOMAIN_SCHEDULING_STATE_DISABLE_FORCE,
    SM_SERVICE_DOMAIN_SCHEDULING_STATE_MAX
} SmServiceDomainSchedulingStateT;

typedef enum
{
    SM_SERVICE_DOMAIN_SCHEDULING_LIST_NIL,
    SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE,
    SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE,
    SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY,
    SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY,
    SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING,
    SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED,
    SM_SERVICE_DOMAIN_SCHEDULING_LIST_FAILED,
    SM_SERVICE_DOMAIN_SCHEDULING_LIST_FATAL,
    SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE,
    SM_SERVICE_DOMAIN_SCHEDULING_LIST_MAX
} SmServiceDomainSchedulingListT;

typedef struct
{
    struct in_addr sin;   // stored in network order
} SmIpv4AddressT;

typedef struct
{
    struct in6_addr sin6; // stored in network order
} SmIpv6AddressT;

typedef struct
{
    SmNetworkTypeT type;

    union
    {
        SmIpv4AddressT ipv4;
        SmIpv6AddressT ipv6;
    } u;
} SmNetworkAddressT;

typedef enum
{
    SM_OKAY,
    SM_NOT_FOUND,
    SM_NO_MSG,
    SM_FAILED,
    SM_NOT_IMPLEMENTED,
    SM_ERROR_MAX
} SmErrorT;

typedef uint32_t SmHeartbeatMsgIfStateT;

typedef enum
{
    SM_SYSTEM_MODE_UNKNOWN,
    SM_SYSTEM_MODE_STANDARD,
    SM_SYSTEM_MODE_CPE_DUPLEX,
    SM_SYSTEM_MODE_CPE_DUPLEX_DC,
    SM_SYSTEM_MODE_CPE_SIMPLEX,
    SM_SYSTEM_MODE_MAX
}SmSystemModeT;

typedef enum
{
    SM_NODE_STATE_UNKNOWN,
    SM_NODE_STATE_ACTIVE,
    SM_NODE_STATE_STANDBY,
    SM_NODE_STATE_INIT,
    SM_NODE_STATE_FAILED,
    SM_NODE_STATE_MAX
}SmNodeScheduleStateT;


typedef enum
{
    SM_FAILOVER_INTERFACE_UNKNOWN,
    SM_FAILOVER_INTERFACE_OK,
    SM_FAILOVER_INTERFACE_MISSING_HEARTBEAT,
    SM_FAILOVER_INTERFACE_DOWN,
    SM_FAILOVER_INTERFACE_RECOVERING
}SmFailoverInterfaceStateT;

// ****************************************************************************
// Types - System Mode String
// =======================================
extern const char* sm_system_mode_str( SmSystemModeT system_mode);
// ****************************************************************************

// ****************************************************************************
// Types - Node Administrative State Value
// =======================================
extern SmNodeAdminStateT sm_node_admin_state_value( const char* state_str );
// ****************************************************************************

// ****************************************************************************
// Types - Node Administrative State String
// ========================================
extern const char* sm_node_admin_state_str( SmNodeAdminStateT state );
// ****************************************************************************

// ****************************************************************************
// Types - Node Operational State Value
// ====================================
extern SmNodeOperationalStateT sm_node_oper_state_value( const char* state_str );
// ****************************************************************************

// ****************************************************************************
// Types - Node Operational State String
// =====================================
extern const char* sm_node_oper_state_str( SmNodeOperationalStateT state );
// ****************************************************************************

// ****************************************************************************
// Types - Node Availability Status Value
// ======================================
extern SmNodeAvailStatusT sm_node_avail_status_value( const char* status_str );
// ****************************************************************************

// ****************************************************************************
// Types - Node Availability Status String
// =======================================
extern const char* sm_node_avail_status_str( SmNodeAvailStatusT status );
// ****************************************************************************

// ****************************************************************************
// Types - Node Ready State Value
// ==============================
extern SmNodeReadyStateT sm_node_ready_state_value( const char* state_str );
// ****************************************************************************

// ****************************************************************************
// Types - Node Ready State String
// ===============================
extern const char* sm_node_ready_state_str( SmNodeReadyStateT state );
// ****************************************************************************

// ****************************************************************************
// Types - Node State String
// =========================
extern const char* sm_node_state_str( SmNodeAdminStateT admin_state,
    SmNodeReadyStateT ready_state );
// ****************************************************************************

// ****************************************************************************
// Types - Node Event Value
// ========================
extern SmNodeEventT sm_node_event_value( const char* event_str );
// ****************************************************************************

// ****************************************************************************
// Types - Node Event String
// =========================
extern const char* sm_node_event_str( SmNodeEventT event );
// ****************************************************************************

// ****************************************************************************
// Types - Interface State Value
// =============================
extern SmInterfaceStateT sm_interface_state_value( const char* state_str );
// ****************************************************************************

// ****************************************************************************
// Types - Interface State String
// ==============================
extern const char* sm_interface_state_str( SmInterfaceStateT state );
// ****************************************************************************

// ****************************************************************************
// Types - Network Type Value
// ==========================
extern SmNetworkTypeT sm_network_type_value( const char* network_type_str );
// ****************************************************************************

// ****************************************************************************
// Types - Network Type String
// ===========================
extern const char* sm_network_type_str( SmNetworkTypeT network_type );
// ****************************************************************************

// ****************************************************************************
// Types - Path Type Value
// =======================
extern SmPathTypeT sm_path_type_value( const char* path_type_str );
// ****************************************************************************

// ****************************************************************************
// Types - Path Type String
// ========================
extern const char* sm_path_type_str( SmPathTypeT path_type );
// ****************************************************************************

// ****************************************************************************
// Types - Authentication Type Value
// =================================
extern SmAuthTypeT sm_auth_type_value( const char* auth_type_str );
// ****************************************************************************

// ****************************************************************************
// Types - Authentication Type String
// ==================================
extern const char* sm_auth_type_str( SmAuthTypeT auth_type );
// ****************************************************************************

// ****************************************************************************
// Types - Orchestration Type Value
// ================================
extern SmOrchestrationTypeT sm_orchestration_type_value( 
    const char* orchestration_type_str );
// ****************************************************************************

// ****************************************************************************
// Types - Orchestration Type String
// =================================
extern const char* sm_orchestration_type_str(
    SmOrchestrationTypeT orchestration_type );
// ****************************************************************************

// ****************************************************************************
// Types - Designation Type Value
// ==============================
extern SmDesignationTypeT sm_designation_type_value( 
    const char* designation_type_str );
// ****************************************************************************

// ****************************************************************************
// Types - Designation Type String
// ===============================
extern const char* sm_designation_type_str( 
    SmDesignationTypeT designation_type );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain State Value
// ==================================
extern SmServiceDomainStateT sm_service_domain_state_value( const char* state_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain State String
// ===================================
extern const char* sm_service_domain_state_str( SmServiceDomainStateT state );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Event Value
// ==================================
extern SmServiceDomainEventT sm_service_domain_event_value( const char* event_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Event String
// ===================================
extern const char* sm_service_domain_event_str( SmServiceDomainEventT event );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Scheduling State Value
// =============================================
extern SmServiceDomainSchedulingStateT sm_service_domain_scheduling_state_value(
    const char* sched_state_str );
// ****************************************************************************

// ***************************************************************************
// Types - Service Domain Scheduling State String
// ==============================================
extern const char* sm_service_domain_scheduling_state_str(
    SmServiceDomainSchedulingStateT sched_state );
// ***************************************************************************

// ****************************************************************************
// Types - Service Domain Scheduling List Value
// ============================================
extern SmServiceDomainSchedulingListT sm_service_domain_scheduling_list_value(
    const char* sched_list_str );
// ****************************************************************************

// ***************************************************************************
// Types - Service Domain Scheduling List String
// =============================================
extern const char* sm_service_domain_scheduling_list_str(
    SmServiceDomainSchedulingListT sched_list );
// ***************************************************************************

// ****************************************************************************
// Types - Service Domain Interface Event Value
// ============================================
extern SmServiceDomainInterfaceEventT sm_service_domain_interface_event_value(
    const char* event_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Interface Event String
// =============================================
extern const char* sm_service_domain_interface_event_str(
    SmServiceDomainInterfaceEventT event );
// ****************************************************************************

// ****************************************************************************
// Types - Failover Event String
// =============================================
extern const char* sm_failover_event_str( SmFailoverEventT event );
// ****************************************************************************

// ****************************************************************************
// Types - Failover State String
// =============================================
extern const char* sm_failover_state_str( SmFailoverStateT state );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Neighbor State Value
// ===========================================
extern SmServiceDomainNeighborStateT sm_service_domain_neighbor_state_value(
    const char* neighbor_state_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Neighbor State String
// ============================================
extern const char* sm_service_domain_neighbor_state_str(
    SmServiceDomainNeighborStateT neighbor_state );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Neighbor Event Value
// ===========================================
extern SmServiceDomainNeighborEventT sm_service_domain_neighbor_event_value(
    const char* neighbor_event_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Neighbor Event String
// ============================================
extern const char* sm_service_domain_neighbor_event_str(
    SmServiceDomainNeighborEventT neighbor_event );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Member Redundancy Model Value
// ====================================================
extern SmServiceDomainMemberRedundancyModelT 
sm_service_domain_member_redundancy_model_value(
    const char* model_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Member Redundancy Model String
// =====================================================
extern const char* sm_service_domain_member_redundancy_model_str(
    SmServiceDomainMemberRedundancyModelT model );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Split Brain Recovery Value
// =================================================
extern SmServiceDomainSplitBrainRecoveryT 
sm_service_domain_split_brain_recovery_value(
    const char* recovery_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Split Brain Recovery String
// ==================================================
extern const char* sm_service_domain_split_brain_recovery_str(
    SmServiceDomainSplitBrainRecoveryT recovery );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Action Value
// ==================================
extern SmServiceGroupActionT sm_service_group_action_value( 
    const char* action_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Action String
// ===================================
extern const char* sm_service_group_action_str( SmServiceGroupActionT action );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group State Value
// =================================
extern SmServiceGroupStateT sm_service_group_state_value(
    const char* state_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group State String
// ==================================
extern const char* sm_service_group_state_str( SmServiceGroupStateT state );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group State Lesser Value
// ========================================
extern SmServiceGroupStateT sm_service_group_state_lesser(
    SmServiceGroupStateT state_a, SmServiceGroupStateT state_b );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Event Value
// =================================
extern SmServiceGroupEventT sm_service_group_event_value( const char* event_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Event String
// ==================================
extern const char* sm_service_group_event_str( SmServiceGroupEventT event );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Status Value
// ==================================
extern SmServiceGroupStatusT sm_service_group_status_value( 
    const char* status_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Status String
// ===================================
extern const char* sm_service_group_status_str( SmServiceGroupStatusT status );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Condition Value
// =====================================
extern SmServiceGroupConditionT sm_service_group_condition_value( 
    const char* condition_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Condition String
// ======================================
extern const char* sm_service_group_condition_str(
    SmServiceGroupConditionT condition );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Notification Value
// ========================================
extern SmServiceGroupNotificationT sm_service_group_notification_value(
    const char* notification_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Notification String
// =========================================
extern const char* sm_service_group_notification_str(
    SmServiceGroupNotificationT notification );
// ****************************************************************************

// ****************************************************************************
// Types - Service Admin State Value
// =================================
extern SmServiceAdminStateT sm_service_admin_state_value( const char* state_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Admin State String
// ==================================
extern const char* sm_service_admin_state_str( SmServiceAdminStateT state );
// ****************************************************************************

// ****************************************************************************
// Types - Service State Value
// ===========================
extern SmServiceStateT sm_service_state_value( const char* state_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service State String
// ============================
extern const char* sm_service_state_str( SmServiceStateT state );
// ****************************************************************************

// ****************************************************************************
// Types - Service State Lesser Value
// ==================================
extern SmServiceStateT sm_service_state_lesser( SmServiceStateT state_a,
    SmServiceStateT state_b );
// ****************************************************************************

// ****************************************************************************
// Types - Service Event Value
// ===========================
extern SmServiceEventT sm_service_event_value( const char* event_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Event String
// ============================
extern const char* sm_service_event_str( SmServiceEventT event );
// ****************************************************************************

// ****************************************************************************
// Types - Service Status Value
// ============================
extern SmServiceStatusT sm_service_status_value( const char* status_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Status String
// =============================
extern const char* sm_service_status_str( SmServiceStatusT status );
// ****************************************************************************

// ****************************************************************************
// Types - Service Condition Value
// ===============================
extern SmServiceConditionT sm_service_condition_value( const char* condition_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Condition String
// ================================
extern const char* sm_service_condition_str( SmServiceConditionT condition );
// ****************************************************************************

// ****************************************************************************
// Types - Service Severity Value
// ==============================
extern SmServiceSeverityT sm_service_severity_value( const char* severity_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Severity String
// ==============================
extern const char* sm_service_severity_str( SmServiceSeverityT severity );
// ****************************************************************************

// ****************************************************************************
// Types - Service Heartbeat Type Value
// ====================================
extern SmServiceHeartbeatTypeT sm_service_heartbeat_type_value( 
    const char* heartbeat_type_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Heartbeat Type String
// =====================================
extern const char* sm_service_heartbeat_type_str(
    SmServiceHeartbeatTypeT heartbeat_type );
// ****************************************************************************

// ****************************************************************************
// Types - Service Heartbeat State Value
// =====================================
extern SmServiceHeartbeatStateT sm_service_heartbeat_state_value( 
    const char* heartbeat_state_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Heartbeat State String
// ======================================
extern const char* sm_service_heartbeat_state_str(
    SmServiceHeartbeatStateT heartbeat_state );
// ****************************************************************************

// ****************************************************************************
// Types - Service Dependency Type Value
// =====================================
extern SmServiceDependencyTypeT sm_service_dependency_type_value(
    const char* type_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Dependency Type String
// ======================================
extern const char* sm_service_dependency_type_str(
    SmServiceDependencyTypeT type );
// ****************************************************************************

// ****************************************************************************
// Types - Service Action Value
// ============================
extern SmServiceActionT sm_service_action_value( const char* action_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Action String
// =============================
extern const char* sm_service_action_str( SmServiceActionT action );
// ****************************************************************************

// ****************************************************************************
// Types - Service Action Result Value
// ===================================
extern SmServiceActionResultT sm_service_action_result_value(
    const char* result_str );
// ****************************************************************************

// ****************************************************************************
// Types - Service Action Result String
// ====================================
extern const char* sm_service_action_result_str(
    SmServiceActionResultT result );
// ****************************************************************************

// ****************************************************************************
// Types - Network Address Value
// =============================
extern bool sm_network_address_value( const char address_str[],
    SmNetworkAddressT* address );
// ****************************************************************************

// ***************************************************************************
// Types - Network Address As String
// =================================
extern void sm_network_address_str( const SmNetworkAddressT* address,
    char addr_str[] );
// ***************************************************************************

// ****************************************************************************
// Types - Error String
// ====================
extern const char* sm_error_str( SmErrorT error );
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface Type
// =====================================
extern SmInterfaceTypeT sm_get_interface_type( const char* domain_interface );
// ****************************************************************************


// ****************************************************************************
// node schedule state
// =====================================
extern const char* sm_node_schedule_state_str( SmNodeScheduleStateT state );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_TYPES_H__
