#    Copyright 2013 IBM Corp.
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
# Copyright (c) 2013-2018 Wind River Systems, Inc.
#


import functools

from sm_api.objects import smo_servicegroup
from sm_api.objects import smo_service
from sm_api.objects import smo_sdm
from sm_api.objects import smo_sda
from sm_api.objects import smo_node
from sm_api.objects import smo_sgm


def objectify(klass):
    """Decorator to convert database results into specified objects."""
    def the_decorator(fn):
        @functools.wraps(fn)
        def wrapper(*args, **kwargs):
            result = fn(*args, **kwargs)
            try:
                return klass._from_db_object(klass(), result)
            except TypeError:
                # TODO(deva): handle lists of objects better
                #             once support for those lands and is imported.
                return [klass._from_db_object(klass(), obj) for obj in result]
        return wrapper
    return the_decorator


service_groups = smo_servicegroup.service_groups
service = smo_service.service
sm_sdm = smo_sdm.sm_sdm
sm_sda = smo_sda.sm_sda
sm_node = smo_node.sm_node
service_group_member = smo_sgm.service_group_member

__all__ = (
           service_groups,
           service_group_member,
           service,
           sm_sdm,
           sm_sda,
           sm_node,
           objectify)
