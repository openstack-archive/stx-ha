# vim: tabstop=4 shiftwidth=4 softtabstop=4
# -*- encoding: utf-8 -*-
#
#
# Copyright 2013 Hewlett-Packard Development Company, L.P.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# Copyright (c) 2013-2014 Wind River Systems, Inc.
#

"""
Base classes for storage engines
"""

import abc

from sm_api.openstack.common.db import api as db_api

_BACKEND_MAPPING = {'sqlalchemy': 'sm_api.db.sqlalchemy.api'}
IMPL = db_api.DBAPI(backend_mapping=_BACKEND_MAPPING)


def get_instance():
    """Return a DB API instance."""
    return IMPL


class Connection(object):
    """Base class for storage system connections."""

    __metaclass__ = abc.ABCMeta

    @abc.abstractmethod
    def __init__(self):
        """Constructor."""

    @abc.abstractmethod
    def iservicegroup_get(self, server):
        """Return a servicegroup.

        :param server: The id or uuid of a servicegroup.
        :returns: An iservicegroup.
        """

    @abc.abstractmethod
    def iservicegroup_get_list(self, limit=None, marker=None,
                               sort_key=None, sort_dir=None):
        """Return a list of servicegroupintances.

        :param limit: Maximum number of iServers to return.
        :param marker: the last item of the previous page; we return the next
                       result set.
        :param sort_key: Attribute by which results should be sorted.
        :param sort_dir: direction in which results should be sorted.
                         (asc, desc)
        """

    @abc.abstractmethod
    def iservice_get(self, server):
        """Return a service instance.

        :param server: The id or uuid of a server.
        :returns: A server.
        """

    @abc.abstractmethod
    def iservice_get_list(self, limit=None, marker=None,
                          sort_key=None, sort_dir=None):
        """Return a list of serviceintances.

        :param limit: Maximum number of iServers to return.
        :param marker: the last item of the previous page; we return the next
                       result set.
        :param sort_key: Attribute by which results should be sorted.
        :param sort_dir: direction in which results should be sorted.
                         (asc, desc)
        """

    @abc.abstractmethod
    def iservice_get_by_name(self, name):
        """Return a list of serviceinstance by service.
            :param name: The name (service group)
            returns: An iservice list
        """

    @abc.abstractmethod
    def sm_sdm_get(self, server, service_group_name):
        """get service_domain_member

        :param server: The name of the service_domain.
        :param service_group: The name of the service_domain_member.
        """

    @abc.abstractmethod
    def sm_sda_get(self, server):
        """get service_domain_assignment

        :param server: The id or uuid of a service_domain_assignment.
        """

    @abc.abstractmethod
    def sm_sda_get_list(self, limit=None, marker=None,
                        sort_key=None, sort_dir=None):
        """Return a list of service_domain_assignments.

        :param limit: Maximum number of entries to return.
        :param marker: the last item of the previous page; we return the next
                       result set.
        :param sort_key: Attribute by which results should be sorted.
        :param sort_dir: direction in which results should be sorted.
                         (asc, desc)
        """

    @abc.abstractmethod
    def sm_node_get_list(self, limit=None, marker=None,
                         sort_key=None, sort_dir=None):
        """Return a list of nodes.

        :param limit: Maximum number of nodes to return.
        :param marker: the last item of the previous page; we return the next
                       result set.
        :param sort_key: Attribute by which results should be sorted.
        :param sort_dir: direction in which results should be sorted.
                         (asc, desc)
        """

    @abc.abstractmethod
    def sm_node_get(self, server):
        """get node

        :param server: The id or uuid of a node.
        """

    @abc.abstractmethod
    def sm_node_get_by_name(self, name):
        """Return a list of nodes by name.
           :param name: The name of the hostname.
        """

    @abc.abstractmethod
    def sm_service_get(self, server):
        """get service

        :param server: The id or uuid of a service.
        """

    @abc.abstractmethod
    def sm_service_get_list(self, limit=None, marker=None,
                            sort_key=None, sort_dir=None):
        """Return a list of services.

        :param limit: Maximum number of services to return.
        :param marker: the last item of the previous page; we return the next
                       result set.
        :param sort_key: Attribute by which results should be sorted.
        :param sort_dir: direction in which results should be sorted.
                         (asc, desc)
        """

    @abc.abstractmethod
    def sm_service_get_by_name(self, name):
        """Return a service by name.
           :param name: The name of the services.
        """

    @abc.abstractmethod
    def sm_service_group_members_get_list(self, service_group_name):
        """Return service group members in a service group
            :param service_group_name: service group name
        """
