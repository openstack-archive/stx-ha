//
// Copyright (c) 2014 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_log.h"

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h> 
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <pthread.h>

#include "sm_limits.h"
#include "sm_types.h"
#include "sm_debug.h"
#include "sm_log_thread.h"
#include "sm_node_utils.h"
#include "sm_service_domain_member_table.h"
#include "sm_service_group_member_table.h"

static int _server_fd = -1;
static int _client_fd = -1;

// ****************************************************************************
// Log - Node Reboot
// =================
void sm_log_node_reboot( char entity_name[], const char reason_text[],
    bool forced )
{
    SmLogThreadMsgT msg;
    SmLogThreadMsgRebootLogT* reboot = &(msg.u.reboot);

    memset( &msg, 0, sizeof(msg) );

    if( forced )
    {
        msg.type = SM_LOG_THREAD_MSG_NODE_REBOOT_FORCE_LOG;
    } else {
        msg.type = SM_LOG_THREAD_MSG_NODE_REBOOT_LOG;
    }

    clock_gettime( CLOCK_REALTIME, &(reboot->ts_real) );

    snprintf( reboot->node_name, sizeof(reboot->node_name),
              "%s", entity_name );
    snprintf( reboot->entity_name, sizeof(reboot->entity_name),
              "%s", entity_name );
    snprintf( reboot->reason_text, sizeof(reboot->reason_text),
              "%s", reason_text );

    send( _client_fd, &msg, sizeof(msg), 0 );
}
// ****************************************************************************

// ****************************************************************************
// Log - Node State Change
// =======================
void sm_log_node_state_change( char entity_name[], const char prev_state[],
    const char state[], const char reason_text[] )
{
    SmLogThreadMsgT msg;
    SmLogThreadMsgStateChangeLogT* state_change = &(msg.u.state_change);

    memset( &msg, 0, sizeof(msg) );
    
    msg.type = SM_LOG_THREAD_MSG_NODE_STATE_CHANGE_LOG;

    clock_gettime( CLOCK_REALTIME, &(state_change->ts_real) );

    snprintf( state_change->node_name, sizeof(state_change->node_name),
              "%s", entity_name );
    snprintf( state_change->entity_name, sizeof(state_change->entity_name),
              "%s", entity_name );
    snprintf( state_change->prev_state, sizeof(state_change->prev_state),
              "%s", prev_state );
    state_change->prev_status[0] = '\0';
    snprintf( state_change->state, sizeof(state_change->state),
              "%s", state );
    state_change->status[0] = '\0';
    snprintf( state_change->reason_text, sizeof(state_change->reason_text),
              "%s", reason_text );

    send( _client_fd, &msg, sizeof(msg), 0 );
}
// ****************************************************************************

// ****************************************************************************
// Log - Interface State Change
// ============================
void sm_log_interface_state_change( char entity_name[],
    const char prev_state[], const char state[], const char reason_text[] )
{
    char hostname[SM_NODE_NAME_MAX_CHAR] = "";
    SmLogThreadMsgT msg;
    SmLogThreadMsgStateChangeLogT* state_change = &(msg.u.state_change);
    SmErrorT error;

    memset( &msg, 0, sizeof(msg) );
    
    msg.type = SM_LOG_THREAD_MSG_INTERFACE_STATE_CHANGE_LOG;

    clock_gettime( CLOCK_REALTIME, &(state_change->ts_real) );

    error = sm_node_utils_get_hostname( hostname );
    if( SM_OKAY == error )
    {
        snprintf( state_change->node_name, sizeof(state_change->node_name),
                  "%s", hostname );
    }

    snprintf( state_change->entity_name, sizeof(state_change->entity_name),
              "%s", entity_name );
    snprintf( state_change->prev_state, sizeof(state_change->prev_state),
              "%s", prev_state );
    state_change->prev_status[0] = '\0';
    snprintf( state_change->state, sizeof(state_change->state),
              "%s", state );
    state_change->status[0] = '\0';
    snprintf( state_change->reason_text, sizeof(state_change->reason_text),
              "%s", reason_text );

    send( _client_fd, &msg, sizeof(msg), 0 );
}
// ****************************************************************************

// ****************************************************************************
// Log -  Comunication State Change
// ================================
void sm_log_communication_state_change( char entity_name[],
    const char reason_text[] )
{
    char hostname[SM_NODE_NAME_MAX_CHAR] = "";
    SmLogThreadMsgT msg;
    SmLogThreadMsgStateChangeLogT* state_change = &(msg.u.state_change);
    SmErrorT error;

    memset( &msg, 0, sizeof(msg) );
    
    msg.type = SM_LOG_THREAD_MSG_COMMUNICATION_STATE_CHANGE_LOG;

    clock_gettime( CLOCK_REALTIME, &(state_change->ts_real) );

    error = sm_node_utils_get_hostname( hostname );
    if( SM_OKAY == error )
    {
        snprintf( state_change->node_name, sizeof(state_change->node_name),
                  "%s", hostname );
    }
    
    snprintf( state_change->entity_name, sizeof(state_change->entity_name),
              "%s", entity_name );
    snprintf( state_change->reason_text, sizeof(state_change->reason_text),
              "%s", reason_text );

    send( _client_fd, &msg, sizeof(msg), 0 );
}
// ****************************************************************************

// ****************************************************************************
// Log - Neighbor State Change
// ===========================
void sm_log_neighbor_state_change( char entity_name[], const char prev_state[],
    const char state[], const char reason_text[] )
{
    char hostname[SM_NODE_NAME_MAX_CHAR] = "";
    SmLogThreadMsgT msg;
    SmLogThreadMsgStateChangeLogT* state_change = &(msg.u.state_change);
    SmErrorT error;

    memset( &msg, 0, sizeof(msg) );
    
    msg.type = SM_LOG_THREAD_MSG_NEIGHBOR_STATE_CHANGE_LOG;

    clock_gettime( CLOCK_REALTIME, &(state_change->ts_real) );

    error = sm_node_utils_get_hostname( hostname );
    if( SM_OKAY == error )
    {
        snprintf( state_change->node_name, sizeof(state_change->node_name),
                  "%s", hostname );
    }

    snprintf( state_change->entity_name, sizeof(state_change->entity_name),
              "%s", entity_name );
    snprintf( state_change->prev_state, sizeof(state_change->prev_state),
              "%s", prev_state );
    state_change->prev_status[0] = '\0';
    snprintf( state_change->state, sizeof(state_change->state),
              "%s", state );
    state_change->status[0] = '\0';
    snprintf( state_change->reason_text, sizeof(state_change->reason_text),
              "%s", reason_text );

    send( _client_fd, &msg, sizeof(msg), 0 );
}
// ****************************************************************************

// ****************************************************************************
// Log - Service Domain State Change
// =================================
void sm_log_service_domain_state_change( char entity_name[],
    const char prev_state[], const char state[], const char reason_text[] )
{
    char hostname[SM_NODE_NAME_MAX_CHAR] = "";
    SmLogThreadMsgT msg;
    SmLogThreadMsgStateChangeLogT* state_change = &(msg.u.state_change);
    SmErrorT error;

    memset( &msg, 0, sizeof(msg) );
    
    msg.type = SM_LOG_THREAD_MSG_SERVICE_DOMAIN_STATE_CHANGE_LOG;

    clock_gettime( CLOCK_REALTIME, &(state_change->ts_real) );

    error = sm_node_utils_get_hostname( hostname );
    if( SM_OKAY == error )
    {
        snprintf( state_change->node_name, sizeof(state_change->node_name),
                  "%s", hostname );
    }

    snprintf( state_change->domain_name, sizeof(state_change->domain_name),
              "%s", entity_name );

    snprintf( state_change->entity_name, sizeof(state_change->entity_name),
              "%s", entity_name );
    snprintf( state_change->prev_state, sizeof(state_change->prev_state),
              "%s", prev_state );
    state_change->prev_status[0] = '\0';
    snprintf( state_change->state, sizeof(state_change->state),
              "%s", state );
    state_change->status[0] = '\0';
    snprintf( state_change->reason_text, sizeof(state_change->reason_text),
              "%s", reason_text );

    send( _client_fd, &msg, sizeof(msg), 0 );
}
// ****************************************************************************

// ****************************************************************************
// Log - Service Group Redundancy Change
// =====================================
void sm_log_service_group_redundancy_change( char entity_name[],
    const char reason_text[] ) 
{
    char hostname[SM_NODE_NAME_MAX_CHAR] = "";
    SmServiceDomainMemberT* domain_member;
    SmLogThreadMsgT msg;
    SmLogThreadMsgStateChangeLogT* state_change = &(msg.u.state_change);
    SmErrorT error;

    memset( &msg, 0, sizeof(msg) );
    
    msg.type = SM_LOG_THREAD_MSG_SERVICE_GROUP_REDUNDANCY_CHANGE_LOG;
    
    clock_gettime( CLOCK_REALTIME, &(state_change->ts_real) );
    
    error = sm_node_utils_get_hostname( hostname );
    if( SM_OKAY == error )
    {
        snprintf( state_change->node_name, sizeof(state_change->node_name),
                  "%s", hostname );
    }

    domain_member 
        = sm_service_domain_member_table_read_service_group( entity_name );
    if( NULL != domain_member )
    {
        snprintf( state_change->domain_name, sizeof(state_change->domain_name),
                  "%s", domain_member->name );
    }

    snprintf( state_change->service_group_name,
              sizeof(state_change->service_group_name),
              "%s", entity_name );

    snprintf( state_change->entity_name, sizeof(state_change->entity_name),
              "%s", entity_name );
    snprintf( state_change->reason_text, sizeof(state_change->reason_text),
              "%s", reason_text );

    send( _client_fd, &msg, sizeof(msg), 0 );
}
// ****************************************************************************

// ****************************************************************************
// Log - Service Group State Change
// ================================
void sm_log_service_group_state_change( char entity_name[],
    const char prev_state[], const char prev_status[], const char state[],
    const char status[], const char reason_text[] ) 
{
    char hostname[SM_NODE_NAME_MAX_CHAR] = "";
    SmServiceDomainMemberT* domain_member;
    SmLogThreadMsgT msg;
    SmLogThreadMsgStateChangeLogT* state_change = &(msg.u.state_change);
    SmErrorT error;

    memset( &msg, 0, sizeof(msg) );
    
    msg.type = SM_LOG_THREAD_MSG_SERVICE_GROUP_STATE_CHANGE_LOG;
    
    clock_gettime( CLOCK_REALTIME, &(state_change->ts_real) );
    
    error = sm_node_utils_get_hostname( hostname );
    if( SM_OKAY == error )
    {
        snprintf( state_change->node_name, sizeof(state_change->node_name),
                  "%s", hostname );
    }

    domain_member 
        = sm_service_domain_member_table_read_service_group( entity_name );
    if( NULL != domain_member )
    {
        snprintf( state_change->domain_name, sizeof(state_change->domain_name),
                  "%s", domain_member->name );
    }

    snprintf( state_change->service_group_name,
              sizeof(state_change->service_group_name),
              "%s", entity_name );

    snprintf( state_change->entity_name, sizeof(state_change->entity_name),
              "%s", entity_name );
    snprintf( state_change->prev_state, sizeof(state_change->prev_state),
              "%s", prev_state );
    snprintf( state_change->prev_status, sizeof(state_change->prev_status),
              "%s", prev_status );
    snprintf( state_change->state, sizeof(state_change->state),
              "%s", state );
    snprintf( state_change->status, sizeof(state_change->status),
              "%s", status );
    snprintf( state_change->reason_text, sizeof(state_change->reason_text),
              "%s", reason_text );

    send( _client_fd, &msg, sizeof(msg), 0 );
}
// ****************************************************************************

// ****************************************************************************
// Log - Service State Change
// ==========================
void sm_log_service_state_change( char entity_name[], 
    const char prev_state[], const char prev_status[], const char state[],
    const char status[], const char reason_text[] )
{
    char hostname[SM_NODE_NAME_MAX_CHAR] = "";
    SmServiceDomainMemberT* domain_member;
    SmServiceGroupMemberT* service_group_member;
    SmLogThreadMsgT msg;
    SmLogThreadMsgStateChangeLogT* state_change = &(msg.u.state_change);
    SmErrorT error;

    memset( &msg, 0, sizeof(msg) );
    
    msg.type = SM_LOG_THREAD_MSG_SERVICE_STATE_CHANGE_LOG;

    clock_gettime( CLOCK_REALTIME, &(state_change->ts_real) );

    error = sm_node_utils_get_hostname( hostname );
    if( SM_OKAY == error )
    {
        snprintf( state_change->node_name, sizeof(state_change->node_name),
                  "%s", hostname );
    }

    service_group_member
        = sm_service_group_member_table_read_by_service( entity_name );
    if( NULL != service_group_member )
    {
        snprintf( state_change->service_group_name,
                  sizeof(state_change->service_group_name),
                  "%s", service_group_member->name );

        domain_member 
            = sm_service_domain_member_table_read_service_group(
                    service_group_member->name );
        if( NULL != domain_member )
        {
            snprintf( state_change->domain_name,
                      sizeof(state_change->domain_name), "%s",
                      domain_member->name );
        }
    }

    snprintf( state_change->entity_name, sizeof(state_change->entity_name),
              "%s", entity_name );
    snprintf( state_change->prev_state, sizeof(state_change->prev_state),
              "%s", prev_state );
    snprintf( state_change->prev_status, sizeof(state_change->prev_status),
              "%s", prev_status );
    snprintf( state_change->state, sizeof(state_change->state),
              "%s", state );
    snprintf( state_change->status, sizeof(state_change->status),
              "%s", status );
    snprintf( state_change->reason_text, sizeof(state_change->reason_text),
              "%s", reason_text );

    send( _client_fd, &msg, sizeof(msg), 0 );
}
// ****************************************************************************

// ****************************************************************************
// Log - Initialize
// ================
SmErrorT sm_log_initialize( void )
{
    int flags;
    int sockets[2];
    int buffer_len = 1048576;
    int result;
    SmErrorT error;

    result = socketpair( AF_UNIX, SOCK_DGRAM, 0, sockets );
    if( 0 > result )
    {
        DPRINTFE( "Failed to create log communication sockets, error=%s.",
                  strerror( errno ) );
        return( SM_FAILED );
    }

    int socket_i;
    for( socket_i=0; socket_i < 2; ++socket_i )
    {
        flags = fcntl( sockets[socket_i] , F_GETFL, 0 );
        if( 0 > flags )
        {
            DPRINTFE( "Failed to get log communication socket (%i) flags, "
                      "error=%s.", socket_i, strerror( errno ) );
            return( SM_FAILED );
        }

        result = fcntl( sockets[socket_i], F_SETFL, flags | O_NONBLOCK );
        if( 0 > result )
        {
            DPRINTFE( "Failed to set log communication socket (%i) to "
                      "non-blocking, error=%s.", socket_i, 
                      strerror( errno ) );
            return( SM_FAILED );
        }

        result = setsockopt( sockets[socket_i], SOL_SOCKET, SO_SNDBUF,
                             &buffer_len, sizeof(buffer_len) );
        if( 0 > result )
        {
            DPRINTFE( "Failed to set log communication socket (%i) "
                      "send buffer length (%i), error=%s.", socket_i,
                      buffer_len, strerror( errno ) );
            return( SM_FAILED );
        }

        result = setsockopt( sockets[socket_i], SOL_SOCKET, SO_RCVBUF,
                             &buffer_len, sizeof(buffer_len) );
        if( 0 > result )
        {
            DPRINTFE( "Failed to set log communication socket (%i) "
                      "receive buffer length (%i), error=%s.", socket_i,
                      buffer_len, strerror( errno ) );
            return( SM_FAILED );
        }
    }

    _server_fd = sockets[0];
    _client_fd = sockets[1];

    error = sm_log_thread_start( _server_fd );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to start log thread, error=%s.",
                  sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Log - Finalize
// ==============
SmErrorT sm_log_finalize( void )
{
    SmErrorT error;
    
    error = sm_log_thread_stop();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to stop log thread, error=%s.",
                  sm_error_str( error ) );
    }

    if( -1 < _server_fd )
    {
        close( _server_fd );
        _server_fd = -1;
    }

    if( -1 < _client_fd )
    {
        close( _client_fd );
        _client_fd = -1;
    }

    return( SM_OKAY );
}
// ****************************************************************************
