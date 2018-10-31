# vim: tabstop=4 shiftwidth=4 softtabstop=4
# -*- encoding: utf-8 -*-
#
# Copyright 2013 Hewlett-Packard Development Company, L.P.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
#
# Copyright (c) 2013-2018 Wind River Systems, Inc.
#


"""
SQLAlchemy models for sm_api data.
"""

import json


from six.moves.urllib.parse import urlparse
from oslo_config import cfg

from sqlalchemy import Column, ForeignKey, Integer
from sqlalchemy import String
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.types import TypeDecorator, VARCHAR

from sm_api.openstack.common.db.sqlalchemy import models

sql_opts = [
    cfg.StrOpt('mysql_engine',
               default='InnoDB',
               help='MySQL engine')
]

cfg.CONF.register_opts(sql_opts, 'database')


def table_args():
    engine_name = urlparse(cfg.CONF.database_connection).scheme
    if engine_name == 'mysql':
        return {'mysql_engine': cfg.CONF.mysql_engine,
                'mysql_charset': "utf8"}
    return None


class JSONEncodedDict(TypeDecorator):
    """Represents an immutable structure as a json-encoded string."""

    impl = VARCHAR

    def process_bind_param(self, value, dialect):
        if value is not None:
            value = json.dumps(value)
        return value

    def process_result_value(self, value, dialect):
        if value is not None:
            value = json.loads(value)
        return value


class Sm_apiBase(models.ModelBase):   # models.TimestampMixin,

    metadata = None

    def as_dict(self):
        d = {}
        for c in self.__table__.columns:
            d[c.name] = self[c.name]
        return d


Base = declarative_base(cls=Sm_apiBase)


# table name in models
class iservicegroup(Base):
    __tablename__ = 'service_groups'

    id = Column(Integer, primary_key=True)
    name = Column(String(255))
    state = Column(String(255))
    status = Column(String(255))


class iservice(Base):
    __tablename__ = 'i_service'
    id = Column(Integer, primary_key=True)
    uuid = Column(String(36))

    servicename = Column(String(255), unique=True)
    hostname = Column(String(36))
    forihostid = Column(Integer, ForeignKey('i_host.id',
                                            ondelete='CASCADE'))

    activity = Column(String(255), default="unknown")
    state = Column(String(255), default="unknown")
    reason = Column(JSONEncodedDict)


class service(Base):
    __tablename__ = 'services'
    id = Column(Integer, primary_key=True)

    name = Column(String(255))
    desired_state = Column(String(255))
    state = Column(String(255))
    status = Column(String(255))


# sm_service_domain_members
class sm_sdm(Base):
    __tablename__ = 'service_domain_members'

    id = Column(Integer, primary_key=True)
    name = Column(String(255))
    service_group_name = Column(String(255))
    redundancy_model = Column(String(255))  # sm_types.h
    n_active = Column(Integer)
    m_standby = Column(Integer)


# sm_service_domain_assignments
class sm_sda(Base):
    __tablename__ = 'service_domain_assignments'
    id = Column(Integer, primary_key=True)
    uuid = Column(String(36))

    name = Column(String(255))
    node_name = Column(String(255))  # hostname
    service_group_name = Column(String(255))
    desired_state = Column(String(255))  # sm_types.h
    state = Column(String(255))  # sm_types.h
    status = Column(String(255))
    condition = Column(String(255))


class sm_node(Base):
    __tablename__ = 'nodes'
    id = Column(Integer, primary_key=True)

    name = Column(String(255))
    administrative_state = Column(String(255))  # sm_types.h
    operational_state = Column(String(255))
    availability_status = Column(String(255))
    ready_state = Column(String(255))


class sm_service_group_member(Base):
    __tablename__ = 'service_group_members'

    id = Column(Integer, primary_key=True)
    provisioned = Column(String(255))
    name = Column(String(255))
    service_name = Column(String(255))
    service_failure_impact = Column(String(255))
