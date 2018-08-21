//
// Copyright (c) 2014-2018 Wind River Systems, Inc.
//
// SPDX-License-Identifier: Apache-2.0
//
#include "sm_msg.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "sm_types.h"
#include "sm_debug.h"
#include "sm_list.h"
#include "sm_selobj.h"
#include "sm_uuid.h"
#include "sm_sha512.h"
#include "sm_hw.h"
#include "sm_node_utils.h"
#include "sm_db.h"
#include "sm_db_service_domain_interfaces.h"

#define SM_MSG_VERSION                                       1
#define SM_MSG_REVISION                                      1
#define SM_MSG_FLAG_ACK                                    0x1
#define SM_MSG_MAX_SIZE                                   1024
#define SM_MSG_BUFFER_MAX_SIZE                            4096
#define SM_MSG_MAX_SEQ_DELTA                              1000

#if __BYTE_ORDER == __BIG_ENDIAN
#define ntohll(x) (x)
#define htonll(x) (x)
#else
# if __BYTE_ORDER == __LITTLE_ENDIAN
# define ntohll(x) sm_msg_little_endian_ntohll(x)
# define htonll(x) sm_msg_little_endian_htonll(x)
# endif
#endif

#define _MSG_NOT_SENT_TO_TARGET -128
typedef enum
{
    SM_MSG_TYPE_UNKNOWN                             = 0x0,
    SM_MSG_TYPE_NODE_HELLO                          = 0x1,
    SM_MSG_TYPE_NODE_UPDATE                         = 0x2,
    SM_MSG_TYPE_NODE_SWACT                          = 0x3,
    SM_MSG_TYPE_NODE_SWACT_ACK                      = 0x4,
    SM_MSG_TYPE_SERVICE_DOMAIN_HELLO                = 0x5,
    SM_MSG_TYPE_SERVICE_DOMAIN_PAUSE                = 0x6,
    SM_MSG_TYPE_SERVICE_DOMAIN_EXCHANGE_START       = 0x7,
    SM_MSG_TYPE_SERVICE_DOMAIN_EXCHANGE             = 0x8,
    SM_MSG_TYPE_SERVICE_DOMAIN_MEMBER_REQUEST       = 0x9,
    SM_MSG_TYPE_SERVICE_DOMAIN_MEMBER_UPDATE        = 0xa,
    SM_MSG_MAX,
} SmMsgTypeT;

typedef struct
{
    uint16_t version;
    uint16_t revision;
    uint16_t max_supported_version;
    uint16_t max_supported_revision;
    uint16_t msg_len;
    uint16_t msg_type;
    uint64_t msg_flags;
    SmUuidT msg_instance;
    uint64_t msg_seq_num;
    char node_name[SM_NODE_NAME_MAX_CHAR];
    uint32_t auth_type;
    uint8_t auth_vector[SM_AUTHENTICATION_VECTOR_MAX_CHAR];
} __attribute__ ((packed)) SmMsgHeaderT;

typedef struct
{
    char node_name[SM_NODE_NAME_MAX_CHAR];
    char admin_state[SM_NODE_ADMIN_STATE_MAX_CHAR];
    char oper_state[SM_NODE_OPERATIONAL_STATE_MAX_CHAR];
    char avail_status[SM_NODE_AVAIL_STATUS_MAX_CHAR];
    char ready_state[SM_NODE_READY_STATE_MAX_CHAR];
    SmUuidT state_uuid;
    uint32_t uptime;
} __attribute__ ((packed)) SmMsgNodeHelloT;

typedef struct
{
    char node_name[SM_NODE_NAME_MAX_CHAR];
    char admin_state[SM_NODE_ADMIN_STATE_MAX_CHAR];
    char oper_state[SM_NODE_OPERATIONAL_STATE_MAX_CHAR];
    char avail_status[SM_NODE_AVAIL_STATUS_MAX_CHAR];
    char ready_state[SM_NODE_READY_STATE_MAX_CHAR];
    SmUuidT old_state_uuid;
    SmUuidT state_uuid;
    uint32_t uptime;
    uint32_t force;
} __attribute__ ((packed)) SmMsgNodeUpdateT;

typedef struct
{
    SmUuidT request_uuid;
    char node_name[SM_NODE_NAME_MAX_CHAR];
    uint32_t force;
} __attribute__ ((packed)) SmMsgNodeSwactT;

typedef struct
{
    SmUuidT request_uuid;
    char node_name[SM_NODE_NAME_MAX_CHAR];
    uint32_t force;
} __attribute__ ((packed)) SmMsgNodeSwactAckT;

typedef struct
{
    char service_domain[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char node_name[SM_NODE_NAME_MAX_CHAR];    
    char orchestration[SM_ORCHESTRATION_MAX_CHAR];
    char designation[SM_DESIGNATION_MAX_CHAR];
    uint32_t generation;
    uint32_t priority;
    uint32_t hello_interval;
    uint32_t dead_interval;
    uint32_t wait_interval;
    uint32_t exchange_interval;
    char leader[SM_NODE_NAME_MAX_CHAR];
} __attribute__ ((packed)) SmMsgServiceDomainHelloT;

typedef struct
{
    char service_domain[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char node_name[SM_NODE_NAME_MAX_CHAR];    
    uint32_t pause_interval;
} __attribute__ ((packed)) SmMsgServiceDomainPauseT;

typedef struct
{
    char service_domain[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char node_name[SM_NODE_NAME_MAX_CHAR];    
    char exchange_node_name[SM_NODE_NAME_MAX_CHAR];    
    uint32_t exchange_seq;
} __attribute__ ((packed)) SmMsgServiceDomainExchangeStartT;

typedef struct
{
    char service_domain[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char node_name[SM_NODE_NAME_MAX_CHAR];    
    char exchange_node_name[SM_NODE_NAME_MAX_CHAR];    
    uint32_t exchange_seq;
    int64_t member_id;
    char member_name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    char member_desired_state[SM_SERVICE_GROUP_STATE_MAX_CHAR];
    char member_state[SM_SERVICE_GROUP_STATE_MAX_CHAR];
    char member_status[SM_SERVICE_GROUP_STATUS_MAX_CHAR];
    char member_condition[SM_SERVICE_GROUP_CONDITION_MAX_CHAR];
    int64_t member_health;
    char reason_text[SM_SERVICE_GROUP_REASON_TEXT_MAX_CHAR];
    uint32_t more_members;
    int64_t last_received_member_id;
} __attribute__ ((packed)) SmMsgServiceDomainExchangeT;

typedef struct
{
    char service_domain[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char node_name[SM_NODE_NAME_MAX_CHAR];    
    char member_node_name[SM_NODE_NAME_MAX_CHAR];    
    int64_t member_id;
    char member_name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    char member_action[SM_SERVICE_GROUP_ACTION_MAX_CHAR];
    uint64_t member_action_flags;
} __attribute__ ((packed)) SmMsgServiceDomainMemberRequestT;

typedef struct
{
    char service_domain[SM_SERVICE_DOMAIN_NAME_MAX_CHAR];
    char node_name[SM_NODE_NAME_MAX_CHAR];    
    char member_node_name[SM_NODE_NAME_MAX_CHAR];    
    int64_t member_id;
    char member_name[SM_SERVICE_GROUP_NAME_MAX_CHAR];
    char member_desired_state[SM_SERVICE_GROUP_STATE_MAX_CHAR];
    char member_state[SM_SERVICE_GROUP_STATE_MAX_CHAR];
    char member_status[SM_SERVICE_GROUP_STATUS_MAX_CHAR];
    char member_condition[SM_SERVICE_GROUP_CONDITION_MAX_CHAR];
    int64_t member_health;
    char reason_text[SM_SERVICE_GROUP_REASON_TEXT_MAX_CHAR];
} __attribute__ ((packed)) SmMsgServiceDomainMemberUpdateT;

typedef struct
{
    SmMsgHeaderT header;

    union
    {   // Node Messages.
        SmMsgNodeHelloT node_hello;
        SmMsgNodeUpdateT node_update;
        SmMsgNodeSwactT node_swact;
        SmMsgNodeSwactAckT node_swact_ack;

        // Service Domain Messages.
        SmMsgServiceDomainHelloT hello;
        SmMsgServiceDomainPauseT pause;
        SmMsgServiceDomainExchangeStartT exchange_start;
        SmMsgServiceDomainExchangeT exchange;
        SmMsgServiceDomainMemberRequestT request;
        SmMsgServiceDomainMemberUpdateT update;
        char raw_msg[SM_MSG_MAX_SIZE-sizeof(SmMsgHeaderT)];
    } u;
} __attribute__ ((packed)) SmMsgT;

typedef struct
{
    bool inuse;
    char node_name[SM_NODE_NAME_MAX_CHAR];
    SmUuidT msg_instance;
    uint64_t msg_last_seq_num;
} SmMsgPeerNodeInfoT; 

typedef struct
{
    bool inuse;
    char interface_name[SM_INTERFACE_NAME_MAX_CHAR];
    SmNetworkAddressT network_address;
    int network_port;
    SmAuthTypeT auth_type;
    char auth_key[SM_AUTHENTICATION_KEY_MAX_CHAR];    
} SmMsgPeerInterfaceInfoT;

static bool _messaging_enabled = false;
static char _hostname[SM_NODE_NAME_MAX_CHAR];
static SmUuidT _msg_instance = {0};
static uint64_t _msg_seq_num = 0;
static char _tx_control_buffer[SM_MSG_BUFFER_MAX_SIZE] __attribute__((aligned));
static char _tx_buffer[SM_MSG_BUFFER_MAX_SIZE] __attribute__((aligned));
static char _rx_control_buffer[SM_MSG_BUFFER_MAX_SIZE] __attribute__((aligned));
static char _rx_buffer[SM_MSG_BUFFER_MAX_SIZE] __attribute__((aligned));
static SmListT* _callbacks = NULL;
static unsigned int _next_free_peer_entry = 0;
static SmMsgPeerNodeInfoT _peers[SM_NODE_MAX];
static SmMsgPeerInterfaceInfoT _peer_interfaces[SM_INTERFACE_PEER_MAX];

// Transmit and Receive Statistics
static uint64_t _rcvd_total_msgs = 0;
static uint64_t _rcvd_msgs_while_disabled = 0;
static uint64_t _rcvd_bad_msg_auth = 0;
static uint64_t _rcvd_bad_msg_version = 0;
static uint64_t _send_node_hello_cnt = 0;
static uint64_t _rcvd_node_hello_cnt = 0;
static uint64_t _send_node_update_cnt = 0;
static uint64_t _rcvd_node_update_cnt = 0;
static uint64_t _send_node_swact_cnt = 0;
static uint64_t _rcvd_node_swact_cnt = 0;
static uint64_t _send_node_swact_ack_cnt = 0;
static uint64_t _rcvd_node_swact_ack_cnt = 0;
static uint64_t _send_service_domain_hello_cnt = 0;
static uint64_t _rcvd_service_domain_hello_cnt = 0;
static uint64_t _send_service_domain_pause_cnt = 0;
static uint64_t _rcvd_service_domain_pause_cnt = 0;
static uint64_t _send_service_domain_exchange_start_cnt = 0;
static uint64_t _rcvd_service_domain_exchange_start_cnt = 0;
static uint64_t _send_service_domain_exchange_cnt = 0;
static uint64_t _rcvd_service_domain_exchange_cnt = 0;
static uint64_t _send_service_domain_member_request_cnt = 0;
static uint64_t _rcvd_service_domain_member_request_cnt = 0;
static uint64_t _send_service_domain_member_update_cnt = 0;
static uint64_t _rcvd_service_domain_member_update_cnt = 0;

// ****************************************************************************
// Messaging - Find Peer Node Info
// ===============================
static SmMsgPeerNodeInfoT* sm_msg_find_peer_node_info( char node_name[] )
{
    SmMsgPeerNodeInfoT* entry;

    unsigned int entry_i;
    for( entry_i=0; SM_NODE_MAX > entry_i; ++entry_i )
    {
        entry = &(_peers[entry_i]);

        if( entry->inuse )
        {
            if( 0 == strcmp( node_name, entry->node_name ) )
            {
                return( entry );
            }
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - In Sequence
// =======================
static bool sm_msg_in_sequence( char node_name[],
    SmUuidT msg_instance, uint64_t msg_seq_num )
{
    bool in_sequence = false;
    SmMsgPeerNodeInfoT* entry;
    
    entry = sm_msg_find_peer_node_info( node_name );
    if( NULL == entry )
    {
        // Cheap way of aging out entries when the maximum node limit is
        // reached.
        entry = &(_peers[_next_free_peer_entry++]);
        if( _next_free_peer_entry >= SM_NODE_MAX )
        {
            _next_free_peer_entry = 0;
        }

        entry->inuse = true;
        snprintf( entry->node_name, sizeof(entry->node_name), "%s",
                  node_name );
        snprintf( entry->msg_instance, sizeof(entry->msg_instance),
                  "%s", msg_instance );
        entry->msg_last_seq_num = msg_seq_num;
        in_sequence = true;

        DPRINTFI( "Message instance (%s) for node (%s) set.", msg_instance,
                  node_name );
    } else {
        if( 0 == strcmp( msg_instance, entry->msg_instance ) )
        {
            uint64_t delta;

            if( msg_seq_num > entry->msg_last_seq_num )
            {
                delta = msg_seq_num- entry->msg_last_seq_num;
                if( SM_MSG_MAX_SEQ_DELTA < delta )
                {
                    DPRINTFI( "Message sequence delta (%" PRIu64 ") too large "
                              "for message instance (%s) from node (%s), "
                              "rcvd_seq=%" PRIu64 ", last_recvd_seq=%" PRIu64 ".",
                              delta, entry->msg_instance, node_name,
                              msg_seq_num, entry->msg_last_seq_num );
                }

                entry->msg_last_seq_num = msg_seq_num;
                in_sequence = true;

            } else {
                delta = entry->msg_last_seq_num - msg_seq_num;
                
                if( SM_MSG_MAX_SEQ_DELTA < delta )
                {
                    DPRINTFI( "Message sequence delta (%" PRIu64 ") too large "
                              "for message instance (%s) from node (%s), "
                              "rcvd_seq=%" PRIu64 ", last_recvd_seq=%" PRIu64 ".",
                              delta, entry->msg_instance, node_name,
                              msg_seq_num, entry->msg_last_seq_num );

                    entry->msg_last_seq_num = msg_seq_num;
                    in_sequence = true;
                }
            }
        } else {
            DPRINTFI( "Message instance (%s) changed for node (%s), now=%s.",
                      entry->msg_instance, node_name, msg_instance );

            snprintf( entry->msg_instance, sizeof(entry->msg_instance),
                      "%s", msg_instance );
            entry->msg_last_seq_num = msg_seq_num;
            in_sequence = true;
        }
    }

    return( in_sequence );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Add Peer Interface 
// ==============================
SmErrorT sm_msg_add_peer_interface( char interface_name[],
    SmNetworkAddressT* network_address, int network_port,
    SmAuthTypeT auth_type, char auth_key[] )
{
    bool found = false;
    SmMsgPeerInterfaceInfoT* entry;

    unsigned int entry_i;
    for( entry_i=0; SM_INTERFACE_PEER_MAX > entry_i; ++entry_i )
    {
        entry = &(_peer_interfaces[entry_i]);

        if( !(entry->inuse) )
            continue;

        if( 0 != strcmp( interface_name, entry->interface_name ) )
            continue;

        if( network_address->type != entry->network_address.type )
            continue;

        if( SM_NETWORK_TYPE_IPV4 == network_address->type )
        {
            if( network_address->u.ipv4.sin.s_addr
                        == entry->network_address.u.ipv4.sin.s_addr )
            {
                found = true;
                break;
            }
        } else if( SM_NETWORK_TYPE_IPV4_UDP == network_address->type ) {
            if(( network_address->u.ipv4.sin.s_addr
                        == entry->network_address.u.ipv4.sin.s_addr )&&
               ( network_port == entry->network_port ))
            {
                found = true;
                break;
            }
        } else if( SM_NETWORK_TYPE_IPV6 == network_address->type ) {
            if( 0 != memcmp( &( network_address->u.ipv6.sin6 ),
                             &( entry->network_address.u.ipv6.sin6 ),
                             sizeof(struct in6_addr) ))
            {
                found = true;
                break;
            }
        } else if( SM_NETWORK_TYPE_IPV6_UDP == network_address->type ) {
            if(( 0 != memcmp( &( network_address->u.ipv6.sin6 ),
                             &( entry->network_address.u.ipv6.sin6 ),
                        sizeof(struct in6_addr) ))&&
               ( network_port == entry->network_port ))
            {
                found = true;
                break;
            }
        }
    }

    if( !found )
    {
        for( entry_i=0; SM_INTERFACE_PEER_MAX > entry_i; ++entry_i )
        {
            entry = &(_peer_interfaces[entry_i]);

            if( !(entry->inuse) )
            {
                entry->inuse = true;
                snprintf( entry->interface_name, sizeof(entry->interface_name),
                          "%s", interface_name );
                memcpy( &(entry->network_address), network_address,
                        sizeof(SmNetworkAddressT) );
                entry->network_port = network_port;
                entry->auth_type = auth_type;
                memcpy( entry->auth_key, auth_key,
                        SM_AUTHENTICATION_KEY_MAX_CHAR );
                break;
            }
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Delete Peer Interface 
// =================================
SmErrorT sm_msg_delete_peer_interface( char interface_name[],
    SmNetworkAddressT* network_address, int network_port )
{
    SmMsgPeerInterfaceInfoT* entry;

    unsigned int entry_i;
    for( entry_i=0; SM_INTERFACE_PEER_MAX > entry_i; ++entry_i )
    {
        entry = &(_peer_interfaces[entry_i]);

        if( !(entry->inuse) )
            continue;

        if( 0 != strcmp( interface_name, entry->interface_name ) )
            continue;

        if( network_address->type != entry->network_address.type )
            continue;

        if( SM_NETWORK_TYPE_IPV4 == network_address->type )
        {
            if( network_address->u.ipv4.sin.s_addr
                        == entry->network_address.u.ipv4.sin.s_addr )
            {
                memset( entry, 0, sizeof(SmMsgPeerInterfaceInfoT) );
                break;
            }
        } else if( SM_NETWORK_TYPE_IPV4_UDP == network_address->type ) {
            if(( network_address->u.ipv4.sin.s_addr
                        == entry->network_address.u.ipv4.sin.s_addr )&&
               ( network_port == entry->network_port ))
            {
                memset( entry, 0, sizeof(SmMsgPeerInterfaceInfoT) );
                break;
            }
        } else if( SM_NETWORK_TYPE_IPV6 == network_address->type ) {
            if( 0 != memcmp( &(network_address->u.ipv6.sin6),
                             &(entry->network_address.u.ipv6.sin6),
                             sizeof(in6_addr) ))
            {
                memset( entry, 0, sizeof(SmMsgPeerInterfaceInfoT) );
                break;
            }
        } else if( SM_NETWORK_TYPE_IPV6_UDP == network_address->type ) {
            if(( 0 != memcmp( &(network_address->u.ipv6.sin6),
                              &(entry->network_address.u.ipv6.sin6),
                              sizeof(in6_addr) ))&&
               ( network_port == entry->network_port ))
            {
                memset( entry, 0, sizeof(SmMsgPeerInterfaceInfoT) );
                break;
            }
        }
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Get Peer Interface
// ==============================
static SmMsgPeerInterfaceInfoT* sm_msg_get_peer_interface(
    char interface_name[], SmNetworkAddressT* network_address,
    int network_port )
{
    SmMsgPeerInterfaceInfoT* entry;
    in6_addr empty_ipv6_address;

    unsigned int entry_i;
    for( entry_i=0; SM_INTERFACE_PEER_MAX > entry_i; ++entry_i )
    {
        entry = &(_peer_interfaces[entry_i]);

        if( !(entry->inuse) )
            continue;

        if( 0 != strcmp( interface_name, entry->interface_name ) )
            continue;

        if( SM_NETWORK_TYPE_IPV4 == entry->network_address.type )
        {
            if( 0 == entry->network_address.u.ipv4.sin.s_addr )
            {
                return( entry );

            } else if( network_address->u.ipv4.sin.s_addr
                            == entry->network_address.u.ipv4.sin.s_addr )
            {
                return( entry );
            }
        } else if( SM_NETWORK_TYPE_IPV4_UDP == entry->network_address.type ) {
            if( 0 == entry->network_address.u.ipv4.sin.s_addr )
            {
                return( entry );

            } else if( -1 == entry->network_port )
            {
                return( entry );
                
            } else if(( network_address->u.ipv4.sin.s_addr
                            == entry->network_address.u.ipv4.sin.s_addr )&&
                      ( network_port == entry->network_port ))
            {
                return( entry );
            }
        } else if( SM_NETWORK_TYPE_IPV6 == entry->network_address.type ) {
            memset( &empty_ipv6_address, 0, sizeof(in6_addr) );
            if( 0 == memcmp( &(entry->network_address.u.ipv6.sin6),
                             &(empty_ipv6_address),
                             sizeof(in6_addr) ))
            {
                return( entry );

            } else if( 0 == memcmp( &(network_address->u.ipv6.sin6),
                                    &(entry->network_address.u.ipv6.sin6),
                                    sizeof(in6_addr) ))
            {
                return( entry );
            }
        } else if( SM_NETWORK_TYPE_IPV6_UDP == entry->network_address.type ) {
            memset( &empty_ipv6_address, 0, sizeof(in6_addr) );
            if( 0 == memcmp( &(entry->network_address.u.ipv6.sin6),
                             &(empty_ipv6_address),
                             sizeof(in6_addr) ))
            {
                return( entry );

            } else if( -1 == entry->network_port )
            {
                return( entry );

            } else if(( 0 == memcmp( &(network_address->u.ipv6.sin6),
                                    &(entry->network_address.u.ipv6.sin6),
                                    sizeof(in6_addr) ))&&
                      ( network_port == entry->network_port ))
            {
                return( entry );
            }
        }
    }

    return( NULL );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Little Endian Network To Host Byte Order For Uint64
// ===============================================================
uint64_t sm_msg_little_endian_ntohll( uint64_t i ) 
{
    uint32_t tmp;

    union 
    {
        uint64_t uint64;
        struct 
        {
            uint32_t word0;
            uint32_t word1;
        } uint32;
    } u;

    u.uint64 = i;

    u.uint32.word0 = ntohl( u.uint32.word0 );
    u.uint32.word1 = ntohl( u.uint32.word1 );

    tmp = u.uint32.word0;
    u.uint32.word0 = u.uint32.word1;
    u.uint32.word1 = tmp;

    return( u.uint64 );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Little Endian Host To Network Byte Order For Uint64
// ===============================================================
uint64_t sm_msg_little_endian_htonll( uint64_t i ) 
{
    uint32_t tmp;

    union 
    {
        uint64_t uint64;
        struct 
        {
            uint32_t word0;
            uint32_t word1;
        } uint32;
    } u;

    u.uint64 = i;

    u.uint32.word0 = htonl( u.uint32.word0 );
    u.uint32.word1 = htonl( u.uint32.word1 );

    tmp = u.uint32.word0;
    u.uint32.word0 = u.uint32.word1;
    u.uint32.word1 = tmp;

    return( u.uint64 );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Enable
// ==================
void sm_msg_enable( void )
{
    _messaging_enabled = true;

    DPRINTFI( "Messaging is now enabled." );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Disable
// ===================
void sm_msg_disable( void )
{
    _messaging_enabled = false;

    DPRINTFI( "Messaging is now disabled." );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Register Callbacks
// ==============================
SmErrorT sm_msg_register_callbacks( SmMsgCallbacksT* callbacks )
{
    SM_LIST_PREPEND( _callbacks, (SmListEntryDataPtrT) callbacks );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Deregister Callbacks
// ================================
SmErrorT sm_msg_deregister_callbacks( SmMsgCallbacksT* callbacks )
{
    SM_LIST_REMOVE( _callbacks, (SmListEntryDataPtrT) callbacks );
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Increment Sequence Number
// =====================================
void sm_msg_increment_seq_num( void )
{
    ++_msg_seq_num;
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Send message from IPv6 source
// ==========================================
static int sm_msg_sendmsg_src_ipv6( int socket, void* msg, size_t msg_len,
    int flags, struct sockaddr_in6* dst_addr, struct in6_addr* src_Addr )
{
    struct msghdr msg_hdr = {};
    struct cmsghdr *cmsg;
    struct in6_pktinfo *pktinfo;
    struct iovec iov = {msg, msg_len};

    memset( &_tx_control_buffer, 0, sizeof(_tx_control_buffer) );
    msg_hdr.msg_iov = &iov;
    msg_hdr.msg_iovlen = 1;
    msg_hdr.msg_name = dst_addr;
    msg_hdr.msg_namelen = sizeof(struct sockaddr_in6);
    msg_hdr.msg_control = _tx_control_buffer;
    msg_hdr.msg_controllen = CMSG_LEN( sizeof(struct in6_pktinfo) );
    cmsg = CMSG_FIRSTHDR( &msg_hdr );
    cmsg->cmsg_level = IPPROTO_IPV6;
    cmsg->cmsg_type = IPV6_PKTINFO;
    cmsg->cmsg_len = CMSG_LEN( sizeof(struct in6_pktinfo) );
    pktinfo = (struct in6_pktinfo*) CMSG_DATA( cmsg );
    pktinfo->ipi6_ifindex = 0;
    pktinfo->ipi6_addr = *src_Addr;

    return sendmsg( socket, &msg_hdr, flags );
}
// ****************************************************************************

static int sm_send_msg(SmServiceDomainInterfaceT* interface, SmMsgT* msg )
{
    struct sockaddr_in dst_addr4;
    int result = -1;
    SmIpv4AddressT* ipv4_dst;

    memset( &dst_addr4, 0, sizeof(dst_addr4) );
    dst_addr4.sin_family = AF_INET;
    dst_addr4.sin_port = htons(interface->network_port);

    if ( SM_INTERFACE_OAM != interface->interface_type )
    {
        ipv4_dst = &(interface->network_multicast.u.ipv4);
    }
    else
    {
        ipv4_dst = &(interface->network_peer_address.u.ipv4);
    }

    if( 0 == ipv4_dst->sin.s_addr )
    {
        //don't send to 0.0.0.0
        return _MSG_NOT_SENT_TO_TARGET;
    }
    dst_addr4.sin_addr.s_addr = ipv4_dst->sin.s_addr;

    result = sendto( interface->unicast_socket, msg, sizeof(SmMsgT),
                     0, (struct sockaddr *) &dst_addr4, sizeof(dst_addr4) );

    return result;
}

static int sm_send_ipv6_msg(SmServiceDomainInterfaceT* interface, SmMsgT* msg )
{
    struct sockaddr_in6 dst_addr6;
    int result = -1;
    SmIpv6AddressT* ipv6_dst;

    memset( &dst_addr6, 0, sizeof(dst_addr6) );
    dst_addr6.sin6_family = AF_INET6;
    dst_addr6.sin6_port = htons(interface->network_port);

    if ( SM_INTERFACE_OAM != interface->interface_type )
    {
        ipv6_dst = &(interface->network_multicast.u.ipv6);
    }else
    {
        ipv6_dst = &(interface->network_peer_address.u.ipv6);
    }
    dst_addr6.sin6_addr = ipv6_dst->sin6;
    result = sm_msg_sendmsg_src_ipv6( interface->unicast_socket, msg, sizeof(SmMsgT),
                                        0, &dst_addr6, &interface->network_address.u.ipv6.sin6 );

    return result;
}
// ****************************************************************************
// Messaging - Send Node Hello
// ===========================
SmErrorT sm_msg_send_node_hello( char node_name[],
    SmNodeAdminStateT admin_state, SmNodeOperationalStateT oper_state,
    SmNodeAvailStatusT avail_status, SmNodeReadyStateT ready_state,
    SmUuidT state_uuid, long uptime, SmServiceDomainInterfaceT* interface )
{
    SmMsgT* msg = (SmMsgT*) _tx_buffer;
    SmMsgNodeHelloT* hello_msg = &(msg->u.node_hello);
    int result = -1;

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    msg->header.version = htons(SM_VERSION);
    msg->header.revision = htons(SM_REVISION);
    msg->header.max_supported_version = htons(SM_VERSION);
    msg->header.max_supported_revision = htons(SM_REVISION);
    msg->header.msg_len = htons(sizeof(SmMsgT));
    msg->header.msg_type = htons(SM_MSG_TYPE_NODE_HELLO);
    msg->header.msg_flags = htonll(0);
    snprintf( msg->header.msg_instance, sizeof(msg->header.msg_instance),
              "%s", _msg_instance );
    msg->header.msg_seq_num = htonll(_msg_seq_num);
    snprintf( msg->header.node_name, sizeof(msg->header.node_name),
              "%s", _hostname );

    snprintf( hello_msg->node_name, sizeof(hello_msg->node_name),
              "%s", node_name );
    snprintf( hello_msg->admin_state, sizeof(hello_msg->admin_state),
              "%s", sm_node_admin_state_str(admin_state) );
    snprintf( hello_msg->oper_state, sizeof(hello_msg->oper_state),
              "%s", sm_node_oper_state_str(oper_state) );
    snprintf( hello_msg->avail_status, sizeof(hello_msg->avail_status),
              "%s", sm_node_avail_status_str(avail_status) );
    snprintf( hello_msg->ready_state, sizeof(hello_msg->ready_state),
              "%s", sm_node_ready_state_str(ready_state) );
    snprintf( hello_msg->state_uuid, sizeof(hello_msg->state_uuid),
              "%s", state_uuid );
    hello_msg->uptime = htonl(uptime);

    if( SM_AUTH_TYPE_HMAC_SHA512 == interface->auth_type )
    {
        SmSha512HashT hash;

        msg->header.auth_type = htonl(interface->auth_type);
        sm_sha512_hmac( msg, sizeof(SmMsgT), interface->auth_key,
                        strlen(interface->auth_key), &hash );
        memcpy( &(msg->header.auth_vector[0]), &(hash.bytes[0]), 
                SM_SHA512_HASH_SIZE );
    } else {
        msg->header.auth_type = htonl(SM_AUTH_TYPE_NONE);
    }

    if(( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )||
       ( SM_NETWORK_TYPE_IPV4 == interface->network_type ))
    {
        result = sm_send_msg(interface, msg);
    } else if(( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type )||
              ( SM_NETWORK_TYPE_IPV6 == interface->network_type )) 
    {
        result = sm_send_ipv6_msg(interface, msg);
    }

    if( 0 > result )
    {
        if ( _MSG_NOT_SENT_TO_TARGET != result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface->interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );
    }

    ++_send_node_hello_cnt;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Node Update
// ============================
SmErrorT sm_msg_send_node_update( char node_name[],
    SmNodeAdminStateT admin_state, SmNodeOperationalStateT oper_state,
    SmNodeAvailStatusT avail_status, SmNodeReadyStateT ready_state,
    SmUuidT old_state_uuid, SmUuidT state_uuid, long uptime, bool force,
    SmServiceDomainInterfaceT* interface )
{
    SmMsgT* msg = (SmMsgT*) _tx_buffer;
    SmMsgNodeUpdateT* update_msg = &(msg->u.node_update);
    int result = -1;

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    msg->header.version = htons(SM_VERSION);
    msg->header.revision = htons(SM_REVISION);
    msg->header.max_supported_version = htons(SM_VERSION);
    msg->header.max_supported_revision = htons(SM_REVISION);
    msg->header.msg_len = htons(sizeof(SmMsgT));
    msg->header.msg_type = htons(SM_MSG_TYPE_NODE_UPDATE);
    msg->header.msg_flags = htonll(0);
    snprintf( msg->header.msg_instance, sizeof(msg->header.msg_instance),
              "%s", _msg_instance );
    msg->header.msg_seq_num = htonll(_msg_seq_num);
    snprintf( msg->header.node_name, sizeof(msg->header.node_name),
              "%s", _hostname );    

    snprintf( update_msg->node_name, sizeof(update_msg->node_name),
              "%s", node_name );
    snprintf( update_msg->admin_state, sizeof(update_msg->admin_state),
              "%s", sm_node_admin_state_str(admin_state) );
    snprintf( update_msg->oper_state, sizeof(update_msg->oper_state),
              "%s", sm_node_oper_state_str(oper_state) );
    snprintf( update_msg->avail_status, sizeof(update_msg->avail_status),
              "%s", sm_node_avail_status_str(avail_status) );
    snprintf( update_msg->ready_state, sizeof(update_msg->ready_state),
              "%s", sm_node_ready_state_str(ready_state) );
    snprintf( update_msg->old_state_uuid,
              sizeof(update_msg->old_state_uuid), "%s", old_state_uuid );
    snprintf( update_msg->state_uuid, sizeof(update_msg->state_uuid),
              "%s", state_uuid );
    update_msg->uptime = htonl(uptime);
    update_msg->force = htonl(force);

    if( SM_AUTH_TYPE_HMAC_SHA512 == interface->auth_type )
    {
        SmSha512HashT hash;

        msg->header.auth_type = htonl(interface->auth_type);
        sm_sha512_hmac( msg, sizeof(SmMsgT), interface->auth_key,
                        strlen(interface->auth_key), &hash );
        memcpy( &(msg->header.auth_vector[0]), &(hash.bytes[0]), 
                SM_SHA512_HASH_SIZE );
    } else {
        msg->header.auth_type = htonl(SM_AUTH_TYPE_NONE);
    }

    if(( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )||
       ( SM_NETWORK_TYPE_IPV4 == interface->network_type ))
    {
        result = sm_send_msg(interface, msg);
    } else if(( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type )||
              ( SM_NETWORK_TYPE_IPV6 == interface->network_type )) 
    {
        result = sm_send_ipv6_msg(interface, msg);
    }
    if( 0 > result )
    {
        if ( _MSG_NOT_SENT_TO_TARGET != result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface->interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );
    }

    ++_send_node_update_cnt;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Node Swact
// ===========================
SmErrorT sm_msg_send_node_swact( char node_name[], bool force,
    SmUuidT request_uuid, SmServiceDomainInterfaceT* interface )
{
    SmMsgT* msg = (SmMsgT*) _tx_buffer;
    SmMsgNodeSwactT* swact_msg = &(msg->u.node_swact);
    int result = -1;

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    msg->header.version = htons(SM_VERSION);
    msg->header.revision = htons(SM_REVISION);
    msg->header.max_supported_version = htons(SM_VERSION);
    msg->header.max_supported_revision = htons(SM_REVISION);
    msg->header.msg_len = htons(sizeof(SmMsgT));
    msg->header.msg_type = htons(SM_MSG_TYPE_NODE_SWACT);
    msg->header.msg_flags = htonll(0);
    snprintf( msg->header.msg_instance, sizeof(msg->header.msg_instance),
              "%s", _msg_instance );
    msg->header.msg_seq_num = htonll(_msg_seq_num);
    snprintf( msg->header.node_name, sizeof(msg->header.node_name),
              "%s", _hostname );    

    snprintf( swact_msg->node_name, sizeof(swact_msg->node_name),
              "%s", node_name );
    snprintf( swact_msg->request_uuid, sizeof(swact_msg->request_uuid),
              "%s", request_uuid );
    swact_msg->force = htonl(force);

    if( SM_AUTH_TYPE_HMAC_SHA512 == interface->auth_type )
    {
        SmSha512HashT hash;

        msg->header.auth_type = htonl(interface->auth_type);
        sm_sha512_hmac( msg, sizeof(SmMsgT), interface->auth_key,
                        strlen(interface->auth_key), &hash );
        memcpy( &(msg->header.auth_vector[0]), &(hash.bytes[0]), 
                SM_SHA512_HASH_SIZE );
    } else {
        msg->header.auth_type = htonl(SM_AUTH_TYPE_NONE);
    }

    if(( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )||
       ( SM_NETWORK_TYPE_IPV4 == interface->network_type ))
    {
        result = sm_send_msg(interface, msg);
    } else if(( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type )||
              ( SM_NETWORK_TYPE_IPV6 == interface->network_type ))
    {
        result = sm_send_ipv6_msg(interface, msg);
    }

    if( 0 > result )
    {
        if ( _MSG_NOT_SENT_TO_TARGET != result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface->interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );
    }

    ++_send_node_swact_cnt;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Node Swact Ack
// ===============================
SmErrorT sm_msg_send_node_swact_ack( char node_name[], bool force,
    SmUuidT request_uuid, SmServiceDomainInterfaceT* interface )
{
    SmMsgT* msg = (SmMsgT*) _tx_buffer;
    SmMsgNodeSwactAckT* swact_ack_msg = NULL;
    int result = -1;

    swact_ack_msg = &(msg->u.node_swact_ack);

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    msg->header.version = htons(SM_VERSION);
    msg->header.revision = htons(SM_REVISION);
    msg->header.max_supported_version = htons(SM_VERSION);
    msg->header.max_supported_revision = htons(SM_REVISION);
    msg->header.msg_len = htons(sizeof(SmMsgT));
    msg->header.msg_type = htons(SM_MSG_TYPE_NODE_SWACT_ACK);
    msg->header.msg_flags = htonll(0);
    snprintf( msg->header.msg_instance, sizeof(msg->header.msg_instance),
              "%s", _msg_instance );
    msg->header.msg_seq_num = htonll(_msg_seq_num);
    snprintf( msg->header.node_name, sizeof(msg->header.node_name),
              "%s", _hostname );    

    snprintf( swact_ack_msg->node_name,
              sizeof(swact_ack_msg->node_name), "%s", node_name );
    snprintf( swact_ack_msg->request_uuid,
              sizeof(swact_ack_msg->request_uuid), "%s", request_uuid );
    swact_ack_msg->force = htonl(force);

    if( SM_AUTH_TYPE_HMAC_SHA512 == interface->auth_type )
    {
        SmSha512HashT hash;

        msg->header.auth_type = htonl(interface->auth_type);
        sm_sha512_hmac( msg, sizeof(SmMsgT), interface->auth_key,
                        strlen(interface->auth_key), &hash );
        memcpy( &(msg->header.auth_vector[0]), &(hash.bytes[0]), 
                SM_SHA512_HASH_SIZE );
    } else {
        msg->header.auth_type = htonl(SM_AUTH_TYPE_NONE);
    }

    if(( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )||
       ( SM_NETWORK_TYPE_IPV4 == interface->network_type ))
    {
        result = sm_send_msg(interface, msg);
    } else if(( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type )||
              ( SM_NETWORK_TYPE_IPV6 == interface->network_type ))
    {
        result = sm_send_ipv6_msg(interface, msg);
    }

    if( 0 > result )
    {
        if ( _MSG_NOT_SENT_TO_TARGET != result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface->interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );
    }

    ++_send_node_swact_ack_cnt;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Hello
// =====================================
SmErrorT sm_msg_send_service_domain_hello( char node_name[],
    SmOrchestrationTypeT orchestration, SmDesignationTypeT designation,
    int generation, int priority, int hello_interval, int dead_interval,
    int wait_interval, int exchange_interval, char leader[],
    SmServiceDomainInterfaceT* interface )
{
    SmMsgT* msg = (SmMsgT*) _tx_buffer;
    SmMsgServiceDomainHelloT* hello_msg = &(msg->u.hello);
    int result = -1;

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    msg->header.version = htons(SM_VERSION);
    msg->header.revision = htons(SM_REVISION);
    msg->header.max_supported_version = htons(SM_VERSION);
    msg->header.max_supported_revision = htons(SM_REVISION);
    msg->header.msg_len = htons(sizeof(SmMsgT));
    msg->header.msg_type = htons(SM_MSG_TYPE_SERVICE_DOMAIN_HELLO);
    msg->header.msg_flags = htonll(0);
    snprintf( msg->header.msg_instance, sizeof(msg->header.msg_instance),
              "%s", _msg_instance );
    msg->header.msg_seq_num = htonll(_msg_seq_num);
    snprintf( msg->header.node_name, sizeof(msg->header.node_name),
              "%s", _hostname );    

    snprintf( hello_msg->service_domain, sizeof(hello_msg->service_domain),
              "%s", interface->service_domain );
    snprintf( hello_msg->node_name, sizeof(hello_msg->node_name),
              "%s", node_name );
    snprintf( hello_msg->orchestration, sizeof(hello_msg->orchestration),
              "%s", sm_orchestration_type_str(orchestration) );
    snprintf( hello_msg->designation, sizeof(hello_msg->designation),
              "%s", sm_designation_type_str(designation) );
    hello_msg->generation = htonl(generation);
    hello_msg->priority = htonl(priority);
    hello_msg->hello_interval = htonl(hello_interval);
    hello_msg->dead_interval = htonl(dead_interval);
    hello_msg->wait_interval = htonl(wait_interval);
    hello_msg->exchange_interval = htonl(exchange_interval);
    snprintf( hello_msg->leader, sizeof(hello_msg->leader), "%s", leader );

    if( SM_AUTH_TYPE_HMAC_SHA512 == interface->auth_type )
    {
        SmSha512HashT hash;

        msg->header.auth_type = htonl(interface->auth_type);
        sm_sha512_hmac( msg, sizeof(SmMsgT), interface->auth_key,
                        strlen(interface->auth_key), &hash );
        memcpy( &(msg->header.auth_vector[0]), &(hash.bytes[0]), 
                SM_SHA512_HASH_SIZE );
    } else {
        msg->header.auth_type = htonl(SM_AUTH_TYPE_NONE);
    }

    if(( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )||
       ( SM_NETWORK_TYPE_IPV4 == interface->network_type ))
    {
        result = sm_send_msg(interface, msg);
    } else if(( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type )||
              ( SM_NETWORK_TYPE_IPV6 == interface->network_type ))
    {
        result = sm_send_ipv6_msg(interface, msg);
    }

    if( 0 > result )
    {
        if ( _MSG_NOT_SENT_TO_TARGET != result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface->interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );
    }

    ++_send_service_domain_hello_cnt;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Pause
// =====================================
SmErrorT sm_msg_send_service_domain_pause( char node_name[],
    int pause_interval, SmServiceDomainInterfaceT* interface )
{
    SmMsgT* msg = (SmMsgT*) _tx_buffer;
    SmMsgServiceDomainPauseT* pause_msg = &(msg->u.pause);
    int result = -1;

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    msg->header.version = htons(SM_VERSION);
    msg->header.revision = htons(SM_REVISION);
    msg->header.max_supported_version = htons(SM_VERSION);
    msg->header.max_supported_revision = htons(SM_REVISION);
    msg->header.msg_len = htons(sizeof(SmMsgT));
    msg->header.msg_type = htons(SM_MSG_TYPE_SERVICE_DOMAIN_PAUSE);
    msg->header.msg_flags = htonll(0);
    snprintf( msg->header.msg_instance, sizeof(msg->header.msg_instance),
              "%s", _msg_instance );
    msg->header.msg_seq_num = htonll(_msg_seq_num);
    snprintf( msg->header.node_name, sizeof(msg->header.node_name),
              "%s", _hostname );    

    snprintf( pause_msg->service_domain, sizeof(pause_msg->service_domain),
              "%s", interface->service_domain );
    snprintf( pause_msg->node_name, sizeof(pause_msg->node_name),
              "%s", node_name );
    pause_msg->pause_interval = htonl(pause_interval);

    if( SM_AUTH_TYPE_HMAC_SHA512 == interface->auth_type )
    {
        SmSha512HashT hash;

        msg->header.auth_type = htonl(interface->auth_type);
        sm_sha512_hmac( msg, sizeof(SmMsgT), interface->auth_key,
                        strlen(interface->auth_key), &hash );
        memcpy( &(msg->header.auth_vector[0]), &(hash.bytes[0]), 
                SM_SHA512_HASH_SIZE );
    } else {
        msg->header.auth_type = htonl(SM_AUTH_TYPE_NONE);
    }

    if(( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )||
       ( SM_NETWORK_TYPE_IPV4 == interface->network_type ))
    {
        result = sm_send_msg(interface, msg);
    } else if(( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type )||
              ( SM_NETWORK_TYPE_IPV6 == interface->network_type ))
    {
        result = sm_send_ipv6_msg(interface, msg);
    }

    if( 0 > result )
    {
        if ( _MSG_NOT_SENT_TO_TARGET != result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface->interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );
    }

    ++_send_service_domain_pause_cnt;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Exchange Start
// ==============================================
SmErrorT sm_msg_send_service_domain_exchange_start( char node_name[],
    char exchange_node_name[], int exchange_seq,
    SmServiceDomainInterfaceT* interface )
{
    SmMsgT* msg = (SmMsgT*) _tx_buffer;
    SmMsgServiceDomainExchangeStartT* exchange_start_msg;
    int result = -1;

    exchange_start_msg = &(msg->u.exchange_start);

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    msg->header.version = htons(SM_VERSION);
    msg->header.revision = htons(SM_REVISION);
    msg->header.max_supported_version = htons(SM_VERSION);
    msg->header.max_supported_revision = htons(SM_REVISION);
    msg->header.msg_len = htons(sizeof(SmMsgT));
    msg->header.msg_type = htons(SM_MSG_TYPE_SERVICE_DOMAIN_EXCHANGE_START);
    msg->header.msg_flags = htonll(0);
    snprintf( msg->header.msg_instance, sizeof(msg->header.msg_instance),
              "%s", _msg_instance );
    msg->header.msg_seq_num = htonll(_msg_seq_num);
    snprintf( msg->header.node_name, sizeof(msg->header.node_name),
              "%s", _hostname );    

    snprintf( exchange_start_msg->service_domain,
              sizeof(exchange_start_msg->service_domain),
              "%s", interface->service_domain );
    snprintf( exchange_start_msg->node_name,
              sizeof(exchange_start_msg->node_name),
              "%s", node_name );
    snprintf( exchange_start_msg->exchange_node_name,
              sizeof(exchange_start_msg->exchange_node_name),
              "%s", exchange_node_name );
    exchange_start_msg->exchange_seq = htonl(exchange_seq);

    if( SM_AUTH_TYPE_HMAC_SHA512 == interface->auth_type )
    {
        SmSha512HashT hash;

        msg->header.auth_type = htonl(interface->auth_type);
        sm_sha512_hmac( msg, sizeof(SmMsgT), interface->auth_key,
                        strlen(interface->auth_key), &hash );
        memcpy( &(msg->header.auth_vector[0]), &(hash.bytes[0]), 
                SM_SHA512_HASH_SIZE );
    } else {
        msg->header.auth_type = htonl(SM_AUTH_TYPE_NONE);
    }

    if(( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )||
       ( SM_NETWORK_TYPE_IPV4 == interface->network_type ))
    {
        result = sm_send_msg(interface, msg);
    } else if(( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type )||
              ( SM_NETWORK_TYPE_IPV6 == interface->network_type ))
    {
        result = sm_send_ipv6_msg(interface, msg);
    }

    if( 0 > result )
    {
        if ( _MSG_NOT_SENT_TO_TARGET != result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface->interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );
    }

    ++_send_service_domain_exchange_start_cnt;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Exchange
// ========================================
SmErrorT sm_msg_send_service_domain_exchange( char node_name[],
    char exchange_node_name[], int exchange_seq, int64_t member_id,
    char member_name[], SmServiceGroupStateT member_desired_state,
    SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
    SmServiceGroupConditionT member_condition, int64_t member_health,
    char reason_text[], bool more_members, int64_t last_received_member_id,
    SmServiceDomainInterfaceT* interface )
{
    SmMsgT* msg = (SmMsgT*) _tx_buffer;
    SmMsgServiceDomainExchangeT* exchange_msg = &(msg->u.exchange);
    int result = -1;

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    msg->header.version = htons(SM_VERSION);
    msg->header.revision = htons(SM_REVISION);
    msg->header.max_supported_version = htons(SM_VERSION);
    msg->header.max_supported_revision = htons(SM_REVISION);
    msg->header.msg_len = htons(sizeof(SmMsgT));
    msg->header.msg_type = htons(SM_MSG_TYPE_SERVICE_DOMAIN_EXCHANGE);
    msg->header.msg_flags = htonll(0);
    snprintf( msg->header.msg_instance, sizeof(msg->header.msg_instance),
              "%s", _msg_instance );
    msg->header.msg_seq_num = htonll(_msg_seq_num);
    snprintf( msg->header.node_name, sizeof(msg->header.node_name),
              "%s", _hostname );    

    snprintf( exchange_msg->service_domain, sizeof(exchange_msg->service_domain),
              "%s", interface->service_domain );
    snprintf( exchange_msg->node_name, sizeof(exchange_msg->node_name),
              "%s", node_name );
    snprintf( exchange_msg->exchange_node_name,
              sizeof(exchange_msg->exchange_node_name),
              "%s", exchange_node_name );
    exchange_msg->exchange_seq = htonl(exchange_seq);
    exchange_msg->member_id = htonll(member_id);
    snprintf( exchange_msg->member_name, sizeof(exchange_msg->member_name), 
              "%s", member_name );
    snprintf( exchange_msg->member_desired_state, 
              sizeof(exchange_msg->member_desired_state), 
              "%s", sm_service_group_state_str(member_desired_state) );
    snprintf( exchange_msg->member_state, sizeof(exchange_msg->member_state), 
              "%s", sm_service_group_state_str(member_state) );
    snprintf( exchange_msg->member_status, sizeof(exchange_msg->member_status), 
              "%s", sm_service_group_status_str(member_status) );
    snprintf(  exchange_msg->member_condition,
               sizeof( exchange_msg->member_condition), 
              "%s", sm_service_group_condition_str(member_condition) );
    exchange_msg->member_health = htonll(member_health);
    snprintf( exchange_msg->reason_text, sizeof(exchange_msg->reason_text), 
              "%s", reason_text );
    exchange_msg->more_members = htonl(more_members ? 1 : 0);
    exchange_msg->last_received_member_id = htonll(last_received_member_id);

    if( SM_AUTH_TYPE_HMAC_SHA512 == interface->auth_type )
    {
        SmSha512HashT hash;

        msg->header.auth_type = htonl(interface->auth_type);
        sm_sha512_hmac( msg, sizeof(SmMsgT), interface->auth_key,
                        strlen(interface->auth_key), &hash );
        memcpy( &(msg->header.auth_vector[0]), &(hash.bytes[0]), 
                SM_SHA512_HASH_SIZE );
    } else {
        msg->header.auth_type = htonl(SM_AUTH_TYPE_NONE);
    }

    if(( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )||
       (  SM_NETWORK_TYPE_IPV4 == interface->network_type ))
    {
        result = sm_send_msg(interface, msg);
    } else if(( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type )||
              ( SM_NETWORK_TYPE_IPV6 == interface->network_type ))
    {
        result = sm_send_ipv6_msg(interface, msg);
    }

    if( 0 > result )
    {
        if ( _MSG_NOT_SENT_TO_TARGET != result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface->interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );
    }

    ++_send_service_domain_exchange_cnt;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Member Request
// ==============================================
SmErrorT sm_msg_send_service_domain_member_request( char node_name[],
    char member_node_name[], int64_t member_id, char member_name[],
    SmServiceGroupActionT member_action,
    SmServiceGroupActionFlagsT member_action_flags,
    SmServiceDomainInterfaceT* interface )
{
    SmMsgT* msg = (SmMsgT*) _tx_buffer;
    SmMsgServiceDomainMemberRequestT* request_msg = &(msg->u.request);
    int result = -1;

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    msg->header.version = htons(SM_VERSION);
    msg->header.revision = htons(SM_REVISION);
    msg->header.max_supported_version = htons(SM_VERSION);
    msg->header.max_supported_revision = htons(SM_REVISION);
    msg->header.msg_len = htons(sizeof(SmMsgT));
    msg->header.msg_type = htons(SM_MSG_TYPE_SERVICE_DOMAIN_MEMBER_REQUEST);
    msg->header.msg_flags = htonll(0);
    snprintf( msg->header.msg_instance, sizeof(msg->header.msg_instance),
              "%s", _msg_instance );
    msg->header.msg_seq_num = htonll(_msg_seq_num);
    snprintf( msg->header.node_name, sizeof(msg->header.node_name),
              "%s", _hostname );    

    snprintf( request_msg->service_domain, sizeof(request_msg->service_domain),
              "%s", interface->service_domain );
    snprintf( request_msg->node_name, sizeof(request_msg->node_name),
              "%s", node_name );
    snprintf( request_msg->member_node_name, sizeof(request_msg->member_node_name),
              "%s", member_node_name );
    request_msg->member_id = htonll(member_id);
    snprintf( request_msg->member_name, sizeof(request_msg->member_name), 
              "%s", member_name );
    snprintf( request_msg->member_action, sizeof(request_msg->member_action), 
              "%s", sm_service_group_action_str(member_action) );
    request_msg->member_action_flags = htonll(member_action_flags);

    if( SM_AUTH_TYPE_HMAC_SHA512 == interface->auth_type )
    {
        SmSha512HashT hash;

        msg->header.auth_type = htonl(interface->auth_type);
        sm_sha512_hmac( msg, sizeof(SmMsgT), interface->auth_key,
                        strlen(interface->auth_key), &hash );
        memcpy( &(msg->header.auth_vector[0]), &(hash.bytes[0]),
                SM_SHA512_HASH_SIZE );
    } else {
        msg->header.auth_type = htonl(SM_AUTH_TYPE_NONE);
    }

    if(( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )||
       ( SM_NETWORK_TYPE_IPV4 == interface->network_type ))
    {
        result = sm_send_msg(interface, msg);
    } else if(( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type )||
              ( SM_NETWORK_TYPE_IPV6 == interface->network_type ))
    {
        result = sm_send_ipv6_msg(interface, msg);
    }

    if( 0 > result )
    {
        if ( _MSG_NOT_SENT_TO_TARGET != result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface->interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );
    }

    ++_send_service_domain_member_request_cnt;
    
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Send Service Domain Member Update
// =============================================
SmErrorT sm_msg_send_service_domain_member_update( char node_name[],
    char member_node_name[], int64_t member_id, char member_name[],
    SmServiceGroupStateT member_desired_state,
    SmServiceGroupStateT member_state, SmServiceGroupStatusT member_status,
    SmServiceGroupConditionT member_condition, int64_t member_health,
    const char reason_text[], SmServiceDomainInterfaceT* interface )
{
    SmMsgT* msg = (SmMsgT*) _tx_buffer;
    SmMsgServiceDomainMemberUpdateT* update_msg = &(msg->u.update);
    int result = -1;

    memset( _tx_buffer, 0, sizeof(_tx_buffer) );

    msg->header.version = htons(SM_VERSION);
    msg->header.revision = htons(SM_REVISION);
    msg->header.max_supported_version = htons(SM_VERSION);
    msg->header.max_supported_revision = htons(SM_REVISION);
    msg->header.msg_len = htons(sizeof(SmMsgT));
    msg->header.msg_type = htons(SM_MSG_TYPE_SERVICE_DOMAIN_MEMBER_UPDATE);
    msg->header.msg_flags = htonll(0);
    snprintf( msg->header.msg_instance, sizeof(msg->header.msg_instance),
              "%s", _msg_instance );
    msg->header.msg_seq_num = htonll(_msg_seq_num);
    snprintf( msg->header.node_name, sizeof(msg->header.node_name),
              "%s", _hostname );    

    snprintf( update_msg->service_domain, sizeof(update_msg->service_domain),
              "%s", interface->service_domain );
    snprintf( update_msg->node_name, sizeof(update_msg->node_name),
              "%s", node_name );
    snprintf( update_msg->member_node_name, sizeof(update_msg->member_node_name),
              "%s", member_node_name );
    update_msg->member_id = htonll(member_id);
    snprintf( update_msg->member_name, sizeof(update_msg->member_name), 
              "%s", member_name );
    snprintf( update_msg->member_desired_state, 
              sizeof(update_msg->member_desired_state), 
              "%s", sm_service_group_state_str(member_desired_state) );
    snprintf( update_msg->member_state, sizeof(update_msg->member_state), 
              "%s", sm_service_group_state_str(member_state) );
    snprintf( update_msg->member_status, sizeof(update_msg->member_status), 
              "%s", sm_service_group_status_str(member_status) );
    snprintf( update_msg->member_condition, sizeof(update_msg->member_condition), 
              "%s", sm_service_group_condition_str(member_condition) );
    update_msg->member_health = htonll(member_health);
    snprintf( update_msg->reason_text, sizeof(update_msg->reason_text), 
              "%s", reason_text );

    if( SM_AUTH_TYPE_HMAC_SHA512 == interface->auth_type )
    {
        SmSha512HashT hash;

        msg->header.auth_type = htonl(interface->auth_type);
        sm_sha512_hmac( msg, sizeof(SmMsgT), interface->auth_key,
                        strlen(interface->auth_key), &hash );
        memcpy( &(msg->header.auth_vector[0]), &(hash.bytes[0]), 
                SM_SHA512_HASH_SIZE );
    } else {
        msg->header.auth_type = htonl(SM_AUTH_TYPE_NONE);
    }

    if(( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )||
       ( SM_NETWORK_TYPE_IPV4 == interface->network_type ))
    {
        result = sm_send_msg(interface, msg);
    } else if(( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type )||
              ( SM_NETWORK_TYPE_IPV6 == interface->network_type )) 
    {
        result = sm_send_ipv6_msg(interface, msg);
    }

    if( 0 > result )
    {
        if ( _MSG_NOT_SENT_TO_TARGET != result )
        {
            DPRINTFE( "Failed to send message on socket for interface (%s), "
                      "error=%s.", interface->interface_name, strerror( errno ) );
            return( SM_FAILED );
        }
        return( SM_OKAY );
    }

    ++_send_service_domain_member_update_cnt;
    
    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Get Ancillary Data 
// ==============================
static void* sm_msg_get_ancillary_data( struct msghdr* msg_hdr, int cmsg_level,
    int cmsg_type )
{
    struct cmsghdr* cmsg;

    for( cmsg = CMSG_FIRSTHDR(msg_hdr); NULL != cmsg;
         cmsg = CMSG_NXTHDR( msg_hdr, cmsg) )
    {
        if(( cmsg_level == cmsg->cmsg_level )&&
           ( cmsg_type == cmsg->cmsg_type ))
        {
            return( CMSG_DATA(cmsg) );
        }
    }

    return( NULL );
}
// ****************************************************************************


// ****************************************************************************
// Messaging - Dispatch Message
// ============================
static void sm_msg_dispatch_msg( bool is_multicast_msg, SmMsgT* msg,
    SmNetworkAddressT* network_address, int network_port,
    SmMsgPeerInterfaceInfoT* peer_interface )
{
    SmListT* entry = NULL;
    SmListEntryDataPtrT entry_data;
    SmMsgCallbacksT* callbacks;
    SmMsgNodeHelloT* node_hello_msg = &(msg->u.node_hello);
    SmMsgNodeUpdateT* node_update_msg = &(msg->u.node_update);
    SmMsgNodeSwactT* node_swact_msg;
    SmMsgNodeSwactAckT* node_swact_ack_msg;
    SmNodeAdminStateT node_admin_state;
    SmNodeOperationalStateT node_oper_state;
    SmNodeAvailStatusT node_avail_status;
    SmNodeReadyStateT node_ready_state;
    SmMsgServiceDomainHelloT* hello_msg = &(msg->u.hello);    
    SmMsgServiceDomainPauseT* pause_msg = &(msg->u.pause);
    SmMsgServiceDomainExchangeStartT* exchange_start_msg;
    SmMsgServiceDomainExchangeT* exchange_msg = &(msg->u.exchange);
    SmMsgServiceDomainMemberRequestT* request_msg = &(msg->u.request); 
    SmMsgServiceDomainMemberUpdateT* update_msg = &(msg->u.update); 
    SmServiceGroupStateT member_desired_state;
    SmServiceGroupStateT member_state;
    SmServiceGroupStatusT member_status;
    SmServiceGroupConditionT member_condition;
    SmServiceGroupActionT member_action;


    node_swact_msg = &(msg->u.node_swact);
    node_swact_ack_msg = &(msg->u.node_swact_ack);
    exchange_start_msg = &(msg->u.exchange_start);

    if( SM_VERSION != ntohs(msg->header.version) )
    {
        ++_rcvd_bad_msg_version;
        DPRINTFD( "Received unsupported version (%i).",
                  ntohs(msg->header.version) );
        return;
    }

    if( 0 == strcmp( _hostname, msg->header.node_name ) )
    {
        DPRINTFD( "Received own message (%i),",
                  ntohs(msg->header.msg_type) );
        return;
    }

    if( !(sm_msg_in_sequence( msg->header.node_name, msg->header.msg_instance,
                              ntohll(msg->header.msg_seq_num) )) )
    {
        DPRINTFD( "Duplicate message received from node (%s), "
                  "msg_seq=%" PRIi64 ".", msg->header.node_name,
                  ntohll(msg->header.msg_seq_num) );
        return;
    } else {
        DPRINTFD( "Message received from node (%s), "
                  "msg_seq=%" PRIi64 ".", msg->header.node_name,
                  ntohll(msg->header.msg_seq_num) );
    }

    if( SM_AUTH_TYPE_HMAC_SHA512 == ntohl(msg->header.auth_type) )
    {
        uint8_t auth_vector[SM_AUTHENTICATION_VECTOR_MAX_CHAR];
        SmSha512HashT hash;

        memcpy( auth_vector, &(msg->header.auth_vector[0]),
                SM_AUTHENTICATION_VECTOR_MAX_CHAR );
        memset( &(msg->header.auth_vector[0]), 0,
                SM_AUTHENTICATION_VECTOR_MAX_CHAR );

        sm_sha512_hmac( msg, sizeof(SmMsgT), peer_interface->auth_key,
                        strlen(peer_interface->auth_key), &hash );

        if( 0 != memcmp( &(hash.bytes[0]), auth_vector, SM_SHA512_HASH_SIZE ) )
        {
            ++_rcvd_bad_msg_auth;
            DPRINTFD( "Authentication check failed on message (%i) from "
                      "node (%s).", ntohs(msg->header.msg_type),
                      msg->header.node_name );
            return;
        } else {
            DPRINTFD( "Authentication check passed on message (%i) from "
                      "node (%s).", ntohs(msg->header.msg_type),
                      msg->header.node_name );
        }
    }

    uint16_t msg_type = ntohs(msg->header.msg_type) ;

    switch( msg_type )
    {
        case SM_MSG_TYPE_NODE_HELLO:
            ++_rcvd_node_hello_cnt;

            node_admin_state
                = sm_node_admin_state_value(node_hello_msg->admin_state);
            node_oper_state
                = sm_node_oper_state_value(node_hello_msg->oper_state);
            node_avail_status
                = sm_node_avail_status_value(node_hello_msg->avail_status);
            node_ready_state
                = sm_node_ready_state_value(node_hello_msg->ready_state);

            SM_LIST_FOREACH( _callbacks, entry, entry_data )
            {
                callbacks = (SmMsgCallbacksT*) entry_data;

                if( NULL == callbacks->node_hello )
                    continue;

                callbacks->node_hello( network_address, network_port,
                                       ntohs(msg->header.version),
                                       ntohs(msg->header.revision),
                                       node_hello_msg->node_name,
                                       node_admin_state, node_oper_state,
                                       node_avail_status, node_ready_state,
                                       node_hello_msg->state_uuid,
                                       ntohl(node_hello_msg->uptime) );
            }
        break;

        case SM_MSG_TYPE_NODE_UPDATE:
            ++_rcvd_node_update_cnt;

            node_admin_state
                = sm_node_admin_state_value(node_update_msg->admin_state);
            node_oper_state
                = sm_node_oper_state_value(node_update_msg->oper_state);
            node_avail_status
                = sm_node_avail_status_value(node_update_msg->avail_status);
            node_ready_state
                = sm_node_ready_state_value(node_update_msg->ready_state);

            SM_LIST_FOREACH( _callbacks, entry, entry_data )
            {
                callbacks = (SmMsgCallbacksT*) entry_data;

                if( NULL == callbacks->node_update )
                    continue;

                callbacks->node_update( network_address, network_port,
                                        ntohs(msg->header.version),
                                        ntohs(msg->header.revision),
                                        node_update_msg->node_name,
                                        node_admin_state, node_oper_state,
                                        node_avail_status, node_ready_state,
                                        node_update_msg->old_state_uuid, 
                                        node_update_msg->state_uuid, 
                                        ntohl(node_update_msg->uptime),
                                        ntohl(node_update_msg->force) );
            }
        break;

        case SM_MSG_TYPE_NODE_SWACT:
            ++_rcvd_node_swact_cnt;
            
            SM_LIST_FOREACH( _callbacks, entry, entry_data )
            {
                callbacks = (SmMsgCallbacksT*) entry_data;

                if( NULL == callbacks->node_swact )
                    continue;

                callbacks->node_swact( network_address, network_port,
                                       ntohs(msg->header.version),
                                       ntohs(msg->header.revision),
                                       node_swact_msg->node_name,
                                       ntohl(node_swact_msg->force),
                                       node_swact_msg->request_uuid );
            }
        break;

        case SM_MSG_TYPE_NODE_SWACT_ACK:
            ++_rcvd_node_swact_ack_cnt;
            
            SM_LIST_FOREACH( _callbacks, entry, entry_data )
            {
                callbacks = (SmMsgCallbacksT*) entry_data;

                if( NULL == callbacks->node_swact_ack )
                    continue;

                callbacks->node_swact_ack( network_address, network_port,
                                           ntohs(msg->header.version),
                                           ntohs(msg->header.revision),
                                           node_swact_ack_msg->node_name,
                                           ntohl(node_swact_ack_msg->force),
                                           node_swact_ack_msg->request_uuid );
            }
        break;


        case SM_MSG_TYPE_SERVICE_DOMAIN_HELLO:
            ++_rcvd_service_domain_hello_cnt;
            
            SM_LIST_FOREACH( _callbacks, entry, entry_data )
            {
                callbacks = (SmMsgCallbacksT*) entry_data;

                if( NULL == callbacks->hello )
                    continue;

                callbacks->hello( network_address, network_port,
                                  ntohs(msg->header.version),
                                  ntohs(msg->header.revision),
                                  hello_msg->service_domain, 
                                  hello_msg->node_name,
                                  hello_msg->orchestration, 
                                  hello_msg->designation,
                                  ntohl(hello_msg->generation),
                                  ntohl(hello_msg->priority),
                                  ntohl(hello_msg->hello_interval),
                                  ntohl(hello_msg->dead_interval),
                                  ntohl(hello_msg->wait_interval),
                                  ntohl(hello_msg->exchange_interval),
                                  hello_msg->leader );
            }
        break;

        case SM_MSG_TYPE_SERVICE_DOMAIN_PAUSE:
            ++_send_service_domain_pause_cnt;
            
            SM_LIST_FOREACH( _callbacks, entry, entry_data )
            {
                callbacks = (SmMsgCallbacksT*) entry_data;

                if( NULL == callbacks->pause )
                    continue;

                callbacks->pause( network_address, network_port,
                                  ntohs(msg->header.version),
                                  ntohs(msg->header.revision),
                                  pause_msg->service_domain, 
                                  pause_msg->node_name,
                                  ntohl(pause_msg->pause_interval) );
            }
        break;

        case SM_MSG_TYPE_SERVICE_DOMAIN_EXCHANGE_START:
            if( 0 != strcmp( exchange_start_msg->exchange_node_name, 
                             _hostname ) )
            {
                DPRINTFD( "Exchange start message not for us." );
                return;
            }

            ++_rcvd_service_domain_exchange_start_cnt;
            
            SM_LIST_FOREACH( _callbacks, entry, entry_data )
            {
                callbacks = (SmMsgCallbacksT*) entry_data;

                if( NULL == callbacks->exchange_start )
                    continue;

                callbacks->exchange_start( network_address, network_port,
                                  ntohs(msg->header.version),
                                  ntohs(msg->header.revision),
                                  exchange_start_msg->service_domain, 
                                  exchange_start_msg->node_name,
                                  ntohl(exchange_start_msg->exchange_seq) );
            }
        break;

        case SM_MSG_TYPE_SERVICE_DOMAIN_EXCHANGE:
            if( 0 != strcmp( exchange_msg->exchange_node_name, _hostname ) )
            {
                DPRINTFD( "Exchange message not for us." );
                return;
            }

            ++_rcvd_service_domain_exchange_cnt;

            member_desired_state = sm_service_group_state_value( 
                                    exchange_msg->member_desired_state );
            member_state = sm_service_group_state_value( 
                                    exchange_msg->member_state );
            member_status = sm_service_group_status_value(
                                    exchange_msg->member_status );

            member_condition = sm_service_group_condition_value(
                                    exchange_msg->member_condition );

            SM_LIST_FOREACH( _callbacks, entry, entry_data )
            {
                callbacks = (SmMsgCallbacksT*) entry_data;

                if( NULL == callbacks->exchange )
                    continue;

                callbacks->exchange( network_address, network_port,
                                  ntohs(msg->header.version),
                                  ntohs(msg->header.revision),
                                  exchange_msg->service_domain, 
                                  exchange_msg->node_name,
                                  ntohl(exchange_msg->exchange_seq),
                                  ntohll(exchange_msg->member_id),
                                  exchange_msg->member_name, 
                                  member_desired_state,
                                  member_state, member_status,
                                  member_condition,
                                  ntohll(exchange_msg->member_health),
                                  exchange_msg->reason_text,
                                  ntohl(exchange_msg->more_members) ? true : false,
                                  ntohll(exchange_msg->last_received_member_id) );
            }
        break;

        case SM_MSG_TYPE_SERVICE_DOMAIN_MEMBER_REQUEST:
            if( 0 != strcmp( request_msg->member_node_name, _hostname ) )
            {
                DPRINTFD( "Member request message not for us." );
                return;
            }
        
            ++_rcvd_service_domain_member_request_cnt;
            
            member_action = sm_service_group_action_value( 
                                        request_msg->member_action );

            SM_LIST_FOREACH( _callbacks, entry, entry_data )
            {
                callbacks = (SmMsgCallbacksT*) entry_data;

                if( NULL == callbacks->member_request )
                    continue;

                callbacks->member_request( network_address, network_port,
                                ntohs(msg->header.version),
                                ntohs(msg->header.revision),
                                request_msg->service_domain, 
                                request_msg->node_name,
                                request_msg->member_node_name,
                                ntohll(request_msg->member_id),
                                request_msg->member_name, 
                                member_action,
                                ntohll(request_msg->member_action_flags) );
            }
        break;

        case SM_MSG_TYPE_SERVICE_DOMAIN_MEMBER_UPDATE:
            ++_rcvd_service_domain_member_update_cnt;

            member_desired_state = sm_service_group_state_value( 
                                    update_msg->member_desired_state );
            member_state = sm_service_group_state_value( 
                                    update_msg->member_state );
            member_status = sm_service_group_status_value(
                                    update_msg->member_status );
            member_condition = sm_service_group_condition_value(
                                    update_msg->member_condition );

            SM_LIST_FOREACH( _callbacks, entry, entry_data )
            {
                callbacks = (SmMsgCallbacksT*) entry_data;

                if( NULL == callbacks->member_update )
                    continue;

                callbacks->member_update( network_address, network_port,
                                          ntohs(msg->header.version),
                                          ntohs(msg->header.revision),
                                          update_msg->service_domain,
                                          update_msg->node_name,
                                          update_msg->member_node_name,
                                          ntohll(update_msg->member_id),
                                          update_msg->member_name,
                                          member_desired_state,
                                          member_state, member_status,
                                          member_condition,
                                          ntohll(update_msg->member_health),
                                          update_msg->reason_text );
            }
        break;

        default:
            DPRINTFI( "Unsupported message type (%i) received.",
                      ntohs(msg->header.msg_type) );
        break;
    }
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Dispatch Ipv4 Udp
// =============================
static void sm_msg_dispatch_ipv4_udp( int selobj, int64_t msg_method )
{
    SmMsgT* msg = (SmMsgT*) _rx_buffer;
    SmNetworkAddressT network_address;
    SmMsgPeerInterfaceInfoT* peer_interface = NULL;
    SmErrorT error;
    int network_port;
    struct sockaddr_in src_addr;
    struct iovec iovec = {(void*)_rx_buffer, sizeof(_rx_buffer)};
    struct msghdr msg_hdr;
    int bytes_read;

    memset( &network_address, 0, sizeof(network_address) );
    memset( _rx_control_buffer, 0, sizeof(_rx_control_buffer) );
    memset( _rx_buffer, 0, sizeof(_rx_buffer) );
    memset( &msg_hdr, 0, sizeof(msg_hdr) );

    msg_hdr.msg_name = &src_addr;
    msg_hdr.msg_namelen= sizeof(src_addr);
    msg_hdr.msg_iov = &iovec;
    msg_hdr.msg_iovlen = 1;
    msg_hdr.msg_control = _rx_control_buffer;
    msg_hdr.msg_controllen = sizeof(_rx_control_buffer);

    int retry_i;
    for( retry_i = 5; retry_i != 0; --retry_i )
    {
        bytes_read = recvmsg( selobj, &msg_hdr, MSG_NOSIGNAL | MSG_DONTWAIT );
        if( 0 < bytes_read )
        {
            break;

        } else if( 0 == bytes_read ) {
            // For connection oriented sockets, this indicates that the peer
            // has performed an orderly shutdown.
            return;

        } else if(( 0 > bytes_read )&&( EINTR != errno )) {
            DPRINTFE( "Failed to receive message, errno=%s.",
                      strerror( errno ) );
            return;
        }

        DPRINTFD( "Interrupted while receiving message, retry=%d, errno=%s.",
                  retry_i, strerror( errno ) );
    }

    ++_rcvd_total_msgs;

    if( !_messaging_enabled )
    {
        ++_rcvd_msgs_while_disabled;
        DPRINTFD( "Messaging is disabled." );
        return;
    }

    if( AF_INET == src_addr.sin_family )
    {
        char if_name[SM_INTERFACE_NAME_MAX_CHAR] = {0};
        struct in_pktinfo* pkt_info;
        char network_address_str[SM_NETWORK_ADDRESS_MAX_CHAR];

        pkt_info = (struct in_pktinfo*) sm_msg_get_ancillary_data( &msg_hdr,
                                                        SOL_IP, IP_PKTINFO );
        if( NULL == pkt_info )
        {
            DPRINTFD( "No packet information available." );
            return;
        }

        error = sm_hw_get_if_name( pkt_info->ipi_ifindex, if_name );
        if( SM_OKAY != error )
        {
            if( SM_NOT_FOUND == error )
            {
                DPRINTFD( "Failed to get interface name for interface index "
                          "(%i), error=%s.", pkt_info->ipi_ifindex,
                          sm_error_str(error) );
            } else {
                DPRINTFE( "Failed to get interface name for interface index "
                          "(%i), error=%s.", pkt_info->ipi_ifindex,
                          sm_error_str(error) );
            }
            return;
        }

        network_address.type = SM_NETWORK_TYPE_IPV4_UDP;
        network_address.u.ipv4.sin.s_addr = src_addr.sin_addr.s_addr;
        network_port = ntohs(src_addr.sin_port);

        sm_network_address_str( &network_address, network_address_str );

        DPRINTFD( "Received message from ip (%s), port (%i) on "
                  "interface (%s).", network_address_str, network_port,
                  if_name );

        peer_interface = sm_msg_get_peer_interface( if_name, &network_address,
                                                    network_port );
        if( NULL == peer_interface )
        {
            DPRINTFD( "Message received not for us, if=%s, ip=%s, port=%i.",
                      if_name, network_address_str, network_port );
            return;
        }
    } else {
        DPRINTFE( "Received unsupported network address type (%i).",
                  src_addr.sin_family );
        return;
    }

    // Once the message is received, pass it to protocol-independent handler
    sm_msg_dispatch_msg( msg_method, msg, &network_address, network_port,
                         peer_interface );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Dispatch Ipv6 Udp
// =============================
static void sm_msg_dispatch_ipv6_udp( int selobj, int64_t msg_method )
{
    SmMsgT* msg = (SmMsgT*) _rx_buffer;
    SmNetworkAddressT network_address;
    SmMsgPeerInterfaceInfoT* peer_interface = NULL;
    SmErrorT error;
    int network_port;
    struct sockaddr_in6 src_addr;
    struct iovec iovec = {(void*)_rx_buffer, sizeof(_rx_buffer)};
    struct msghdr msg_hdr;
    int bytes_read;

    memset( &network_address, 0, sizeof(network_address) );
    memset( _rx_control_buffer, 0, sizeof(_rx_control_buffer) );
    memset( _rx_buffer, 0, sizeof(_rx_buffer) );
    memset( &msg_hdr, 0, sizeof(msg_hdr) );

    msg_hdr.msg_name = &src_addr;
    msg_hdr.msg_namelen= sizeof(src_addr);
    msg_hdr.msg_iov = &iovec;
    msg_hdr.msg_iovlen = 1;
    msg_hdr.msg_control = _rx_control_buffer;
    msg_hdr.msg_controllen = sizeof(_rx_control_buffer);

    int retry_i;
    for( retry_i = 5; retry_i != 0; --retry_i )
    {
        bytes_read = recvmsg( selobj, &msg_hdr, MSG_NOSIGNAL | MSG_DONTWAIT );
        if( 0 < bytes_read )
        {
            break;

        } else if( 0 == bytes_read ) {
            // For connection oriented sockets, this indicates that the peer
            // has performed an orderly shutdown.
            return;

        } else if(( 0 > bytes_read )&&( EINTR != errno )) {
            DPRINTFE( "Failed to receive message, errno=%s.",
                      strerror( errno ) );
            return;
        }

        DPRINTFD( "Interrupted while receiving message, retry=%d, errno=%s.",
                  retry_i, strerror( errno ) );
    }

    ++_rcvd_total_msgs;

    if( !_messaging_enabled )
    {
        ++_rcvd_msgs_while_disabled;
        DPRINTFD( "Messaging is disabled." );
        return;
    }

    if( AF_INET6 == src_addr.sin6_family )
    {
        char if_name[SM_INTERFACE_NAME_MAX_CHAR] = {0};
        struct in6_pktinfo* pkt_info;
        char network_address_str[SM_NETWORK_ADDRESS_MAX_CHAR];

        pkt_info = (struct in6_pktinfo*) sm_msg_get_ancillary_data( &msg_hdr,
                                                        SOL_IPV6, IPV6_PKTINFO );
        if( NULL == pkt_info )
        {
            DPRINTFD( "No packet information available." );
            return;
        }

        error = sm_hw_get_if_name( pkt_info->ipi6_ifindex, if_name );
        if( SM_OKAY != error )
        {
            if( SM_NOT_FOUND == error )
            {
                DPRINTFD( "Failed to get interface name for interface index "
                          "(%i), error=%s.", pkt_info->ipi6_ifindex,
                          sm_error_str(error) );
            } else {
                DPRINTFE( "Failed to get interface name for interface index "
                          "(%i), error=%s.", pkt_info->ipi6_ifindex,
                          sm_error_str(error) );
            }
            return;
        }

        network_address.type = SM_NETWORK_TYPE_IPV6_UDP;
        network_address.u.ipv6.sin6 = src_addr.sin6_addr;
        network_port = ntohs(src_addr.sin6_port);

        sm_network_address_str( &network_address, network_address_str );

        DPRINTFD( "Received message from ip (%s), port (%i) on "
                  "interface (%s).", network_address_str, network_port,
                  if_name );

        peer_interface = sm_msg_get_peer_interface( if_name, &network_address,
                                                    network_port );
        if( NULL == peer_interface )
        {
            DPRINTFD( "Message received not for us, if=%s, ip=%s, port=%i.",
                      if_name, network_address_str, network_port );
            return;
        }
    } else {
        DPRINTFE( "Received unsupported network address type (%i).",
                  src_addr.sin6_family );
        return;
    }

    // Once the message is received, pass it to protocol-independent handler
    sm_msg_dispatch_msg( msg_method, msg, &network_address, network_port,
                         peer_interface );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Register Ipv4 Multicast Address
// ===========================================
static SmErrorT sm_msg_register_ipv4_multicast(
    SmServiceDomainInterfaceT* interface )
{
    int if_index;
    struct ip_mreqn mreq;
    int result = 0;
    SmIpv4AddressT* ipv4_multicast = &(interface->network_multicast.u.ipv4);
    SmErrorT error;

    error = sm_hw_get_if_index( interface->interface_name, &if_index );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to convert interface name (%s) to interface index, "
                  "error=%s.", interface->interface_name, 
                  sm_error_str( error ) );
        return( error );
    }

    memset( &mreq, 0, sizeof(mreq) );

    mreq.imr_multiaddr.s_addr = ipv4_multicast->sin.s_addr;
    mreq.imr_ifindex          = if_index;

    result = setsockopt( interface->multicast_socket, IPPROTO_IP,
                         IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq) );
    if( 0 > result ) 
    {
        DPRINTFE( "Failed to add multicast membership to interface (%s), "
                  "errno=%s.", interface->interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Register Ipv6 Multicast Address
// ===========================================
static SmErrorT sm_msg_register_ipv6_multicast(
    SmServiceDomainInterfaceT* interface )
{
    int if_index;
    struct ipv6_mreq mreq;
    int result = 0;
    SmIpv6AddressT* ipv6_multicast = &(interface->network_multicast.u.ipv6);
    SmErrorT error;

    error = sm_hw_get_if_index( interface->interface_name, &if_index );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to convert interface name (%s) to interface index, "
                  "error=%s.", interface->interface_name,
                  sm_error_str( error ) );
        return( error );
    }

    memset( &mreq, 0, sizeof(mreq) );

    mreq.ipv6mr_multiaddr = ipv6_multicast->sin6;
    mreq.ipv6mr_interface = if_index;

    result = setsockopt( interface->multicast_socket, IPPROTO_IPV6,
                         IPV6_JOIN_GROUP, &mreq, sizeof(mreq) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to add multicast membership to interface (%s), "
                  "errno=%s.", interface->interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Deregister Ipv4 Multicast Address
// =============================================
static SmErrorT sm_msg_deregister_ipv4_multicast( 
    SmServiceDomainInterfaceT* interface )
{
    int if_index;
    struct ip_mreqn mreq;
    int result = 0;
    SmIpv4AddressT* ipv4_multicast = &(interface->network_multicast.u.ipv4);
    SmErrorT error;

    error = sm_hw_get_if_index( interface->interface_name, &if_index );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to convert interface name (%s) to interface index, "
                  "error=%s.", interface->interface_name,
                  sm_error_str( error ) );
        return( error );
    }

    memset( &mreq, 0, sizeof(mreq) );

    mreq.imr_multiaddr.s_addr = ipv4_multicast->sin.s_addr;
    mreq.imr_ifindex          = if_index; 

    result = setsockopt( interface->multicast_socket, IPPROTO_IP,
                         IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq) );
    if( 0 > result ) 
    {
        DPRINTFE( "Failed to drop multicast membership from interface (%s), "
                  "errno=%s.", interface->interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Deregister Ipv6 Multicast Address
// =============================================
static SmErrorT sm_msg_deregister_ipv6_multicast(
    SmServiceDomainInterfaceT* interface )
{
    int if_index;
    struct ipv6_mreq mreq;
    int result = 0;
    SmIpv6AddressT* ipv6_multicast = &(interface->network_multicast.u.ipv6);
    SmErrorT error;

    error = sm_hw_get_if_index( interface->interface_name, &if_index );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to convert interface name (%s) to interface index, "
                  "error=%s.", interface->interface_name,
                  sm_error_str( error ) );
        return( error );
    }

    memset( &mreq, 0, sizeof(mreq) );

    mreq.ipv6mr_multiaddr = ipv6_multicast->sin6;
    mreq.ipv6mr_interface = if_index;

    result = setsockopt( interface->multicast_socket, IPPROTO_IPV6,
                         IPV6_LEAVE_GROUP, &mreq, sizeof(mreq) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to drop multicast membership from interface (%s), "
                  "errno=%s.", interface->interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Open Ipv4 UDP Multicast Socket
// ==========================================
static SmErrorT sm_msg_open_ipv4_udp_multicast_socket( 
    SmServiceDomainInterfaceT* interface, int* socket_fd )
{
    int flags;
    int sock;
    int result;
    struct ifreq ifr;
    struct sockaddr_in addr;
    SmIpv4AddressT* ipv4_address = &(interface->network_address.u.ipv4);
    SmIpv4AddressT* ipv4_multicast = &(interface->network_multicast.u.ipv4);

    *socket_fd = -1;

    // Create socket.
    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to open socket for interface (%s), errno=%s.",
                  interface->interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    // Set socket to non-blocking.
    flags = fcntl( sock, F_GETFL, 0 );
    if( 0 > flags )
    {
        DPRINTFE( "Failed to get flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    if( 0 > fcntl( sock, F_SETFL, flags | O_NONBLOCK ) )
    {
        DPRINTFE( "Failed to set flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Close on exec.
    flags = fcntl( sock, F_GETFD, 0 );
    if( 0 > flags )
    {
        DPRINTFE( "Failed to get flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }
    
    if( 0 > fcntl( sock, F_SETFD, flags | FD_CLOEXEC ) )
    {
        DPRINTFE( "Failed to set flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Allow address reuse on socket.
    flags = 1;
    result = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, 
                         (void*) &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set reuseaddr socket option on interface (%s), "
                  "errno=%s.", interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind address to socket.
    memset( &addr, 0, sizeof(addr) );

    addr.sin_family = AF_INET;
    addr.sin_port = htons(interface->network_port);
    addr.sin_addr.s_addr = ipv4_multicast->sin.s_addr;

    result = bind( sock, (struct sockaddr *) &addr, sizeof(addr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind multicast address to socket for "
                  "interface (%s), error=%s.", interface->interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Set multicast interface on socket.
    result = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_IF,
                        (struct in_addr*) &(ipv4_address->sin.s_addr),
                        sizeof(struct in_addr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set unicast address on socket for "
                  "interface (%s), error=%s.", interface->interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind socket to interface.
    memset(&ifr, 0, sizeof(ifr));
    snprintf( ifr.ifr_name, sizeof(ifr.ifr_name), "%s",
              interface->interface_name );
    
    result = setsockopt( sock, SOL_SOCKET, SO_BINDTODEVICE,
                         (void *)&ifr, sizeof(ifr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind socket to interface (%s), errno=%s.",
                  interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set multicast TTL.
    flags = 1;
    result = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_TTL,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set multicast ttl for interface (%s), errno=%s.",
                  interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Disable looping of sent multicast packets on socket.
    flags = 0;
    result = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_LOOP,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to stop looping of multicast messages for "
                  "interface (%s), errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket send priority on interface.
    flags = IPTOS_CLASS_CS6;
    result = setsockopt( sock, IPPROTO_IP, IP_TOS,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "multicast messages, errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    flags = 6;
    result = setsockopt( sock, SOL_SOCKET, SO_PRIORITY,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "multicast messages, errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket receive packet information.
    flags = 1;
    result = setsockopt( sock, SOL_IP, IP_PKTINFO,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket receive packet information for "
                  "interface (%s) multicast messages, errno=%s.",
                  interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    *socket_fd = sock;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Open IPv6 UDP Multicast Socket
// ==========================================
static SmErrorT sm_msg_open_ipv6_udp_multicast_socket(
    SmServiceDomainInterfaceT* interface, int* socket_fd )
{
    int flags;
    int sock;
    int result;
    struct ifreq ifr;
    struct sockaddr_in6 addr;
    SmIpv6AddressT* ipv6_multicast = &(interface->network_multicast.u.ipv6);

    *socket_fd = -1;

    // Create socket.
    sock = socket( AF_INET6, SOCK_DGRAM, IPPROTO_UDP );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to open socket for interface (%s), errno=%s.",
                  interface->interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    // Set socket to non-blocking.
    flags = fcntl( sock, F_GETFL, 0 );
    if( 0 > flags )
    {
        DPRINTFE( "Failed to get flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    if( 0 > fcntl( sock, F_SETFL, flags | O_NONBLOCK ) )
    {
        DPRINTFE( "Failed to set flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Close on exec.
    flags = fcntl( sock, F_GETFD, 0 );
    if( 0 > flags )
    {
        DPRINTFE( "Failed to get flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    if( 0 > fcntl( sock, F_SETFD, flags | FD_CLOEXEC ) )
    {
        DPRINTFE( "Failed to set flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Allow address reuse on socket.
    flags = 1;
    result = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR,
                         (void*) &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set reuseaddr socket option on interface (%s), "
                  "errno=%s.", interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind address to socket.
    memset( &addr, 0, sizeof(addr) );

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(interface->network_port);
    addr.sin6_addr = ipv6_multicast->sin6;

    result = bind( sock, (struct sockaddr *) &addr, sizeof(addr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind multicast address to socket for "
                  "interface (%s), error=%s.", interface->interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Set multicast interface on socket.
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_IF,
                        &(interface->id), sizeof(interface->id) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set unicast address on socket for "
                  "interface (%s), error=%s.", interface->interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind socket to interface.
    memset( &ifr, 0, sizeof(ifr) );
    snprintf( ifr.ifr_name, sizeof(ifr.ifr_name), "%s",
              interface->interface_name );

    result = setsockopt( sock, SOL_SOCKET, SO_BINDTODEVICE,
                         (void* )&ifr, sizeof(ifr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind socket to interface (%s), errno=%s.",
                  interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set multicast TTL.
    flags = 1;
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set multicast ttl for interface (%s), errno=%s.",
                  interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Disable looping of sent multicast packets on socket.
    flags = 0;
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to stop looping of multicast messages for "
                  "interface (%s), errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket send priority on interface.
    flags = IPTOS_CLASS_CS6;
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_TCLASS,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "multicast messages, errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    flags = 6;
    result = setsockopt( sock, SOL_SOCKET, SO_PRIORITY,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "multicast messages, errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket receive packet information.
    flags = 1;
    result = setsockopt( sock, SOL_IPV6, IPV6_RECVPKTINFO,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket receive packet information for "
                  "interface (%s) multicast messages, errno=%s.",
                  interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    *socket_fd = sock;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Open Ipv4 UDP Unicast Socket
// ========================================
static SmErrorT sm_msg_open_ipv4_udp_unicast_socket( 
    SmServiceDomainInterfaceT* interface, int* socket_fd )
{
    int flags;
    int sock;
    int result;
    struct sockaddr_in src_addr;
    SmIpv4AddressT* ipv4_address = &(interface->network_address.u.ipv4);

    *socket_fd = -1;

    // Create socket.
    sock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to open socket for interface (%s), errno=%s.",
                  interface->interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    // Set socket to non-blocking.
    flags = fcntl( sock, F_GETFL, 0 );
    if( 0 > flags )
    {
        DPRINTFE( "Failed to get flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    if( 0 > fcntl( sock, F_SETFL, flags | O_NONBLOCK ) )
    {
        DPRINTFE( "Failed to set flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Close on exec.
    if( 0 > fcntl( sock, F_SETFD, FD_CLOEXEC ) )
    {
        DPRINTFE( "Failed to set fd, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Allow address reuse on socket.
    flags = 1;
    result = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, 
                         (void*) &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set reuseaddr socket option on interface (%s), "
                  "errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind address to socket.
    memset( &src_addr, 0, sizeof(src_addr) );

    src_addr.sin_family = AF_INET;
    src_addr.sin_port = htons(interface->network_port);
    src_addr.sin_addr.s_addr = ipv4_address->sin.s_addr;

    result = bind( sock, (struct sockaddr *) &src_addr, sizeof(src_addr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind unicast address to socket for "
                  "interface (%s), error=%s.", interface->interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind socket to interface.
    result = setsockopt( sock, SOL_SOCKET, SO_BINDTODEVICE,
                         interface->interface_name,
                         strlen(interface->interface_name)+1 );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind socket to interface (%s), "
                  "errno=%s.", interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Disable looping of sent multicast packets on socket.
    flags = 0;
    result = setsockopt( sock, IPPROTO_IP, IP_MULTICAST_LOOP,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to stop looping of unicast messages for "
                  "interface (%s), errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket send priority on interface.
    flags = IPTOS_CLASS_CS6;
    result = setsockopt( sock, IPPROTO_IP, IP_TOS,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "unicast messages, errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    flags = 6;
    result = setsockopt( sock, SOL_SOCKET, SO_PRIORITY,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "unicast messages, errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket receive packet information.
    flags = 1;
    result = setsockopt( sock, SOL_IP, IP_PKTINFO,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket receive packet information for "
                  "interface (%s) unicast messages, errno=%s.",
                  interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    *socket_fd = sock;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Open IPv6 UDP Unicast Socket
// ========================================
static SmErrorT sm_msg_open_ipv6_udp_unicast_socket(
    SmServiceDomainInterfaceT* interface, int* socket_fd )
{
    int flags;
    int sock;
    int result;
    struct sockaddr_in6 src_addr;
    SmIpv6AddressT* ipv6_address = &(interface->network_address.u.ipv6);

    *socket_fd = -1;

    // Create socket.
    sock = socket( AF_INET6, SOCK_DGRAM, IPPROTO_UDP );
    if( 0 > sock )
    {
        DPRINTFE( "Failed to open socket for interface (%s), errno=%s.",
                  interface->interface_name, strerror(errno) );
        return( SM_FAILED );
    }

    // Set socket to non-blocking.
    flags = fcntl( sock, F_GETFL, 0 );
    if( 0 > flags )
    {
        DPRINTFE( "Failed to get flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    if( 0 > fcntl( sock, F_SETFL, flags | O_NONBLOCK ) )
    {
        DPRINTFE( "Failed to set flags, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Close on exec.
    if( 0 > fcntl( sock, F_SETFD, FD_CLOEXEC ) )
    {
        DPRINTFE( "Failed to set fd, error=%s.", strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Allow address reuse on socket.
    flags = 1;
    result = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR,
                         (void*) &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set reuseaddr socket option on interface (%s), "
                  "errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind address to socket.
    memset( &src_addr, 0, sizeof(src_addr) );

    src_addr.sin6_family = AF_INET6;
    src_addr.sin6_port = htons(interface->network_port);
    src_addr.sin6_addr = ipv6_address->sin6;

    result = bind( sock, (struct sockaddr *) &src_addr, sizeof(src_addr) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind unicast address to socket for "
                  "interface (%s), error=%s.", interface->interface_name,
                  strerror( errno ) );
        close( sock );
        return( SM_FAILED );
    }

    // Bind socket to interface.
    result = setsockopt( sock, SOL_SOCKET, SO_BINDTODEVICE,
                         interface->interface_name,
                         strlen(interface->interface_name)+1 );
    if( 0 > result )
    {
        DPRINTFE( "Failed to bind socket to interface (%s), "
                  "errno=%s.", interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Disable looping of sent multicast packets on socket.
    flags = 0;
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to stop looping of unicast messages for "
                  "interface (%s), errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket send priority on interface.
    flags = IPTOS_CLASS_CS6;
    result = setsockopt( sock, IPPROTO_IPV6, IPV6_TCLASS,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "unicast messages, errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    flags = 6;
    result = setsockopt( sock, SOL_SOCKET, SO_PRIORITY,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket send priority for interface (%s) "
                  "unicast messages, errno=%s.", interface->interface_name,
                  strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    // Set socket receive packet information.
    flags = 1;
    result = setsockopt( sock, SOL_IPV6, IPV6_RECVPKTINFO,
                         &flags, sizeof(flags) );
    if( 0 > result )
    {
        DPRINTFE( "Failed to set socket receive packet information for "
                  "interface (%s) unicast messages, errno=%s.",
                  interface->interface_name, strerror(errno) );
        close( sock );
        return( SM_FAILED );
    }

    *socket_fd = sock;

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Open Sockets
// ========================
SmErrorT sm_msg_open_sockets( SmServiceDomainInterfaceT* interface )
{
    SmErrorT error;

    if( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )
    {
        int unicast_socket, multicast_socket;

        error = sm_msg_open_ipv4_udp_unicast_socket( interface, 
                                                     &unicast_socket );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to open unicast socket for interface (%s), "
                      "error=%s.", interface->interface_name,
                      sm_error_str(error) );
            return( error );
        }

        error = sm_selobj_register( unicast_socket, sm_msg_dispatch_ipv4_udp,
                                    (int64_t) false );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to register selection object for "
                      "interface (%s), error=%s.", interface->interface_name,
                      sm_error_str(error) );
            close( unicast_socket );
            return( SM_FAILED );
        }

        if( interface->network_multicast.type != SM_NETWORK_TYPE_NIL )
        {
            error = sm_msg_open_ipv4_udp_multicast_socket( interface,
                                                           &multicast_socket );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to open multicast socket for interface (%s), "
                          "error=%s.", interface->interface_name,
                          sm_error_str(error) );
                close( unicast_socket );
                return( error );
            }

            error = sm_selobj_register( multicast_socket, sm_msg_dispatch_ipv4_udp,
                                        (int64_t) false );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to register selection object for "
                          "interface (%s), error=%s.", interface->interface_name,
                          sm_error_str(error) );
                close( unicast_socket );
                close( multicast_socket );
                return( SM_FAILED );
            }

            interface->unicast_socket = unicast_socket;
            interface->multicast_socket = multicast_socket;

            error = sm_msg_register_ipv4_multicast( interface );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to register multicast address for "
                          "interface (%s), error=%s.", interface->interface_name,
                          sm_error_str(error) );
                close( unicast_socket );
                interface->unicast_socket = -1;
                close( multicast_socket );
                interface->multicast_socket = -1;
                return( SM_FAILED );
            }
        } else
        {
            DPRINTFD( "Multicast not configured in the interface %s", interface->interface_name );
            interface->unicast_socket = unicast_socket;
            interface->multicast_socket = -1;
        }

        return( SM_OKAY );

    } else if( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type ) {
        int unicast_socket, multicast_socket;

        error = sm_msg_open_ipv6_udp_unicast_socket( interface,
                                                     &unicast_socket );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to open unicast socket for interface (%s), "
                      "error=%s.", interface->interface_name,
                      sm_error_str(error) );
            return( error );
        }

        error = sm_selobj_register( unicast_socket, sm_msg_dispatch_ipv6_udp,
                                    (int64_t) false);
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to register selection object for "
                      "interface (%s), error=%s.", interface->interface_name,
                      sm_error_str(error) );
            close( unicast_socket );
            return( SM_FAILED );
        }

        if( interface->network_multicast.type != SM_NETWORK_TYPE_NIL )
        {
            error = sm_msg_open_ipv6_udp_multicast_socket( interface,
                                                           &multicast_socket );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to open multicast socket for interface (%s), "
                          "error=%s.", interface->interface_name,
                          sm_error_str(error) );
                close( unicast_socket );
                return( error );
            }

            error = sm_selobj_register( multicast_socket, sm_msg_dispatch_ipv6_udp,
                                        (int64_t) false );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to register selection object for "
                          "interface (%s), error=%s.", interface->interface_name,
                          sm_error_str(error) );
                close( unicast_socket );
                close( multicast_socket );
                return( SM_FAILED );
            }

            interface->unicast_socket = unicast_socket;
            interface->multicast_socket = multicast_socket;

            error = sm_msg_register_ipv6_multicast( interface );
            if( SM_OKAY != error )
            {
                DPRINTFE( "Failed to register multicast address for "
                          "interface (%s), error=%s.", interface->interface_name,
                          sm_error_str(error) );
                close( unicast_socket );
                interface->unicast_socket = -1;
                close( multicast_socket );
                interface->multicast_socket = -1;
                return( SM_FAILED );
            }
        } else
        {
            interface->unicast_socket = unicast_socket;
            interface->multicast_socket = -1;
        }
        return( SM_OKAY );

    } else {
        DPRINTFE( "Unsupported network type (%s).",
                  sm_network_type_str( interface->network_type ) );
        return( SM_FAILED );
    }
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Close Sockets
// =========================
SmErrorT sm_msg_close_sockets( SmServiceDomainInterfaceT* interface )
{
    SmErrorT error;

    if( -1 < interface->unicast_socket )
    {
        error = sm_selobj_deregister( interface->unicast_socket );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object for "
                      "interface (%s), error=%s.", interface->interface_name,
                      sm_error_str(error) );
        }

        close( interface->unicast_socket );
        interface->unicast_socket = -1;
    }

    if( -1 < interface->multicast_socket )
    {
        error = sm_selobj_deregister( interface->multicast_socket );
        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister selection object for "
                      "interface (%s), error=%s.", interface->interface_name,
                      sm_error_str(error) );
        }

        if(( SM_NETWORK_TYPE_IPV4_UDP == interface->network_type )||
           ( SM_NETWORK_TYPE_IPV4 == interface->network_type ))
        {
            error = sm_msg_deregister_ipv4_multicast( interface );

        } else if(( SM_NETWORK_TYPE_IPV6_UDP == interface->network_type )||
                  ( SM_NETWORK_TYPE_IPV6 == interface->network_type )) 
        {
            error = sm_msg_deregister_ipv6_multicast( interface );
        }

        if( SM_OKAY != error )
        {
            DPRINTFE( "Failed to deregister multicast address for "
                      "interface (%s), error=%s.", interface->interface_name,
                      sm_error_str(error) );
        }

        close( interface->multicast_socket );
        interface->multicast_socket = -1;
    }

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Dump Data
// =====================
void sm_msg_dump_data( FILE* log )
{
    fprintf( log, "--------------------------------------------------------------------\n" );
    fprintf( log, "MESSAGING DATA\n" );
    fprintf( log, "  rcvd_total_msgs..............................%" PRIu64 "\n", _rcvd_total_msgs );
    fprintf( log, "  rcvd_msgs_while_disabled.....................%" PRIu64 "\n", _rcvd_msgs_while_disabled );
    fprintf( log, "  rcvd_bad_msg_auth............................%" PRIu64 "\n", _rcvd_bad_msg_auth );
    fprintf( log, "  rcvd_bad_msg_version.........................%" PRIu64 "\n", _rcvd_bad_msg_version );
    fprintf( log, "  send_node_hello_count........................%" PRIu64 "\n", _send_node_hello_cnt );
    fprintf( log, "  rcvd_node_hello_count........................%" PRIu64 "\n", _rcvd_node_hello_cnt );
    fprintf( log, "  send_node_update_count.......................%" PRIu64 "\n", _send_node_update_cnt );
    fprintf( log, "  rcvd_node_update_count.......................%" PRIu64 "\n", _rcvd_node_update_cnt );
    fprintf( log, "  send_node_swact_count........................%" PRIu64 "\n", _send_node_swact_cnt );
    fprintf( log, "  rcvd_node_swact_count........................%" PRIu64 "\n", _rcvd_node_swact_cnt );
    fprintf( log, "  send_node_swact_ack_count....................%" PRIu64 "\n", _send_node_swact_ack_cnt );
    fprintf( log, "  rcvd_node_swact_ack_count....................%" PRIu64 "\n", _rcvd_node_swact_ack_cnt );
    fprintf( log, "  send_service_domain_hello_count..............%" PRIu64 "\n", _send_service_domain_hello_cnt );
    fprintf( log, "  rcvd_service_domain_hello_count..............%" PRIu64 "\n", _rcvd_service_domain_hello_cnt );
    fprintf( log, "  send_service_domain_pause_count..............%" PRIu64 "\n", _send_service_domain_pause_cnt );
    fprintf( log, "  rcvd_service_domain_pause_count..............%" PRIu64 "\n", _rcvd_service_domain_pause_cnt );
    fprintf( log, "  send_service_domain_exchange_start_count.....%" PRIu64 "\n", _send_service_domain_exchange_start_cnt );
    fprintf( log, "  rcvd_service_domain_exchange_start_count.....%" PRIu64 "\n", _rcvd_service_domain_exchange_start_cnt );
    fprintf( log, "  send_service_domain_exchange_count...........%" PRIu64 "\n", _send_service_domain_exchange_cnt );
    fprintf( log, "  rcvd_service_domain_exchange_count...........%" PRIu64 "\n", _rcvd_service_domain_exchange_cnt );
    fprintf( log, "  send_service_domain_member_request_count.....%" PRIu64 "\n", _send_service_domain_member_request_cnt );
    fprintf( log, "  rcvd_service_domain_member_request_count.....%" PRIu64 "\n", _rcvd_service_domain_member_request_cnt );
    fprintf( log, "  send_service_domain_member_update_count......%" PRIu64 "\n", _send_service_domain_member_update_cnt );
    fprintf( log, "  rcvd_service_domain_member_update_count......%" PRIu64 "\n", _rcvd_service_domain_member_update_cnt );
    fprintf( log, "--------------------------------------------------------------------\n" );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Initialize
// ======================
SmErrorT sm_msg_initialize( void )
{
    SmErrorT error;

    _hostname[0] = '\0';

    error = sm_node_utils_get_hostname( _hostname );
    if( SM_OKAY != error )
    {
        DPRINTFE( "Failed to get local hostname." );
        return( SM_FAILED );
    }

    _messaging_enabled = false;
    sm_uuid_create( _msg_instance );
    DPRINTFI( "Message instance (%s) created.", _msg_instance );


    DPRINTFV( "Hello message size is %i.", sizeof(SmMsgNodeHelloT) );
    DPRINTFV( "Node update message size is %i.", sizeof(SmMsgNodeUpdateT) );
    DPRINTFV( "Swact message size is %i.", sizeof(SmMsgNodeSwactT) );
    DPRINTFV( "Swact ack message size is %i.", sizeof(SmMsgNodeSwactAckT) );
    DPRINTFV( "Service Domain Hello message size is %i.",
              sizeof(SmMsgServiceDomainHelloT) );
    DPRINTFV( "Service Domain Pause message size is %i.",
              sizeof(SmMsgServiceDomainPauseT) );
    DPRINTFV( "Service Domain Exchange Start message size is %i.",
              sizeof(SmMsgServiceDomainExchangeStartT) );
    DPRINTFV( "Service Domain Exchange message size is %i.",
              sizeof(SmMsgServiceDomainExchangeT) );
    DPRINTFV( "Service Domain Member Request message size is %i.",
              sizeof(SmMsgServiceDomainMemberRequestT) );
    DPRINTFV( "Service Domain Member Update message size is %i.", 
              sizeof(SmMsgServiceDomainMemberUpdateT) );

    return( SM_OKAY );
}
// ****************************************************************************

// ****************************************************************************
// Messaging - Finalize
// ====================
SmErrorT sm_msg_finalize( void )
{
    _hostname[0] = '\0';
    _messaging_enabled = false;
    memset( _msg_instance, 0, sizeof(_msg_instance) );

    return( SM_OKAY );
}
// ****************************************************************************
