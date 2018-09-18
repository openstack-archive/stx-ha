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


class NodesCommand(wsme_types.Base):
    path = wsme_types.text
    value = wsme_types.text
    op = wsme_types.text


class NodesCommandResult(wsme_types.Base):
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


class Nodes(base.APIBase):
    id = int
    name = wsme_types.text
    administrative_state = wsme_types.text
    operational_state = wsme_types.text
    availability_status = wsme_types.text
    ready_state = wsme_types.text

    links = [link.Link]
    "A list containing a self link and associated nodes links"

    def __init__(self, **kwargs):
        self.fields = list(objects.sm_node.fields)
        for k in self.fields:
            setattr(self, k, kwargs.get(k))

    @classmethod
    def convert_with_links(cls, rpc_nodes, expand=True):
        minimum_fields = ['id', 'name', 'administrative_state',
                          'operational_state', 'availability_status',
                          'ready_state']
        fields = minimum_fields if not expand else None
        nodes = Nodes.from_rpc_object(
                           rpc_nodes, fields)

        return nodes


class NodesCollection(collection.Collection):
    """API representation of a collection of nodes."""

    nodes = [Nodes]
    "A list containing nodes objects"

    def __init__(self, **kwargs):
        self._type = 'nodes'

    @classmethod
    def convert_with_links(cls, nodes, limit, url=None,
                           expand=False, **kwargs):
        collection = NodesCollection()
        collection.nodes = [
                    Nodes.convert_with_links(ch, expand)
                              for ch in nodes]
        url = url or None
        collection.next = collection.get_next(limit, url=url, **kwargs)
        return collection


# class Nodess(wsme_types.Base):
#     nodes = wsme_types.text
class NodesController(rest.RestController):

    def _get_nodes(self, marker, limit, sort_key, sort_dir):

        limit = utils.validate_limit(limit)
        sort_dir = utils.validate_sort_dir(sort_dir)
        marker_obj = None
        if marker:
            marker_obj = objects.sm_node.get_by_uuid(
                                 pecan.request.context, marker)

        nodes = pecan.request.dbapi.sm_node_get_list(limit,
                                                     marker_obj,
                                                     sort_key=sort_key,
                                                     sort_dir=sort_dir)
        return nodes

    @wsme_pecan.wsexpose(Nodes, unicode)
    def get_one(self, uuid):
        try:
            rpc_sg = objects.sm_node.get_by_uuid(pecan.request.context, uuid)
        except exception.ServerNotFound:
            return None

        return  Nodes.convert_with_links(rpc_sg)

    @wsme_pecan.wsexpose(NodesCollection, unicode, int,
                         unicode, unicode)
    def get_all(self, marker=None, limit=None,
                sort_key='name', sort_dir='asc'):
        """Retrieve list of nodes."""

        nodes = self._get_nodes(marker,
                                limit,
                                sort_key,
                                sort_dir)

        return NodesCollection.convert_with_links(nodes, limit,
                                                 sort_key=sort_key,
                                                 sort_dir=sort_dir)

    @wsme_pecan.wsexpose(NodesCommandResult, unicode,
                         body=NodesCommand)
    def put(self, hostname, command):

        raise NotImplementedError()

    @wsme_pecan.wsexpose(NodesCommandResult, unicode,
                         body=NodesCommand)
    def patch(self, hostname, command):

        raise NotImplementedError()
