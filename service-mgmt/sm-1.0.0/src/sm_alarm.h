//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_ALARM_H__
#define __SM_ALARM_H__

#include <stdbool.h>

#include "sm_types.h"
#include "sm_alarm_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

// ****************************************************************************
// Alarm - Write Alarm Additional Text
// ===================================
extern bool sm_alarm_write_alarm_additional_text( const char data[],
    char alarm_additional_text[], int alarm_additional_text_size, int reserve );
// ****************************************************************************

// ****************************************************************************
// Alarm - Raise State Alarm
// =========================
extern void sm_alarm_raise_state_alarm( const SmAlarmT alarm,
    const SmAlarmNodeNameT node_name, const SmAlarmDomainNameT domain_name,
    const SmAlarmEntityNameT entity_name, const SmAlarmSeverityT severity,
    const SmAlarmProbableCauseT probable_cause, const char entity_state[],
    const char entity_status[], const char entity_condition[],
    const SmAlarmSpecificProblemTextT specific_problem_text,
    const SmAlarmAdditionalTextT additional_text,
    const SmAlarmProposedRepairActionT proposed_repair_action,
    bool suppression_allowed );
// ****************************************************************************

// ****************************************************************************
// Alarm - Raise Threshold Alarm
// =============================
extern void sm_alarm_raise_threshold_alarm( const SmAlarmT alarm,
    const SmAlarmNodeNameT node_name, const SmAlarmDomainNameT domain_name,
    const SmAlarmEntityNameT entity_name, const SmAlarmSeverityT severity,
    const SmAlarmProbableCauseT probable_cause, 
    const unsigned int threshold_value, const unsigned int observed_value,
    const SmAlarmSpecificProblemTextT specific_problem_text,
    const SmAlarmAdditionalTextT additional_text,
    const SmAlarmProposedRepairActionT proposed_repair_action,
    bool suppression_allowed );
// ****************************************************************************

// ****************************************************************************
// Alarm - Raise Node Alarm
// ========================
extern void sm_alarm_raise_node_alarm( const SmAlarmT alarm,
    const SmAlarmNodeNameT node_name, const SmAlarmSeverityT severity,
    const SmAlarmProbableCauseT probable_cause,
    const SmAlarmSpecificProblemTextT specific_problem_text,
    const SmAlarmAdditionalTextT additional_text,
    const SmAlarmProposedRepairActionT proposed_repair_action,
    bool suppression_allowed );
// ****************************************************************************

// ****************************************************************************
// Alarm - Raise Software Alarm
// ============================
extern void sm_alarm_raise_software_alarm( const SmAlarmT alarm,
    const SmAlarmNodeNameT node_name, const SmAlarmSeverityT severity,
    const SmAlarmProbableCauseT probable_cause,
    const SmAlarmSpecificProblemTextT specific_problem_text,
    const SmAlarmAdditionalTextT additional_text,
    const SmAlarmProposedRepairActionT proposed_repair_action,
    bool suppression_allowed );
// ****************************************************************************

// ****************************************************************************
// Alarm - Raise Communication Alarm
// =================================
extern void sm_alarm_raise_communication_alarm( const SmAlarmT alarm,
    const SmAlarmNodeNameT node_name, const SmAlarmEntityNameT entity_name,
    const SmAlarmSeverityT severity,
    const SmAlarmProbableCauseT probable_cause,
    const SmAlarmSpecificProblemTextT specific_problem_text,
    const SmAlarmAdditionalTextT additional_text,
    const SmAlarmProposedRepairActionT proposed_repair_action,
    bool suppression_allowed );
// ****************************************************************************

// ****************************************************************************
// Alarm - Clear
// =============
extern void sm_alarm_clear( const SmAlarmT alarm, 
    const SmAlarmNodeNameT node_name, const SmAlarmDomainNameT domain_name,
    const SmAlarmEntityNameT entity_name );
// ****************************************************************************

// ****************************************************************************
// Alarm - Clear All
// =================
extern void sm_alarm_clear_all( const SmAlarmDomainNameT domain_name );
// ****************************************************************************

// ****************************************************************************
// Alarm - Manage Domain Alarms
// ============================
extern void sm_alarm_manage_domain_alarms( const SmAlarmDomainNameT domain_name,
    bool manage );
// ****************************************************************************

// ****************************************************************************
// Alarm - Initialize
// ==================
extern SmErrorT sm_alarm_initialize( void );
// ****************************************************************************

// ****************************************************************************
// Alarm - Finalize
// ================
extern SmErrorT sm_alarm_finalize( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_ALARM_H__
