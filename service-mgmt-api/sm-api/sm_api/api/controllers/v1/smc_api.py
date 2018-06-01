#
# Copyright (c) 2014-2018 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

import os
import socket

from sm_api.openstack.common import log


LOG = log.getLogger(__name__)

SM_API_SERVER_ADDR = "/tmp/.sm_server_api"
SM_API_CLIENT_ADDR = "/tmp/.sm_client_api"

SM_API_MSG_VERSION = "1"
SM_API_MSG_REVISION = "1"
SM_API_MSG_TYPE_SET_NODE = "SET_NODE"
SM_API_MSG_TYPE_SET_NODE_ACK = "SET_NODE_ACK"

SM_API_MSG_NODE_ADMINSTATE_LOCK = "LOCK"
SM_API_MSG_NODE_ADMINSTATE_UNLOCK = "UNLOCK"

# offsets
SM_API_MSG_VERSION_FIELD = 0
SM_API_MSG_REVISION_FIELD = 1
SM_API_MSG_SEQNO_FIELD = 2
SM_API_MSG_TYPE_FIELD = 3
SM_API_MSG_ORIGIN_FIELD = 4
SM_API_MSG_NODE_NAME_FIELD = 5
SM_API_MSG_NODE_ACTION_FIELD = 6
SM_API_MSG_NODE_ADMIN_FIELD = 7
SM_API_MSG_NODE_OPER_FIELD = 8
SM_API_MSG_NODE_AVAIL_FIELD = 9
SM_API_MAX_MSG_SIZE = 2048

SM_NODE_ACTION_UNLOCK = "unlock"
SM_NODE_ACTION_LOCK = "lock"
SM_NODE_ACTION_LOCK_FORCE = "lock-force"
SM_NODE_ACTION_LOCK_PRE_CHECK = "lock-pre-check"
SM_NODE_ACTION_SWACT_PRE_CHECK = "swact-pre-check"
SM_NODE_ACTION_SWACT = "swact"
SM_NODE_ACTION_SWACT_FORCE = "swact-force"
SM_NODE_ACTION_EVENT = "event"


def sm_api_notify(sm_dict):

    sm_ack_dict = {}

    sm_buf_dict = {'SM_API_MSG_VERSION': SM_API_MSG_VERSION,
                   'SM_API_MSG_REVISION': SM_API_MSG_REVISION}

    sm_buf_dict.update(sm_dict)
    sm_buf = ("%s,%s,%i,%s,%s,%s,%s,%s,%s,%s" % (
              sm_buf_dict['SM_API_MSG_VERSION'],
              sm_buf_dict['SM_API_MSG_REVISION'],
              sm_buf_dict['SM_API_MSG_SEQNO'],
              sm_buf_dict['SM_API_MSG_TYPE'],
              sm_buf_dict['SM_API_MSG_ORIGIN'],
              sm_buf_dict['SM_API_MSG_NODE_NAME'],
              sm_buf_dict['SM_API_MSG_NODE_ACTION'],
              sm_buf_dict['SM_API_MSG_NODE_ADMIN'],
              sm_buf_dict['SM_API_MSG_NODE_OPER'],
              sm_buf_dict['SM_API_MSG_NODE_AVAIL']))

    LOG.debug("sm-api buffer to SM API: %s" % sm_buf)

    # notify SM
    s = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
    try:
        if os.path.exists(SM_API_CLIENT_ADDR):
            os.unlink(SM_API_CLIENT_ADDR)

        s.setblocking(1)  # blocking, timeout must be specified
        s.settimeout(6)   # give sm a few secs to respond
        s.bind(SM_API_CLIENT_ADDR)
        s.sendto(sm_buf, SM_API_SERVER_ADDR)

        count = 0
        while count < 5:
            count += 1
            sm_ack = s.recv(1024)

            try:
                sm_ack_list = sm_ack.split(",")
                if sm_ack_list[SM_API_MSG_SEQNO_FIELD] == \
                   str(sm_buf_dict['SM_API_MSG_SEQNO']):
                    break
                else:
                    LOG.debug(_("sm-api mismatch seqno tx message: %s  rx message: %s  " % (sm_buf, sm_ack)))
            except:
                LOG.exception(_("sm-api bad rx message: %s" % sm_ack))

    except socket.error, e:
        LOG.exception(_("sm-api socket error: %s on  %s") % (e, sm_buf))
        sm_ack_dict = {
           'SM_API_MSG_TYPE': "unknown_set_node",
           'SM_API_MSG_NODE_ACTION': sm_dict['SM_API_MSG_NODE_ACTION'],
           'SM_API_MSG_ORIGIN': "sm",
           'SM_API_MSG_NODE_NAME': sm_dict['SM_API_MSG_NODE_NAME'],
           'SM_API_MSG_NODE_ADMIN': "unknown",
           'SM_API_MSG_NODE_OPER': "unknown",
           'SM_API_MSG_NODE_AVAIL': "unknown"}

        return sm_ack_dict

    finally:
        s.close()
        if os.path.exists(SM_API_CLIENT_ADDR):
            os.unlink(SM_API_CLIENT_ADDR)

    LOG.debug("sm-api set node state sm_ack %s " % sm_ack)
    try:
        sm_ack_list = sm_ack.split(",")
        sm_ack_dict = {
          'SM_API_MSG_VERSION': sm_ack_list[SM_API_MSG_VERSION_FIELD],
          'SM_API_MSG_REVISION': sm_ack_list[SM_API_MSG_REVISION_FIELD],
          'SM_API_MSG_SEQNO': sm_ack_list[SM_API_MSG_SEQNO_FIELD],
          'SM_API_MSG_TYPE': sm_ack_list[SM_API_MSG_TYPE_FIELD],
          'SM_API_MSG_NODE_ACTION': sm_ack_list[SM_API_MSG_NODE_ACTION_FIELD],

          'SM_API_MSG_ORIGIN': sm_ack_list[SM_API_MSG_ORIGIN_FIELD],
          'SM_API_MSG_NODE_NAME': sm_ack_list[SM_API_MSG_NODE_NAME_FIELD],
          'SM_API_MSG_NODE_ADMIN': sm_ack_list[SM_API_MSG_NODE_ADMIN_FIELD],
          'SM_API_MSG_NODE_OPER': sm_ack_list[SM_API_MSG_NODE_OPER_FIELD],
          'SM_API_MSG_NODE_AVAIL': sm_ack_list[SM_API_MSG_NODE_AVAIL_FIELD]
           }
    except:
        LOG.exception(_("sm-api ack message error: %s" % sm_ack))
        sm_ack_dict = {
           'SM_API_MSG_TYPE': "unknown_set_node",

           'SM_API_MSG_ORIGIN': "sm",
           'SM_API_MSG_NODE_NAME': sm_dict['SM_API_MSG_NODE_NAME'],
           'SM_API_MSG_NODE_ADMIN': "unknown",
           'SM_API_MSG_NODE_OPEsR': "unknown",
           'SM_API_MSG_NODE_AVAIL': "unknown"
         }

    return sm_ack_dict


def sm_api_set_node_state(origin, hostname, action, admin, avail, oper, seqno):
    sm_ack_dict = {}
    sm_dict = {'SM_API_MSG_TYPE':  SM_API_MSG_TYPE_SET_NODE,

               'SM_API_MSG_ORIGIN': origin,
               'SM_API_MSG_NODE_NAME': hostname,

               'SM_API_MSG_NODE_ACTION': action,
               'SM_API_MSG_NODE_ADMIN':  admin,
               'SM_API_MSG_NODE_OPER':  oper,
               'SM_API_MSG_NODE_AVAIL':  avail,
               'SM_API_MSG_SEQNO': seqno,
             }

    sm_ack_dict = sm_api_notify(sm_dict)

    return sm_ack_dict
