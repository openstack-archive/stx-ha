#
# Copyright (c) 2014-2018 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

import json
import wsme
from wsme import types as wsme_types
import wsmeext.pecan as wsme_pecan
import pecan
from pecan import rest

from sm_api.api.controllers.v1 import base
from sm_api.api.controllers.v1 import collection
from sm_api.api.controllers.v1 import link
from sm_api.api.controllers.v1 import utils
from sm_api.common import exception
from sm_api.common import log
from sm_api import objects


LOG = log.get_logger(__name__)


class ServiceGroupCommand(wsme_types.Base):
    path = wsme_types.text
    value = wsme_types.text
    op = wsme_types.text


class ServiceGroupCommandResult(wsme_types.Base):
    # Host Information
    hostname = wsme_types.text
    state = wsme_types.text
    # Command
    path = wsme_types.text
    value = wsme_types.text
    op = wsme_types.text
    # Result
    error_code = wsme_types.text
    error_details = wsme_types.text


class ServiceGroup(base.APIBase):
    id = int
    uuid = wsme_types.text
    "The UUID of the sm_sda"

    name = wsme_types.text
    node_name = wsme_types.text
    service_group_name = wsme_types.text
    state = wsme_types.text
    desired_state = wsme_types.text
    status = wsme_types.text
    condition = wsme_types.text

    links = [link.Link]
    "A list containing a self link and associated sm_sda links"

    def __init__(self, **kwargs):
        self.fields = list(objects.sm_sda.fields)
        for k in self.fields:
            setattr(self, k, kwargs.get(k))

    @classmethod
    def convert_with_links(cls, rpc_service_groups, expand=True):
        minimum_fields = ['id', 'uuid', 'name', 'node_name',
                          'service_group_name', 'desired_state',
                          'state', 'status', 'condition']

        fields = minimum_fields if not expand else None
        service_groups = ServiceGroup.from_rpc_object(
                           rpc_service_groups, fields)

        return service_groups


class ServiceGroupCollection(collection.Collection):
    """API representation of a collection of service_groups."""

    service_groups = [ServiceGroup]
    "A list containing service_groups objects"

    def __init__(self, **kwargs):
        self._type = 'service_groups'

    @classmethod
    def convert_with_links(cls, service_groups, limit, url=None,
                           expand=False, **kwargs):
        collection = ServiceGroupCollection()
        collection.service_groups = [
                    ServiceGroup.convert_with_links(ch, expand)
                    for ch in service_groups]
        url = url or None
        collection.next = collection.get_next(limit, url=url, **kwargs)
        return collection


class ServiceGroups(wsme_types.Base):
    service_groups = wsme_types.text


class ServiceGroupController(rest.RestController):

    def _get_service_groups(self, marker, limit, sort_key, sort_dir):

        limit = utils.validate_limit(limit)
        sort_dir = utils.validate_sort_dir(sort_dir)
        marker_obj = None
        if marker:
            marker_obj = objects.sm_sda.get_by_uuid(pecan.request.context,
                                                    marker)

        sm_sdas = pecan.request.dbapi.sm_sda_get_list(limit,
                                                      marker_obj,
                                                      sort_key=sort_key,
                                                      sort_dir=sort_dir)

        # Remap OpenStack_Services to Cloud_Services
        for sm_sda in sm_sdas:
            if sm_sda.service_group_name.lower() == "openstack_services":
                sm_sda.service_group_name = "Cloud_Services"

        return sm_sdas

    @wsme_pecan.wsexpose(ServiceGroup, unicode)
    def get_one(self, uuid):
        try:
            rpc_sg = objects.sm_sda.get_by_uuid(pecan.request.context, uuid)
        except exception.ServerNotFound:
            return None

        return ServiceGroup.convert_with_links(rpc_sg)

    @wsme_pecan.wsexpose(ServiceGroupCollection, unicode, int,
                         unicode, unicode)
    def get_all(self, marker=None, limit=None,
                sort_key='name', sort_dir='asc'):
        """Retrieve list of servicegroups."""

        service_groups = self._get_service_groups(marker,
                                                limit,
                                                sort_key,
                                                sort_dir)

        return ServiceGroupCollection.convert_with_links(service_groups, limit,
                                                 sort_key=sort_key,
                                                 sort_dir=sort_dir)

    @wsme_pecan.wsexpose(ServiceGroupCommandResult, unicode,
                         body=ServiceGroupCommand)
    def put(self, hostname, command):

        raise NotImplementedError()

    @wsme_pecan.wsexpose(ServiceGroupCommandResult, unicode,
                         body=ServiceGroupCommand)
    def patch(self, hostname, command):

        raise NotImplementedError()
