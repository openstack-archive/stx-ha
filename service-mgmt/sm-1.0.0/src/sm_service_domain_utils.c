//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_utils.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_msg.h"
#include "sm_node_api.h"
#include "sm_service_group_api.h"
#include "sm_service_group_table.h"
#include "sm_service_domain_table.h"
#include "sm_service_domain_member_table.h"
#include "sm_service_domain_neighbor_table.h"
#include "sm_service_domain_interface_table.h"
#include "sm_service_domain_assignment_table.h"

#define NS_PER_SEC 1000000000
#define NS_PER_MS 1000000
// ****************************************************************************
// Service Domain Utilities - Core Service Group Enabled
// =====================================================
static void sm_service_domain_utils_core_service_group_enabled(
    void* user_data[], SmServiceGroupT* service_group )
{
    bool* enabled = (bool*) user_data[0];

    if(( service_group->core )&&
       ( SM_SERVICE_GROUP_STATE_ACTIVE != service_group->state ))
    {
        *enabled = false;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Primary Inteface Enabled
// ===================================================
static void sm_service_domain_utils_primary_interface_enabled( 
    void* user_data[], SmServiceDomainInterfaceT* interface )
{
    bool* enabled = (bool*) user_data[0];

    if( SM_INTERFACE_STATE_NOT_IN_USE == interface->interface_state )
    {
        return;
    }

    if(( SM_PATH_TYPE_PRIMARY == interface->path_type ) &&
       ( SM_INTERFACE_STATE_ENABLED != interface->interface_state ))
    {
        if( SM_SERVICE_DOMAIN_INTERFACE_CONNECT_TYPE_TOR == interface->interface_connect_type )
        {
            DPRINTFI( "Primary interface is DOWN. Node is disabled." );
            *enabled = false;
        }
        else
        {
            DPRINTFD( "Primary interface is DOWN, but it is direct connect interface." );
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Service Domain Enabled
// =================================================
SmErrorT sm_service_domain_utils_service_domain_enabled( char name[],
    bool* enabled  )
{
    void* user_data[] = { enabled };

    *enabled = true;

    sm_service_domain_interface_table_foreach_service_domain( name, user_data,
                            sm_service_domain_utils_primary_interface_enabled );

    if( *enabled )
    {
        sm_service_group_table_foreach( user_data, 
                sm_service_domain_utils_core_service_group_enabled );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Interface Send Hello
// ===============================================
static void sm_service_domain_utils_interface_send_hello( void* user_data[],
    SmServiceDomainInterfaceT* interface )
{
    char* hostname = (char*) user_data[0];
    SmServiceDomainT* domain = (SmServiceDomainT*) user_data[1];
    SmErrorT error;

    if(( SM_INTERFACE_STATE_ENABLED == interface->interface_state )&&
       ( SM_PATH_TYPE_STATUS_ONLY != interface->path_type ))
    {
        error = sm_msg_send_service_domain_hello( hostname, 
                    domain->orchestration, domain->designation,
                    domain->generation, domain->priority,
                    domain->hello_interval, domain->dead_interval,
                    domain->wait_interval, domain->exchange_interval,
                    domain->leader, interface );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send hello for service domain (%s) on "
                      "interface (%s), error=%s.", domain->name,
                      interface->service_domain_interface,
                      sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Hello
// =====================================
SmErrorT sm_service_domain_utils_send_hello( char name[] )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmServiceDomainT* domain = NULL;
    void* user_data[] = { hostname, domain };
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    domain = sm_service_domain_table_read( name );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain (%s), error=%s.",
                  name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    user_data[1] = domain;

    sm_msg_increment_seq_num();

    sm_service_domain_interface_table_foreach_service_domain( name, user_data,
                            sm_service_domain_utils_interface_send_hello );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Interface Send Pause
// ===============================================
static void sm_service_domain_utils_interface_send_pause( void* user_data[],
    SmServiceDomainInterfaceT* interface )
{
    char* hostname = (char*) user_data[0];
    SmServiceDomainT* domain = (SmServiceDomainT*) user_data[1];
    int pause_interval = *(int*) user_data[2];
    SmErrorT error;

    if(( SM_INTERFACE_STATE_ENABLED == interface->interface_state )&&
       ( SM_PATH_TYPE_STATUS_ONLY != interface->path_type ))
    {
        error = sm_msg_send_service_domain_pause( hostname, pause_interval,
                                                  interface );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send pause for service domain (%s) on "
                      "interface (%s), error=%s.", domain->name,
                      interface->service_domain_interface,
                      sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Pause
// =====================================
SmErrorT sm_service_domain_utils_send_pause( char name[], int pause_interval )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmServiceDomainT* domain = NULL;
    void* user_data[] = { hostname, domain, &pause_interval };
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    domain = sm_service_domain_table_read( name );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain (%s), error=%s.",
                  name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    user_data[1] = domain;

    sm_msg_increment_seq_num();

    sm_service_domain_interface_table_foreach_service_domain( name, user_data,
                            sm_service_domain_utils_interface_send_pause );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Inteface Send Exchange Start
// =======================================================
static void sm_service_domain_utils_interface_send_exchange_start(
    void* user_data[], SmServiceDomainInterfaceT* interface )
{
    char* hostname = (char*) user_data[0];
    SmServiceDomainT* domain = (SmServiceDomainT*) user_data[1];
    SmServiceDomainNeighborT* neighbor = (SmServiceDomainNeighborT*) user_data[2];
    int exchange_seq = *(int*) user_data[3];
    SmErrorT error;

    if(( SM_INTERFACE_STATE_ENABLED == interface->interface_state )&&
       ( SM_PATH_TYPE_STATUS_ONLY != interface->path_type ))
    {
        error = sm_msg_send_service_domain_exchange_start( hostname,
                                    neighbor->name, exchange_seq, interface );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send exchange start for service domain (%s) "
                      "on interface (%s), error=%s.", domain->name,
                      interface->service_domain_interface,
                      sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Exchange Start throttle
// ==============================================
SmErrorT sm_service_domain_utils_send_exchange_start_throttle( char name[],
    SmServiceDomainNeighborT* neighbor, int exchange_seq, int min_interval_ms )
{
    struct timespec now;
    int sec, nsec;
    clock_gettime( CLOCK_MONOTONIC_RAW, &now );
    sec = now.tv_sec - neighbor->last_exchange_start.tv_sec;
    nsec = now.tv_nsec - neighbor->last_exchange_start.tv_nsec;
    if (min_interval_ms >= (sec * NS_PER_SEC + nsec) / NS_PER_MS)
    {
        DPRINTFI("Donot send exchange start too often (%d ms)", min_interval_ms);
        return SM_OKAY;
    }

    return sm_service_domain_utils_send_exchange_start(name, neighbor, exchange_seq);
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Exchange Start
// ==============================================
SmErrorT sm_service_domain_utils_send_exchange_start( char name[],
    SmServiceDomainNeighborT* neighbor, int exchange_seq )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmServiceDomainT* domain = NULL;
    void* user_data[] = { hostname, domain, neighbor, &exchange_seq };
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    domain = sm_service_domain_table_read( name );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain (%s), error=%s.",
                  name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    user_data[1] = domain;

    sm_msg_increment_seq_num();

    sm_service_domain_interface_table_foreach_service_domain( name, user_data,
                    sm_service_domain_utils_interface_send_exchange_start );

    clock_gettime( CLOCK_MONOTONIC_RAW, &neighbor->last_exchange_start );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Inteface Send Exchange
// =================================================
static void sm_service_domain_utils_interface_send_exchange(
    void* user_data[], SmServiceDomainInterfaceT* interface )
{
    char* hostname = (char*) user_data[0];
    SmServiceDomainT* domain = (SmServiceDomainT*) user_data[1];
    SmServiceDomainNeighborT* neighbor = (SmServiceDomainNeighborT*) user_data[2];
    int exchange_seq = *(int*) user_data[3];
    int64_t member_id = *(int64_t*) user_data[4];
    char* member_name = (char*) user_data[5];
    SmServiceGroupStateT member_desired_state = *(SmServiceGroupStateT*) user_data[6];
    SmServiceGroupStateT member_state = *(SmServiceGroupStateT*) user_data[7];
    SmServiceGroupStatusT member_status = *(SmServiceGroupStatusT*) user_data[8];
    SmServiceGroupConditionT member_condition = *(SmServiceGroupConditionT*) user_data[9];
    int64_t member_health = *(int64_t*) user_data[10];
    char * reason_text = (char*) user_data[11];
    bool more_members = *(bool*) user_data[12];
    int64_t last_received_member_id = *(int64_t*) user_data[13];
    SmErrorT error;

    if(( SM_INTERFACE_STATE_ENABLED == interface->interface_state )&&
       ( SM_PATH_TYPE_STATUS_ONLY != interface->path_type ))
    {
        error = sm_msg_send_service_domain_exchange( hostname, neighbor->name,
                    exchange_seq, member_id, member_name, member_desired_state,
                    member_state, member_status, member_condition, member_health,
                    reason_text, more_members, last_received_member_id, interface );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send exchange for service domain (%s) "
                      "on interface (%s), error=%s.", domain->name,
                      interface->service_domain_interface,
                      sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Exchange
// ========================================
SmErrorT sm_service_domain_utils_send_exchange( char name[],
    SmServiceDomainNeighborT* neighbor, int exchange_seq, int64_t member_id,
    char member_name[], SmServiceGroupStateT member_desired_state,
    SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
    SmServiceGroupConditionT member_condition, int64_t member_health,
    const char reason_text[], bool more_members, int64_t last_received_member_id )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmServiceDomainT* domain = NULL;
    void* user_data[] = { hostname, domain, neighbor, &exchange_seq,
                          &member_id, member_name, &member_desired_state, 
                          &member_state, &member_status,  &member_condition,
                          &member_health, (void*) &(reason_text[0]), 
                          &more_members, &last_received_member_id };
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    domain = sm_service_domain_table_read( name );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain (%s), error=%s.",
                  name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    user_data[1] = domain;

    sm_msg_increment_seq_num();

    sm_service_domain_interface_table_foreach_service_domain( name, user_data,
                            sm_service_domain_utils_interface_send_exchange );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Inteface Send Member Request
// =======================================================
static void sm_service_domain_utils_interface_send_member_request(
    void* user_data[], SmServiceDomainInterfaceT* interface )
{
    char* hostname = (char*) user_data[0];
    SmServiceDomainT* domain = (SmServiceDomainT*) user_data[1];
    char* member_node_name = (char*) user_data[2];
    int64_t member_id = *(int64_t*) user_data[3];
    char* member_name = (char*) user_data[4];
    SmServiceGroupActionT member_action = *(SmServiceGroupActionT*) user_data[5];
    SmServiceGroupActionFlagsT member_action_flags = *(SmServiceGroupActionFlagsT*)
                                                     user_data[6];
    SmErrorT error;

    if(( SM_INTERFACE_STATE_ENABLED == interface->interface_state )&&
       ( SM_PATH_TYPE_STATUS_ONLY != interface->path_type ))
    {
        error = sm_msg_send_service_domain_member_request( hostname,
                        member_node_name, member_id, member_name, member_action,
                        member_action_flags, interface );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send member (%s) request for service domain "
                      "(%s) on interface (%s), error=%s.", member_name,
                      domain->name, interface->service_domain_interface,
                      sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Member Request
// ==============================================
SmErrorT sm_service_domain_utils_send_member_request( char name[],
    char member_node_name[], int64_t member_id, char member_name[],
    SmServiceGroupActionT member_action,
    SmServiceGroupActionFlagsT member_action_flags )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmServiceDomainT* domain = NULL;
    void* user_data[] = { hostname, domain, member_node_name,
                          &member_id, member_name, &member_action,
                          &member_action_flags };
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    domain = sm_service_domain_table_read( name );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain (%s), error=%s.",
                  name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    user_data[1] = domain;

    sm_msg_increment_seq_num();

    sm_service_domain_interface_table_foreach_service_domain( name, user_data,
                        sm_service_domain_utils_interface_send_member_request );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Inteface Send Member Update
// ======================================================
static void sm_service_domain_utils_interface_send_member_update(
    void* user_data[], SmServiceDomainInterfaceT* interface )
{
    char* hostname = (char*) user_data[0];
    char* member_node_name = (char*) user_data[1];
    int64_t member_id = *(int64_t*) user_data[2];
    char* member_name = (char*) user_data[3];
    SmServiceGroupStateT member_desired_state = *(SmServiceGroupStateT*) user_data[4];
    SmServiceGroupStateT member_state = *(SmServiceGroupStateT*) user_data[5];
    SmServiceGroupStatusT member_status = *(SmServiceGroupStatusT*) user_data[6];
    SmServiceGroupConditionT member_condition = *(SmServiceGroupConditionT*) user_data[7];
    int64_t health = *(int64_t*) user_data[8];
    const char* reason_text = (char*) user_data[9];
    SmErrorT error;

    if(( SM_INTERFACE_STATE_ENABLED == interface->interface_state )&&
       ( SM_PATH_TYPE_STATUS_ONLY != interface->path_type ))
    {
        error = sm_msg_send_service_domain_member_update( hostname,
                            member_node_name, member_id, member_name, 
                            member_desired_state, member_state, member_status,
                            member_condition, health, reason_text, interface );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to send member (%s) update for service domain "
                      "(%s) on interface (%s), error=%s.", member_name,
                      interface->service_domain,
                      interface->service_domain_interface,
                      sm_error_str( error ) );
            return;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Send Member Update
// =============================================
SmErrorT sm_service_domain_utils_send_member_update( char name[],
    char member_node_name[], int64_t member_id,
    char member_name[], SmServiceGroupStateT member_desired_state,
    SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
    SmServiceGroupConditionT member_condition, int64_t health,
    const char reason_text[] )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmServiceDomainT* domain;
    void* user_data[] = { hostname, member_node_name, &member_id, member_name,
                          &member_desired_state, &member_state, &member_status,
                          &member_condition, &health, (void*) &(reason_text[0]) };
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    domain = sm_service_domain_table_read( name );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain (%s), error=%s.",
                  name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    sm_msg_increment_seq_num();

    sm_service_domain_interface_table_foreach_service_domain( name, user_data,
                        sm_service_domain_utils_interface_send_member_update );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Neighbor Elect
// =========================================
static void sm_service_domain_utils_neighbor_elect( void* user_data[],
    SmServiceDomainNeighborT* neighbor )
{
    int* leader_generation = (int*) user_data[0];
    int* leader_priority = (int*) user_data[1];
    char* leader_name = (char*) user_data[2];

    if( SM_SERVICE_DOMAIN_NEIGHBOR_STATE_DOWN == neighbor->state )
    {
        DPRINTFD( "Neighbor (%s) is in the down state, skip.", neighbor->name );
        return;
    }

    if( *leader_generation < neighbor->generation )
    {    
        DPRINTFI( "New leader (%s), old=%s.", leader_name, neighbor->name );

        *leader_generation = neighbor->generation;
        *leader_priority = neighbor->priority;
        snprintf( leader_name, SM_NODE_NAME_MAX_CHAR, "%s", neighbor->name );

    } else if( *leader_generation == neighbor->generation ) {

        if( *leader_priority < neighbor->priority )
        {
            DPRINTFI( "New leader (%s), old=%s.", leader_name,
                      neighbor->name );

            *leader_generation = neighbor->generation;
            *leader_priority = neighbor->priority;
            snprintf( leader_name, SM_NODE_NAME_MAX_CHAR, "%s",
                      neighbor->name );

        } else if( *leader_priority == neighbor->priority ) {

            if( 0 < strcmp( neighbor->name, leader_name  ) )
            {
                DPRINTFI( "New leader (%s), old=%s.", leader_name,
                          neighbor->name );

                *leader_generation = neighbor->generation;
                *leader_priority = neighbor->priority;
                snprintf( leader_name, SM_NODE_NAME_MAX_CHAR, "%s",
                          neighbor->name );
            }
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Hold Election
// ========================================
SmErrorT sm_service_domain_utils_hold_election( char name[], char leader[],
    bool* elected )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];
    int leader_generation;
    int leader_priority;
    void* user_data[] = { &leader_generation, &leader_priority, leader };
    SmServiceDomainT* domain;
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    domain = sm_service_domain_table_read( name );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain (%s), error=%s.",
                  name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    // Elect self.
    leader_generation = domain->generation;
    leader_priority = domain->priority;
    snprintf( leader, SM_NODE_NAME_MAX_CHAR, "%s", hostname );

    sm_service_domain_neighbor_table_foreach( domain->name, user_data,
                                sm_service_domain_utils_neighbor_elect );

    *elected = (0 == strcmp( leader, hostname ) );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Service Domain Neighbor InSync
// =========================================================
SmErrorT sm_service_domain_utils_service_domain_neighbor_insync(
    char name[], char node_name[], bool* insync  )
{
    SmServiceDomainNeighborT* neighbor;

    *insync = false;

    neighbor = sm_service_domain_neighbor_table_read( node_name, name );
    if( NULL == neighbor )
    {
        DPRINTFE( "Service domain (%s) neighbor (%s) not found.",
                  name, node_name );
        return( SM_NOT_FOUND );
    }

    if( SM_SERVICE_DOMAIN_NEIGHBOR_STATE_FULL == neighbor->state )
    {
        *insync = true;
    }
    else
    {
        DPRINTFI( "Service domain (%s) neighbor (%s) state is %s.",
                  name, node_name, sm_service_domain_neighbor_state_str(neighbor->state) );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Neighbor State Full
// ==============================================
static void sm_service_domain_utils_neighbor_state_full( void* user_data[],
    SmServiceDomainNeighborT* neighbor )
{
    bool* insync = (bool*) user_data[0];

    if( SM_SERVICE_DOMAIN_NEIGHBOR_STATE_FULL != neighbor->state )
    {
        *insync = false;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Service Domain Neighbors InSync
// ==========================================================
SmErrorT sm_service_domain_utils_service_domain_neighbors_insync(
    char name[], bool* insync )
{
    void* user_data[] = { insync };
    SmServiceDomainT* domain;

    *insync = true;

    domain = sm_service_domain_table_read( name );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain (%s), error=%s.",
                  name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    sm_service_domain_neighbor_table_foreach( domain->name, user_data,
                                sm_service_domain_utils_neighbor_state_full );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Neighbor State Syncing
// =================================================
static void sm_service_domain_utils_neighbor_state_syncing( void* user_data[],
    SmServiceDomainNeighborT* neighbor )
{
    bool* syncing = (bool*) user_data[0];

    if(( SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE_START == neighbor->state )||
       ( SM_SERVICE_DOMAIN_NEIGHBOR_STATE_EXCHANGE == neighbor->state ))
    {
        *syncing = true;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Service Domain Neighbors Syncing
// ===========================================================
SmErrorT sm_service_domain_utils_service_domain_neighbors_syncing(
    char name[], bool* syncing )
{
    void* user_data[] = { syncing };
    SmServiceDomainT* domain;

    *syncing = false;

    domain = sm_service_domain_table_read( name );
    if( NULL == domain )
    {
        DPRINTFE( "Failed to read service domain (%s), error=%s.",
                  name, sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    sm_service_domain_neighbor_table_foreach( domain->name, user_data,
                            sm_service_domain_utils_neighbor_state_syncing );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Update Assignment
// ============================================
SmErrorT sm_service_domain_utils_update_assignment( char name[],
    char node_name[], char service_group_name[],
    SmServiceGroupStateT desired_state, SmServiceGroupStateT state,
    SmServiceGroupStatusT status, SmServiceGroupConditionT condition,
    int64_t health, const char reason_text[] )
{
    struct timespec ts;
    SmServiceDomainAssignmentT* assignment;
    SmErrorT error;

    clock_gettime( CLOCK_MONOTONIC_RAW, &ts );

    assignment = sm_service_domain_assignment_table_read( name, node_name,
                                                          service_group_name );
    if( NULL != assignment )
    {
        if(( assignment->desired_state != desired_state )||
           ( assignment->state != state )||( assignment->status != status ))
        {
            assignment->last_state_change = (long) ts.tv_sec;
        }

        assignment->desired_state = desired_state;
        assignment->state = state;
        assignment->status = status;
        assignment->condition = condition;
        assignment->health = health;

        if( 0 != strcmp( assignment->reason_text, reason_text ) )
        {
            snprintf( assignment->reason_text, sizeof(assignment->reason_text),
                      "%s", reason_text );
        }

       error = sm_service_domain_assignment_table_persist( assignment );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to update assignment (%s) for service domain "
                      "(%s) node (%s), error=%s.", service_group_name, name,
                      node_name, sm_error_str( error ) );
            return( error );
        }
    } else {
        error = sm_service_domain_assignment_table_insert( name, node_name,
                    service_group_name, desired_state, state, status, condition,
                    health, (long) ts.tv_sec, reason_text );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to insert assignment (%s) for service domain "
                      "(%s) node (%s), error=%s.", service_group_name, name,
                      node_name, sm_error_str( error ) );
            return( error );
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Update Own Assignment
// ================================================
static void sm_service_domain_utils_update_own_assignment( void* user_data[],
    SmServiceDomainMemberT* member )
{
    char* service_domain_name = (char*) user_data[0];
    char* node_name = (char*) user_data[1];
    SmServiceGroupT* service_group;
    SmErrorT error;

    service_group = sm_service_group_table_read( member->service_group_name );
    if( NULL == service_group )
    {
        DPRINTFE( "Failed to read service group (%s), error=%s.",
                  member->service_group_name, sm_error_str(SM_NOT_FOUND) );
        return;
    }

    error = sm_service_domain_utils_update_assignment( service_domain_name,
                node_name, service_group->name, service_group->desired_state,
                service_group->state, service_group->status,
                service_group->condition, service_group->health,
                service_group->reason_text );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update own assignments for service domain "
                  "(%s) node (%s) service group (%s), error=%s.",
                  service_domain_name, node_name, service_group->name,
                  sm_error_str( error ) );
        return;
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Update Own Assignments
// =================================================
SmErrorT sm_service_domain_utils_update_own_assignments( char name[] )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];
    void* user_data[] = { name, hostname };
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    sm_service_domain_member_table_foreach( name, user_data,
                            sm_service_domain_utils_update_own_assignment );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Disable Assignment
// =============================================
static void sm_service_domain_utils_disable_assignment( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{
    SmErrorT error;

    error = sm_service_group_api_disable( assignment->service_group_name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to perform disable action on member (%s) "
                  "for service domain (%s) on node (%s), error=%s.",
                  assignment->service_group_name, assignment->name,
                  assignment->node_name, sm_error_str(error) );
        return;
    }

    assignment->desired_state = SM_SERVICE_GROUP_STATE_DISABLED;
    assignment->state = SM_SERVICE_GROUP_STATE_DISABLED;
    assignment->status = SM_SERVICE_GROUP_STATUS_NONE;
    assignment->health = 0;

    error = sm_service_domain_assignment_table_persist( assignment );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update assignment (%s) for service domain "
                  "(%s) node (%s), error=%s.", 
                  assignment->service_group_name, assignment->name,
                  assignment->node_name, sm_error_str( error ) );
        return;
    }

    DPRINTFI( "Assignment (%s) for service domain (%s) node (%s) disabled.",
              assignment->service_group_name, assignment->name,
              assignment->node_name );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Disable Self
// =======================================
SmErrorT sm_service_domain_utils_service_domain_disable_self( char name[] )
{
    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    sm_service_domain_assignment_table_foreach_node_in_service_domain( name,
            hostname, NULL, sm_service_domain_utils_disable_assignment );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Neighbor active assignement
// ======================================================
static void sm_service_domain_utils_active_assignment( void* user_data[],
    SmServiceDomainAssignmentT* assignment )
{

    SmErrorT error;

    error = sm_service_group_api_go_active( assignment->service_group_name );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to perform disable action on member (%s) "
                  "for service domain (%s) on node (%s), error=%s.",
                  assignment->service_group_name, assignment->name,
                  assignment->node_name, sm_error_str(error) );
        return;
    }

    assignment->desired_state = SM_SERVICE_GROUP_STATE_ACTIVE;
    assignment->state = SM_SERVICE_GROUP_STATE_GO_ACTIVE;
    assignment->status = SM_SERVICE_GROUP_STATUS_NONE;
    assignment->health = 0;

    error = sm_service_domain_assignment_table_persist( assignment );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update assignment (%s) for service domain "
                  "(%s) node (%s), error=%s.",
                  assignment->service_group_name, assignment->name,
                  assignment->node_name, sm_error_str( error ) );
        return;
    }

    DPRINTFI( "Assignment (%s) for service domain (%s) node (%s) active.",
              assignment->service_group_name, assignment->name,
              assignment->node_name );

}

// ****************************************************************************
// Service Domain Utilities - Neighbor active self
// ======================================================
SmErrorT sm_service_domain_utils_service_domain_active_self( char name[] )
{

    char hostname[SM_NODE_NAME_MAX_CHAR];
    SmErrorT error;

    error = sm_node_api_get_hostname( hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get hostname, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    sm_service_domain_assignment_table_foreach_node_in_service_domain( name,
            hostname, NULL, sm_service_domain_utils_active_assignment );

    return( SM_OKAY );

}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Neighbor Cleanup Assignment
// ======================================================
static void sm_service_domain_utils_neighbor_cleanup_assignment(
    void* user_data[], SmServiceDomainAssignmentT* assignment )
{
    SmErrorT error;

    assignment->desired_state = SM_SERVICE_GROUP_STATE_DISABLED;
    assignment->state = SM_SERVICE_GROUP_STATE_DISABLED;
    assignment->status = SM_SERVICE_GROUP_STATUS_NONE;
    assignment->health = 0;

    error = sm_service_domain_assignment_table_persist( assignment );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to update assignment (%s) for service domain "
                  "(%s) node (%s), error=%s.", 
                  assignment->service_group_name, assignment->name,
                  assignment->node_name, sm_error_str( error ) );
        return;
    }

    DPRINTFI( "Assignment (%s) for service domain (%s) node (%s) disabled.",
              assignment->service_group_name, assignment->name,
              assignment->node_name );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Neighbor Cleanup
// ===========================================
SmErrorT sm_service_domain_utils_service_domain_neighbor_cleanup( 
    char name[], char node_name[] )
{
    sm_service_domain_assignment_table_foreach_node_in_service_domain( name,
        node_name, NULL, sm_service_domain_utils_neighbor_cleanup_assignment );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities is aa service group
// =====================================
bool sm_is_aa_service_group(char* service_group_name)
{
    SmServiceDomainMemberT* service_domain_member;
    service_domain_member = sm_service_domain_member_table_read_service_group( service_group_name );
    if( NULL != service_domain_member )
    {
        if( SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N == service_domain_member->redundancy_model &&
            service_domain_member->n_active > 1 )
        {
            return true;
        }
    }
    return false;
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Initialize
// =====================================
SmErrorT sm_service_domain_utils_initialize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Utilities - Finalize
// ===================================
SmErrorT sm_service_domain_utils_finalize( void )
{
    return( SM_OKAY );
}
// ****************************************************************************
