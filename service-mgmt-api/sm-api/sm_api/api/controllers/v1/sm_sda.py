#
# Copyright (c) 2014 Wind River Systems, Inc.
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

from sm_api.common import log
from sm_api import objects


LOG = log.get_logger(__name__)


class SmSdaCommand(wsme_types.Base):
    path = wsme_types.text
    value = wsme_types.text
    op = wsme_types.text


class SmSdaCommandResult(wsme_types.Base):
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


class SmSda(base.APIBase):
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
        self.fields = objects.sm_sda.fields.keys()
        for k in self.fields:
            setattr(self, k, kwargs.get(k))

    @classmethod
    def convert_with_links(cls, rpc_sm_sda, expand=True):
        minimum_fields = ['id', 'uuid', 'name', 'node_name',
                          'service_group_name', 'desired_state',
                          'state', 'status', 'condition']
        fields = minimum_fields if not expand else None
        sm_sda = SmSda.from_rpc_object(
                           rpc_sm_sda, fields)

        return sm_sda


class SmSdaCollection(collection.Collection):
    """API representation of a collection of sm_sda."""

    sm_sda = [SmSda]
    "A list containing sm_sda objects"

    def __init__(self, **kwargs):
        self._type = 'sm_sda'

    @classmethod
    def convert_with_links(cls, sm_sda, limit, url=None,
                           expand=False, **kwargs):
        collection = SmSdaCollection()
        collection.sm_sda = [
                    SmSda.convert_with_links(ch, expand)
                              for ch in sm_sda]
        url = url or None
        collection.next = collection.get_next(limit, url=url, **kwargs)
        return collection


class SmSdas(wsme_types.Base):
    sm_sda = wsme_types.text


class SmSdaController(rest.RestController):

    def _get_sm_sda(self, marker, limit, sort_key, sort_dir):

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

    @wsme_pecan.wsexpose(SmSda, unicode)
    def get_one(self, uuid):

        rpc_sda = objects.sm_sda.get_by_uuid(pecan.request.context, uuid)

        # temp: remap OpenStack_Services to Cloud_Services
        if rpc_sda.service_group_name.lower() == "openstack_services":
            rpc_sda.service_group_name = "Cloud_Services"

        return SmSda.convert_with_links(rpc_sda)

    @wsme_pecan.wsexpose(SmSdaCollection, unicode, int,
                         unicode, unicode)
    def get_all(self, marker=None, limit=None,
                sort_key='name', sort_dir='asc'):
        """Retrieve list of sm_sdas."""

        sm_sda = self._get_sm_sda(marker,
                                  limit,
                                  sort_key,
                                  sort_dir)

        return SmSdaCollection.convert_with_links(sm_sda, limit,
                                                  sort_key=sort_key,
                                                  sort_dir=sort_dir)

    @wsme_pecan.wsexpose(SmSdaCommandResult, unicode,
                         body=SmSdaCommand)
    def put(self, hostname, command):

        raise NotImplementedError()

    @wsme_pecan.wsexpose(SmSdaCommandResult, unicode,
                         body=SmSdaCommand)
    def patch(self, hostname, command):

        raise NotImplementedError()
