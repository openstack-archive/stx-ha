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


def _print_sm_sda_show(sm_sda):
    fields = ['id', 'uuid', 'name', 'node_name', 'service_group_name',
              'desired_state', 'state', 'status', 'condition']
    data = dict([(f, getattr(sm_sda, f, '')) for f in fields])
    utils.print_dict(data, wrap=72)


def do_sda_list(cc, args):
    """List Service Domain Assignments."""
    sm_sda = cc.sm_sda.list()
    fields = ['uuid', 'service_group_name', 'node_name', 'state', 'status',
              'condition']
    field_labels = ['uuid', 'service_group_name', 'node_name',
                    'state', 'status', 'condition']
    utils.print_list(sm_sda, fields, field_labels, sortby=1)


@utils.arg('sm_sda', metavar='<sm_sda uuid>',
           help="uuid of a Service Domain Assignment")
def do_sda_show(cc, args):
    """Show a Service Domain Assignment."""
    try:
        sm_sda = cc.sm_sda.get(args.sm_sda)
    except exc.HTTPNotFound:
        raise exc.CommandError(
                  'Service Domain Assignment not found: %s' % args.sm_sda)
    else:
        _print_sm_sda_show(sm_sda)
