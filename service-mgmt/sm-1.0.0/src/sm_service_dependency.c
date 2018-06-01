//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_dependency.h"

#include <stdio.h>
#include <stdbool.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_time.h"
#include "sm_service_table.h"
#include "sm_service_dependency_table.h"
#include "sm_service_domain_member_table.h"

// ****************************************************************************
// Service Dependency - Dependent State Compare
// ============================================
static void sm_service_dependency_dependent_state_compare(
    void* user_data[], SmServiceDependencyT* service_dependency )
{
    SmServiceT* service = (SmServiceT*) user_data[0];
    bool* met = (bool*) user_data[1];
    SmCompareOperatorT compare_operator = *(SmCompareOperatorT*) user_data[2];
    bool* at_least_one = (bool*) user_data[3];
    SmServiceT* dependent_service;
    SmServiceStateT state_result;
   
    if( '\0' == service_dependency->dependent[0] )
    {
        DPRINTFI( "Service (%s) has no dependencies.", service->name );
        return;
    }

    dependent_service = sm_service_table_read( service_dependency->dependent );
    if( NULL == dependent_service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.", 
                  service_dependency->service_name,
                  sm_error_str(SM_NOT_FOUND) );
        return;
    }

    *at_least_one = true;

    state_result = sm_service_state_lesser( service_dependency->dependent_state,
                                            dependent_service->state );

    if( SM_COMPARE_OPERATOR_LE == compare_operator )
    {
        if(( service_dependency->dependent_state == dependent_service->state )||
           ( state_result == service_dependency->dependent_state ))
        {
            DPRINTFD( "Dependency (%s) for service (%s) was met, "
                      "dependent_state=%s, dependency_state=%s, op=le.",
                      service_dependency->dependent, service->name,
                      sm_service_state_str(dependent_service->state),
                      sm_service_state_str(service_dependency->dependent_state) );
        } else {
            DPRINTFD( "Dependency (%s) for service (%s) was not met, "
                      "dependent_state=%s, dependency_state=%s, op=le.", 
                      service_dependency->dependent, service->name,
                      sm_service_state_str(dependent_service->state),
                      sm_service_state_str(service_dependency->dependent_state) );
            *met = false;
        }
    } else if( SM_COMPARE_OPERATOR_GE == compare_operator )
    {
        if(( service_dependency->dependent_state == dependent_service->state )||
           ( state_result == dependent_service->state ))
        {
            DPRINTFD( "Dependency (%s) for service (%s) was met, "
                      "dependent_state=%s, dependency_state=%s, op=ge.", 
                      service_dependency->dependent, service->name,
                      sm_service_state_str(dependent_service->state),
                      sm_service_state_str(service_dependency->dependent_state) );
        } else {
            DPRINTFD( "Dependency (%s) for service (%s) was not met, "
                      "dependent_state=%s, dependency_state=%s, op=ge.", 
                      service_dependency->dependent, service->name,
                      sm_service_state_str(dependent_service->state),
                      sm_service_state_str(service_dependency->dependent_state) );
            *met = false;
        }
    } else {
        *met = false;
        DPRINTFE( "Unknown compare operator (%i).", compare_operator );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Go-Active Met 
// ==================================
SmErrorT sm_service_dependency_go_active_met( SmServiceT* service, bool* met )
{
    bool at_least_one = false;
    bool dependency_met = true;
    SmCompareOperatorT compare_operator = SM_COMPARE_OPERATOR_LE;
    void* user_data[] = {service, &dependency_met, &compare_operator,
                         &at_least_one};

    *met = false;

    sm_service_dependency_table_foreach( SM_SERVICE_DEPENDENCY_TYPE_ACTION,
            service->name, SM_SERVICE_STATE_NA, SM_SERVICE_ACTION_GO_ACTIVE,
            user_data, sm_service_dependency_dependent_state_compare );

    if( at_least_one )
    {
        *met = dependency_met;
    } else {
        *met = true;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Go-Standby Met 
// ===================================
SmErrorT sm_service_dependency_go_standby_met( SmServiceT* service, bool* met )
{
    bool at_least_one = false;
    bool dependency_met = true;
    SmCompareOperatorT compare_operator = SM_COMPARE_OPERATOR_GE;
    void* user_data[] = {service, &dependency_met, &compare_operator,
                         &at_least_one};

    *met = false;

    sm_service_dependency_table_foreach( SM_SERVICE_DEPENDENCY_TYPE_ACTION,
            service->name, SM_SERVICE_STATE_NA, SM_SERVICE_ACTION_GO_STANDBY,
            user_data, sm_service_dependency_dependent_state_compare );

    if( at_least_one )
    {
        *met = dependency_met;
    } else {
        *met = true;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Enable Met, per dependent
// ================================
static void _sm_service_enable_dependency_met(
    void* user_data[], SmServiceDependencyT* service_dependency )
{
    bool *dependency_met = (bool*)user_data[0];
    if( '\0' == service_dependency->dependent[0] )
    {
        DPRINTFD( "Service (%s) has no dependencies.", service_dependency->service_name );
        return;
    }

    SmServiceT* dependent_service = sm_service_table_read( service_dependency->dependent );
    if( NULL == dependent_service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  service_dependency->service_name,
                  sm_error_str(SM_NOT_FOUND) );
        return;
    }

    if( SM_SERVICE_STATE_ENABLED_ACTIVE != dependent_service->state &&
        SM_SERVICE_STATE_ENABLED_STANDBY != dependent_service->desired_state)
    {
        *dependency_met = false;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Enable Met 
// ===============================
SmErrorT sm_service_dependency_enable_met( SmServiceT* service, bool* met )
{
    bool dependency_met = true;
    void* user_data[] = {&dependency_met};

    *met = false;
    sm_service_dependency_table_foreach( SM_SERVICE_DEPENDENCY_TYPE_ACTION,
            service->name, SM_SERVICE_STATE_NA, SM_SERVICE_ACTION_ENABLE,
            user_data, _sm_service_enable_dependency_met );

    *met = dependency_met;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Disable Met per dependent
// ================================
static void _sm_service_disable_dependency_met(
    void* user_data[], SmServiceDependencyT* service_dependency )
{
    bool *dependency_met = (bool*)user_data[0];
    if( '\0' == service_dependency->dependent[0] )
    {
        DPRINTFD( "Service (%s) has no dependencies.", service_dependency->service_name );
        return;
    }

    SmServiceT* dependent_service = sm_service_table_read( service_dependency->dependent );
    if( NULL == dependent_service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.",
                  service_dependency->service_name,
                  sm_error_str(SM_NOT_FOUND) );
        return;
    }

    if( SM_SERVICE_STATE_DISABLED != dependent_service->state &&
        SM_SERVICE_STATE_ENABLED_ACTIVE != dependent_service->desired_state)
    {
        *dependency_met = false;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Disable Met 
// ================================
SmErrorT sm_service_dependency_disable_met( SmServiceT* service, bool* met )
{
    bool dependency_met = true;
    void* user_data[] = {&dependency_met};

    *met = false;
    sm_service_dependency_table_foreach( SM_SERVICE_DEPENDENCY_TYPE_ACTION,
            service->name, SM_SERVICE_STATE_NA, SM_SERVICE_ACTION_DISABLE,
            user_data, _sm_service_disable_dependency_met );

    *met = dependency_met;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Enabled Active State Met 
// =============================================
SmErrorT sm_service_dependency_enabled_active_state_met( SmServiceT* service,
    bool* met )
{
    bool at_least_one = false;
    bool dependency_met = true;
    SmCompareOperatorT compare_operator = SM_COMPARE_OPERATOR_LE;
    void* user_data[] = {service, &dependency_met, &compare_operator,
                         &at_least_one};

    *met = false;

    sm_service_dependency_table_foreach( SM_SERVICE_DEPENDENCY_TYPE_STATE,
            service->name, SM_SERVICE_STATE_ENABLED_ACTIVE, SM_SERVICE_ACTION_NA,
            user_data, sm_service_dependency_dependent_state_compare );

    if( at_least_one )
    {
        *met = dependency_met;
    } else {
        *met = true;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Enabled Standby State Met 
// ==============================================
SmErrorT sm_service_dependency_enabled_standby_state_met( SmServiceT* service,
    bool* met )
{
    bool at_least_one = false;
    bool dependency_met = true;
    SmCompareOperatorT compare_operator = SM_COMPARE_OPERATOR_LE;
    void* user_data[] = {service, &dependency_met, &compare_operator, 
                         &at_least_one};

    *met = false;

    sm_service_dependency_table_foreach( SM_SERVICE_DEPENDENCY_TYPE_STATE,
            service->name, SM_SERVICE_STATE_ENABLED_STANDBY, SM_SERVICE_ACTION_NA,
            user_data, sm_service_dependency_dependent_state_compare );


    if( at_least_one )
    {
        *met = dependency_met;
    } else {
        *met = true;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Dependent Action Require State
// ===================================================
static void sm_service_dependency_dependent_action_require_state(
    void* user_data[], SmServiceDependencyT* service_dependency )
{
    bool state_match = false;
    SmServiceT* service = (SmServiceT*) user_data[0];
    SmServiceStateT required_state = *(SmServiceStateT*) user_data[1];
    bool* require_state = (bool*) user_data[2];
    bool* at_least_one = (bool*) user_data[3];
    SmServiceT* dependent_service;
    
    if( '\0' == service_dependency->dependent[0] )
    {
        DPRINTFI( "Service (%s) has no dependencies.", service->name );
        return;
    }

    dependent_service = sm_service_table_read( service_dependency->service_name );
    if( NULL == dependent_service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.", 
                  service_dependency->service_name,
                  sm_error_str(SM_NOT_FOUND) );
        return;
    }

    *at_least_one = true;

    switch( service_dependency->action )
    {
        case SM_SERVICE_ACTION_ENABLE:
            if( SM_SERVICE_STATE_ENABLING == dependent_service->state )
            {
                state_match = true;
            }
        break;

        case SM_SERVICE_ACTION_DISABLE:
            if( SM_SERVICE_STATE_DISABLING == dependent_service->state &&
                !dependent_service->disable_skip_dependent)
            {
                state_match = true;
            }
        break;

        case SM_SERVICE_ACTION_GO_ACTIVE:
            if( SM_SERVICE_STATE_ENABLED_GO_ACTIVE == dependent_service->state )
            {
                state_match = true;
            }
        break;

        case SM_SERVICE_ACTION_GO_STANDBY:
            if( SM_SERVICE_STATE_ENABLED_GO_STANDBY == dependent_service->state )
            {
                state_match = true;
            }
        break;

        case SM_SERVICE_ACTION_AUDIT_ENABLED:
        case SM_SERVICE_ACTION_AUDIT_DISABLED:
            // Ignore.
        break;

        default:
            DPRINTFE( "Unknown service (%s) dependency action(%s).",
                      service_dependency->service_name,
                      sm_service_action_str(service_dependency->action) );
            return;
    }

    if( state_match )
    {
        DPRINTFD( "Dependency (%s) for service (%s) requires state (%s), "
                  "dependent_state=%s, dependency_state=%s.", 
                  service->name, service_dependency->service_name,
                  sm_service_state_str(required_state),
                  sm_service_state_str(dependent_service->state),
                  sm_service_state_str(service_dependency->dependent_state) );

        *require_state = true;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - State Require State
// ========================================
static void sm_service_dependency_dependent_state_require_state(
    void* user_data[], SmServiceDependencyT* service_dependency )
{
    SmServiceT* service = (SmServiceT*) user_data[0];
    SmServiceStateT required_state = *(SmServiceStateT*) user_data[1];
    bool* require_state = (bool*) user_data[2];
    bool* at_least_one = (bool*) user_data[3];
    SmServiceT* dependent_service;
    
    if( '\0' == service_dependency->dependent[0] )
    {
        DPRINTFI( "Service (%s) has no dependencies.", service->name );
        return;
    }

    dependent_service = sm_service_table_read( service_dependency->service_name );
    if( NULL == dependent_service )
    {
        DPRINTFE( "Failed to read service (%s), error=%s.", 
                  service_dependency->service_name, 
                  sm_error_str(SM_NOT_FOUND) );
        return;
    }

    *at_least_one = true;

    if( service_dependency->state == dependent_service->state )
    {
        if( service_dependency->dependent_state != service->state )
        {
            DPRINTFD( "Dependency (%s) for service (%s) requires state (%s), "
                      "dependent_state=%s, dependency_state=%s.", 
                      service->name, service_dependency->service_name,
                      sm_service_state_str(required_state),
                      sm_service_state_str(dependent_service->state),
                      sm_service_state_str(service_dependency->dependent_state) );

            *require_state = true;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Dependents Require Disable
// ===============================================
SmErrorT sm_service_dependency_dependents_require_disable( SmServiceT* service,
    bool* disable_required )
{
    bool at_least_one = false;
    bool require = false;
    SmServiceStateT required_state = SM_SERVICE_STATE_DISABLED;
    void* user_data[] = {service, &required_state, &require, &at_least_one};

    *disable_required = false;

    if( SM_SERVICE_ACTION_NONE != service->action_running )
    {
        DPRINTFD( "Service (%s) running an action.", service->name );
        return( SM_OKAY );
    }

    sm_service_dependency_table_foreach_dependent(
            SM_SERVICE_DEPENDENCY_TYPE_ACTION, service->name, required_state,
            user_data, sm_service_dependency_dependent_action_require_state );

    if( at_least_one )
    {
        *disable_required = require;
    } else {        
        *disable_required = false;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Dependents Require Notification
// ====================================================
SmErrorT sm_service_dependency_dependents_require_notification(
    SmServiceT* service, bool* notification )
{
    bool at_least_one = false;
    bool require = false;
    SmServiceStateT required_state = service->state;
    void* user_data[] = {service, &required_state, &require, &at_least_one};

    *notification = false;

    if( SM_SERVICE_ACTION_NONE != service->action_running )
    {
        DPRINTFD( "Service (%s) running an action.", service->name );
        return( SM_OKAY );
    }

    sm_service_dependency_table_foreach_dependent(
            SM_SERVICE_DEPENDENCY_TYPE_ACTION, service->name, required_state,
            user_data, sm_service_dependency_dependent_action_require_state );

    if( at_least_one )
    {
        *notification = require;
    } else {        
        *notification = false;
    }

    at_least_one = false;

    sm_service_dependency_table_foreach_dependent(
            SM_SERVICE_DEPENDENCY_TYPE_STATE, service->name, required_state,
            user_data, sm_service_dependency_dependent_state_require_state );

    if( at_least_one )
    {
        *notification |= require;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Initialize
// ===============================
SmErrorT sm_service_dependency_initialize( void )
{
    SmErrorT error;

    error = sm_service_dependency_table_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed initialize service dependency table, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Dependency - Finalize
// =============================
SmErrorT sm_service_dependency_finalize( void )
{
    SmErrorT error;

    error = sm_service_dependency_table_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed finalize service dependency table, error=%s.",
                  sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
