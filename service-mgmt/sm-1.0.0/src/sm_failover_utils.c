#include "sm_failover_utils.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "sm_debug.h"
#include "sm_service_domain_member_table.h"
#include "sm_service_domain_assignment_table.h"
#include "sm_service_domain_table.h"

#define SM_NODE_STAY_FAILED_FILE      "/var/run/.sm_stay_fail"

// ****************************************************************************
// Failover utilities - loop all service domain members
// ==================
static void service_domain_member_foreach_cb(void* user_data[], SmServiceDomainMemberT* member)
{
    char* node_name = (char*)user_data[4];
    if( 0 == strcmp(member->service_group_aggregate, "controller-aggregate"))
    {
        SmServiceDomainAssignmentT* assignment = sm_service_domain_assignment_table_read(
                member->name,
                node_name,
                member->service_group_name
            );

        bool* is_active = (bool*) user_data[0];
        bool* is_standby = (bool*) user_data[1];
        bool* is_init = (bool*) user_data[2];
        bool* is_failed = (bool*) user_data[3];
        if( NULL == assignment )
        {
            *is_init = true;
            DPRINTFD("Waiting for service assignments to be scheduled.");
            return;
        }
        if( SM_SERVICE_GROUP_STATE_ACTIVE == assignment->desired_state )
        {
            *is_active = true;
        }else if (SM_SERVICE_GROUP_STATE_STANDBY == assignment->desired_state )
        {
            *is_standby = true;
        }else if ( SM_SERVICE_GROUP_STATE_DISABLED == assignment->desired_state)
        {
            *is_failed = true;
        }else if ( SM_SERVICE_GROUP_STATE_INITIAL == assignment->desired_state )
        {
            *is_init = true;
        }
    }
}
// ****************************************************************************

// ****************************************************************************
// Failover utilities - callback for service domain table loop
// ==================
static void service_domain_table_each_callback(void* user_data[], SmServiceDomainT* domain)
{
    sm_service_domain_member_table_foreach(
        domain->name,
        user_data,
        service_domain_member_foreach_cb);
}
// ****************************************************************************

// ****************************************************************************
// Failover utilities - get controller state
// ==================
SmNodeScheduleStateT sm_get_controller_state(
    const char node_name[])
{
    SmNodeScheduleStateT state = SM_NODE_STATE_UNKNOWN;
    bool is_active = false;
    bool is_standby = false;
    bool is_init = false;
    bool is_failed = false;
    void* user_data[] = {(void*) &is_active, (void*) &is_standby,
                         (void*) &is_init, (void*) &is_failed, (void*)node_name};
    sm_service_domain_table_foreach( user_data, service_domain_table_each_callback);
    if( is_init )
    {
        state = SM_NODE_STATE_INIT;
    }
    else if ( is_standby )
    {
        state = SM_NODE_STATE_STANDBY;
    }
    else if ( is_active )
    {
        state = SM_NODE_STATE_ACTIVE;
    }
    return state;
}
// ****************************************************************************



// ****************************************************************************
// Failover Utilities - Set StayFailed
// ==============================
SmErrorT sm_failover_utils_set_stayfailed_flag()
{
    int fd = open( SM_NODE_STAY_FAILED_FILE,
                   O_RDWR | O_CREAT, S_IRUSR | S_IRGRP | S_IROTH);
    if( 0 > fd )
    {
        DPRINTFE( "Failed to create file (%s), error=%s.",
                  SM_NODE_STAY_FAILED_FILE, strerror(errno) );
        return( SM_FAILED );
    }

    close(fd);
    return( SM_OKAY );

}
// ****************************************************************************

// ****************************************************************************
// Failover Utilities - Set StayFailed
// ==============================
SmErrorT sm_failover_utils_reset_stayfailed_flag()
{
    unlink( SM_NODE_STAY_FAILED_FILE );
    return( SM_OKAY );
}
// ****************************************************************************


// ****************************************************************************
// Failover Utilities - check StayFailed
// ==============================
bool sm_failover_utils_is_stayfailed()
{
    if( 0 == access( SM_NODE_STAY_FAILED_FILE,  F_OK ) )
    {
        return true;
    }
    return false;
}
// ****************************************************************************
