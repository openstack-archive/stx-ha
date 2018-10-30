#
# Copyright (c) 2014-2015 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
import sys
import argparse
import sqlite3
from netaddr import IPNetwork
from sm_api_msg_utils import database_name as database_name

cpe_duplex = "duplex"
cpe_duplex_direct = "duplex-direct"
mgmt_if = 'management-interface'
infra_if = 'infrastructure-interface'
tor_connect = 'tor'
dc_connect = 'dc'
database_name = "/var/lib/sm/sm.db"


def main():
    try:
        parser = argparse.ArgumentParser(description='SM Configuration')
        subparsers = parser.add_subparsers(help='types')

        if_parser = subparsers.add_parser('interface',
                                          help='Interface Configuration')
        if_parser.set_defaults(which='interface')
        if_parser.add_argument('service_domain', help='service domain name')
        if_parser.add_argument('service_domain_interface',
                               help='service domain interface name')
        if_parser.add_argument('network_multicast', help='network multicast')
        if_parser.add_argument('network_address', help='network address')
        if_parser.add_argument('network_port', help='network port')
        if_parser.add_argument('network_heartbeat_port',
                               help='network heartbeat port')
        if_parser.add_argument('network_peer_address',
                               help='network peer address')
        if_parser.add_argument('network_peer_port', help='network peer port')
        if_parser.add_argument('network_peer_heartbeat_port',
                               help='network peer heartbeat port')

        si_parser = subparsers.add_parser('service_instance',
                                          help='Service Instance '
                                          'Configuration')
        si_parser.set_defaults(which='service_instance')
        si_parser.add_argument('service', help='service name')
        si_parser.add_argument('instance', help='instance name')
        si_parser.add_argument('parameters', help='instance parameters')

        sys_parser = subparsers.add_parser('system',
                                          help='system Configuration')
        sys_parser.set_defaults(which='system')
        sys_parser.add_argument(
            "--cpe_mode", choices=[cpe_duplex, cpe_duplex_direct],
            help='cpe mode, available selections: %s, %s' % (
                cpe_duplex, cpe_duplex_direct)
        )

        sys_parser.add_argument(
            "--sm_server_port",
            help='port sm receives '
                 'hbs agent cluster information update'
        )

        sys_parser.add_argument(
            "--sm_client_port",
            help='port mtce receives sm commands from'
        )

        sg_parser = subparsers.add_parser('service_group',
                                          help='Service Group '
                                          'Configuration')
        sg_parser.set_defaults(which='service_group')
        sg_parser.add_argument('provisioned', help='provisioned')
        sg_parser.add_argument('service_domain', help='service domain name')
        sg_parser.add_argument('service_group', help='service group name')
        sg_parser.add_argument('redundancy', help='redundancy mode')
        sg_parser.add_argument('active', help='number of active unit')
        sg_parser.add_argument('standby', help='number of standby unit')
        sg_parser.add_argument('aggregate', help='service group aggregate')
        sg_parser.add_argument('active_only', help='active only if active')

        args = parser.parse_args()

        if args.which == "system":
            if cpe_duplex == args.cpe_mode:
                configure_cpe_duplex()
            elif cpe_duplex_direct == args.cpe_mode:
                configure_cpe_dc()

            if args.sm_server_port:
                configure_system_opt("sm_server_port", args.sm_server_port)

            if args.sm_client_port:
                configure_system_opt("sm_client_port", args.sm_client_port)
        else:
            database = sqlite3.connect(database_name)
            _dispatch_config_action(args, database)
            database.close()

        sys.exit(0)

    except KeyboardInterrupt:
        sys.exit()

    except Exception as e:
        print(e)
        sys.exit(-1)


def _dispatch_config_action(args, database):
    if args.which == 'interface':

        cursor = database.cursor()

        cursor.execute("SELECT INTERFACE_NAME FROM "
                       "SERVICE_DOMAIN_INTERFACES WHERE "
                       "SERVICE_DOMAIN = '%s' and "
                       "SERVICE_DOMAIN_INTERFACE = '%s';"
                       % (args.service_domain,
                          args.service_domain_interface))

        data = cursor.fetchone()

        if IPNetwork(args.network_address).version == 6:
            network_type = "ipv6-udp"
        else:
            network_type = "ipv4-udp"

        if data is not None:
            cursor.execute("UPDATE SERVICE_DOMAIN_INTERFACES SET "
                           "NETWORK_TYPE = '%s', "
                           "NETWORK_MULTICAST = '%s', "
                           "NETWORK_ADDRESS = '%s', "
                           "NETWORK_PORT = '%s', "
                           "NETWORK_HEARTBEAT_PORT = '%s', "
                           "NETWORK_PEER_ADDRESS = '%s', "
                           "NETWORK_PEER_PORT = '%s', "
                           "NETWORK_PEER_HEARTBEAT_PORT = '%s' "
                           "WHERE SERVICE_DOMAIN = '%s' and "
                           "SERVICE_DOMAIN_INTERFACE = '%s';"
                           % (network_type,
                              args.network_multicast,
                              args.network_address,
                              args.network_port,
                              args.network_heartbeat_port,
                              args.network_peer_address,
                              args.network_peer_port,
                              args.network_peer_heartbeat_port,
                              args.service_domain,
                              args.service_domain_interface))
            database.commit()

    if args.which == 'service_instance':

        cursor = database.cursor()

        cursor.execute("SELECT SERVICE_NAME FROM SERVICE_INSTANCES "
                       "WHERE SERVICE_NAME = '%s';" % args.service)

        data = cursor.fetchone()

        if data is None:
            cursor.execute("INSERT INTO SERVICE_INSTANCES "
                           "VALUES( NULL, '%s', '%s', '%s' );"
                           % (args.service, args.instance,
                              args.parameters))
        else:
            cursor.execute("UPDATE SERVICE_INSTANCES SET "
                           "INSTANCE_NAME = '%s', "
                           "INSTANCE_PARAMETERS = '%s' "
                           "WHERE SERVICE_NAME = '%s';"
                           % (args.instance, args.parameters,
                              args.service))

        database.commit()

    if args.which == 'service_group':

        cursor = database.cursor()

        cursor.execute("SELECT SERVICE_GROUP_NAME FROM SERVICE_DOMAIN_MEMBERS "
                       "WHERE NAME = '%s' and "
                       "SERVICE_GROUP_NAME = '%s';"
                       % (args.service_domain,
                          args.service_group))

        data = cursor.fetchone()

        if data is None:
            cursor.execute("INSERT INTO SERVICE_DOMAIN_MEMBERS "
                           "VALUES( NULL, '%s', '%s', '%s', '%s',"
                           "'%s', '%s', '%s', '%s');"
                           % (args.provisioned,
                              args.service_domain,
                              args.service_group,
                              args.redundancy,
                              args.active,
                              args.standby,
                              args.aggregate,
                              args.active_only))
        else:
            cursor.execute("UPDATE SERVICE_DOMAIN_MEMBERS SET "
                           "REDUNDANCY_MODEL = '%s', "
                           "N_ACTIVE = '%s', "
                           "M_STANDBY = '%s', "
                           "SERVICE_GROUP_AGGREGATE = '%s' "
                           "WHERE SERVICE_GROUP_NAME = '%s';"
                           % (args.redundancy,
                              args.active,
                              args.standby,
                              args.aggregate,
                              args.service_group
                              ))

        database.commit()


def configure_cpe_duplex():
    configure_if_connect_type(mgmt_if, tor_connect)
    configure_if_connect_type(infra_if, tor_connect)


def configure_cpe_dc():
    configure_if_connect_type(mgmt_if, dc_connect)
    configure_if_connect_type(infra_if, dc_connect)


def configure_if_connect_type(if_name, connect_type):
    database = sqlite3.connect(database_name)

    cursor = database.cursor()
    sql = "UPDATE SERVICE_DOMAIN_INTERFACES SET INTERFACE_CONNECT_TYPE='%s' " \
          "WHERE SERVICE_DOMAIN_INTERFACE = '%s'" % (connect_type, if_name)

    cursor.execute(sql)
    database.commit()
    database.close()


def configure_system_opt(key, value):
    database = sqlite3.connect(database_name)

    cursor = database.cursor()
    sql = "UPDATE CONFIGURATION SET VALUE='%s' " \
          "WHERE KEY = '%s'" % (value, key)

    cursor.execute(sql)
    database.commit()
    database.close()
