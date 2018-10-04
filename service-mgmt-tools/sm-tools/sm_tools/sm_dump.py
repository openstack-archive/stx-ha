#
# Copyright (c) 2014-2015 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
import os
import sys
import argparse
import sqlite3
import psutil
import ntpath
import os.path

database_name = "/var/run/sm/sm.db"


def get_pid(pid_file):
    if os.path.isfile(pid_file):
        with open(pid_file, "r") as f:
            pid = f.readline().strip('\n ')
            try:
                pid = int(pid)
            except:
                pid = -1
            return pid
    return -1


def get_process_name(pid):
    if pid < 0:
        return ''

    try:
        p = psutil.Process(pid)
        name = p.name()
        if name == 'python':
            cmd_line = p.cmdline()
            if len(cmd_line) > 1:
                name = ntpath.basename(cmd_line[1])

        return name
    except:
        # most likely it is a leftover pid
        return ''


def main():
    try:
        parser = argparse.ArgumentParser(description='SM Dump')
        parser.add_argument("--verbose", help="increase dump output",
                            action="store_true")
        parser.add_argument("--pid", help="print pid",
                            action="store_true")
        parser.add_argument("--pn", help="print process name",
                            action="store_true")
        parser.add_argument("--pid_file", help="print pid file name",
                            action="store_true")
        parser.add_argument("--impact", help="severity of service failure",
                            action="store_true")
        args = parser.parse_args()

        if not os.path.exists(database_name):
            print("%s not available." % database_name)
            sys.exit(0)

        database = sqlite3.connect(database_name)

        cursor = database.cursor()

        if args.verbose:
            # Service-Groups Dump
            print("\n-Service_Groups%s" % ('-' * 92))

            cursor.execute("SELECT name, desired_state, state, status, "
                           "condition from service_groups WHERE "
                           "PROVISIONED = 'yes';")

            data = cursor.fetchall()

            if data is not None:
                for row in data:
                    print("%-32s %-20s %-20s %-10s %-20s" % (row[0], row[1],
                                                             row[2], row[3],
                                                             row[4]))

            print("%s" % ('-' * 107))

            # Services Dump
            len = 98
            if args.impact:
                len += 10
            if args.pid:
                len += 9
            if args.pn:
                len += 22
            if args.pid_file:
                len += 28

            print("\n-Services%s" % ('-' * len))

            cursor.execute("SELECT s.name, s.desired_state, s.state, "
                           "s.status, s.condition, s.pid_file, "
                           "g.SERVICE_FAILURE_IMPACT "
                           "from services s, service_group_members g "
                           "WHERE s.PROVISIONED = 'yes' and "
                           "s.name = g.service_name;")

            data = cursor.fetchall()

            if data is not None:
                for row in data:
                    pid_file = row[5]
                    pid = get_pid(pid_file)
                    pn = get_process_name(pid)
                    msg = "%-32s %-20s %-20s " % (row[0], row[1],row[2])
                    if args.impact:
                        msg += "%-10s" % (row[6])
                    if args.pid:
                        msg += "%-7s" % (pid if pid > 0 else '')
                    if args.pn:
                        msg += "%-20s" % (pn)
                    if args.pid_file:
                        msg += "%-25s" % (pid_file)
                    msg += "%-10s %20s" % (row[3], row[4])
                    print msg

            print("%s" % ('-' * len))

        else:
            # Service-Groups Dump
            print("\n-Service_Groups%s" % ('-' * 72))

            cursor.execute("SELECT name, desired_state, state, status "
                           "from service_groups WHERE PROVISIONED = 'yes';")

            data = cursor.fetchall()

            if data is not None:
                for row in data:
                    print("%-32s %-20s %-20s %-10s" % (row[0], row[1], row[2],
                                                       row[3]))

            print("%s" % ('-' * 87))

            len = 78
            if args.impact:
                len += 10
            if args.pid:
                len += 9
            if args.pn:
                len += 22
            if args.pid_file:
                len += 28

            # Services Dump
            print("\n-Services%s" % ('-' * len))

            cursor.execute("SELECT s.name, s.desired_state, s.state, s.status, "
                           "s.pid_file, g.SERVICE_FAILURE_IMPACT "
                           "from services s, service_group_members g "
                           "where s.provisioned = 'yes' and "
                           "s.name = g.service_name;")

            data = cursor.fetchall()

            if data is not None:
                for row in data:
                    pid_file = row[4]
                    pid = get_pid(pid_file)
                    pn = get_process_name(pid)
                    msg = "%-32s %-20s %-20s " % (row[0], row[1],row[2])
                    if args.impact:
                        msg += "%-10s" % (row[5])
                    if args.pid:
                        msg += "%-7s" % (pid if pid > 0 else '')
                    if args.pn:
                        msg += "%-20s" % (pn)
                    if args.pid_file:
                        msg += "%-25s" % (pid_file)
                    msg += "%-10s " % (row[3])
                    print(msg)

            print("%s" % ('-' * len))

        database.close()

    except KeyboardInterrupt:
        sys.exit()

    except Exception as e:
        print(e)
        sys.exit(-1)

    try:
        sys.stdout.close()
    except:
        pass
    try:
        sys.stderr.close()
    except:
        pass
