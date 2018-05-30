//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_ALARM_DEFINITIONS_H__
#define __SM_ALARM_DEFINITIONS_H__

#include <stdbool.h>

#include "sm_limits.h"

#ifdef __cplusplus
extern "C" {
#endif

// X.731 - Systems Management: State Management Function
// X.732 - Systems Management: Attributes for Representing Relationships
// X.733 - Systems Management: Alarm Reporting Function
// RFC 4628 - Entity State MIB

typedef char SmAlarmNodeNameT[SM_NODE_NAME_MAX_CHAR];

typedef char SmAlarmDomainNameT[SM_ALARM_DOMAIN_NAME_MAX_CHAR];

typedef char SmAlarmEntityNameT[SM_ALARM_ENTITY_NAME_MAX_CHAR];

typedef enum
{
    SM_ALARM_EVENT_TYPE_UNKNOWN,
    SM_ALARM_EVENT_TYPE_COMMUNICATIONS_ALARM,
    SM_ALARM_EVENT_TYPE_PROCESSING_ERROR_ALARM,
    SM_ALARM_EVENT_TYPE_ENVIRONMENTAL_ALARM,
    SM_ALARM_EVENT_TYPE_QUALITY_OF_SERVICE_ALARM,
    SM_ALARM_EVENT_TYPE_EQUIPMENT_ALARM,
    SM_ALARM_EVENT_TYPE_INTEGRITY_VIOLATION,
    SM_ALARM_EVENT_TYPE_OPERATIONAL_VIOLATION,
    SM_ALARM_EVENT_TYPE_PHYSICAL_VIOLATION,
    SM_ALARM_EVENT_TYPE_SECURITY_SERVICE_VIOLATION,
    SM_ALARM_EVENT_TYPE_MECHANISM_VIOLATION,
    SM_ALARM_EVENT_TYPE_TIME_DOMAIN_VIOLATION,
    SM_ALARM_EVENT_TYPE_MAX
} SmAlarmEventTypeT;

typedef enum
{
    SM_ALARM_PROBABLE_CAUSE_UNKNOWN,
    SM_ALARM_PROBABLE_CAUSE_INDETERMINATE,
    SM_ALARM_PROBABLE_CAUSE_SOFTWARE_ERROR,
    SM_ALARM_PROBABLE_CAUSE_SOFTWARE_PROGRAM_ERROR,
    SM_ALARM_PROBABLE_CAUSE_UNDERLYING_RESOURCE_UNAVAILABLE,
    SM_ALARM_PROBABLE_CAUSE_KEY_EXPIRED,
    SM_ALARM_PROBABLE_CAUSE_MAX
} SmAlarmProbableCauseT;

typedef enum
{
    SM_ALARM_SEVERITY_UNKNOWN,
    SM_ALARM_SEVERITY_CLEARED,
    SM_ALARM_SEVERITY_INDETERMINATE,
    SM_ALARM_SEVERITY_WARNING,
    SM_ALARM_SEVERITY_MINOR,
    SM_ALARM_SEVERITY_MAJOR,
    SM_ALARM_SEVERITY_CRITICAL,
    SM_ALARM_SEVERITY_MAX
} SmAlarmSeverityT;

typedef enum
{
    SM_ALARM_TREND_INDICATION_UNKNOWN,
    SM_ALARM_TREND_INDICATION_LESS_SEVERE,
    SM_ALARM_TREND_INDICATION_NO_CHANGE,
    SM_ALARM_TREND_INDICATION_MORE_SEVERE,
    SM_ALARM_TREND_INDICATION_MAX
} SmAlarmTrendIndicationT;

typedef struct
{
    bool applicable;
    char state[SM_ALARM_ENTITY_STATE_MAX_CHAR];
    char status[SM_ALARM_ENTITY_STATUS_MAX_CHAR];
    char condition[SM_ALARM_ENTITY_CONDITION_MAX_CHAR];
} SmAlarmStateInfoT;

typedef struct
{
    bool applicable;
    unsigned int threshold_value;
    unsigned int observed_value;
} SmAlarmThresholdInfoT;

typedef char SmAlarmProposedRepairActionT[SM_ALARM_PROPOSED_REPAIR_ACTION_MAX_CHAR];

typedef char SmAlarmSpecificProblemTextT[SM_ALARM_SPECIFIC_PROBLEM_TEXT_MAX_CHAR];

typedef char SmAlarmAdditionalTextT[SM_ALARM_ADDITIONAL_TEXT_MAX_CHAR];

typedef enum
{
    SM_ALARM_UNKNOWN,
    SM_ALARM_SERVICE_GROUP_STATE,
    SM_ALARM_SERVICE_GROUP_REDUNDANCY,
    SM_ALARM_SOFTWARE_MODIFICATION,
    SM_ALARM_COMMUNICATION_FAILURE,
    SM_ALARM_MAX
} SmAlarmT;

#define SM_ALARM_SERVICE_GROUP_STATE_ALARM_ID           "400.001"
#define SM_ALARM_SERVICE_GROUP_REDUNDANCY_ALARM_ID      "400.002"
#define SM_ALARM_SOFTWARE_MODIFICATION_ALARM_ID         "400.004"
#define SM_ALARM_COMMUNICATION_FAILURE_ALARM_ID         "400.005"

typedef struct
{
    SmAlarmEventTypeT event_type;
    SmAlarmProbableCauseT probable_cause; 
    SmAlarmSpecificProblemTextT specific_problem_text;
    SmAlarmSeverityT perceived_severity;
    SmAlarmTrendIndicationT trend_indication;
    SmAlarmStateInfoT state_info;
    SmAlarmThresholdInfoT threshold_info;
    SmAlarmProposedRepairActionT proposed_repair_action;
    SmAlarmAdditionalTextT additional_text;
    bool service_affecting;
    bool suppression_allowed;
} SmAlarmDataT;

#ifdef __cplusplus
}
#endif

#endif // __SM_ALARM_DEFINITIONS_H__
