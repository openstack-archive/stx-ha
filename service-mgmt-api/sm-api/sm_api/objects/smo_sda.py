#
# Copyright (c) 2013-2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

# vim: tabstop=4 shiftwidth=4 softtabstop=4
# coding=utf-8
#

from sm_api.db import api as db_api
from sm_api.objects import base
from sm_api.objects import utils


class sm_sda(base.Sm_apiObject):

    dbapi = db_api.get_instance()

    fields = {
        'id': int,
        'uuid': utils.str_or_none,
        # 'deleted': utils.str_or_none,

        # 'created_at': utils.datetime_str_or_none,
        # 'updated_at': utils.datetime_str_or_none,
        'name': utils.str_or_none,
        'node_name': utils.str_or_none,
        'service_group_name': utils.str_or_none,
        'state': utils.str_or_none,
        'desired_state': utils.str_or_none,
        'status': utils.str_or_none,
        'condition': utils.str_or_none,
    }

    @staticmethod
    def _from_db_object(server, db_server):
        """Converts a database entity to a formal object."""
        for field in server.fields:
            server[field] = db_server[field]

        server.obj_reset_changes()
        return server

    @base.remotable_classmethod
    def get_by_uuid(cls, context, uuid):
        """Find a server based on uuid and return a Node object.

        :param uuid: the uuid of a server.
        :returns: a :class:`Node` object.
        """
        # TODO(deva): enable getting ports for this server
        db_server = cls.dbapi.sm_sda_get(uuid)
        return sm_sda._from_db_object(cls(), db_server)

    # @base.remotable
    # def save(self, context):
    #     """Save updates to this Node.

    #     Column-wise updates will be made based on the result of
    #     self.what_changed(). If target_power_state is provided,
    #     it will be checked against the in-database copy of the
    #     server before updates are made.

    #     :param context: Security context
    #     """
    #    updates = {}
    #    changes = self.obj_what_changed()
    #    for field in changes:
    #        updates[field] = self[field]
    #    self.dbapi.sm_sda_update(self.uuid, updates)
    #
    #    self.obj_reset_changes()

    @base.remotable
    def refresh(self, context):
        current = self.__class__.get_by_uuid(context, uuid=self.uuid)
        for field in self.fields:
            if (hasattr(self, base.get_attrname(field)) and
                    self[field] != current[field]):
                self[field] = current[field]
