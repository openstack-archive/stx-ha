#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
import os
import sys
import argparse
import sqlite3

database_name = "/var/run/sm/sm.db"


def main():

    if not os.path.exists(database_name):
        print("%s not available." % database_name)
        sys.exit(0)

    try:
        parser = argparse.ArgumentParser(description='SM Query')
        subparsers = parser.add_subparsers(help='types')

        s_parser = subparsers.add_parser('service', help='Query Service')
        s_parser.set_defaults(which='service')
        s_parser.add_argument('service_name', help='service name')
        s_parser = subparsers.add_parser('service-group', help='Query Service Group')
        s_parser.set_defaults(which='service-group')
        s_parser.add_argument('service_group_name', nargs='+', help='service group name')
        s_parser.add_argument("--desired-state", help="display desired state", action="store_true")

        args = parser.parse_args()

        if args.which == 'service':
            database = sqlite3.connect(database_name)

            cursor = database.cursor()

            cursor.execute("SELECT NAME, DESIRED_STATE, STATE, STATUS FROM "
                           "SERVICES WHERE NAME = '%s';"
                           % args.service_name)

            row = cursor.fetchone()

            if row is None:
                print("%s is disabled." % args.service_name)

            else:
                service_name = row[0]
                state = row[2]
                status = row[3]

                if status == 'none':
                    print("%s is %s" % (service_name, state))
                else:
                    print("%s is %s-%s" % (service_name, state, status))

            database.close()

        elif args.which == 'service-group':
            database = sqlite3.connect(database_name)

            cursor = database.cursor()

            cursor.execute("SELECT NAME, DESIRED_STATE, STATE FROM "
                           "SERVICE_GROUPS WHERE NAME IN (%s) AND PROVISIONED='yes';"
                           % ','.join("'%s'"%i for i in args.service_group_name))

            rows = cursor.fetchall()

            if args.desired_state:
                fmt = "{0} {2} {1}"
            else:
                fmt = "{0} {1}"

            found_list = []
            for row in rows:
                service_group_name = row[0]
                found_list.append(service_group_name)
                desired_state = row[1]
                state = row[2]

                print(fmt.format(service_group_name, state, desired_state))

            database.close()

            not_found_list = []
            for g in args.service_group_name:
                if g not in found_list:
                    not_found_list.append(g)

            if len(not_found_list) > 1:
                print("%s are not provisioned"%','.join( (g for g in not_found_list)))
            elif len(not_found_list) == 1:
                print("%s is not provisioned" % ','.join((g for g in not_found_list)))

    except KeyboardInterrupt:
        sys.exit()

    except Exception as e:
        print(e)
        sys.exit(-1)
