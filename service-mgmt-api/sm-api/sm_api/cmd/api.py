#!/usr/bin/env python
# -*- encoding: utf-8 -*-
#
# vim: tabstop=4 shiftwidth=4 softtabstop=4
#
# Copyright 2013 Hewlett-Packard Development Company, L.P.
# All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
#
# Copyright (c) 2013-2014 Wind River Systems, Inc.
#


"""The Service Management API."""

import logging
import os.path
import sys
import time


from oslo_config import cfg
from wsgiref import simple_server

from sm_api.api import app
from sm_api.common import service as sm_api_service
from sm_api.openstack.common import log

CONF = cfg.CONF


def get_handler_cls():
    cls = simple_server.WSGIRequestHandler

    # old-style class doesn't support super
    class MyHandler(cls, object):
        def address_string(self):
            # In the future, we could provide a config option to allow reverse DNS lookup
            return self.client_address[0]

    return MyHandler


def main():
    # Parse config file and command line options, then start logging

    # Periodically check every minute for want_sm_config
    while os.path.exists("/etc/sm/.not_want_sm_config"):
        time.sleep(60)

    sm_api_service.prepare_service(sys.argv)

    # Build and start the WSGI app
    # host = CONF.sm_api_api_bind_ip
    # port = CONF.sm_api_api_port
    host = 'localhost'
    port = 7777
    wsgi = simple_server.make_server(host, port,
                                     app.VersionSelectorApplication(),
                                     handler_class=get_handler_cls())

    LOG = log.getLogger(__name__)
    LOG.info(_("Serving on http://%(host)s:%(port)s") %
             {'host': host, 'port': port})
    LOG.info(_("Configuration:"))
    CONF.log_opt_values(LOG, logging.INFO)

    try:
        wsgi.serve_forever()
    except KeyboardInterrupt:
        pass
