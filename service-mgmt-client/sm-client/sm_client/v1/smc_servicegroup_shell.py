#!/usr/bin/env python
# vim: tabstop=4 shiftwidth=4 softtabstop=4
#
#
# Copyright (c) 2018 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
#
from sm_client.common import utils
from sm_client import exc


def _print_servicegroup_show(servicegroup):
    fields = ['uuid', 'name', 'hostname', 'service_group_name', 'state']
    utils.print_mapping(servicegroup, fields, wrap=72)


def do_servicegroup_list(cc, args):
    """List Service Groups."""
    try:
        servicegroup = cc.smc_servicegroup.list()
    except exc.Forbidden:
        raise exc.CommandError("Not authorized. The requested action "
                               "requires 'admin' level")
    else:
        fields = ['uuid', 'service_group_name', 'node_name', 'state']
        field_labels = ['uuid', 'service_group_name', 'hostname', 'state']
        for s in servicegroup:
            if s.status:
                s.state = (s.state + '-' + s.status)
        utils.print_list(servicegroup, fields, field_labels, sortby=1)


@utils.arg('servicegroup', metavar='<servicegroup uuid>',
           help="UUID of servicegroup")
def do_servicegroup_show(cc, args):
    """Show a Service Group."""
    try:
        servicegroup = cc.smc_servicegroup.get(args.servicegroup)
    except exc.HTTPNotFound:
        raise exc.CommandError('Service Group not found: %s' % args.servicegroup)
    except exc.Forbidden:
        raise exc.CommandError("Not authorized. The requested action "
                               "requires 'admin' level")
    else:
        if servicegroup is None:
            print("Service group %s could not be found" % args.servicegroup)
            return
        if servicegroup.status:
            servicegroup.state = (servicegroup.state + '-' +
                                  servicegroup.status)
        servicegroup.hostname = servicegroup.node_name
        _print_servicegroup_show(servicegroup)
