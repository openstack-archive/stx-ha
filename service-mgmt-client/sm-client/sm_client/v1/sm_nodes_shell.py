#!/usr/bin/env python
# vim: tabstop=4 shiftwidth=4 softtabstop=4

# Copyright 2013 Red Hat, Inc.
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


from sm_client.common import utils
from sm_client import exc


def _print_sm_node_show(node):
    fields = ['id', 'name', 'state', 'online']
    data = dict([(f, getattr(node, f, '')) for f in fields])
    utils.print_dict(data, wrap=72)


def do_node_list(cc, args):
    """List Node(s)."""
    node = cc.sm_nodes.list()
    fields = ['id', 'name', 'state', 'online']
    field_labels = ['id', 'name', 'state', 'online']
    utils.print_list(node, fields, field_labels, sortby=1)


@utils.arg('node', metavar='<node uuid>',
           help="uuid of a Service Domain Assignment")
def do_node_show(cc, args):
    """Show a Node."""
    try:
        node = cc.sm_nodes.get(args.node)
    except exc.HTTPNotFound:
        raise exc.CommandError(
                  'Service Domain Assignment not found: %s' % args.node)
    else:
        _print_sm_node_show(node)
