# -*- encoding: utf-8 -*-
#
# Copyright © 2013 Red Hat, Inc
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


from sm_client.common import base
from sm_client import exc


CREATION_ATTRIBUTES = ['servicename', 'state']


class sm_Sda(base.Resource):
    def __repr__(self):
        return "<sm_Sda %s>" % self._info


class Sm_SdaManager(base.Manager):
    resource_class = sm_Sda

    @staticmethod
    def _path(id=None):
        return '/v1/sm_sda/%s' % id if id else '/v1/sm_sda'

    def list(self):
        return self._list(self._path(), "sm_sda")

    def get(self, sm_sda_id):
        try:
            return self._list(self._path(sm_sda_id))[0]
        except IndexError:
            return None

    def create(self, **kwargs):
        new = {}
        for (key, value) in kwargs.items():
            if key in CREATION_ATTRIBUTES:
                new[key] = value
            else:
                raise exc.InvalidAttribute()
        return self._create(self._path(), new)

    def delete(self, sm_sda_id):
        return self._delete(self._path(sm_sda_id))

    def update(self, sm_sda_id, patch):
        return self._update(self._path(sm_sda_id), patch)
