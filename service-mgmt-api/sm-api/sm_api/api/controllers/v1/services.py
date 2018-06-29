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


class ServicesCommand(wsme_types.Base):
    path = wsme_types.text
    value = wsme_types.text
    op = wsme_types.text


class ServicesCommandResult(wsme_types.Base):
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


class Services(base.APIBase):
    id = int
    name = wsme_types.text
    desired_state = wsme_types.text
    state = wsme_types.text
    status = wsme_types.text

    # online_uuid = wsme_types.text
    # "The UUID of the services"

    links = [link.Link]
    "A list containing a self link and associated services links"

    def __init__(self, **kwargs):
        self.fields = objects.service.fields.keys()
        for k in self.fields:
            setattr(self, k, kwargs.get(k))

    @classmethod
    def convert_with_links(cls, rpc_services, expand=True):
        minimum_fields = ['id', 'name', 'desired_state', 'state', 'status']
        fields = minimum_fields if not expand else None
        services = Services.from_rpc_object(
                           rpc_services, fields)

        return services


class ServicesCollection(collection.Collection):
    """API representation of a collection of services."""

    services = [Services]
    "A list containing services objects"

    def __init__(self, **kwargs):
        self._type = 'services'

    @classmethod
    def convert_with_links(cls, services, limit, url=None,
                           expand=False, **kwargs):
        collection = ServicesCollection()
        collection.services = [
                    Services.convert_with_links(ch, expand)
                              for ch in services]
        url = url or None
        collection.next = collection.get_next(limit, url=url, **kwargs)
        return collection


# class Servicess(wsme_types.Base):
#     services = wsme_types.text
class ServicesController(rest.RestController):

    def _get_services(self, marker, limit, sort_key, sort_dir):

        limit = utils.validate_limit(limit)
        sort_dir = utils.validate_sort_dir(sort_dir)
        marker_obj = None
        if marker:
            marker_obj = objects.service.get_by_uuid(
                                 pecan.request.context, marker)

        services = pecan.request.dbapi.sm_service_get_list(limit,
                                                     marker_obj,
                                                     sort_key=sort_key,
                                                     sort_dir=sort_dir)
        return services

    @wsme_pecan.wsexpose(Services, unicode)
    def get_one(self, uuid):
        try:
            rpc_sg = objects.service.get_by_uuid(pecan.request.context, uuid)
        except exception.ServerNotFound:
            return None

        return Services.convert_with_links(rpc_sg)

    @wsme_pecan.wsexpose(Services, unicode)
    def get_service(self, name):
        try:
            rpc_sg = objects.service.get_by_name(pecan.request.context, name)
        except exception.ServerNotFound:
            return None
        return Services.convert_with_links(rpc_sg)

    @wsme_pecan.wsexpose(ServicesCollection, unicode, int,
                         unicode, unicode)
    def get_all(self, marker=None, limit=None,
                sort_key='name', sort_dir='asc'):
        """Retrieve list of services."""

        services = self._get_services(marker,
                                limit,
                                sort_key,
                                sort_dir)

        return ServicesCollection.convert_with_links(services, limit,
                                                     sort_key=sort_key,
                                                     sort_dir=sort_dir)

    @wsme_pecan.wsexpose(ServicesCommandResult, unicode,
                         body=ServicesCommand)
    def put(self, hostname, command):

        raise NotImplementedError()

    @wsme_pecan.wsexpose(ServicesCommandResult, unicode,
                         body=ServicesCommand)
    def patch(self, hostname, command):

        raise NotImplementedError()
