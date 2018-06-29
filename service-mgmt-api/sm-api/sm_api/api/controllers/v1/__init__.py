#
# Copyright (c) 2014-2018 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

import pecan
from pecan import rest

from wsme import types as wsme_types
import wsmeext.pecan as wsme_pecan

from sm_api.api.controllers.v1 import link
from sm_api.api.controllers.v1 import service_groups
from sm_api.api.controllers.v1 import services
from sm_api.api.controllers.v1 import servicenode
from sm_api.api.controllers.v1 import sm_sda
from sm_api.api.controllers.v1 import nodes


class Version1(wsme_types.Base):
    """ Version-1 of the API.
    """

    id = wsme_types.text
    "The ID of the version, also acts as the release number"

    links = [link.Link]
    "Links that point to a specific URL for this version and documentation"

    nodes = [link.Link]
    "Links to the SM node resource"

    service_groups = [link.Link]
    "Links to the SM service-group resource"

    services = [link.Link]
    "Links to the SM service resource"

    servicenode = [link.Link]
    "Links to the SM node operation resource"

    sm_sda = [link.Link]
    "Links to the SM service domain assignement resource"

    @classmethod
    def convert(cls):
        v1 = Version1()
        v1.id = "v1"
        v1.links = [link.Link.make_link('self', pecan.request.host_url,
                                        'v1', '', bookmark=True)]
        v1.nodes = [link.Link.make_link('self',
                                        pecan.request.host_url,
                                        'nodes', ''),
                    link.Link.make_link('bookmark',
                                        pecan.request.host_url,
                                        'nodes', '',
                                        bookmark=True)]

        v1.service_groups = [link.Link.make_link('self',
                                                 pecan.request.host_url,
                                                 'service_groups', ''),
                             link.Link.make_link('bookmark',
                                                 pecan.request.host_url,
                                                 'service_groups', '',
                                                 bookmark=True)]
        v1.services = [link.Link.make_link('self',
                                           pecan.request.host_url,
                                           'services', ''),
                       link.Link.make_link('bookmark',
                                           pecan.request.host_url,
                                           'services', '',
                                           bookmark=True)]

        v1.servicenode = [link.Link.make_link('self',
                                              pecan.request.host_url,
                                              'servicenode', ''),
                         link.Link.make_link('bookmark',
                                             pecan.request.host_url,
                                             'nodes', '',
                                             bookmark=True)]
        v1.sm_sda = [link.Link.make_link('self',
                                         pecan.request.host_url,
                                         'sm_sda', ''),
                     link.Link.make_link('bookmark',
                                         pecan.request.host_url,
                                         'sm_sda', '',
                                         bookmark=True)]

        return v1


class Controller(rest.RestController):
    """Version 1 API controller root."""

    service_groups = service_groups.ServiceGroupController()
    services = services.ServicesController()
    servicenode = servicenode.ServiceNodeController()
    sm_sda = sm_sda.SmSdaController()
    nodes = nodes.NodesController()

    @wsme_pecan.wsexpose(Version1)
    def get(self):
        return Version1.convert()

__all__ = Controller
