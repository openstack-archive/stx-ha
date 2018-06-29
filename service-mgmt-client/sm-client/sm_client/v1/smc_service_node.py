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

class SmcNode(base.Resource):
    def __repr__(self):
        return "<SmcNode %s>" % self._info


class SmcNodeManager(base.Manager):
    resource_class = SmcNode

    @staticmethod
    def _path(id=None):
        return '/v1/nodes/%s' % id if id else '/v1/nodes'

    def list(self):
        return self._list(self._path(), "nodes")

    def get(self, nodes_id):
        result = self._list(self._path(nodes_id))
        if len(result) > 0:
            return result[0]
        else:
            return None
