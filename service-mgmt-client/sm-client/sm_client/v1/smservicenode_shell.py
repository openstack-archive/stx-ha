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


def _print_smservicenode_show(smservicenode):
    fields = ['id', 'uuid', 'name', 'node_name', 'service_group_name',
              'desired_state',  'state', 'status']
    data = dict([(f, getattr(smservicenode, f, '')) for f in fields])
    utils.print_dict(data, wrap=72)


@utils.arg('hostname', metavar='<hostname>',
           help="hostname of servicenode")
def donot_servicenode_list(cc, args):
    """List sm servicenodes by hostname."""
    smservicenode = cc.smservicenode.list(args.hostname)
    fields = ['uuid', 'name', 'node_name', 'service_group_name',
              'state', 'status']
    field_labels = ['uuid', 'name', 'node_name', 'service_group_name',
                    'state', 'status']
    utils.print_list(smservicenode, fields, field_labels, sortby=1)


@utils.arg('smservicenode', metavar='<smservicenode uuid>',
           help="uuid of smservicenode")
def donot_servicenode_show(cc, args):
    """Show an smservicenode."""
    try:
        smservicenode = cc.smservicenode.get(args.smservicenode)
    except exc.HTTPNotFound:
        raise exc.CommandError(
                  'servicenode not found: %s' % args.smservicenode)
    else:
        _print_smservicenode_show(smservicenode)
