#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
import os
import sys
import argparse
import sqlite3

database_patches_dir = "/var/lib/sm/patches/"
database_master_name = "/var/lib/sm/sm.db"
database_running_name = "/var/run/sm/sm.db"


def main():

    try:
        parser = argparse.ArgumentParser(description='SM Patch')
        subparsers = parser.add_subparsers(help='types')
        db_parser = subparsers.add_parser('database', help='Database')
        db_parser.set_defaults(which='database')
        db_parser.add_argument('which_database', help='master or running')
        db_parser.add_argument('patch_name', help='patch name')

        args = parser.parse_args()

        if args.which == 'database':
            if args.which_database == 'master':
                if not os.path.exists(database_master_name):
                    print ("%s not available." % database_master_name)
                    sys.exit()

                database = sqlite3.connect(database_master_name)
            else:
                if not os.path.exists(database_running_name):
                    print ("%s not available." % database_running_name)
                    sys.exit()

                database = sqlite3.connect(database_running_name)

            cursor = database.cursor()

            with open(database_patches_dir + args.patch_name) as patch_file:
                for line in patch_file:
                    if not line.startswith('#'):
                        cursor.execute(line)

            database.commit()
            database.close()

        sys.exit(0)

    except KeyboardInterrupt:
        sys.exit()

    except Exception as e:
        print (e)
        sys.exit(-1)
