#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

from wsme import types as wsme_types


class Link(wsme_types.Base):
    """ Representation of a link.
    """

    href = wsme_types.text
    "The url of a link."

    rel = wsme_types.text
    "The name of a link."

    type = wsme_types.text
    "The type of document or link."

    @classmethod
    def make_link(cls, rel_name, url, resource, resource_args,
                  bookmark=False, type=wsme_types.Unset):
        template = '%s/%s' if bookmark else '%s/v1/%s'
        template += '%s' if resource_args.startswith('?') else '/%s'

        return Link(href=template % (url, resource, resource_args),
                    rel=rel_name, type=type)
