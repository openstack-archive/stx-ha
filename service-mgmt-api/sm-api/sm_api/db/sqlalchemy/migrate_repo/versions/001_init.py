#
# Copyright (c) 2013-2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

# vim: tabstop=4 shiftwidth=4 softtabstop=4

#

from sqlalchemy import Column, MetaData, String, Table, UniqueConstraint
from sqlalchemy import Integer, Text, ForeignKey, DateTime

ENGINE = 'InnoDB'
CHARSET = 'utf8'


def upgrade(migrate_engine):
    meta = MetaData()
    meta.bind = migrate_engine

    i_ServiceGroup = Table(
        'i_servicegroup',
        meta,
        Column('created_at', DateTime),
        Column('updated_at', DateTime),
        Column('deleted_at', DateTime),

        Column('id', Integer, primary_key=True, nullable=False),
        Column('uuid', String(36), unique=True),

        Column('servicename', String(255), unique=True),
        Column('state', String(255), default="unknown"),

        mysql_engine=ENGINE,
        mysql_charset=CHARSET,
    )
    i_ServiceGroup.create()

    i_Service = Table(
        'i_service',
        meta,
        Column('created_at', DateTime),
        Column('updated_at', DateTime),
        Column('deleted_at', DateTime),

        Column('id', Integer, primary_key=True, nullable=False),  # autoincr
        Column('uuid', String(36), unique=True),

        Column('servicename', String(255)),
        Column('hostname', String(255)),
        Column('forihostid', Integer,
               ForeignKey('i_host.id', ondelete='CASCADE')),

        Column('activity', String),  # active/standby
        Column('state', String),
        Column('reason', Text),  # JSON encodedlist of string

        UniqueConstraint('servicename', 'hostname',
                         name='u_servicehost'),

        mysql_engine=ENGINE,
        mysql_charset=CHARSET,
    )
    i_Service.create()


def downgrade(migrate_engine):
    raise NotImplementedError('Downgrade from Initial is unsupported.')

    # t = Table('i_disk', meta, autoload=True)
    # t.drop()
