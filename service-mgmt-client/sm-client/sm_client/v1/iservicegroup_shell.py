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


def _print_iservicegroup_show(iservicegroup):
    fields = ['id', 'name', 'state', 'status']
    data = dict([(f, getattr(iservicegroup, f, '')) for f in fields])
    utils.print_dict(data, wrap=72)


def do_servicegroup_list(cc, args):
    """List Service Groups."""
    iservicegroup = cc.iservicegroup.list()
    fields = ['id', 'name', 'state', 'status']
    field_labels = ['id', 'name', 'state', 'status']
    utils.print_list(iservicegroup, fields, field_labels, sortby=1)


@utils.arg('servicegroup', metavar='<servicegroup id>',
           help="ID of iservicegroup")
def do_servicegroup_show(cc, args):
    """Show a Service Group."""
    try:
        servicegroup = cc.iservicegroup.get(args.servicegroup)
    except exc.HTTPNotFound:
        raise exc.CommandError(
                  'Service Group not found: %s' % args.servicegroup)
    else:
        _print_iservicegroup_show(servicegroup)


@utils.arg('-n', '--name',
           metavar='<name>',
           help='name of the service group [REQUIRED]')
@utils.arg('-s', '--state',
           metavar='<state>',
           help='state of the servicegroup [REQUIRED]')
def donot_servicegroup_create(cc, args):
    """Create a new servicegroup."""
    field_list = ['name', 'state']
    fields = dict((k, v) for (k, v) in vars(args).items()
                  if k in field_list and not (v is None))
    # fields = utils.args_array_to_dict(fields, 'activity')
    iservicegroup = cc.iservicegroup.create(**fields)

    field_list.append('uuid')
    data = dict([(f, getattr(iservicegroup, f, '')) for f in field_list])
    utils.print_dict(data, wrap=72)


@utils.arg('iservicegroup',
           metavar='<iservicegroup id>',
           nargs='+',
           help="ID of iservicegroup")
def donot_servicegroup_delete(cc, args):
    """Delete a servicegroup."""
    for c in args.iservicegroup:
        try:
            cc.iservicegroup.delete(c)
        except exc.HTTPNotFound:
            raise exc.CommandError('Service not found: %s' % c)
        print 'Deleted servicegroup %s' % c


@utils.arg('iservicegroup',
           metavar='<iservicegroup id>',
           help="ID of iservicegroup")
@utils.arg('attributes',
           metavar='<path=value>',
           nargs='+',
           action='append',
          default=[],
          help="Attributes to add/replace or remove ")
def donot_servicegroup_modify_labonly(cc, args):
    """LAB ONLY Update a servicegroup. """
    # JKUNG comment this out prior to delivery
    patch = utils.args_array_to_patch("replace", args.attributes[0])
    try:
        iservicegroup = cc.iservicegroup.update(args.iservicegroup, patch)
    except exc.HTTPNotFound:
        raise exc.CommandError(
                  'Service Group not found: %s' % args.iservicegroup)
    _print_iservicegroup_show(iservicegroup)
