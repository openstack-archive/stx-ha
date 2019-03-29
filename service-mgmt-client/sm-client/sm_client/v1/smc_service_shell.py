#!/usr/bin/env python
# vim: tabstop=4 shiftwidth=4 softtabstop=4
#
#
# Copyright (c) 2018 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
#
import socket

from sm_client.common import utils
from sm_client import exc


def _print_service_show(service):
    fields = ['id', 'service_name', 'hostname', 'state']
    data = dict([(f, getattr(service, f, '')) for f in fields])
    data['hostname'] = getattr(service, 'node_name', '')
    utils.print_dict(data, fields, wrap=72)


def do_service_list(cc, args):
    """List Services."""
    try:
        service = cc.smc_service.list()
    except exc.Forbidden:
        raise exc.CommandError("Not authorized. The requested action "
                               "requires 'admin' level")
    else:
        fields = ['id', 'name', 'node_name', 'state']
        field_labels = ['id', 'service_name', 'hostname', 'state']
        # remove the entry in the initial state
        clean_list = [x for x in service if x.state != 'initial']
        for s in clean_list:
            if s.status:
                s.state = (s.state + '-' + s.status)
            if getattr(s, 'node_name', None) is None:
                s.node_name = socket.gethostname()

        utils.print_list(clean_list, fields, field_labels, sortby=1)


@utils.arg('service', metavar='<service id>', help="ID of service")
def do_service_show(cc, args):
    """Show a Service."""
    try:
        service = cc.smc_service.get(args.service)
    except exc.HTTPNotFound:
        raise exc.CommandError('service not found: %s' % args.service)
    except exc.Forbidden:
        raise exc.CommandError("Not authorized. The requested action "
                               "requires 'admin' level")
    else:
        if service is None:
            print("Service %s could not be found" % args.service)
            return
        if service.status:
            service.state = (service.state + '-' + service.status)
        service.service_name = service.name
        if getattr(service, 'node_name', None) is None:
            service.hostname = socket.gethostname()
        _print_service_show(service)
