#!/usr/bin/env python
# vim: tabstop=4 shiftwidth=4 softtabstop=4
#
#
# Copyright (c) 2018 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
#
from sm_client import exc
from sm_client.common import utils


def _print_sm_service_node_show(node):
    fields = ['id', 'name', 'administrative_state', 'operational_state',
              'availability_status', 'ready_state']
    utils.print_mapping(node, fields, wrap=72)


def do_servicenode_list(cc, args):
    """List Service Nodes."""
    try:
        node = cc.smc_service_node.list()
    except exc.Forbidden:
        raise exc.CommandError("Not authorized. The requested action "
                               "requires 'admin' level")
    else:
        fields = ['id', 'name', 'administrative_state', 'operational_state',
                  'availability_status', 'ready_state']
        field_labels = ['id', 'name', 'administrative', 'operational',
                        'availability', 'ready_state']
        utils.print_list(node, fields, field_labels, sortby=1)


@utils.arg('node', metavar='<node uuid>',
           help="uuid of a Service Node")
def do_servicenode_show(cc, args):
    """Show a Service Node's attributes."""
    try:
        node = cc.smc_service_node.get(args.node)
    except exc.HTTPNotFound:
        raise exc.CommandError('Service Node not found: %s' % args.node)
    except exc.Forbidden:
        raise exc.CommandError("Not authorized. The requested action "
                               "requires 'admin' level")
    else:
        if node is None:
            print("Service node %s could not be found" % args.node)
            return
        _print_sm_service_node_show(node)
