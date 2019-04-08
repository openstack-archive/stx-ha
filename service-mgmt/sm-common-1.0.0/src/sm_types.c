//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_types.h"

#include <stdint.h>
#include <string.h>
#include <glib.h>
#include <arpa/inet.h>

#include "sm_limits.h"
#include "sm_debug.h"

#define SM_VALUE_STR_MAPPING_MAX_CHAR   128

typedef struct
{
    int value;
    char str[SM_VALUE_STR_MAPPING_MAX_CHAR];
} SmValueStrMappingT;

static SmValueStrMappingT 
_sm_node_admin_state_mappings[SM_NODE_ADMIN_STATE_MAX] =
{
    { SM_NODE_ADMIN_STATE_NIL,      "nil"      },
    { SM_NODE_ADMIN_STATE_UNKNOWN,  "unknown"  },
    { SM_NODE_ADMIN_STATE_LOCKED,   "locked"   },
    { SM_NODE_ADMIN_STATE_UNLOCKED, "unlocked" },
};

static SmValueStrMappingT
_sm_node_oper_state_mappings[SM_NODE_OPERATIONAL_STATE_MAX] =
{
    { SM_NODE_OPERATIONAL_STATE_NIL,      "nil"      },
    { SM_NODE_OPERATIONAL_STATE_UNKNOWN,  "unknown"  },
    { SM_NODE_OPERATIONAL_STATE_ENABLED,  "enabled"  },
    { SM_NODE_OPERATIONAL_STATE_DISABLED, "disabled" },
};

static SmValueStrMappingT
_sm_node_avail_status_mappings[SM_NODE_AVAIL_STATUS_MAX] =
{
    { SM_NODE_AVAIL_STATUS_NIL,       "nil"       },
    { SM_NODE_AVAIL_STATUS_UNKNOWN,   "unknown"   },
    { SM_NODE_AVAIL_STATUS_NONE,      ""          },
    { SM_NODE_AVAIL_STATUS_AVAILABLE, "available" },
    { SM_NODE_AVAIL_STATUS_DEGRADED,  "degraded"  },
    { SM_NODE_AVAIL_STATUS_FAILED,    "failed"    },
};

static SmValueStrMappingT
_sm_node_ready_state_mappings[SM_NODE_READY_STATE_MAX] =
{
    { SM_NODE_READY_STATE_NIL,      "nil"      },
    { SM_NODE_READY_STATE_UNKNOWN,  "unknown"  },
    { SM_NODE_READY_STATE_ENABLED,  "enabled"  },
    { SM_NODE_READY_STATE_DISABLED, "disabled" },
};

static SmValueStrMappingT 
_sm_node_event_mappings[SM_NODE_EVENT_MAX] =
{
    { SM_NODE_EVENT_NIL,      "nil"      },
    { SM_NODE_EVENT_UNKNOWN,  "unknown"  },
    { SM_NODE_EVENT_ENABLED,  "enabled"  },
    { SM_NODE_EVENT_DISABLED, "disabled" },
    { SM_NODE_EVENT_AUDIT,    "audit"    },
};

static SmValueStrMappingT 
_sm_interface_state_mappings[SM_INTERFACE_STATE_MAX] =
{
    { SM_INTERFACE_STATE_NIL,           "nil"      },
    { SM_INTERFACE_STATE_UNKNOWN,       "unknown"  },
    { SM_INTERFACE_STATE_ENABLED,       "enabled"  },
    { SM_INTERFACE_STATE_DISABLED,      "disabled" },
    { SM_INTERFACE_STATE_NOT_IN_USE,    "not-in-use"   }
};

static SmValueStrMappingT 
_sm_network_type_mappings[SM_NETWORK_TYPE_MAX] =
{
    { SM_NETWORK_TYPE_NIL,      "nil"      },
    { SM_NETWORK_TYPE_UNKNOWN,  "unknown"  },
    { SM_NETWORK_TYPE_IPV4,     "ipv4"     },
    { SM_NETWORK_TYPE_IPV6,     "ipv6"     },
    { SM_NETWORK_TYPE_IPV4_UDP, "ipv4-udp" },
    { SM_NETWORK_TYPE_IPV6_UDP, "ipv6-udp" },
};

static SmValueStrMappingT 
_sm_path_type_mappings[SM_PATH_TYPE_MAX] =
{
    { SM_PATH_TYPE_NIL,         "nil"         },
    { SM_PATH_TYPE_UNKNOWN,     "unknown"     },
    { SM_PATH_TYPE_PRIMARY,     "primary"     },
    { SM_PATH_TYPE_SECONDARY,   "secondary"   },
    { SM_PATH_TYPE_STATUS_ONLY, "status-only" },
};

static SmValueStrMappingT 
_sm_auth_type_mappings[SM_AUTH_TYPE_MAX] =
{
    { SM_AUTH_TYPE_NIL,         "nil"         },
    { SM_AUTH_TYPE_UNKNOWN,     "unknown"     },
    { SM_AUTH_TYPE_NONE,        "none"        },
    { SM_AUTH_TYPE_HMAC_SHA512, "hmac-sha512" },
};

static SmValueStrMappingT 
_sm_orchestration_type_mappings[SM_ORCHESTRATION_TYPE_MAX] =
{
    { SM_ORCHESTRATION_TYPE_NIL,          "nil"          },
    { SM_ORCHESTRATION_TYPE_UNKNOWN,      "unknown"      },
    { SM_ORCHESTRATION_TYPE_GEOGRAPHICAL, "geographical" },
    { SM_ORCHESTRATION_TYPE_REGIONAL,     "regional"     },
    { SM_ORCHESTRATION_TYPE_HYBRID,       "hybrid"       },
};

static SmValueStrMappingT 
_sm_designation_type_mappings[SM_DESIGNATION_TYPE_MAX] =
{
    { SM_DESIGNATION_TYPE_NIL,     "nil"     },
    { SM_DESIGNATION_TYPE_UNKNOWN, "unknown" },
    { SM_DESIGNATION_TYPE_LEADER,  "leader"  },
    { SM_DESIGNATION_TYPE_BACKUP,  "backup"  },
    { SM_DESIGNATION_TYPE_OTHER,   "other"   },
};

static SmValueStrMappingT 
_sm_service_domain_state_mappings[SM_SERVICE_DOMAIN_STATE_MAX] =
{
    { SM_SERVICE_DOMAIN_STATE_NIL,     "nil"     },
    { SM_SERVICE_DOMAIN_STATE_UNKNOWN, "unknown" },
    { SM_SERVICE_DOMAIN_STATE_INITIAL, "initial" },
    { SM_SERVICE_DOMAIN_STATE_WAITING, "waiting" },
    { SM_SERVICE_DOMAIN_STATE_LEADER,  "leader"  },
    { SM_SERVICE_DOMAIN_STATE_BACKUP,  "backup"  },
    { SM_SERVICE_DOMAIN_STATE_OTHER,   "other"   },
};

static SmValueStrMappingT 
_sm_service_domain_event_mappings[SM_SERVICE_DOMAIN_EVENT_MAX] =
{
    { SM_SERVICE_DOMAIN_EVENT_NIL,                "nil"                },
    { SM_SERVICE_DOMAIN_EVENT_UNKNOWN,            "unknown"            },
    { SM_SERVICE_DOMAIN_EVENT_HELLO_MSG,          "hello-message"      },
    { SM_SERVICE_DOMAIN_EVENT_NEIGHBOR_AGEOUT,    "neighbor-ageout"    },
    { SM_SERVICE_DOMAIN_EVENT_INTERFACE_ENABLED,  "interface-enabled"  },
    { SM_SERVICE_DOMAIN_EVENT_INTERFACE_DISABLED, "interface-disabled" },
    { SM_SERVICE_DOMAIN_EVENT_WAIT_EXPIRED,       "wait-expired"       },
    { SM_SERVICE_DOMAIN_EVENT_CHANGING_LEADER,    "change-leader"      },
};

static SmValueStrMappingT 
_sm_service_domain_interface_event_mappings[SM_SERVICE_DOMAIN_INTERFACE_EVENT_MAX] =
{
    { SM_SERVICE_DOMAIN_INTERFACE_EVENT_NIL,           "nil"           },
    { SM_SERVICE_DOMAIN_INTERFACE_EVENT_UNKNOWN,       "unknown"       },
    { SM_SERVICE_DOMAIN_INTERFACE_EVENT_NODE_ENABLED,  "node-enabled"  },
    { SM_SERVICE_DOMAIN_INTERFACE_EVENT_NODE_DISABLED, "node-disabled" },
    { SM_SERVICE_DOMAIN_INTERFACE_EVENT_ENABLED,       "enabled"       },
    { SM_SERVICE_DOMAIN_INTERFACE_EVENT_DISABLED,      "disabled"      },
    { SM_SERVICE_DOMAIN_INTERFACE_EVENT_AUDIT,         "audit"         },
    { SM_SERVICE_DOMAIN_INTERFACE_EVENT_NOT_IN_USE,    "not-in-use"        }
};

static SmValueStrMappingT
_sm_failover_event_mappings[SM_FAILOVER_EVENT_MAX] =
{
    {SM_FAILOVER_EVENT_HEARTBEAT_ENABLED,       "heartbeat-enabled"},
    {SM_FAILOVER_EVENT_IF_STATE_CHANGED,        "interface-state-changed"},
    {SM_FAILOVER_EVENT_FAIL_PENDING_TIMEOUT,    "fail-pending-timeout"},
    {SM_FAILOVER_EVENT_NODE_ENABLED,            "node-enabled"}
};

static SmValueStrMappingT
_sm_failover_state_mappings[SM_FAILOVER_STATE_MAX] =
{
    {SM_FAILOVER_STATE_INITIAL,                 "initial"},
    {SM_FAILOVER_STATE_NORMAL,                  "normal"},
    {SM_FAILOVER_STATE_FAIL_PENDING,            "fail-pending"},
    {SM_FAILOVER_STATE_FAILED,                  "failed"},
    {SM_FAILOVER_STATE_SURVIVED,                "survived"}
};

static SmValueStrMappingT 
_sm_service_domain_neighbor_state_mappings[SM_SERVICE_DOMAIN_NEIGHBOR_STATE_MAX] =
{
    { SM_SERVICE_DOMAIN_NEIGHBOR_STATE_NIL,            "nil"            },
    { SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN,           "down"           },
    { SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE_START, "exchange-start" },
    { SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE,       "exchange"       },
    { SM_SERVICE_DOMAIN_NEIGHBOR_STATE_FULL,           "full"           },
};

static SmValueStrMappingT 
_sm_service_domain_neighbor_event_mappings[SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_MAX] =
{
    { SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_NIL,                  "nil"                    },
    { SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_UNKNOWN,              "unknown"                },
    { SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_HELLO_MSG,            "hello-message"          },
    { SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_MSG,   "exchange-start-message" },
    { SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_START_TIMER, "exchange-start-timer"   },
    { SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_MSG,         "exchange-message"       },
    { SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_EXCHANGE_TIMEOUT,     "exchange-timeout"       },
    { SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_DOWN,                 "down"                   },
};

static SmValueStrMappingT 
_sm_service_domain_member_redundancy_model_mappings[SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_MAX] =
{
    { SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_NIL,      "nil"     },
    { SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_UNKNOWN,  "unknown" },
    { SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_NONE,     "none"    },
    { SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N,        "N"       },
    { SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_PLUS_M, "N + M"   },
    { SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_TO_1,   "N to 1"  },
    { SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_TO_N,   "N to N"  },
};

static SmValueStrMappingT 
_sm_service_domain_split_brain_recovery_mappings[SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_MAX] =
{
    { SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_NIL,                "nil"                },
    { SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_UNKNOWN,            "unknown"            },
    { SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_DISABLE_ALL_ACTIVE, "disable-all-active" },
    { SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_SELECT_BEST_ACTIVE, "select-best-active" },
};

static SmValueStrMappingT 
_sm_service_group_action_mappings[SM_SERVICE_GROUP_ACTION_MAX] =
{
    { SM_SERVICE_GROUP_ACTION_NIL,        "nil"        },
    { SM_SERVICE_GROUP_ACTION_UNKNOWN,    "unknown"    },
    { SM_SERVICE_GROUP_ACTION_DISABLE,    "disable"    },
    { SM_SERVICE_GROUP_ACTION_GO_ACTIVE,  "go-active"  },
    { SM_SERVICE_GROUP_ACTION_GO_STANDBY, "go-standby" },
    { SM_SERVICE_GROUP_ACTION_AUDIT,      "audit"      },
    { SM_SERVICE_GROUP_ACTION_RECOVER,    "recover"    },
};

static SmValueStrMappingT 
_sm_service_group_state_mappings[SM_SERVICE_GROUP_STATE_MAX] =
{
    { SM_SERVICE_GROUP_STATE_NIL,        "nil"            },
    { SM_SERVICE_GROUP_STATE_NA,         "not-applicable" },
    { SM_SERVICE_GROUP_STATE_INITIAL,    "initial"        },
    { SM_SERVICE_GROUP_STATE_UNKNOWN,    "unknown"        },
    { SM_SERVICE_GROUP_STATE_STANDBY,    "standby"        },
    { SM_SERVICE_GROUP_STATE_GO_STANDBY, "go-standby"     },
    { SM_SERVICE_GROUP_STATE_GO_ACTIVE,  "go-active"      },
    { SM_SERVICE_GROUP_STATE_ACTIVE,     "active"         },
    { SM_SERVICE_GROUP_STATE_DISABLING,  "disabling"      },
    { SM_SERVICE_GROUP_STATE_DISABLED,   "disabled"       },
    { SM_SERVICE_GROUP_STATE_SHUTDOWN,   "shutdown"       },
};

static SmValueStrMappingT 
_sm_service_group_event_mappings[SM_SERVICE_GROUP_EVENT_MAX] =
{
    { SM_SERVICE_GROUP_EVENT_NIL,                  "nil"                   },
    { SM_SERVICE_GROUP_EVENT_UNKNOWN,              "unknown"               },
    { SM_SERVICE_GROUP_EVENT_GO_ACTIVE,            "go-active"             },
    { SM_SERVICE_GROUP_EVENT_GO_STANDBY,           "go-standby"            },
    { SM_SERVICE_GROUP_EVENT_DISABLE,              "disable"               },
    { SM_SERVICE_GROUP_EVENT_AUDIT,                "audit"                 },
    { SM_SERVICE_GROUP_EVENT_SERVICE_SCN,          "service-state-change"  },
    { SM_SERVICE_GROUP_EVENT_SHUTDOWN,             "shutdown"              },
    { SM_SERVICE_GROUP_EVENT_NOTIFICATION_SUCCESS, "notification-success"  },
    { SM_SERVICE_GROUP_EVENT_NOTIFICATION_FAILED,  "notification-failed"   },
    { SM_SERVICE_GROUP_EVENT_NOTIFICATION_TIMEOUT, "notification-timeout"  },
};

static SmValueStrMappingT 
_sm_service_group_status_mappings[SM_SERVICE_GROUP_STATUS_MAX] =
{
    { SM_SERVICE_GROUP_STATUS_NIL,      "nil"      },
    { SM_SERVICE_GROUP_STATUS_UNKNOWN,  "unknown"  },
    { SM_SERVICE_GROUP_STATUS_NONE,     ""         },
    { SM_SERVICE_GROUP_STATUS_WARN,     "warn"     },
    { SM_SERVICE_GROUP_STATUS_DEGRADED, "degraded" },
    { SM_SERVICE_GROUP_STATUS_FAILED,   "failed"   },
};

static SmValueStrMappingT 
_sm_service_group_condition_mappings[SM_SERVICE_GROUP_CONDITION_MAX] =
{
    { SM_SERVICE_GROUP_CONDITION_NIL,               "nil"               },
    { SM_SERVICE_GROUP_CONDITION_UNKNOWN,           "unknown"           },
    { SM_SERVICE_GROUP_CONDITION_NONE,              ""                  },
    { SM_SERVICE_GROUP_CONDITION_DATA_INCONSISTENT, "data-inconsistent" },
    { SM_SERVICE_GROUP_CONDITION_DATA_OUTDATED,     "data-outdated"     },
    { SM_SERVICE_GROUP_CONDITION_DATA_CONSISTENT,   "data-consistent"   },
    { SM_SERVICE_GROUP_CONDITION_DATA_SYNC,         "data-syncing"      },
    { SM_SERVICE_GROUP_CONDITION_DATA_STANDALONE,   "data-standalone"   },
    { SM_SERVICE_GROUP_CONDITION_RECOVERY_FAILURE,  "recovery-failure"  },
    { SM_SERVICE_GROUP_CONDITION_ACTION_FAILURE,    "action-failure"    },
    { SM_SERVICE_GROUP_CONDITION_FATAL_FAILURE,     "fatal-failure"     },
};

static SmValueStrMappingT 
_sm_service_group_notification_mappings[SM_SERVICE_GROUP_NOTIFICATION_MAX] =
{
    { SM_SERVICE_GROUP_NOTIFICATION_NIL,        "nil"        },
    { SM_SERVICE_GROUP_NOTIFICATION_UNKNOWN,    "unknown"    },
    { SM_SERVICE_GROUP_NOTIFICATION_STANDBY,    "standby"    },
    { SM_SERVICE_GROUP_NOTIFICATION_GO_STANDBY, "go-standby" },
    { SM_SERVICE_GROUP_NOTIFICATION_GO_ACTIVE,  "go-active"  },
    { SM_SERVICE_GROUP_NOTIFICATION_ACTIVE,     "active"     },
    { SM_SERVICE_GROUP_NOTIFICATION_DISABLING,  "disabling"  },
    { SM_SERVICE_GROUP_NOTIFICATION_DISABLED,   "disabled"   },
    { SM_SERVICE_GROUP_NOTIFICATION_SHUTDOWN,   "shutdown"   },
};

static SmValueStrMappingT
_sm_service_admin_state_mappings[SM_SERVICE_ADMIN_STATE_MAX] =
{
    { SM_SERVICE_ADMIN_STATE_NIL,      "nil"            },
    { SM_SERVICE_ADMIN_STATE_NA,       "not-applicable" },
    { SM_SERVICE_ADMIN_STATE_LOCKED,   "initial"        },
    { SM_SERVICE_ADMIN_STATE_UNLOCKED, "unknown"        },
};

static SmValueStrMappingT 
_sm_service_state_mappings[SM_SERVICE_STATE_MAX] =
{
    { SM_SERVICE_STATE_NIL,                "nil"                },
    { SM_SERVICE_STATE_NA,                 "not-applicable"     },
    { SM_SERVICE_STATE_INITIAL,            "initial"            },
    { SM_SERVICE_STATE_UNKNOWN,            "unknown"            },
    { SM_SERVICE_STATE_ENABLED_STANDBY,    "enabled-standby"    },
    { SM_SERVICE_STATE_ENABLED_GO_STANDBY, "enabled-go-standby" },
    { SM_SERVICE_STATE_ENABLED_GO_ACTIVE,  "enabled-go-active"  },
    { SM_SERVICE_STATE_ENABLED_ACTIVE,     "enabled-active"     },
    { SM_SERVICE_STATE_ENABLING,           "enabling"           },
    { SM_SERVICE_STATE_ENABLING_THROTTLE,  "enabling-throttle"  },
    { SM_SERVICE_STATE_DISABLING,          "disabling"          },
    { SM_SERVICE_STATE_DISABLED,           "disabled"           },
    { SM_SERVICE_STATE_SHUTDOWN,           "shutdown"           },
};

static SmValueStrMappingT 
_sm_service_event_mappings[SM_SERVICE_EVENT_MAX] =
{
    { SM_SERVICE_EVENT_NIL,                "nil"                },
    { SM_SERVICE_EVENT_UNKNOWN,            "unknown"            },
    { SM_SERVICE_EVENT_ENABLE_THROTTLE,    "enable-throttle"    },
    { SM_SERVICE_EVENT_ENABLE,             "enable"             },
    { SM_SERVICE_EVENT_ENABLE_SUCCESS,     "enable-success"     },
    { SM_SERVICE_EVENT_ENABLE_FAILED,      "enable-failed"      },
    { SM_SERVICE_EVENT_ENABLE_TIMEOUT,     "enable-timeout"     },
    { SM_SERVICE_EVENT_GO_ACTIVE,          "go-active"          },
    { SM_SERVICE_EVENT_GO_ACTIVE_SUCCESS,  "go-active-success"  },
    { SM_SERVICE_EVENT_GO_ACTIVE_FAILED,   "go-active-failed"   },
    { SM_SERVICE_EVENT_GO_ACTIVE_TIMEOUT,  "go-active-timeout"  },
    { SM_SERVICE_EVENT_GO_STANDBY,         "go-standby"         },
    { SM_SERVICE_EVENT_GO_STANDBY_SUCCESS, "go-standby-success" },
    { SM_SERVICE_EVENT_GO_STANDBY_FAILED,  "go-standby-failed"  },
    { SM_SERVICE_EVENT_GO_STANDBY_TIMEOUT, "go-standby-timeout" },
    { SM_SERVICE_EVENT_DISABLE,            "disable"            },
    { SM_SERVICE_EVENT_DISABLE_SUCCESS,    "disable-success"    },
    { SM_SERVICE_EVENT_DISABLE_FAILED,     "disable-failed"     },
    { SM_SERVICE_EVENT_DISABLE_TIMEOUT,    "disable-timeout"    },
    { SM_SERVICE_EVENT_AUDIT,              "audit"              },
    { SM_SERVICE_EVENT_AUDIT_SUCCESS,      "audit-success"      },
    { SM_SERVICE_EVENT_AUDIT_FAILED,       "audit-failed"       },
    { SM_SERVICE_EVENT_AUDIT_TIMEOUT,      "audit-timeout"      },
    { SM_SERVICE_EVENT_AUDIT_MISMATCH,     "audit-mismatch"     },
    { SM_SERVICE_EVENT_HEARTBEAT_OKAY,     "heartbeat-okay"     },
    { SM_SERVICE_EVENT_HEARTBEAT_WARN,     "heartbeat-warn"     },
    { SM_SERVICE_EVENT_HEARTBEAT_DEGRADE,  "heartbeat-degrade"  },
    { SM_SERVICE_EVENT_HEARTBEAT_FAIL,     "heartbeat-fail"     },
    { SM_SERVICE_EVENT_PROCESS_FAILURE,    "process-failure"    },
    { SM_SERVICE_EVENT_SHUTDOWN,           "shutdown"           },
};

static SmValueStrMappingT 
_sm_service_status_mappings[SM_SERVICE_STATUS_MAX] =
{
    { SM_SERVICE_STATUS_NIL,      "nil"      },
    { SM_SERVICE_STATUS_UNKNOWN,  "unknown"  },
    { SM_SERVICE_STATUS_NONE,     ""         },
    { SM_SERVICE_STATUS_WARN,     "warn"     },
    { SM_SERVICE_STATUS_DEGRADED, "degraded" },
    { SM_SERVICE_STATUS_FAILED,   "failed"   },
};

static SmValueStrMappingT 
_sm_service_condition_mappings[SM_SERVICE_CONDITION_MAX] =
{
    { SM_SERVICE_CONDITION_NIL,               "nil"               },
    { SM_SERVICE_CONDITION_UNKNOWN,           "unknown"           },
    { SM_SERVICE_CONDITION_NONE,              ""                  },
    // Degraded Conditions.
    { SM_SERVICE_CONDITION_DATA_INCONSISTENT, "data-inconsistent" },
    { SM_SERVICE_CONDITION_DATA_OUTDATED,     "data-outdated"     },
    { SM_SERVICE_CONDITION_DATA_CONSISTENT,   "data-consistent"   },
    { SM_SERVICE_CONDITION_DATA_SYNC,         "data-syncing"      },
    { SM_SERVICE_CONDITION_DATA_STANDALONE,   "data-standalone"   },
    // Failed Conditions.
    { SM_SERVICE_CONDITION_RECOVERY_FAILURE,  "recovery-failure"  },
    { SM_SERVICE_CONDITION_ACTION_FAILURE,    "action-failure"    },
    { SM_SERVICE_CONDITION_FATAL_FAILURE,     "fatal-failure"     },
};

static SmValueStrMappingT 
_sm_service_severity_mappings[SM_SERVICE_SEVERITY_MAX] =
{
    { SM_SERVICE_SEVERITY_NIL,      "nil"      },
    { SM_SERVICE_SEVERITY_UNKNOWN,  "unknown"  },
    { SM_SERVICE_SEVERITY_NONE,     "none"     },
    { SM_SERVICE_SEVERITY_MINOR,    "minor"    },
    { SM_SERVICE_SEVERITY_MAJOR,    "major"    },
    { SM_SERVICE_SEVERITY_CRITICAL, "critical" },
};

static SmValueStrMappingT 
_sm_service_heartbeat_type_mappings[SM_SERVICE_HEARTBEAT_TYPE_MAX] =
{
    { SM_SERVICE_HEARTBEAT_TYPE_NIL,     "nil"     },
    { SM_SERVICE_HEARTBEAT_TYPE_UNKNOWN, "unknown" },
    { SM_SERVICE_HEARTBEAT_TYPE_UNIX,    "unix"    },
    { SM_SERVICE_HEARTBEAT_TYPE_UDP,     "udp"     },
};

static SmValueStrMappingT 
_sm_service_heartbeat_state_mappings[SM_SERVICE_HEARTBEAT_STATE_MAX] =
{
    { SM_SERVICE_HEARTBEAT_STATE_NIL,     "nil"     },
    { SM_SERVICE_HEARTBEAT_STATE_UNKNOWN, "unknown" },
    { SM_SERVICE_HEARTBEAT_STATE_STARTED, "started" },
    { SM_SERVICE_HEARTBEAT_STATE_STOPPED, "stopped" },
};

static SmValueStrMappingT 
_sm_service_dependency_type_mappings[SM_SERVICE_DEPENDENCY_TYPE_MAX] =
{
    { SM_SERVICE_DEPENDENCY_TYPE_NIL,     "nil"     },
    { SM_SERVICE_DEPENDENCY_TYPE_UNKNOWN, "unknown" },
    { SM_SERVICE_DEPENDENCY_TYPE_ACTION,  "action"  },
    { SM_SERVICE_DEPENDENCY_TYPE_STATE,   "state"   },
};

static SmValueStrMappingT 
_sm_service_action_mappings[SM_SERVICE_ACTION_MAX] =
{
    { SM_SERVICE_ACTION_NIL,            "nil"            },
    { SM_SERVICE_ACTION_NA,             "not-applicable" },
    { SM_SERVICE_ACTION_UNKNOWN,        "unknown"        },
    { SM_SERVICE_ACTION_NONE,           "none"           },
    { SM_SERVICE_ACTION_ENABLE,         "enable"         },
    { SM_SERVICE_ACTION_DISABLE,        "disable"        },
    { SM_SERVICE_ACTION_GO_ACTIVE,      "go-active"      },
    { SM_SERVICE_ACTION_GO_STANDBY,     "go-standby"     },
    { SM_SERVICE_ACTION_AUDIT_ENABLED,  "audit-enabled"  },
    { SM_SERVICE_ACTION_AUDIT_DISABLED, "audit-disabled" },
};

static SmValueStrMappingT 
_sm_service_action_result_mappings[SM_SERVICE_ACTION_RESULT_MAX] =
{
    { SM_SERVICE_ACTION_RESULT_NIL,     "nil"     },
    { SM_SERVICE_ACTION_RESULT_UNKNOWN, "unknown" },
    { SM_SERVICE_ACTION_RESULT_SUCCESS, "success" },
    { SM_SERVICE_ACTION_RESULT_FATAL,   "fatal"   },
    { SM_SERVICE_ACTION_RESULT_FAILED,  "failed"  },
    { SM_SERVICE_ACTION_RESULT_TIMEOUT, "timeout" },
};

static SmValueStrMappingT 
_sm_service_domain_scheduling_state_mappings[SM_SERVICE_DOMAIN_SCHEDULING_STATE_MAX] =
{
    { SM_SERVICE_DOMAIN_SCHEDULING_STATE_NIL,             "nil"           },
    { SM_SERVICE_DOMAIN_SCHEDULING_STATE_UNKNOWN,         "unknown"       },
    { SM_SERVICE_DOMAIN_SCHEDULING_STATE_NONE,            "none"          },
    { SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT,           "swact"         },
    { SM_SERVICE_DOMAIN_SCHEDULING_STATE_SWACT_FORCE,     "swact-force"   },
    { SM_SERVICE_DOMAIN_SCHEDULING_STATE_DISABLE,         "disable"       },
    { SM_SERVICE_DOMAIN_SCHEDULING_STATE_DISABLE_FORCE,   "disable-force" },
};

static SmValueStrMappingT 
_sm_service_domain_scheduling_list_mappings[] =
{
    { SM_SERVICE_DOMAIN_SCHEDULING_LIST_NIL,         "nil"         },
    { SM_SERVICE_DOMAIN_SCHEDULING_LIST_ACTIVE,      "active"      },
    { SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_ACTIVE,   "go-active"   },
    { SM_SERVICE_DOMAIN_SCHEDULING_LIST_GO_STANDBY,  "go-standby"  },
    { SM_SERVICE_DOMAIN_SCHEDULING_LIST_STANDBY,     "standby"     },
    { SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLING,   "disabling"   },
    { SM_SERVICE_DOMAIN_SCHEDULING_LIST_DISABLED,    "disabled"    },
    { SM_SERVICE_DOMAIN_SCHEDULING_LIST_FAILED,      "failed"      },
    { SM_SERVICE_DOMAIN_SCHEDULING_LIST_FATAL,       "fatal"       },
    { SM_SERVICE_DOMAIN_SCHEDULING_LIST_UNAVAILABLE, "unavailable" },
};

static SmValueStrMappingT _sm_error_mappings[SM_ERROR_MAX] =
{
    { SM_OKAY,            "OKAY"            },
    { SM_NOT_FOUND,       "NOT_FOUND"       },
    { SM_NO_MSG,          "NO_MSG"          },
    { SM_NOT_IMPLEMENTED, "NOT_IMPLEMENTED" },
    { SM_FAILED,          "FAILED"          },
};

typedef struct
{
    SmNodeAdminStateT admin_state;
    SmNodeReadyStateT ready_state;
    char str[SM_VALUE_STR_MAPPING_MAX_CHAR];
} SmNodeStateStrMappingT;

static SmNodeStateStrMappingT 
_sm_node_state_mappings[SM_NODE_ADMIN_STATE_MAX*SM_NODE_READY_STATE_MAX] =
{
    { SM_NODE_ADMIN_STATE_NIL,      SM_NODE_READY_STATE_NIL,      "nil-nil"           },
    { SM_NODE_ADMIN_STATE_NIL,      SM_NODE_READY_STATE_UNKNOWN,  "nil-unknown"       },
    { SM_NODE_ADMIN_STATE_NIL,      SM_NODE_READY_STATE_ENABLED,  "nil-enabled"       },
    { SM_NODE_ADMIN_STATE_NIL,      SM_NODE_READY_STATE_DISABLED, "nil-disabled"      },
    { SM_NODE_ADMIN_STATE_UNKNOWN,  SM_NODE_READY_STATE_NIL,      "unknown-nil"       },
    { SM_NODE_ADMIN_STATE_UNKNOWN,  SM_NODE_READY_STATE_UNKNOWN,  "unknown-unknown"   },
    { SM_NODE_ADMIN_STATE_UNKNOWN,  SM_NODE_READY_STATE_ENABLED,  "unknown-enabled"   },
    { SM_NODE_ADMIN_STATE_UNKNOWN,  SM_NODE_READY_STATE_DISABLED, "unknown-disabled"  },
    { SM_NODE_ADMIN_STATE_LOCKED,   SM_NODE_READY_STATE_NIL,      "locked-nil"        },
    { SM_NODE_ADMIN_STATE_LOCKED,   SM_NODE_READY_STATE_UNKNOWN,  "locked-unknown"    },
    { SM_NODE_ADMIN_STATE_LOCKED,   SM_NODE_READY_STATE_ENABLED,  "locked-enabled"    },
    { SM_NODE_ADMIN_STATE_LOCKED,   SM_NODE_READY_STATE_DISABLED, "locked-disabled"   },
    { SM_NODE_ADMIN_STATE_UNLOCKED, SM_NODE_READY_STATE_NIL,      "unlocked-nil"      },
    { SM_NODE_ADMIN_STATE_UNLOCKED, SM_NODE_READY_STATE_UNKNOWN,  "unlocked-unknown"  },
    { SM_NODE_ADMIN_STATE_UNLOCKED, SM_NODE_READY_STATE_ENABLED,  "unlocked-enabled"  },
    { SM_NODE_ADMIN_STATE_UNLOCKED, SM_NODE_READY_STATE_DISABLED, "unlocked-disabled" },
};

static SmValueStrMappingT _sm_system_mode_mapping[SM_SYSTEM_MODE_MAX] =
{
    { SM_SYSTEM_MODE_UNKNOWN,       "UNKNOWN"       },
    { SM_SYSTEM_MODE_STANDARD,      "standard"      },
    { SM_SYSTEM_MODE_CPE_DUPLEX,    "duplex"        },
    { SM_SYSTEM_MODE_CPE_DUPLEX_DC, "duplex-direct" },
    { SM_SYSTEM_MODE_CPE_SIMPLEX,   "simplex"       },
};

static SmValueStrMappingT
 _sm_node_schedule_state_mappings[SM_NODE_STATE_MAX] =
{
    { SM_NODE_STATE_UNKNOWN, "unknown"  },
    { SM_NODE_STATE_ACTIVE,  "active"   },
    { SM_NODE_STATE_STANDBY, "standby"  },
    { SM_NODE_STATE_INIT,    "init"     },
    { SM_NODE_STATE_FAILED,  "failed"   }
};

// ****************************************************************************
// Types - Mapping Get String
// ==========================
static const char* sm_mapping_get_str( SmValueStrMappingT mappings[], 
    unsigned int num_mappings, int value )
{
    unsigned int map_i;
    for( map_i=0; num_mappings > map_i; ++map_i )
    {
        if( value == mappings[map_i].value )
        {
            return( &(mappings[map_i].str[0]) );
        }
    }

    return( "???" );
}
// ****************************************************************************

// ****************************************************************************
// Types - Mapping Get Value
// =========================
static int sm_mapping_get_value( SmValueStrMappingT mappings[],
    unsigned int num_mappings, const char* str )
{
    unsigned int map_i;
    for( map_i=0; num_mappings > map_i; ++map_i )
    {
        if( 0 == strcmp( &(mappings[map_i].str[0]), str ) )
        {
            return( mappings[map_i].value );
        }
    }

    return( 0 );
}
// ****************************************************************************

// ****************************************************************************
// Types - Node Administrative State Value
// =======================================
SmNodeAdminStateT sm_node_admin_state_value( const char* state_str )
{
    return( (SmNodeAdminStateT) 
            sm_mapping_get_value( _sm_node_admin_state_mappings,
                                  SM_NODE_ADMIN_STATE_MAX,
                                  state_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Node Administrative State String
// ========================================
const char* sm_node_admin_state_str( SmNodeAdminStateT state )
{
    return( sm_mapping_get_str( _sm_node_admin_state_mappings, 
                                SM_NODE_ADMIN_STATE_MAX,
                                state ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Node Operational State Value
// ====================================
SmNodeOperationalStateT sm_node_oper_state_value( const char* state_str )
{
    return( (SmNodeOperationalStateT) 
            sm_mapping_get_value( _sm_node_oper_state_mappings,
                                  SM_NODE_OPERATIONAL_STATE_MAX,
                                  state_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Node Operational State String
// =====================================
const char* sm_node_oper_state_str( SmNodeOperationalStateT state )
{
    return( sm_mapping_get_str( _sm_node_oper_state_mappings, 
                                SM_NODE_OPERATIONAL_STATE_MAX,
                                state ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Node Availability Status Value
// ======================================
SmNodeAvailStatusT sm_node_avail_status_value( const char* status_str )
{
    return( (SmNodeAvailStatusT) 
            sm_mapping_get_value( _sm_node_avail_status_mappings,
                                  SM_NODE_AVAIL_STATUS_MAX,
                                  status_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Node Availability Status String
// =======================================
const char* sm_node_avail_status_str( SmNodeAvailStatusT status )
{
    return( sm_mapping_get_str( _sm_node_avail_status_mappings, 
                                SM_NODE_AVAIL_STATUS_MAX,
                                status ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Node Ready State Value
// ==============================
SmNodeReadyStateT sm_node_ready_state_value( const char* state_str )
{
    return( (SmNodeReadyStateT) 
            sm_mapping_get_value( _sm_node_ready_state_mappings,
                                  SM_NODE_READY_STATE_MAX,
                                  state_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Node Ready State String
// ===============================
const char* sm_node_ready_state_str( SmNodeReadyStateT state )
{
    return( sm_mapping_get_str( _sm_node_ready_state_mappings, 
                                SM_NODE_READY_STATE_MAX,
                                state ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Node State String
// =========================
const char* sm_node_state_str( SmNodeAdminStateT admin_state,
    SmNodeReadyStateT ready_state )
{
    unsigned int map_i;
    for( map_i=0; (SM_NODE_ADMIN_STATE_MAX*SM_NODE_READY_STATE_MAX) > map_i;
         ++map_i )
    {
        if(( admin_state == _sm_node_state_mappings[map_i].admin_state )&&
           ( ready_state == _sm_node_state_mappings[map_i].ready_state ))
        {
            return( &(_sm_node_state_mappings[map_i].str[0]) );
        }
    }

    return( "???" );
}
// ****************************************************************************

// ****************************************************************************
// Types - Node Event Value
// ========================
SmNodeEventT sm_node_event_value( const char* event_str )
{
    return( (SmNodeEventT) 
            sm_mapping_get_value( _sm_node_event_mappings,
                                  SM_NODE_EVENT_MAX, event_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Node Event String
// =========================
const char* sm_node_event_str( SmNodeEventT event )
{
    return( sm_mapping_get_str( _sm_node_event_mappings,
                                SM_NODE_EVENT_MAX, event ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Interface State Value
// =============================
SmInterfaceStateT sm_interface_state_value( const char* state_str )
{
    return( (SmInterfaceStateT) 
            sm_mapping_get_value( _sm_interface_state_mappings,
                                  SM_INTERFACE_STATE_MAX, state_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Interface State String
// ==============================
const char* sm_interface_state_str( SmInterfaceStateT state )
{
    return( sm_mapping_get_str( _sm_interface_state_mappings,
                                SM_INTERFACE_STATE_MAX, state ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Network Type Value
// ==========================
SmNetworkTypeT sm_network_type_value( const char* network_type_str )
{
    return( (SmNetworkTypeT) 
            sm_mapping_get_value( _sm_network_type_mappings,
                                  SM_NETWORK_TYPE_MAX, network_type_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Network Type String
// ===========================
const char* sm_network_type_str( SmNetworkTypeT network_type )
{
    return( sm_mapping_get_str( _sm_network_type_mappings,
                                SM_NETWORK_TYPE_MAX, network_type ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Path Type Value
// =======================
SmPathTypeT sm_path_type_value( const char* path_type_str )
{
    return( (SmPathTypeT) 
            sm_mapping_get_value( _sm_path_type_mappings,
                                  SM_PATH_TYPE_MAX, path_type_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Path Type String
// ========================
const char* sm_path_type_str( SmPathTypeT path_type )
{
    return( sm_mapping_get_str( _sm_path_type_mappings,
                                SM_PATH_TYPE_MAX, path_type ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Authentication Type Value
// =================================
SmAuthTypeT sm_auth_type_value( const char* auth_type_str )
{
    return( (SmAuthTypeT) 
            sm_mapping_get_value( _sm_auth_type_mappings,
                                  SM_AUTH_TYPE_MAX, auth_type_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Authentication Type String
// ==================================
const char* sm_auth_type_str( SmAuthTypeT auth_type )
{
    return( sm_mapping_get_str( _sm_auth_type_mappings,
                                SM_AUTH_TYPE_MAX, auth_type ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Orchestration Type Value
// ================================
SmOrchestrationTypeT sm_orchestration_type_value( 
    const char* orchestration_type_str )
{
    return( (SmOrchestrationTypeT) 
            sm_mapping_get_value( _sm_orchestration_type_mappings,
                                  SM_ORCHESTRATION_TYPE_MAX, 
                                  orchestration_type_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Orchestration Type String
// =================================
const char* sm_orchestration_type_str( SmOrchestrationTypeT orchestration_type )
{
    return( sm_mapping_get_str( _sm_orchestration_type_mappings,
                                SM_ORCHESTRATION_TYPE_MAX, 
                                orchestration_type ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Designation Type Value
// ==============================
SmDesignationTypeT sm_designation_type_value( 
    const char* designation_type_str )
{
    return( (SmDesignationTypeT) 
            sm_mapping_get_value( _sm_designation_type_mappings,
                                  SM_DESIGNATION_TYPE_MAX, 
                                  designation_type_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Designation Type String
// ===============================
const char* sm_designation_type_str( SmDesignationTypeT designation_type )
{
    return( sm_mapping_get_str( _sm_designation_type_mappings,
                                SM_DESIGNATION_TYPE_MAX, designation_type ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain State Value
// ==================================
SmServiceDomainStateT sm_service_domain_state_value( const char* state_str )
{
    return( (SmServiceDomainStateT) 
            sm_mapping_get_value( _sm_service_domain_state_mappings,
                                  SM_SERVICE_DOMAIN_STATE_MAX, state_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain State String
// ===================================
const char* sm_service_domain_state_str( SmServiceDomainStateT state )
{
    return( sm_mapping_get_str( _sm_service_domain_state_mappings,
                                SM_SERVICE_DOMAIN_STATE_MAX, state ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Event Value
// ==================================
SmServiceDomainEventT sm_service_domain_event_value( const char* event_str )
{
    return( (SmServiceDomainEventT) 
            sm_mapping_get_value( _sm_service_domain_event_mappings,
                                  SM_SERVICE_DOMAIN_EVENT_MAX, event_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Event String
// ===================================
const char* sm_service_domain_event_str( SmServiceDomainEventT event )
{
    return( sm_mapping_get_str( _sm_service_domain_event_mappings,
                                SM_SERVICE_DOMAIN_EVENT_MAX, event ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Scheduling State Value
// =============================================
SmServiceDomainSchedulingStateT sm_service_domain_scheduling_state_value(
    const char* sched_state_str )
{
    return( (SmServiceDomainSchedulingStateT) 
            sm_mapping_get_value( _sm_service_domain_scheduling_state_mappings,
                                  SM_SERVICE_DOMAIN_SCHEDULING_STATE_MAX, 
                                  sched_state_str ) );
}
// ****************************************************************************

// ***************************************************************************
// Types - Service Domain Scheduling State String
// ==============================================
const char* sm_service_domain_scheduling_state_str(
    SmServiceDomainSchedulingStateT sched_state )
{
    return( sm_mapping_get_str( _sm_service_domain_scheduling_state_mappings,
                                SM_SERVICE_DOMAIN_SCHEDULING_STATE_MAX,
                                sched_state ) );
}
// ***************************************************************************

// ****************************************************************************
// Types - Service Domain Scheduling List Value
// ============================================
SmServiceDomainSchedulingListT sm_service_domain_scheduling_list_value(
    const char* sched_list_str )
{
    return( (SmServiceDomainSchedulingListT) 
            sm_mapping_get_value( _sm_service_domain_scheduling_list_mappings,
                                  SM_SERVICE_DOMAIN_SCHEDULING_LIST_MAX, 
                                  sched_list_str ) );
}
// ****************************************************************************

// ***************************************************************************
// Types - Service Domain Scheduling List String
// =============================================
const char* sm_service_domain_scheduling_list_str(
    SmServiceDomainSchedulingListT sched_list )
{
    return( sm_mapping_get_str( _sm_service_domain_scheduling_list_mappings,
                                SM_SERVICE_DOMAIN_SCHEDULING_LIST_MAX,
                                sched_list ) );
}
// ***************************************************************************

// ****************************************************************************
// Types - Service Domain Interface Event Value
// ============================================
SmServiceDomainInterfaceEventT sm_service_domain_interface_event_value(
    const char* event_str )
{
    return( (SmServiceDomainInterfaceEventT) 
            sm_mapping_get_value( _sm_service_domain_interface_event_mappings,
                                  SM_SERVICE_DOMAIN_INTERFACE_EVENT_MAX,
                                  event_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Interface Event String
// =============================================
const char* sm_service_domain_interface_event_str(
    SmServiceDomainInterfaceEventT event )
{
    return( sm_mapping_get_str( _sm_service_domain_interface_event_mappings,
                                SM_SERVICE_DOMAIN_INTERFACE_EVENT_MAX,
                                event ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Failover Event String
// =============================================
const char* sm_failover_event_str( SmFailoverEventT event )
{
    return( sm_mapping_get_str( _sm_failover_event_mappings,
                                SM_FAILOVER_EVENT_MAX,
                                event ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Failover State String
// =============================================
const char* sm_failover_state_str( SmFailoverStateT state )
{
    return( sm_mapping_get_str( _sm_failover_state_mappings,
                                SM_FAILOVER_STATE_MAX,
                                state ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Neighbor State Value
// ===========================================
SmServiceDomainNeighborStateT sm_service_domain_neighbor_state_value(
    const char* neighbor_state_str )
{
    return( (SmServiceDomainNeighborStateT) 
            sm_mapping_get_value( _sm_service_domain_neighbor_state_mappings,
                                  SM_SERVICE_DOMAIN_NEIGHBOR_STATE_MAX, 
                                  neighbor_state_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Neighbor State String
// ============================================
const char* sm_service_domain_neighbor_state_str(
    SmServiceDomainNeighborStateT neighbor_state )
{
    return( sm_mapping_get_str( _sm_service_domain_neighbor_state_mappings,
                                SM_SERVICE_DOMAIN_NEIGHBOR_STATE_MAX,
                                neighbor_state ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Neighbor Event Value
// ===========================================
SmServiceDomainNeighborEventT sm_service_domainneighbor_event_value(
    const char* neighbor_event_str )
{
    return( (SmServiceDomainNeighborEventT) 
            sm_mapping_get_value( _sm_service_domain_neighbor_event_mappings,
                                  SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_MAX, 
                                  neighbor_event_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Neighbor Event String
// ============================================
const char* sm_service_domain_neighbor_event_str(
    SmServiceDomainNeighborEventT neighbor_event )
{
    return( sm_mapping_get_str( _sm_service_domain_neighbor_event_mappings,
                                SM_SERVICE_DOMAIN_NEIGHBOR_EVENT_MAX,
                                neighbor_event ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Member Redundancy Model Value
// ====================================================
SmServiceDomainMemberRedundancyModelT sm_service_domain_member_redundancy_model_value(
    const char* model_str )
{
    return( (SmServiceDomainMemberRedundancyModelT)
            sm_mapping_get_value( _sm_service_domain_member_redundancy_model_mappings,
                                  SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_MAX,
                                  model_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Member Redundancy Model String
// =====================================================
const char* sm_service_domain_member_redundancy_model_str(
    SmServiceDomainMemberRedundancyModelT model )
{
    return( sm_mapping_get_str( _sm_service_domain_member_redundancy_model_mappings,
                                SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_MAX,
                                model ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Split Brain Recovery Value
// =================================================
SmServiceDomainSplitBrainRecoveryT sm_service_domain_split_brain_recovery_value(
    const char* recovery_str )
{
    return( (SmServiceDomainSplitBrainRecoveryT)
            sm_mapping_get_value( _sm_service_domain_split_brain_recovery_mappings,
                                  SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_MAX,
                                  recovery_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Domain Split Brain Recovery String
// ==================================================
const char* sm_service_domain_split_brain_recovery_str(
    SmServiceDomainSplitBrainRecoveryT recovery )
{
    return( sm_mapping_get_str( _sm_service_domain_split_brain_recovery_mappings,
                                SM_SERVICE_DOMAIN_SPLIT_BRAIN_RECOVERY_MAX,
                                recovery ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Action Value
// ==================================
SmServiceGroupActionT sm_service_group_action_value( 
    const char* action_str )
{
    return( (SmServiceGroupActionT)
            sm_mapping_get_value( _sm_service_group_action_mappings,
                                  SM_SERVICE_GROUP_ACTION_MAX, action_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Action String
// ===================================
const char* sm_service_group_action_str( SmServiceGroupActionT action )
{
    return( sm_mapping_get_str( _sm_service_group_action_mappings, 
                                SM_SERVICE_GROUP_ACTION_MAX, action ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group State Value
// =================================
SmServiceGroupStateT sm_service_group_state_value( 
    const char* state_str )
{
    return( (SmServiceGroupStateT)
            sm_mapping_get_value( _sm_service_group_state_mappings,
                                  SM_SERVICE_GROUP_STATE_MAX, state_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group State String
// ==================================
const char* sm_service_group_state_str( SmServiceGroupStateT state )
{
    return( sm_mapping_get_str( _sm_service_group_state_mappings, 
                                SM_SERVICE_GROUP_STATE_MAX, state ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group State Lesser Value
// ========================================
SmServiceGroupStateT sm_service_group_state_lesser(
    SmServiceGroupStateT state_a, SmServiceGroupStateT state_b )
{
    SmServiceGroupStateT lesser_state = SM_SERVICE_GROUP_STATE_UNKNOWN;

    switch( state_a )
    {
        case SM_SERVICE_GROUP_STATE_NIL:
            lesser_state = state_b;
        break;

        case SM_SERVICE_GROUP_STATE_INITIAL:
            if( SM_SERVICE_GROUP_STATE_NIL == state_b )
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_GROUP_STATE_UNKNOWN:
            if(( SM_SERVICE_GROUP_STATE_NIL     == state_b )||
               ( SM_SERVICE_GROUP_STATE_INITIAL == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_GROUP_STATE_DISABLED:
            if(( SM_SERVICE_GROUP_STATE_NIL     == state_b )||
               ( SM_SERVICE_GROUP_STATE_INITIAL == state_b )||
               ( SM_SERVICE_GROUP_STATE_UNKNOWN == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_GROUP_STATE_DISABLING:
            if(( SM_SERVICE_GROUP_STATE_NIL      == state_b )||
               ( SM_SERVICE_GROUP_STATE_INITIAL  == state_b )||
               ( SM_SERVICE_GROUP_STATE_UNKNOWN  == state_b )||
               ( SM_SERVICE_GROUP_STATE_DISABLED == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_GROUP_STATE_STANDBY:
            if(( SM_SERVICE_GROUP_STATE_NIL       == state_b )||
               ( SM_SERVICE_GROUP_STATE_INITIAL   == state_b )||
               ( SM_SERVICE_GROUP_STATE_UNKNOWN   == state_b )||
               ( SM_SERVICE_GROUP_STATE_DISABLING == state_b )||
               ( SM_SERVICE_GROUP_STATE_DISABLED  == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_GROUP_STATE_GO_ACTIVE:
            if(( SM_SERVICE_GROUP_STATE_NIL       == state_b )||
               ( SM_SERVICE_GROUP_STATE_INITIAL   == state_b )||
               ( SM_SERVICE_GROUP_STATE_UNKNOWN   == state_b )||
               ( SM_SERVICE_GROUP_STATE_DISABLING == state_b )||
               ( SM_SERVICE_GROUP_STATE_DISABLED  == state_b )||
               ( SM_SERVICE_GROUP_STATE_STANDBY   == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_GROUP_STATE_GO_STANDBY:
            if(( SM_SERVICE_GROUP_STATE_NIL       == state_b )||
               ( SM_SERVICE_GROUP_STATE_INITIAL   == state_b )||
               ( SM_SERVICE_GROUP_STATE_UNKNOWN   == state_b )||
               ( SM_SERVICE_GROUP_STATE_DISABLING == state_b )||
               ( SM_SERVICE_GROUP_STATE_DISABLED  == state_b )||
               ( SM_SERVICE_GROUP_STATE_STANDBY   == state_b )||
               ( SM_SERVICE_GROUP_STATE_GO_ACTIVE == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_GROUP_STATE_ACTIVE:
            if(( SM_SERVICE_GROUP_STATE_NIL        == state_b )||
               ( SM_SERVICE_GROUP_STATE_INITIAL    == state_b )||
               ( SM_SERVICE_GROUP_STATE_UNKNOWN    == state_b )||
               ( SM_SERVICE_GROUP_STATE_DISABLING  == state_b )||
               ( SM_SERVICE_GROUP_STATE_DISABLED   == state_b )||
               ( SM_SERVICE_GROUP_STATE_STANDBY    == state_b )||
               ( SM_SERVICE_GROUP_STATE_GO_ACTIVE  == state_b )||
               ( SM_SERVICE_GROUP_STATE_GO_STANDBY == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        default:
            DPRINTFE( "Unknown state (%s) given.",
                      sm_service_group_state_str( state_a ) );
        break;
    }

    return( lesser_state );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Event Value
// =================================
SmServiceGroupEventT sm_service_group_event_value( const char* event_str )
{
    return( (SmServiceGroupEventT)
            sm_mapping_get_value( _sm_service_group_event_mappings,
                                  SM_SERVICE_GROUP_EVENT_MAX, event_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Event String
// ==================================
const char* sm_service_group_event_str( SmServiceGroupEventT event )
{
    return( sm_mapping_get_str( _sm_service_group_event_mappings, 
                                SM_SERVICE_GROUP_EVENT_MAX, event ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Status Value
// ==================================
SmServiceGroupStatusT sm_service_group_status_value( 
    const char* status_str )
{
    return( (SmServiceGroupStatusT)
            sm_mapping_get_value( _sm_service_group_status_mappings,
                                  SM_SERVICE_GROUP_STATUS_MAX, status_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Status String
// ===================================
const char* sm_service_group_status_str( SmServiceGroupStatusT status )
{
    return( sm_mapping_get_str( _sm_service_group_status_mappings, 
                                SM_SERVICE_GROUP_STATUS_MAX, status ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Condition Value
// =====================================
SmServiceGroupConditionT sm_service_group_condition_value( 
    const char* condition_str )
{
    return( (SmServiceGroupConditionT)
            sm_mapping_get_value( _sm_service_group_condition_mappings,
                                  SM_SERVICE_GROUP_CONDITION_MAX,
                                  condition_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Condition String
// ======================================
const char* sm_service_group_condition_str( SmServiceGroupConditionT condition )
{
    return( sm_mapping_get_str( _sm_service_group_condition_mappings, 
                                SM_SERVICE_GROUP_CONDITION_MAX, condition ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Notification Value
// ========================================
SmServiceGroupNotificationT sm_service_group_notification_value(
    const char* notification_str )
{
    return( (SmServiceGroupNotificationT)
            sm_mapping_get_value( _sm_service_group_notification_mappings,
                                  SM_SERVICE_GROUP_NOTIFICATION_MAX,
                                  notification_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Group Notification String
// =========================================
const char* sm_service_group_notification_str(
    SmServiceGroupNotificationT notification )
{
    return( sm_mapping_get_str( _sm_service_group_notification_mappings,
                                SM_SERVICE_GROUP_NOTIFICATION_MAX,
                                notification ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Admin State Value
// =================================
SmServiceAdminStateT sm_service_admin_state_value( const char* state_str )
{
    return( (SmServiceAdminStateT)
            sm_mapping_get_value( _sm_service_admin_state_mappings, 
                                  SM_SERVICE_ADMIN_STATE_MAX, state_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Admin State String
// ==================================
const char* sm_service_admin_state_str( SmServiceAdminStateT state )
{
    return( sm_mapping_get_str( _sm_service_admin_state_mappings, 
                                SM_SERVICE_ADMIN_STATE_MAX, state ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service State Value
// ===========================
SmServiceStateT sm_service_state_value( const char* state_str )
{
    return( (SmServiceStateT)
            sm_mapping_get_value( _sm_service_state_mappings, 
                                  SM_SERVICE_STATE_MAX, state_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service State String
// ============================
const char* sm_service_state_str( SmServiceStateT state )
{
    return( sm_mapping_get_str( _sm_service_state_mappings, 
                                SM_SERVICE_STATE_MAX, state ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service State Lesser Value
// ==================================
SmServiceStateT sm_service_state_lesser( SmServiceStateT state_a,
    SmServiceStateT state_b )
{
    SmServiceStateT lesser_state = SM_SERVICE_STATE_UNKNOWN;

    switch( state_a )
    {
        case SM_SERVICE_STATE_NIL:
            lesser_state = state_b;
        break;

        case SM_SERVICE_STATE_INITIAL:
            if( SM_SERVICE_STATE_NIL == state_b )
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_STATE_UNKNOWN:
            if(( SM_SERVICE_STATE_NIL     == state_b )||
               ( SM_SERVICE_STATE_INITIAL == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_STATE_DISABLED:
            if(( SM_SERVICE_STATE_NIL     == state_b )||
               ( SM_SERVICE_STATE_INITIAL == state_b )||
               ( SM_SERVICE_STATE_UNKNOWN == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_STATE_ENABLING:
            if(( SM_SERVICE_STATE_NIL               == state_b )||
               ( SM_SERVICE_STATE_INITIAL           == state_b )||
               ( SM_SERVICE_STATE_UNKNOWN           == state_b )||
               ( SM_SERVICE_STATE_ENABLING_THROTTLE == state_b )||
               ( SM_SERVICE_STATE_DISABLED          == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_STATE_DISABLING:
            if(( SM_SERVICE_STATE_NIL      == state_b )||
               ( SM_SERVICE_STATE_INITIAL  == state_b )||
               ( SM_SERVICE_STATE_UNKNOWN  == state_b )||
               ( SM_SERVICE_STATE_DISABLED == state_b )||
               ( SM_SERVICE_STATE_ENABLING == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_STATE_ENABLED_STANDBY:
            if(( SM_SERVICE_STATE_NIL               == state_b )||
               ( SM_SERVICE_STATE_INITIAL           == state_b )||
               ( SM_SERVICE_STATE_UNKNOWN           == state_b )||
               ( SM_SERVICE_STATE_ENABLING_THROTTLE == state_b )||
               ( SM_SERVICE_STATE_ENABLING          == state_b )||
               ( SM_SERVICE_STATE_DISABLING         == state_b )||
               ( SM_SERVICE_STATE_DISABLED          == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_ACTIVE:
            if(( SM_SERVICE_STATE_NIL                   == state_b )||
               ( SM_SERVICE_STATE_INITIAL               == state_b )||
               ( SM_SERVICE_STATE_UNKNOWN               == state_b )||
               ( SM_SERVICE_STATE_ENABLING_THROTTLE     == state_b )||
               ( SM_SERVICE_STATE_ENABLING              == state_b )||
               ( SM_SERVICE_STATE_DISABLING             == state_b )||
               ( SM_SERVICE_STATE_DISABLED              == state_b )||
               ( SM_SERVICE_STATE_ENABLED_STANDBY       == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_STATE_ENABLED_GO_STANDBY:
            if(( SM_SERVICE_STATE_NIL               == state_b )||
               ( SM_SERVICE_STATE_INITIAL           == state_b )||
               ( SM_SERVICE_STATE_UNKNOWN           == state_b )||
               ( SM_SERVICE_STATE_ENABLING_THROTTLE == state_b )||
               ( SM_SERVICE_STATE_ENABLING          == state_b )||
               ( SM_SERVICE_STATE_DISABLING         == state_b )||
               ( SM_SERVICE_STATE_DISABLED          == state_b )||
               ( SM_SERVICE_STATE_ENABLED_STANDBY   == state_b )||
               ( SM_SERVICE_STATE_ENABLED_GO_ACTIVE == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        case SM_SERVICE_STATE_ENABLED_ACTIVE:
            if(( SM_SERVICE_STATE_NIL                == state_b )||
               ( SM_SERVICE_STATE_INITIAL            == state_b )||
               ( SM_SERVICE_STATE_UNKNOWN            == state_b )||
               ( SM_SERVICE_STATE_ENABLING_THROTTLE  == state_b )||
               ( SM_SERVICE_STATE_ENABLING           == state_b )||
               ( SM_SERVICE_STATE_DISABLING          == state_b )||
               ( SM_SERVICE_STATE_DISABLED           == state_b )||
               ( SM_SERVICE_STATE_ENABLED_STANDBY    == state_b )||
               ( SM_SERVICE_STATE_ENABLED_GO_ACTIVE  == state_b )||
               ( SM_SERVICE_STATE_ENABLED_GO_STANDBY == state_b ))
            {
                lesser_state = state_b;
            } else {
                lesser_state = state_a;
            }
        break;

        default:
            DPRINTFE( "Unknown state (%s) given.",
                      sm_service_state_str( state_a ) );
        break;
    }

    return( lesser_state );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Event Value
// ===========================
SmServiceEventT sm_service_event_value( const char* event_str )
{
    return( (SmServiceEventT)
            sm_mapping_get_value( _sm_service_event_mappings, 
                                  SM_SERVICE_EVENT_MAX, event_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Event String
// ============================
const char* sm_service_event_str( SmServiceEventT event )
{
    return( sm_mapping_get_str( _sm_service_event_mappings, 
                                SM_SERVICE_EVENT_MAX, event ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Status Value
// ============================
SmServiceStatusT sm_service_status_value( const char* status_str )
{
    return( (SmServiceStatusT)
            sm_mapping_get_value( _sm_service_status_mappings,
                                  SM_SERVICE_STATUS_MAX, status_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Status String
// =============================
const char* sm_service_status_str( SmServiceStatusT status )
{
    return( sm_mapping_get_str( _sm_service_status_mappings, 
                                SM_SERVICE_STATUS_MAX, status ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Condition Value
// ===============================
SmServiceConditionT sm_service_condition_value( const char* condition_str )
{
    return( (SmServiceConditionT)
            sm_mapping_get_value( _sm_service_condition_mappings,
                                  SM_SERVICE_CONDITION_MAX, condition_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Condition String
// ================================
const char* sm_service_condition_str( SmServiceConditionT condition )
{
    return( sm_mapping_get_str( _sm_service_condition_mappings, 
                                SM_SERVICE_CONDITION_MAX, condition ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Severity Value
// ==============================
SmServiceSeverityT sm_service_severity_value( const char* severity_str )
{
    return( (SmServiceSeverityT)
            sm_mapping_get_value( _sm_service_severity_mappings,
                                  SM_SERVICE_SEVERITY_MAX, severity_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Severity String
// ===============================
const char* sm_service_severity_str( SmServiceSeverityT severity )
{
    return( sm_mapping_get_str( _sm_service_severity_mappings, 
                                SM_SERVICE_SEVERITY_MAX, severity ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Heartbeat Type Value
// ====================================
SmServiceHeartbeatTypeT sm_service_heartbeat_type_value( 
    const char* heartbeat_type_str )
{
    return( (SmServiceHeartbeatTypeT)
            sm_mapping_get_value( _sm_service_heartbeat_type_mappings,
                                  SM_SERVICE_HEARTBEAT_TYPE_MAX,
                                  heartbeat_type_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Heartbeat Type String
// =====================================
const char* sm_service_heartbeat_type_str(
    SmServiceHeartbeatTypeT heartbeat_type )
{
    return( sm_mapping_get_str( _sm_service_heartbeat_type_mappings, 
                                SM_SERVICE_HEARTBEAT_TYPE_MAX,
                                heartbeat_type ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Heartbeat State Value
// =====================================
SmServiceHeartbeatStateT sm_service_heartbeat_state_value( 
    const char* heartbeat_state_str )
{
    return( (SmServiceHeartbeatStateT)
            sm_mapping_get_value( _sm_service_heartbeat_state_mappings,
                                  SM_SERVICE_HEARTBEAT_STATE_MAX,
                                  heartbeat_state_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Heartbeat State String
// ======================================
const char* sm_service_heartbeat_state_str(
    SmServiceHeartbeatStateT heartbeat_state )
{
    return( sm_mapping_get_str( _sm_service_heartbeat_state_mappings, 
                                SM_SERVICE_HEARTBEAT_STATE_MAX,
                                heartbeat_state ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Dependency Type Value
// =====================================
SmServiceDependencyTypeT sm_service_dependency_type_value(
    const char* type_str )
{
    return( (SmServiceDependencyTypeT)
            sm_mapping_get_value( _sm_service_dependency_type_mappings, 
                                  SM_SERVICE_DEPENDENCY_TYPE_MAX, type_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Dependency Type String
// ======================================
const char* sm_service_dependency_type_str( SmServiceDependencyTypeT type )
{
    return( sm_mapping_get_str( _sm_service_dependency_type_mappings, 
                                SM_SERVICE_DEPENDENCY_TYPE_MAX, type ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Action Value
// ============================
SmServiceActionT sm_service_action_value( const char* action_str )
{
    return( (SmServiceActionT)
            sm_mapping_get_value( _sm_service_action_mappings,
                                  SM_SERVICE_ACTION_MAX, action_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Action String
// =============================
const char* sm_service_action_str( SmServiceActionT action )
{
    return( sm_mapping_get_str( _sm_service_action_mappings, 
                                SM_SERVICE_ACTION_MAX, action ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Action Result Value
// ===================================
SmServiceActionResultT sm_service_action_result_value( const char* result_str )
{
    return( (SmServiceActionResultT)
            sm_mapping_get_value( _sm_service_action_result_mappings,
                                  SM_SERVICE_ACTION_RESULT_MAX, result_str ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Service Action Result String
// ====================================
const char* sm_service_action_result_str( SmServiceActionResultT result )
{
    return( sm_mapping_get_str( _sm_service_action_result_mappings, 
                                SM_SERVICE_ACTION_RESULT_MAX, result ) );
}
// ****************************************************************************

// ****************************************************************************
// Types - Network Address Value
// =============================
bool sm_network_address_value( const char address_str[],
    SmNetworkAddressT* address )
{
    int result = 0;

    if( NULL == address )
    {
        return false;
    }

    if(( SM_NETWORK_TYPE_IPV4 == address->type )||
       ( SM_NETWORK_TYPE_IPV4_UDP == address->type ))
    {
        result = inet_pton( AF_INET, address_str, &(address->u.ipv4.sin) );

    } else if(( SM_NETWORK_TYPE_IPV6 == address->type )||
              ( SM_NETWORK_TYPE_IPV6_UDP == address->type )) 
    {
        result = inet_pton( AF_INET6, address_str, &(address->u.ipv6.sin6) );

    } else {
        DPRINTFD( "Unsupported network address type (%s).",
                  sm_network_type_str( address->type ) );
    }

    if( 0 >= result )
    {
        memset( address, 0, sizeof(SmNetworkAddressT) );
        return false;
    }

    return true;
}
// ****************************************************************************

// ****************************************************************************
// Types - Network Address String
// ==============================
void sm_network_address_str( const SmNetworkAddressT* address, 
    char address_str[] )
{
    address_str[0] = '\0';

    if( NULL != address )
    {
        const char* result = NULL;

        if(( SM_NETWORK_TYPE_IPV4 == address->type )||
           ( SM_NETWORK_TYPE_IPV4_UDP == address->type ))
        {
            result = inet_ntop( AF_INET, &(address->u.ipv4.sin),
                                address_str, SM_NETWORK_ADDRESS_MAX_CHAR );

        } else if(( SM_NETWORK_TYPE_IPV6 == address->type )||
                  ( SM_NETWORK_TYPE_IPV6_UDP == address->type ))
        {
            result = inet_ntop( AF_INET6, &(address->u.ipv6.sin6),
                                address_str, SM_NETWORK_ADDRESS_MAX_CHAR );

        } else {
            DPRINTFD( "Unsupported network address type (%s).",
                      sm_network_type_str( address->type ) );
        }

        if( NULL == result )
        {
            address_str[0] = '\0';
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Types - Error String
// ====================
const char* sm_error_str( SmErrorT error )
{
    return( sm_mapping_get_str( _sm_error_mappings, SM_ERROR_MAX, error ) );
}
// ****************************************************************************


// ****************************************************************************
// Types - System Mode String
// =======================================
const char* sm_system_mode_str( SmSystemModeT system_mode)
{
    return( sm_mapping_get_str(_sm_system_mode_mapping, SM_SYSTEM_MODE_MAX, system_mode) );
}
// ****************************************************************************


// ****************************************************************************
// Service Domain Interface Type
// =====================================
SmInterfaceTypeT sm_get_interface_type( const char* domain_interface )
{
    if ( 0 == strcmp ( SM_SERVICE_DOMAIN_MGMT_INTERFACE, domain_interface ) )
    {
        return SM_INTERFACE_MGMT;
    } else if ( 0 == strcmp ( SM_SERVICE_DOMAIN_OAM_INTERFACE, domain_interface ) )
    {
        return SM_INTERFACE_OAM;
    } else if ( 0 == strcmp ( SM_SERVICE_DOMAIN_CLUSTER_HOST_INTERFACE, domain_interface ) )
    {
        return SM_INTERFACE_CLUSTER_HOST;
    }

    return SM_INTERFACE_UNKNOWN;
}
// ****************************************************************************

// ****************************************************************************
// node schedule state
// =====================================
const char* sm_node_schedule_state_str( SmNodeScheduleStateT state )
{
    return( sm_mapping_get_str(_sm_node_schedule_state_mappings, SM_NODE_STATE_MAX, state) );
}
// ****************************************************************************

