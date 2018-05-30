#
# Copyright (c) 2016 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
import os
import sys
import argparse
import sqlite3

from sm_api_msg_utils import restart_service as restart_service
from sm_api_msg_utils import restart_service_safe as restart_service_safe
from sm_api_msg_utils import database_running_name as database_name


def main():
    filename = os.path.basename(sys.argv[0])
    if "sm-manage" == filename:
        action = "manage"
    elif "sm-unmanage" == filename:
        action = "unmanage"
    elif "sm-restart-safe" == filename:
        action = "restart-safe"
    else:
        action = "restart"

    try:
        parser = argparse.ArgumentParser(description='SM Action ')
        subparsers = parser.add_subparsers(help='types')

        # Service
        service_parser = subparsers.add_parser('service', help='service action')
        service_parser.set_defaults(which='service')
        service_parser.add_argument('service', help='service name')

        args = parser.parse_args()

        if args.which == 'service':
            database = sqlite3.connect(database_name)

            cursor = database.cursor()

            cursor.execute("SELECT * FROM SERVICES WHERE NAME = '%s';"
                           % args.service)
            row = cursor.fetchone()
            if row is None:
                print "Given service (%s) does not exist." % args.service
                sys.exit()

            database.close()

            SM_VAR_RUN_SERVICES_DIR = '/var/run/sm/services'
            unmanage_filepath = SM_VAR_RUN_SERVICES_DIR + '/'            
            unmanage_filename = "%s.unmanaged" % args.service

            if 'manage' == action:
                if os.path.exists(SM_VAR_RUN_SERVICES_DIR):
                    if os.path.isfile(unmanage_filepath + unmanage_filename):
                        os.remove(unmanage_filepath + unmanage_filename)

                print "Service (%s) is now being managed." % args.service

            elif 'unmanage' == action:
                if not os.path.exists(SM_VAR_RUN_SERVICES_DIR):
                    os.makedirs(SM_VAR_RUN_SERVICES_DIR)

                if not os.path.isfile(unmanage_filepath + unmanage_filename):
                    open(unmanage_filepath + unmanage_filename, 'w').close()

                print "Service (%s) is no longer being managed." % args.service

            elif 'restart-safe' == action:
                restart_service_safe(args.service)
                print "Service (%s) is restarting." % args.service

            else:
                restart_service(args.service)

                print "Service (%s) is restarting." % args.service

        sys.exit(0)

    except KeyboardInterrupt:
        sys.exit()

    except Exception as e:
        print e
        sys.exit(-1)