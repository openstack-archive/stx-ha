#
# Copyright (c) 2016 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

import time
import socket

database_name = "/var/lib/sm/sm.db"
database_running_name = "/var/run/sm/sm.db"

SM_API_SERVER_ADDR = "/tmp/.sm_server_api"

SM_API_MSG_VERSION = "1"
SM_API_MSG_REVISION = "1"

SM_API_MSG_TYPE_RESTART_SERVICE = "RESTART_SERVICE"
SM_API_MSG_SKIP_DEP_CHECK = "skip-dep"

SM_API_MSG_TYPE_RELOAD_DATA = "RELOAD_DATA"

SM_API_MSG_TYPE_SDI_SET_STATE = "SERVICE_DOMAIN_INTERFACE_SET_STATE"

# offsets
SM_API_MSG_VERSION_FIELD = 0
SM_API_MSG_REVISION_FIELD = 1
SM_API_MSG_SEQNO_FIELD = 2
SM_API_MSG_TYPE_FIELD = 3
SM_API_MSG_ORIGIN_FIELD = 4
SM_API_MSG_SERVICE_NAME_FIELD = 5
SM_API_MSG_PARAM = 6


def _send_msg_to_sm(sm_api_msg):
    s = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    try:
        s.setblocking(True)
        s.sendto(sm_api_msg, SM_API_SERVER_ADDR)
        time.sleep(1)

    except socket.error, e:
        print ("sm-api socket error: %s on  %s" % (e, sm_api_msg))


def restart_service(service_name):
    """
    Message SM to restart a service
    """
    sm_api_msg = ("%s,%s,%i,%s,%s,%s"
                  % (SM_API_MSG_VERSION, SM_API_MSG_REVISION, 1,
                     SM_API_MSG_TYPE_RESTART_SERVICE, "sm-action",
                     service_name))

    _send_msg_to_sm(sm_api_msg)


def restart_service_safe(service_name):
    """
    Message SM to restart a service w/o checking dependency
    """
    sm_api_msg = ("%s,%s,%i,%s,%s,%s,%s"
                  % (SM_API_MSG_VERSION, SM_API_MSG_REVISION, 1,
                     SM_API_MSG_TYPE_RESTART_SERVICE, "sm-action",
                     service_name, SM_API_MSG_SKIP_DEP_CHECK))

    _send_msg_to_sm(sm_api_msg)

