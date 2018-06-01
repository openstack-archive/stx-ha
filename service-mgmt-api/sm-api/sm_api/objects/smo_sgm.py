#
# Copyright (c) 2018 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

# vim: tabstop=4 shiftwidth=4 softtabstop=4
# coding=utf-8
#

from sm_api.db import api as db_api
from sm_api.objects import base
from sm_api.objects import utils


class service_group_member(base.Sm_apiObject):

    dbapi = db_api.get_instance()

    fields = {
            'id': utils.int_or_none,
            'name': utils.str_or_none,
            'service_name': utils.str_or_none,
            'service_failure_impact': utils.str_or_none
            }

    @staticmethod
    def _from_db_object(server, db_server):
        """Converts a database entity to a formal object."""
        for field in server.fields:
            server[field] = db_server[field]

        server.obj_reset_changes()
        return server

    @base.remotable_classmethod
    def get_by_service_group(cls, context, service_group_name):
        """Find a server based on uuid and return a Node object.

        :param uuid: the uuid of a server.
        :returns: a :class:`Node` object.
        """
        db_server = cls.dbapi.iservicegroup_member_get(service_group_name)
        return service_group_member._from_db_object(cls(), db_server)

    @base.remotable
    def save(self, context):
        """Save service group member to this Node.
        :param context: Security context
        """
        raise NotImplemented("This method is intentially not implemented")

    @base.remotable
    def refresh(self, context):
        current = self.__class__.get_by_uuid(context, uuid=self.uuid)
        for field in self.fields:
            if (hasattr(self, base.get_attrname(field)) and
                    self[field] != current[field]):
                self[field] = current[field]
