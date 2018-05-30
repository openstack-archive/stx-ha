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
        print "%s not available." % database_name
        sys.exit(0)

    try:
        parser = argparse.ArgumentParser(description='SM Query')
        subparsers = parser.add_subparsers(help='types')

        s_parser = subparsers.add_parser('service', help='Query Service')
        s_parser.set_defaults(which='service')
        s_parser.add_argument('service_name', help='service name')

        args = parser.parse_args()

        if args.which == 'service':
            database = sqlite3.connect(database_name)

            cursor = database.cursor()

            cursor.execute("SELECT NAME, DESIRED_STATE, STATE, STATUS FROM "
                           "SERVICES WHERE NAME = '%s';"
                           % args.service_name)

            row = cursor.fetchone()

            if row is None:
                print "%s is disabled." % args.service_name

            else:
                service_name = row[0]
                state = row[2]
                status = row[3]

                if status == 'none':
                    print "%s is %s" % (service_name, state)
                else:
                    print "%s is %s-%s" % (service_name, state, status)

            database.close()

    except KeyboardInterrupt:
        sys.exit()

    except Exception as e:
        print e
        sys.exit(-1)
