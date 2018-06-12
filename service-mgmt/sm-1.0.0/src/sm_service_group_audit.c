//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_group_audit.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_time.h"
#include "sm_service_group_table.h"
#include "sm_service_group_member_table.h"
#include "sm_service_api.h"
#include "sm_service_group_health.h"
#include "sm_service_domain_utils.h"

// ****************************************************************************
// Service Group Audit - Set Service Reason Text
// =============================================
static void sm_service_group_audit_set_service_reason_text( char reason_text[],
    char service_name[], SmServiceStateT state, SmServiceStatusT status,
    SmServiceConditionT condition )
{
    bool skip_condition = false;

    if(( SM_SERVICE_CONDITION_NONE             == condition )||
       ( SM_SERVICE_CONDITION_RECOVERY_FAILURE == condition )||
       ( SM_SERVICE_CONDITION_ACTION_FAILURE   == condition )||
       ( SM_SERVICE_CONDITION_FATAL_FAILURE    == condition ))
    {
        skip_condition = true;
    }

    snprintf( reason_text, SM_SERVICE_GROUP_REASON_TEXT_MAX_CHAR,
              "%s(%s, %s%s%s)", service_name,
              sm_service_state_str( state ), sm_service_status_str( status ),
              skip_condition ? "" : ", ",
              skip_condition ? "" : sm_service_condition_str( condition ) );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Audit - Write Reason Text
// =======================================
static bool sm_service_group_audit_write_reason_text( const char data[],
    char reason_text[], int reason_text_size )
{
    bool have_space = true;

    if( '\0' == reason_text[0] )
    {
        snprintf( reason_text, reason_text_size, "%s", data );
    } else {
        int len = strlen(reason_text);
        int left = reason_text_size - len - 1 ;
        int data_len = (int) strlen(data);

        if( left > (data_len + 32) ) // reserve extra room
        {
            snprintf( &(reason_text[len]), left, ", %s", data );
        } else {
            snprintf( &(reason_text[len]), left, ", ..." );
            have_space = false;
        }
    }

    return( have_space );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Audit - Map Service Condition
// ===========================================
static void sm_service_group_audit_map_service_condition(
    SmServiceConditionT service_condition, SmServiceGroupConditionT* condition )
{
    switch( service_condition )
    {
        case SM_SERVICE_CONDITION_NIL:
            *condition = SM_SERVICE_GROUP_CONDITION_NIL;
        break;

        case SM_SERVICE_CONDITION_UNKNOWN:
            *condition = SM_SERVICE_GROUP_CONDITION_UNKNOWN;
        break;

        case SM_SERVICE_CONDITION_NONE:
            *condition = SM_SERVICE_GROUP_CONDITION_NONE;
        break;

        case SM_SERVICE_CONDITION_DATA_INCONSISTENT:
            *condition = SM_SERVICE_GROUP_CONDITION_DATA_INCONSISTENT;
        break;

        case SM_SERVICE_CONDITION_DATA_OUTDATED:
            *condition = SM_SERVICE_GROUP_CONDITION_DATA_OUTDATED;
        break;

        case SM_SERVICE_CONDITION_DATA_CONSISTENT:
            *condition = SM_SERVICE_GROUP_CONDITION_DATA_CONSISTENT;
        break;

        case SM_SERVICE_CONDITION_DATA_SYNC:
            *condition = SM_SERVICE_GROUP_CONDITION_DATA_SYNC;
        break;

        case SM_SERVICE_CONDITION_DATA_STANDALONE:
            *condition = SM_SERVICE_GROUP_CONDITION_DATA_STANDALONE;
        break;

        case SM_SERVICE_CONDITION_RECOVERY_FAILURE:
            *condition = SM_SERVICE_GROUP_CONDITION_RECOVERY_FAILURE;
        break;

        case SM_SERVICE_CONDITION_ACTION_FAILURE:
            *condition = SM_SERVICE_GROUP_CONDITION_ACTION_FAILURE;
        break;

        case SM_SERVICE_CONDITION_FATAL_FAILURE:
            *condition = SM_SERVICE_GROUP_CONDITION_FATAL_FAILURE;
        break;

        default:
            *condition = SM_SERVICE_GROUP_CONDITION_UNKNOWN;
            DPRINTFE( "Unknown service condition (%i) given.",
                      service_condition );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Audit - Service For Status
// ========================================
static void sm_service_group_audit_service_for_status( void* user_data[],
    SmServiceGroupMemberT* service_group_member )
{
    bool do_increment = true;
    long elapsed_ms, delta_ms;
    SmServiceGroupT* service_group = (SmServiceGroupT*) user_data[0];
    SmServiceGroupStatusT* status = (SmServiceGroupStatusT*) user_data[1];
    SmServiceGroupConditionT* condition = (SmServiceGroupConditionT*) user_data[2];
    int* failed = (int*) user_data[3];
    int* degraded = (int*) user_data[4];
    int* warn = (int*) user_data[5];
    int* healthy = (int*) user_data[6];
    bool* reason_text_writable = (bool*) user_data[7];
    char* reason_text = (char*) user_data[8];
    int reason_text_size = *(int*) user_data[9];
    SmServiceGroupStatusT prev_status = *status;
    SmServiceGroupConditionT prev_condition = *condition;
    SmServiceStatusT sgm_imply_status;
    SmServiceGroupStatusT mapped_status;
    SmServiceGroupConditionT mapped_condition;
    char service_reason_text[SM_SERVICE_GROUP_REASON_TEXT_MAX_CHAR] = "";

    sgm_imply_status = service_group_member->service_status;
    if( 0 != service_group_member->service_failure_timestamp )
    {
        elapsed_ms = sm_time_get_elapsed_ms( NULL );
        delta_ms = elapsed_ms - service_group_member->service_failure_timestamp;

        if( service_group->failure_debounce_in_ms >= delta_ms )
        {
            DPRINTFD( "Service group (%s) member (%s) failure debounce (%d) "
                      "still in effect since (%li), indicating member as unhealthy, "
                      "delta_ms=%li.", service_group->name,
                      service_group_member->service_name,
                      service_group->failure_debounce_in_ms,
                      service_group_member->service_failure_timestamp,
                      delta_ms );

            do_increment = false;

            switch( service_group_member->service_failure_impact )
            {
                case SM_SERVICE_SEVERITY_NONE:
                    sgm_imply_status  = SM_SERVICE_STATUS_NONE;
                break;

                case SM_SERVICE_SEVERITY_MINOR:
                    sgm_imply_status  = SM_SERVICE_STATUS_WARN;
                    ++(*warn);
                break;

                case SM_SERVICE_SEVERITY_MAJOR:
                    sgm_imply_status  = SM_SERVICE_STATUS_DEGRADED;
                    ++(*degraded);
                break;

                case SM_SERVICE_SEVERITY_CRITICAL:
                    sgm_imply_status  = SM_SERVICE_STATUS_FAILED;
                    ++(*failed);
                break;

                default:
                break;
            }
        } else {
            ++(*healthy);
            DPRINTFD( "Service group (%s) member (%s) failure debounce "
                      "no longer in effect, delta_ms=%li.", service_group->name,
                      service_group_member->service_name, delta_ms );
        }
    }else
    {
        ++(*healthy);
    }

    switch( sgm_imply_status )
    {
        case SM_SERVICE_STATUS_NONE:
            mapped_status = SM_SERVICE_GROUP_STATUS_NONE;
            mapped_condition = SM_SERVICE_GROUP_CONDITION_NONE;
        break;

        case SM_SERVICE_STATUS_WARN:
            mapped_status = SM_SERVICE_GROUP_STATUS_WARN;
            mapped_condition = SM_SERVICE_GROUP_CONDITION_NONE;

            sm_service_group_audit_set_service_reason_text(
                                    service_reason_text,
                                    service_group_member->service_name,
                                    service_group_member->service_state,
                                    service_group_member->service_status,
                                    service_group_member->service_condition );

            if( do_increment )
            {
                ++(*warn);
            }
        break;

        case SM_SERVICE_STATUS_DEGRADED:
            mapped_status = SM_SERVICE_GROUP_STATUS_DEGRADED;

            sm_service_group_audit_map_service_condition(
                        service_group_member->service_condition,
                        &mapped_condition );

            sm_service_group_audit_set_service_reason_text(
                                    service_reason_text,
                                    service_group_member->service_name,
                                    service_group_member->service_state,
                                    service_group_member->service_status,
                                    service_group_member->service_condition );

            if( do_increment )
            {
                ++(*degraded);
            }
        break;

        case SM_SERVICE_STATUS_FAILED:
            switch( service_group_member->service_failure_impact )
            {
                case SM_SERVICE_SEVERITY_NONE:
                    mapped_status = SM_SERVICE_GROUP_STATUS_NONE;
                    mapped_condition = SM_SERVICE_GROUP_CONDITION_NONE;
                break;

                case SM_SERVICE_SEVERITY_MINOR:
                    mapped_status = SM_SERVICE_GROUP_STATUS_WARN;
                    mapped_condition = SM_SERVICE_GROUP_CONDITION_NONE;

                    if( do_increment )
                    {
                        ++(*warn);
                    }
                break;

                case SM_SERVICE_SEVERITY_MAJOR:
                    mapped_status = SM_SERVICE_GROUP_STATUS_DEGRADED;
                    mapped_condition = SM_SERVICE_GROUP_CONDITION_NONE;

                    if( do_increment )
                    {
                        ++(*degraded);
                    }
                break;

                case SM_SERVICE_SEVERITY_CRITICAL:
                    mapped_status = SM_SERVICE_GROUP_STATUS_FAILED;

                    sm_service_group_audit_map_service_condition(
                                    service_group_member->service_condition,
                                    &mapped_condition );

                    if( do_increment )
                    {
                        ++(*failed);
                    }
                break;

                default:
                    mapped_status = SM_SERVICE_GROUP_STATUS_UNKNOWN;
                    mapped_condition = SM_SERVICE_GROUP_CONDITION_UNKNOWN;
                break;
            }

            sm_service_group_audit_set_service_reason_text(
                                    service_reason_text,
                                    service_group_member->service_name,
                                    service_group_member->service_state,
                                    service_group_member->service_status,
                                    service_group_member->service_condition );
        break;

        default:
            mapped_status = SM_SERVICE_GROUP_STATUS_UNKNOWN;
            mapped_condition = SM_SERVICE_GROUP_CONDITION_UNKNOWN;
        break;
    }

    if( SM_SERVICE_GROUP_STATUS_UNKNOWN != mapped_status )
    {
        switch( *status )
        {
            case SM_SERVICE_GROUP_STATUS_NONE:
                *status = mapped_status;
            break;

            case SM_SERVICE_GROUP_STATUS_WARN:
                if( SM_SERVICE_GROUP_STATUS_NONE != mapped_status )
                {
                    *status = mapped_status;
                }
            break;

            case SM_SERVICE_GROUP_STATUS_DEGRADED:
               if(( SM_SERVICE_GROUP_STATUS_NONE != mapped_status )&&
                  ( SM_SERVICE_GROUP_STATUS_WARN != mapped_status ))
                {
                    *status = mapped_status;
                }
            break;

            case SM_SERVICE_GROUP_STATUS_FAILED:
                if(( SM_SERVICE_GROUP_STATUS_NONE     != mapped_status )&&
                   ( SM_SERVICE_GROUP_STATUS_WARN     != mapped_status )&&
                   ( SM_SERVICE_GROUP_STATUS_DEGRADED != mapped_status ))
                {
                    *status = mapped_status;
                }
            break;

            default:
                DPRINTFE( "Service group (%s) status (%s) not handled.",
                          service_group->name, 
                          sm_service_group_status_str( mapped_status ) );
            break;
        }

        switch( *condition )
        {
            case SM_SERVICE_GROUP_CONDITION_NONE:
                *condition = mapped_condition;
            break;

            case SM_SERVICE_GROUP_CONDITION_DATA_INCONSISTENT:
            case SM_SERVICE_GROUP_CONDITION_DATA_OUTDATED:
            case SM_SERVICE_GROUP_CONDITION_DATA_CONSISTENT:
            case SM_SERVICE_GROUP_CONDITION_DATA_SYNC:
            case SM_SERVICE_GROUP_CONDITION_DATA_STANDALONE:
                if( SM_SERVICE_GROUP_CONDITION_NONE != mapped_condition )
                {
                    *condition = mapped_condition;
                }
            break;

            case SM_SERVICE_GROUP_CONDITION_RECOVERY_FAILURE:
                if(( SM_SERVICE_GROUP_CONDITION_NONE != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_INCONSISTENT != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_OUTDATED != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_CONSISTENT != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_SYNC != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_STANDALONE != mapped_condition ))
                {
                    *condition = mapped_condition;
                }
            break;

            case SM_SERVICE_GROUP_CONDITION_ACTION_FAILURE:
                if(( SM_SERVICE_GROUP_CONDITION_NONE != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_INCONSISTENT != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_OUTDATED != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_CONSISTENT != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_SYNC != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_STANDALONE != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_RECOVERY_FAILURE != mapped_condition ))
                {
                    *condition = mapped_condition;
                }
            break;

            case SM_SERVICE_GROUP_CONDITION_FATAL_FAILURE:
                if(( SM_SERVICE_GROUP_CONDITION_NONE != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_INCONSISTENT != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_OUTDATED != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_CONSISTENT != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_SYNC != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_DATA_STANDALONE != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_RECOVERY_FAILURE != mapped_condition )&&
                   ( SM_SERVICE_GROUP_CONDITION_ACTION_FAILURE != mapped_condition ))
                {
                    *condition = mapped_condition;
                }
            break;

            default:
                DPRINTFE( "Service group (%s) condition (%s) not handled.",
                          service_group->name, 
                          sm_service_group_condition_str( mapped_condition ) );
            break;
        }
    } else {
        DPRINTFE( "Service (%s) has unmapped severity (%s) for service "
                  "group (%s).", service_group_member->service_name,
                  sm_service_severity_str(
                          service_group_member->service_failure_impact ),
                  service_group->name );
    }

    if(( *status != service_group->status )&&( prev_status != *status ))
    {
        DPRINTFI( "Service group (%s) member (%s) has mapped status (%s), "
                  "overall_status=%s.", service_group_member->name,
                  service_group_member->service_name,
                  sm_service_group_status_str(mapped_status),
                  sm_service_group_status_str(*status) );
    }

    if(( *condition != service_group->condition )&&
       ( prev_condition != *condition ))
    {
        DPRINTFI( "Service group (%s) member (%s) has mapped condition (%s), "
                  "overall_condition=%s.", service_group_member->name,
                  service_group_member->service_name,
                  sm_service_group_condition_str(mapped_condition),
                  sm_service_group_condition_str(*condition) );
    }

    if( SM_SERVICE_GROUP_STATUS_NONE != *status )
    {
        if( prev_status != *status )
        {
            *reason_text_writable = true;
            reason_text[0] = '\0';
        }

        if(( '\0' != service_reason_text[0] )&&( *reason_text_writable ))
        {
            *reason_text_writable
                = sm_service_group_audit_write_reason_text(
                        service_reason_text, reason_text, reason_text_size );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Group Audit - Status
// ============================
SmErrorT sm_service_group_audit_status( SmServiceGroupT* service_group )
{
    int failed=0, degraded=0, warn=0, healthy=0;
    SmServiceGroupStatusT audit_status = SM_SERVICE_GROUP_STATUS_NONE;
    SmServiceGroupConditionT audit_condition = SM_SERVICE_GROUP_CONDITION_NONE;
    int64_t audit_health = 0;
    bool reason_text_writable = true;
    char reason_text[SM_SERVICE_GROUP_REASON_TEXT_MAX_CHAR] = "";
    int reason_text_size = SM_SERVICE_GROUP_REASON_TEXT_MAX_CHAR;
    void* user_data[] = { service_group, &audit_status, &audit_condition,
                          &failed, &degraded, &warn, &healthy, &reason_text_writable,
                          reason_text, &reason_text_size };

    sm_service_group_member_table_foreach_member( service_group->name,
                    user_data, sm_service_group_audit_service_for_status );

    audit_health = sm_service_group_health_calculate( failed, degraded, warn );

    if(SM_SERVICE_GROUP_STATUS_FAILED == audit_status && 0 < healthy &&
        sm_is_aa_service_group(service_group->name))
    {
        service_group->status = SM_SERVICE_GROUP_STATUS_DEGRADED;
    }else
    {
        service_group->status = audit_status;
    }
    service_group->condition = audit_condition;
    service_group->health = audit_health;

    snprintf( service_group->reason_text, sizeof(service_group->reason_text),
              "%s", reason_text );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Audit - Initialize
// ================================
SmErrorT sm_service_group_audit_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Group Audit - Finalize
// ==============================
SmErrorT sm_service_group_audit_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
