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
from sm_client.common import base


class smc_Servicegroup(base.Resource):
    def __repr__(self):
        return "<smc_Servicegroup %s>" % self._info


class SmcServiceGroupManager(base.Manager):
    resource_class = smc_Servicegroup

    @staticmethod
    def _path(id=None):
        return '/v1/service_groups/%s' % id if id else '/v1/service_groups'

    def list(self):
        return self._list(self._path(), "service_groups")

    def get(self, sm_servicegroup_id):
        try:
            result = self._list(self._path(sm_servicegroup_id))
        except IndexError:
            return None

        if len(result) > 0:
            return result[0]
        else:
            return None
