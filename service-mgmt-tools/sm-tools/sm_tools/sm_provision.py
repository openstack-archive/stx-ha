#
# Copyright (c) 2015 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
import os
import sys
import argparse
import sqlite3

database_name = "/var/lib/sm/sm.db"


def main():
    file_name = os.path.basename(sys.argv[0])
    if "sm-provision" == file_name:
        provision_str = "yes"
    else:
        provision_str = "no"

    try:
        parser = argparse.ArgumentParser(description='SM Provision ')
        subparsers = parser.add_subparsers(help='types')

        # Domain
        sd = subparsers.add_parser('service-domain',
                                   help='Provision Service Domain')
        sd.set_defaults(which='service_domain')
        sd.add_argument('service_domain', help='service domain name')

        # Domain Member
        sd_member_parser = subparsers.add_parser('service-domain-member',
                                                 help='Provision Service '
                                                 'Domain Member')
        sd_member_parser.set_defaults(which='service_domain_member')
        sd_member_parser.add_argument('service_domain',
                                      help='service domain name')
        sd_member_parser.add_argument('service_group',
                                      help='service group name')

        # Domain Interface
        sd_member_parser = subparsers.add_parser('service-domain-interface',
                                                 help='Provision Service '
                                                 'Domain Interface')
        sd_member_parser.set_defaults(which='service_domain_interface')
        sd_member_parser.add_argument('service_domain',
                                      help='service domain name')
        sd_member_parser.add_argument('service_domain_interface',
                                      help='service domain interface name')

        # Service-Group
        sg = subparsers.add_parser('service-group',
                                   help='Provision Service Group')
        sg.set_defaults(which='service_group')
        sg.add_argument('service_group', help='service group name')

        # Service-Group-Member
        sg_member_parser = subparsers.add_parser('service-group-member',
                                                 help='Provision Service Group '
                                                 'Member')
        sg_member_parser.set_defaults(which='service_group_member')
        sg_member_parser.add_argument('service_group',
                                      help='service group name')
        sg_member_parser.add_argument('service_group_member',
                                      help='service group member name')

        # Service
        service_parser = subparsers.add_parser('service',
                                               help='Provision Service')
        service_parser.set_defaults(which='service')
        service_parser.add_argument('service', help='service name')

        args = parser.parse_args()

        if args.which == 'service_domain':
            database = sqlite3.connect(database_name)

            cursor = database.cursor()

            cursor.execute("UPDATE SERVICE_DOMAINS SET "
                           "PROVISIONED = '%s' WHERE NAME = '%s';"
                           % (provision_str, args.service_domain))

            database.commit()
            database.close()

        elif args.which == 'service_domain_member':
            database = sqlite3.connect(database_name)

            cursor = database.cursor()

            cursor.execute("UPDATE SERVICE_DOMAIN_MEMBERS SET "
                           "PROVISIONED = '%s' WHERE NAME = '%s' and "
                           "SERVICE_GROUP_NAME = '%s';"
                           % (provision_str, args.service_domain,
                              args.service_group))

            database.commit()
            database.close()

        elif args.which == 'service_domain_interface':
            database = sqlite3.connect(database_name)

            cursor = database.cursor()

            cursor.execute("UPDATE SERVICE_DOMAIN_INTERFACES SET "
                           "PROVISIONED = '%s' WHERE SERVICE_DOMAIN = '%s' and "
                           "SERVICE_DOMAIN_INTERFACE = '%s';"
                           % (provision_str, args.service_domain,
                              args.service_domain_interface))

            database.commit()
            database.close()

        elif args.which == 'service_group':
            database = sqlite3.connect(database_name)

            cursor = database.cursor()

            cursor.execute("UPDATE SERVICE_GROUPS SET "
                           "PROVISIONED = '%s' WHERE NAME = '%s';"
                           % (provision_str, args.service_group))

            database.commit()
            database.close()

        elif args.which == 'service_group_member':
            database = sqlite3.connect(database_name)

            cursor = database.cursor()

            cursor.execute("UPDATE SERVICE_GROUP_MEMBERS SET "
                           "PROVISIONED = '%s' WHERE NAME = '%s' and "
                           "SERVICE_NAME = '%s';"
                           % (provision_str, args.service_group,
                              args.service_group_member))

            database.commit()
            database.close()

        elif args.which == 'service':
            database = sqlite3.connect(database_name)

            cursor = database.cursor()

            cursor.execute("UPDATE SERVICES SET "
                           "PROVISIONED = '%s' WHERE NAME = '%s';"
                           % (provision_str, args.service))

            database.commit()
            database.close()

        sys.exit(0)

    except KeyboardInterrupt:
        sys.exit()

    except Exception as e:
        print (e)
        sys.exit(-1)
