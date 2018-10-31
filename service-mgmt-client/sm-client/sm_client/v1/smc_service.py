#!/usr/bin/env python
# vim: tabstop=4 shiftwidth=4 softtabstop=4
#
# Copyright (c) 2018 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
#
from sm_client.common import base


class SmcService(base.Resource):
    def __repr__(self):
        return "<SmcService %s>" % self._info


class SmcServiceManager(base.Manager):
    resource_class = SmcService

    @staticmethod
    def _path(id=None):
        return '/v1/services/%s' % id if id else '/v1/services'

    def list(self):
        return self._list(self._path(), "services")

    def get(self, iservice_id):
        result = self._list(self._path(iservice_id))
        if len(result) == 0:
            return None
        else:
            return result[0]
