//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __SM_ALARM_THREAD_H__
#define __SM_ALARM_THREAD_H__

#include "sm_types.h"
#include "sm_alarm_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    SM_ALARM_THREAD_MSG_RAISE_ALARM,
    SM_ALARM_THREAD_MSG_CLEAR_ALARM,
    SM_ALARM_THREAD_MSG_CLEAR_ALL_ALARMS,
    SM_ALARM_THREAD_MSG_MANAGE_DOMAIN_ALARMS,
} SmAlarmThreadMsgTypeT;

typedef struct
{
    struct timespec ts_real;
    SmAlarmT alarm;
    SmAlarmNodeNameT node_name;
    SmAlarmDomainNameT domain_name;
    SmAlarmEntityNameT entity_name;
    SmAlarmDataT data;
} SmAlarmThreadMsgRaiseAlarmT;

typedef struct
{
    struct timespec ts_real;
    SmAlarmT alarm;
    SmAlarmNodeNameT node_name;
    SmAlarmDomainNameT domain_name;
    SmAlarmEntityNameT entity_name;
} SmAlarmThreadMsgClearAlarmT;

typedef struct
{
    struct timespec ts_real;
    SmAlarmDomainNameT domain_name;
} SmAlarmThreadMsgClearAllAlarmsT;

typedef struct
{
    bool manage;
    SmAlarmDomainNameT domain_name;
} SmAlarmThreadMsgManageDomainAlarmsT;

typedef struct
{
    SmAlarmThreadMsgTypeT type;

    union
    {
        SmAlarmThreadMsgRaiseAlarmT raise_alarm;
        SmAlarmThreadMsgClearAlarmT clear_alarm;
        SmAlarmThreadMsgClearAllAlarmsT clear_all_alarms;
        SmAlarmThreadMsgManageDomainAlarmsT manage_alarms;
    }u;
} SmAlarmThreadMsgT;

// ****************************************************************************
// Alarm Thread - Start
// ====================
extern SmErrorT sm_alarm_thread_start( int server_fd );
// ****************************************************************************

// ****************************************************************************
// Alarm Thread - Stop
// ===================
extern SmErrorT sm_alarm_thread_stop( void );
// ****************************************************************************

#ifdef __cplusplus
}
#endif

#endif // __SM_ALARM_THREAD_H__
