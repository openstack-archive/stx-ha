//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#ifndef __FM_API_WRAPPER_H__
#define __FM_API_WRAPPER_H__


#include <fmAPI.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// The following functions are wrapped to guarantee thread safety of the
// FM API library.
//
extern EFmErrorT fm_set_fault_wrapper( const SFmAlarmDataT* alarm, 
    fm_uuid_t* uuid);

extern EFmErrorT fm_clear_fault_wrapper( AlarmFilter *filter );

extern EFmErrorT fm_clear_all_wrapper( fm_ent_inst_t *inst_id );

extern EFmErrorT fm_get_fault_wrapper( AlarmFilter *filter,
    SFmAlarmDataT* alarm );

EFmErrorT fm_get_faults_wrapper( fm_ent_inst_t *inst_id,
    SFmAlarmDataT *alarm, unsigned int* max_alarms_to_get );

EFmErrorT fm_get_faults_by_id_wrapper( fm_alarm_id* alarm_id, 
        SFmAlarmDataT* alarm, unsigned int* max_alarms_to_get );

#ifdef __cplusplus
}
#endif

#endif // __FM_API_WRAPPER_H__
