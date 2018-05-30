#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

import pecan
from pecan import rest
from wsme import types as wsme_types
from wsmeext import pecan as wsme_pecan

from sm_api.api.controllers import v1
from sm_api.api.controllers.v1 import base
from sm_api.api.controllers.v1 import link


class Version(base.APIBase):
    """An API version representation."""

    id = wsme_types.text
    "The ID of the version, also acts as the release number"

    links = [link.Link]
    "A Link that point to a specific version of the API"

    @classmethod
    def convert(cls, id):
        version = Version()
        version.id = id
        version.links = [link.Link.make_link('self', pecan.request.host_url,
                                             id, '', bookmark=True)]
        return version


class Root(base.APIBase):

    name = wsme_types.text
    "The name of the API"

    description = wsme_types.text
    "Some information about this API"

    version = [Version]
    "Links to all the versions available in this API"

    default_version = Version
    "A link to the default version of the API"

    @classmethod
    def convert(cls):
        root = Root()
        root.name = "System Management API"
        root.description = "System Management API from Wind River"
        root.version = [Version.convert("v1")]
        root.default_version = Version.convert("v1")
        return root


class RootController(rest.RestController):

    v1 = v1.Controller()

    @wsme_pecan.wsexpose(Root)
    def get(self):
        return Root.convert()
