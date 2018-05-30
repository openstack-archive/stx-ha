//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_LIMITS_H__
#define __SM_LIMITS_H__

#ifdef __cplusplus
extern "C" {
#endif

// Process Limits.
#define SM_PROCESS_NAME_MAX_CHAR                                    32
#define SM_PROCESS_MAX_RECURSIVE_CALL_DEPTH                          4

// Thread Limits.
#define SM_THREADS_MAX                                               8
#define SM_THREAD_NAME_MAX_CHAR                                     32
#define SM_THREAD_SELECT_OBJS_MAX                                   64

// Log Limits.
#define SM_LOGS_MAX                                               2048
#define SM_LOG_DOMAINS_MAX                                           8
#define SM_LOG_DOMAIN_NAME_MAX_CHAR                                 32
#define SM_LOG_SERVICE_GROUP_NAME_MAX_CHAR                          32
#define SM_LOG_ENTITY_NAME_MAX_CHAR                                 64
#define SM_LOG_ENTITY_STATE_MAX_CHAR                                32
#define SM_LOG_ENTITY_STATUS_MAX_CHAR                               32
#define SM_LOG_ENTITY_CONDITION_MAX_CHAR                            32
#define SM_LOG_REASON_TEXT_MAX_CHAR                                256

// Alarm Limits.
#define SM_ALARMS_MAX                                               64
#define SM_ALARM_DOMAINS_MAX                                         8
#define SM_ALARM_DOMAIN_NAME_MAX_CHAR                               32
#define SM_ALARM_ENTITY_NAME_MAX_CHAR                               32
#define SM_ALARM_ENTITY_STATE_MAX_CHAR                              32
#define SM_ALARM_ENTITY_STATUS_MAX_CHAR                             32
#define SM_ALARM_ENTITY_CONDITION_MAX_CHAR                          32
#define SM_ALARM_PROPOSED_REPAIR_ACTION_MAX_CHAR                   256
#define SM_ALARM_SPECIFIC_PROBLEM_TEXT_MAX_CHAR                    256
#define SM_ALARM_ADDITIONAL_TEXT_MAX_CHAR                          256

// ISO8601 -Representation of Dates and Times Limits.
#define SM_ISO8601_DATE_TIME_MAX_CHAR                               64

// Database Limits.
#define SM_DB_DISTINCT_STATEMENT_MAX_CHAR                          512
#define SM_DB_QUERY_STATEMENT_MAX_CHAR                            1024

// SQL Limits.
#define SM_SQL_STATEMENT_MAX_CHAR                                 2048

// Node Limits.
#define SM_NODE_MAX                                                 16
#define SM_NODE_NAME_MAX_CHAR                                       32
#define SM_NODE_ADMIN_STATE_MAX_CHAR                                32
#define SM_NODE_OPERATIONAL_STATE_MAX_CHAR                          32
#define SM_NODE_AVAIL_STATUS_MAX_CHAR                               32
#define SM_NODE_READY_STATE_MAX_CHAR                                32
#define SM_NODE_TYPE_MAX_CHAR                                       64
#define SM_NODE_SUB_FUNCTIONS_MAX_CHAR                             256

// Disk Limits.
#define SM_DISK_MAX                                                 64

// Interface Limits.
#define SM_INTERFACE_MAX                                             4
#define SM_INTERFACE_PEER_MAX           (SM_NODE_MAX*SM_INTERFACE_MAX)
#define SM_INTERFACE_NAME_MAX_CHAR                                  32
#define SM_INTERFACE_STATE_MAX_CHAR                                 32

// Network Address Limits.
#define SM_NETWORK_TYPE_MAX_CHAR                                    32
#define SM_NETWORK_ADDRESS_MAX_CHAR                                256

// Authentication Limits.
#define SM_AUTHENTICATION_TYPE_MAX_CHAR                             32
#define SM_AUTHENTICATION_KEY_MAX_CHAR                             128
#define SM_AUTHENTICATION_VECTOR_MAX_CHAR                           64

// Orchestration Limits.
#define SM_ORCHESTRATION_MAX_CHAR                                   32

// Designation Limits.
#define SM_DESIGNATION_MAX_CHAR                                     32

// Path Limits.
#define SM_PATH_TYPE_MAX_CHAR                                       32

// System mode limits
#define SM_SYSTEM_MODE_MAX_CHAR                                     32

// System type limits
#define SM_SYSTEM_TYPE_MAX_CHAR                                     32

// Service Domain Limits.
#define SM_SERVICE_DOMAIN_PROVISIONED_MAX_CHAR                      32
#define SM_SERVICE_DOMAIN_NAME_MAX_CHAR                             32
#define SM_SERVICE_DOMAIN_INTERFACE_NAME_MAX_CHAR                   32
#define SM_SERVICE_DOMAIN_PREEMPT_MAX_CHAR                          32
#define SM_SERVICE_DOMAIN_STATE_MAX_CHAR                            32
#define SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_MAX_CHAR             32
#define SM_SERVICE_DOMAIN_SCHED_STATE_MAX_CHAR                      32
#define SM_SERVICE_DOMAIN_SCHED_LIST_MAX_CHAR                       32
#define SM_SERVICE_DOMAIN_INTERFACE_PROVISIONED_MAX_CHAR            32

// Service Domain Member Limits.
#define SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_MAX_CHAR          32
#define SM_SERVICE_DOMAIN_MEMBER_PROVISIONED_MAX_CHAR               32

// Service Domain Neighbor Limits.
#define SM_SERVICE_DOMAIN_NEIGHBOR_STATE_MAX_CHAR                   32
#define SM_SERVICE_DOMAIN_NEIGHBOR_EXCHANGE_MASTER_MAX_CHAR         32

// Service Group Limits.
#define SM_SERVICE_GROUP_PROVISIONED_MAX_CHAR                       32
#define SM_SERVICE_GROUP_NAME_MAX_CHAR                              32
#define SM_SERVICE_GROUP_AUTO_RECOVER_MAX_CHAR                      32
#define SM_SERVICE_GROUP_CORE_MAX_CHAR                              32
#define SM_SERVICE_GROUP_STATE_MAX_CHAR                             32
#define SM_SERVICE_GROUP_STATUS_MAX_CHAR                            32
#define SM_SERVICE_GROUP_CONDITION_MAX_CHAR                         32
#define SM_SERVICE_GROUP_ACTION_MAX_CHAR                            32
#define SM_SERVICE_GROUP_AGGREGATE_NAME_MAX_CHAR                    32
#define SM_SERVICE_GROUP_REASON_TEXT_MAX_CHAR                      256
#define SM_SERVICE_GROUP_FATAL_ERROR_REBOOT_MAX_CHAR                32

// Configuration limits
#define SM_CONFIGURATION_KEY_MAX_CHAR                               32
#define SM_CONFIGURATION_VALUE_MAX_CHAR                             32

// Service Group Member Limits.
#define SM_SERVICE_GROUP_MEMBER_PROVISIONED_MAX_CHAR                32

// Service Limits.
#define SM_SERVICE_PROVISIONED_MAX_CHAR                             32
#define SM_SERVICE_NAME_MAX_CHAR                                    32
#define SM_SERVICE_ADMIN_STATE_MAX_CHAR                             32
#define SM_SERVICE_STATE_MAX_CHAR                                   32
#define SM_SERVICE_STATUS_MAX_CHAR                                  32
#define SM_SERVICE_CONDITION_MAX_CHAR                               32
#define SM_SERVICE_SEVERITY_MAX_CHAR                                32
#define SM_SERVICE_PID_FILE_MAX_CHAR                               256

// Service Heartbeat Limits.
#define SM_SERVICE_HEARTBEAT_PROVISIONED_MAX_CHAR                   32
#define SM_SERVICE_HEARTBEAT_NAME_MAX_CHAR                          32
#define SM_SERVICE_HEARTBEAT_TYPE_MAX_CHAR                          32
#define SM_SERVICE_HEARTBEAT_ADDRESS_MAX_CHAR                      256
#define SM_SERVICE_HEARTBEAT_MESSAGE_MAX_CHAR                      256
#define SM_SERVICE_HEARTBEAT_STATE_MAX_CHAR                         32

// Service Dependency Limits.
#define SM_SERVICE_DEPENDENCY_PROVISIONED_MAX_CHAR                  32
#define SM_SERVICE_DEPENDENCY_MAX_CHAR                              32

// Service Instance Limits.
#define SM_SERVICE_INSTANCE_NAME_MAX_CHAR                           32
#define SM_SERVICE_INSTANCE_PARAMS_MAX_CHAR                       1024

// Service Action Limits.
#define SM_SERVICE_ACTION_NAME_MAX_CHAR                             32
#define SM_SERVICE_ACTION_PLUGIN_EXEC_MAX_CHAR                     256 
#define SM_SERVICE_ACTION_PLUGIN_TYPE_MAX_CHAR                      32
#define SM_SERVICE_ACTION_PLUGIN_CLASS_MAX_CHAR                     32
#define SM_SERVICE_ACTION_PLUGIN_NAME_MAX_CHAR                      80
#define SM_SERVICE_ACTION_PLUGIN_COMMAND_MAX_CHAR                   80
#define SM_SERVICE_ACTION_PLUGIN_PARAMS_MAX_CHAR                  1024
#define SM_SERVICE_ACTION_PLUGIN_EXIT_CODE_MAX_CHAR                 10

// Service Action Result Limits.
#define SM_SERVICE_ACTION_RESULT_MAX_CHAR                           32
#define SM_SERVICE_ACTION_RESULT_REASON_TEXT_MAX_CHAR              256

#ifdef __cplusplus
}
#endif

#endif // __SM_LIMITS_H__
