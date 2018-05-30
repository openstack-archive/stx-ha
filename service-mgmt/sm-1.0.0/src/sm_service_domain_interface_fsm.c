//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_service_domain_interface_fsm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_service_domain_interface_table.h"
#include "sm_service_domain_interface_unknown_state.h"
#include "sm_service_domain_interface_enabled_state.h"
#include "sm_service_domain_interface_disabled_state.h"
#include "sm_service_domain_interface_not_in_use_state.h"
#include "sm_log.h"

static char _reason_text[SM_LOG_REASON_TEXT_MAX_CHAR] = "";
static SmListT* _callbacks = NULL;

// ****************************************************************************
// Service Domain Interface FSM - Register Callback
// ================================================
SmErrorT sm_service_domain_interface_fsm_register_callback(
    SmServiceDomainInterfaceFsmCallbackT callback )
{
    SM_LIST_PREPEND( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Deregister Callback
// ==================================================
SmErrorT sm_service_domain_interface_fsm_deregister_callback(
    SmServiceDomainInterfaceFsmCallbackT callback )
{
    SM_LIST_REMOVE( _callbacks, (SmListEntryDataPtrT) callback );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Notify
// =====================================
static void sm_service_domain_interface_fsm_notify( 
    SmServiceDomainInterfaceT* interface )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmServiceDomainInterfaceFsmCallbackT callback;

    SM_LIST_FOREACH( _callbacks, entry, entry_data )
    {
        callback = (SmServiceDomainInterfaceFsmCallbackT) entry_data;

        callback( interface->service_domain,
                  interface->service_domain_interface,
                  interface->interface_state );
    }
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Enter State
// ==========================================
static SmErrorT sm_service_domain_interface_fsm_enter_state(
    SmServiceDomainInterfaceT* interface )
{
    SmErrorT error;

    switch( interface->interface_state )
    {
        case SM_INTERFACE_STATE_UNKNOWN:
            error = sm_service_domain_interface_unknown_state_entry( interface );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to enter "
                          "state (%s), error=%s.", interface->service_domain,
                          interface->service_domain_interface, 
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_INTERFACE_STATE_ENABLED:
            error = sm_service_domain_interface_enabled_state_entry( interface );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to enter "
                          "state (%s), error=%s.", interface->service_domain,
                          interface->service_domain_interface, 
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_INTERFACE_STATE_DISABLED:
            error = sm_service_domain_interface_disabled_state_entry( interface );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to enter "
                          "state (%s), error=%s.", interface->service_domain,
                          interface->service_domain_interface, 
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown service domain (%s) interface (%s) "
                      "state (%s).", interface->service_domain,
                      interface->service_domain_interface,
                      sm_interface_state_str( interface->interface_state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Exit State
// =========================================
static SmErrorT sm_service_domain_interface_fsm_exit_state(
    SmServiceDomainInterfaceT* interface )
{
    SmErrorT error;

    switch( interface->interface_state )
    {
        case SM_INTERFACE_STATE_UNKNOWN:
            error = sm_service_domain_interface_unknown_state_exit( interface );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to exit "
                          "state (%s), error=%s.", interface->service_domain,
                          interface->service_domain_interface,
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_INTERFACE_STATE_ENABLED:
            error = sm_service_domain_interface_enabled_state_exit( interface );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to exit "
                          "state (%s), error=%s.", interface->service_domain,
                          interface->service_domain_interface,
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_INTERFACE_STATE_DISABLED:
            error = sm_service_domain_interface_disabled_state_exit( interface );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to exit "
                          "state (%s), error=%s.", interface->service_domain,
                          interface->service_domain_interface,
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown service domain (%s) interface (%s) state (%s).",
                      interface->service_domain, 
                      interface->service_domain_interface,
                      sm_interface_state_str( interface->interface_state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Transition State
// ===============================================
static SmErrorT sm_service_domain_interface_fsm_transition_state(
    SmServiceDomainInterfaceT* interface, SmInterfaceStateT from_state )
{
    SmErrorT error;

    switch( interface->interface_state )
    {
        case SM_INTERFACE_STATE_UNKNOWN:
            error = sm_service_domain_interface_unknown_state_transition(
                                                        interface, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to "
                          "transition from state (%s) to state (%s), error=%s.",
                          interface->service_domain,
                          interface->service_domain_interface,
                          sm_interface_state_str( from_state ),
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_INTERFACE_STATE_ENABLED:
            error = sm_service_domain_interface_enabled_state_transition(
                                                        interface, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to "
                          "transition from state (%s) to state (%s), error=%s.",
                          interface->service_domain,
                          interface->service_domain_interface,
                          sm_interface_state_str( from_state ),
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_INTERFACE_STATE_DISABLED:
            error = sm_service_domain_interface_disabled_state_transition(
                                                        interface, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to "
                          "transition from state (%s) to state (%s), error=%s.",
                          interface->service_domain,
                          interface->service_domain_interface,
                          sm_interface_state_str( from_state ),
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_INTERFACE_STATE_NOT_IN_USE:
            error = sm_service_domain_interface_niu_state_transition(
                                                        interface, from_state );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to "
                          "transition from state (%s) to state (%s), error=%s.",
                          interface->service_domain,
                          interface->service_domain_interface,
                          sm_interface_state_str( from_state ),
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;
        default:
            DPRINTFE( "Unknown service domain (%s) interface (%s) state (%s).",
                      interface->service_domain,
                      interface->service_domain_interface,
                      sm_interface_state_str( interface->interface_state ) );
        break;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Set State
// ========================================
SmErrorT sm_service_domain_interface_fsm_set_state( 
    char service_domain[], char service_domain_interface[],
    SmInterfaceStateT state, const char reason_text[] ) 
{
    SmInterfaceStateT prev_state;
    SmServiceDomainInterfaceT* interface;
    SmErrorT error, error2;

    interface = sm_service_domain_interface_table_read( service_domain, 
                                                    service_domain_interface );
    if( NULL == interface )
    {
        DPRINTFE( "Failed to read service domain (%s) interface (%s), "
                  "error=%s.", service_domain, service_domain_interface,
                  sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    prev_state = interface->interface_state;

    error = sm_service_domain_interface_fsm_exit_state( interface );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to exit state (%s) service domain (%s) "
                  "interface (%s), error=%s.",
                  sm_interface_state_str( interface->interface_state ),
                  service_domain, service_domain_interface,
                  sm_error_str( error ) );
        return( error );
    }

    interface->interface_state = state;

    error = sm_service_domain_interface_fsm_transition_state( interface,
                                                              prev_state );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to transition to state (%s) service domain (%s) "
                  "interface (%s), error=%s.",
                  sm_interface_state_str( interface->interface_state ), 
                  service_domain, service_domain_interface,
                  sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }

    error = sm_service_domain_interface_fsm_enter_state( interface );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to enter state (%s) service domain (%s) "
                  "interface (%s), error=%s.", 
                  sm_interface_state_str( interface->interface_state ),
                  service_domain, service_domain_interface,
                  sm_error_str( error ) );
        goto STATE_CHANGE_ERROR;
    }

    if( '\0' != _reason_text[0] )
    {
        snprintf( _reason_text, sizeof(_reason_text), "%s", reason_text );
    }

    return( SM_OKAY );

STATE_CHANGE_ERROR:
    error2 = sm_service_domain_interface_fsm_exit_state( interface ); 
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to exit state (%s) service domain (%s) "
                  "interface (%s), error=%s.",
                  sm_interface_state_str( interface->interface_state ),
                  service_domain, service_domain_interface,
                  sm_error_str( error2 ) );
        abort();
    }

    interface->interface_state = prev_state;

    error2 = sm_service_domain_interface_fsm_transition_state( interface,
                                                               state );
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to transition to state (%s) service domain (%s) "
                  "interface (%s), error=%s.",
                  sm_interface_state_str( interface->interface_state ),
                  service_domain, service_domain_interface,
                  sm_error_str( error2 ) );
        abort();
    }

    error2 = sm_service_domain_interface_fsm_enter_state( interface ); 
    if( SM_OKAY != error2 )
    {
        DPRINTFE( "Failed to enter state (%s) service domain (%s) "
                  "interface (%s), error=%s.",
                  sm_interface_state_str( interface->interface_state ),
                  service_domain, service_domain_interface,
                  sm_error_str( error2 ) );
        abort();
    }

    return( error );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Event Handler
// ============================================
SmErrorT sm_service_domain_interface_fsm_event_handler( 
    char service_domain[], char service_domain_interface[],
    SmServiceDomainInterfaceEventT event, void* event_data[],
    const char reason_text[] )
{
    SmInterfaceStateT prev_state;
    SmServiceDomainInterfaceT* interface;
    SmErrorT error;

    snprintf( _reason_text, sizeof(_reason_text), "%s", reason_text );

    interface = sm_service_domain_interface_table_read( service_domain,
                                                service_domain_interface );
    if( NULL == interface )
    {
        DPRINTFE( "Failed to read service domain (%s) interface (%s), "
                  "error=%s.", service_domain, service_domain_interface,
                  sm_error_str(SM_NOT_FOUND) );
        return( SM_NOT_FOUND );
    }

    prev_state = interface->interface_state;

    switch( interface->interface_state )
    {
        case SM_INTERFACE_STATE_UNKNOWN:
            error = sm_service_domain_interface_unknown_state_event_handler(
                                                interface, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to handle "
                          "event (%s) in state (%s), error=%s.", service_domain,
                          service_domain_interface, 
                          sm_service_domain_interface_event_str( event ),
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_INTERFACE_STATE_ENABLED:
            error = sm_service_domain_interface_enabled_state_event_handler(
                                                interface, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to handle "
                          "event (%s) in state (%s), error=%s.", service_domain,
                          service_domain_interface,
                          sm_service_domain_interface_event_str( event ),
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_INTERFACE_STATE_DISABLED:
            error = sm_service_domain_interface_disabled_state_event_handler(
                                                interface, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to handle "
                          "event (%s) in state (%s), error=%s.", service_domain,
                          service_domain_interface,
                          sm_service_domain_interface_event_str( event ),
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        case SM_INTERFACE_STATE_NOT_IN_USE:
            error = sm_service_domain_interface_niu_state_event_handler(
                                                interface, event, event_data );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Service domain (%s) interface (%s) unable to handle "
                          "event (%s) in state (%s), error=%s.", service_domain,
                          service_domain_interface,
                          sm_service_domain_interface_event_str( event ),
                          sm_interface_state_str( interface->interface_state ),
                          sm_error_str( error ) );
                return( error );
            }
        break;

        default:
            DPRINTFE( "Unknown state (%s) for service domain (%s) "
                      "interface (%s).",
                      sm_interface_state_str( interface->interface_state ),
                      service_domain, service_domain_interface );
        break;
    }

    if( prev_state != interface->interface_state )
    {
        DPRINTFI( "Service domain (%s) interface (%s) received event (%s) was "
                  "in the %s state and is now in the %s.", service_domain,
                  service_domain_interface,
                  sm_service_domain_interface_event_str( event ),
                  sm_interface_state_str( prev_state ),
                  sm_interface_state_str( interface->interface_state ) );

        error = sm_service_domain_interface_table_persist( interface );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to persist service domain (%s) interface (%s) "
                      "data, error=%s.", service_domain,
                      service_domain_interface, sm_error_str( error ) );
            return( error );
        }

        sm_log_interface_state_change( service_domain_interface,
                        sm_interface_state_str( prev_state ),
                        sm_interface_state_str( interface->interface_state ),
                        _reason_text );

        sm_service_domain_interface_fsm_notify( interface );

    } else if( SM_SERVICE_DOMAIN_INTERFACE_EVENT_AUDIT == event ) {
        sm_service_domain_interface_fsm_notify( interface );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// service domain interface - dump ipv4 addr
// ==================================================================
static void _sm_service_domain_interface_dump_ipv4_addr(const struct in_addr& sin, FILE* fp)
{
    char str[INET_ADDRSTRLEN];

    if(NULL == fp) return;
    if( 0 == sin.s_addr )
    {
        str[0] = '\0';
    }else
    {
        inet_ntop(AF_INET, &sin, str, sizeof(str));
    }
    fprintf(fp, "%s\n", str);
}
// ****************************************************************************

// ****************************************************************************
// service domain interface - dump interface ipv6
// ==================================================================
static void _sm_service_domain_interface_dump_network_ipv4(
        SmServiceDomainInterfaceT* interface,
        FILE* fp )
{
    if(NULL == fp) return;

    fprintf( fp, "      IP Address: " );
    _sm_service_domain_interface_dump_ipv4_addr(interface->network_address.u.ipv4.sin, fp);
    fprintf( fp, "       Multicast: " );
    _sm_service_domain_interface_dump_ipv4_addr(interface->network_multicast.u.ipv4.sin, fp);
    fprintf( fp, "         Peer IP: " );
    _sm_service_domain_interface_dump_ipv4_addr(interface->network_peer_address.u.ipv4.sin, fp);
}
// ****************************************************************************

// ****************************************************************************
// service domain interface - dump interface ipv6
// ==================================================================
static void _sm_service_domain_interface_dump_network_ipv6(
        SmServiceDomainInterfaceT* interface,
        FILE* fp )
{
    char str[INET_ADDRSTRLEN];
    if(NULL == fp) return;

    inet_ntop(AF_INET, &(interface->network_address.u.ipv6.sin6), str, sizeof(str));
    fprintf( fp, "      IP Address: %s\n", str );
    inet_ntop(AF_INET, &(interface->network_multicast.u.ipv6.sin6), str, sizeof(str));
    fprintf( fp, "       Multicast: %s\n", str );
    inet_ntop(AF_INET, &(interface->network_peer_address.u.ipv6.sin6), str, sizeof(str));
    fprintf( fp, "         Peer IP: %s\n", str );
}
// ****************************************************************************

// ****************************************************************************
// service domain interface - dump interface state
// ==================================================================
static void _sm_service_domain_interface_dump_if_state(void* user_data[],
                                                SmServiceDomainInterfaceT* interface)
{
    FILE* fp = (FILE*)user_data[0];
    if(NULL == fp) return;

    fprintf( fp, "  Interface %s(%s): %s\n",
        interface->service_domain_interface,
        interface->interface_name,
        sm_interface_state_str(interface->interface_state) );
    fprintf( fp, "      Network Type: %s\n", sm_network_type_str(interface->network_type));
    if( interface->network_type == SM_NETWORK_TYPE_IPV4 || interface->network_type == SM_NETWORK_TYPE_IPV4_UDP )
    {
        _sm_service_domain_interface_dump_network_ipv4( interface, fp );
    }else if ( interface->network_type == SM_NETWORK_TYPE_IPV6
                    || interface->network_type == SM_NETWORK_TYPE_IPV6_UDP )
    {
        _sm_service_domain_interface_dump_network_ipv6( interface, fp );
    }
}
// ****************************************************************************

// ****************************************************************************
// service domain interface - dump interface state
// ==================================================================
void sm_service_domain_interface_dump_state(FILE* fp)
{
    fprintf( fp, "\nService Domain Interfaces\n");
    void* user_data[1] = {fp};
    sm_service_domain_interface_table_foreach(
        user_data,
        _sm_service_domain_interface_dump_if_state
    );


    fprintf( fp, "\n" );
}
// ****************************************************************************


// ****************************************************************************
// Service Domain Interface FSM - Initialize
// =========================================
SmErrorT sm_service_domain_interface_fsm_initialize( void )
{
    SmErrorT error;

    error = sm_service_domain_interface_unknown_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain interface unknown "
                  "state module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_interface_enabled_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain interface enabled "
                  "state module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_interface_disabled_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain interface disabled "
                  "state module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    error = sm_service_domain_interface_niu_state_initialize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to initialize service domain interface not in use "
                  "state module, error=%s.", sm_error_str( error ) );
        return( error );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Service Domain Interface FSM - Finalize
// =======================================
SmErrorT sm_service_domain_interface_fsm_finalize( void )
{
    SmErrorT error;

    error = sm_service_domain_interface_unknown_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain interface unknown "
                  "state module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_interface_enabled_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain interface enabled "
                  "state module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_interface_disabled_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain interface disabled "
                  "state module, error=%s.", sm_error_str( error ) );
    }

    error = sm_service_domain_interface_niu_state_finalize();
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to finalize service domain interface not in use "
                  "state module, error=%s.", sm_error_str( error ) );
    }

    return( SM_OKAY );
}
// ****************************************************************************
