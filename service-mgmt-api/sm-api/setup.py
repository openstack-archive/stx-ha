#
# Copyright (c) 2013-2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
import setuptools

setuptools.setup(
    name='sm_api',
    description='Service Management API',
    version='1.0.0',
    license='Apache-2.0',
    packages=['sm_api', 'sm_api.common', 'sm_api.db', 'sm_api.objects',
              'sm_api.api', 'sm_api.api.controllers', 'sm_api.api.middleware',
              'sm_api.api.controllers.v1', 'sm_api.cmd',
              'sm_api.db.sqlalchemy',
              'sm_api.db.sqlalchemy.migrate_repo',
              'sm_api.db.sqlalchemy.migrate_repo.versions',
              'sm_api.openstack', 'sm_api.openstack.common',
              'sm_api.openstack.common.db',
              'sm_api.openstack.common.db.sqlalchemy',
              'sm_api.openstack.common.rootwrap',
              'sm_api.openstack.common.rpc',
              'sm_api.openstack.common.notifier',
              'sm_api.openstack.common.config',
              'sm_api.openstack.common.fixture'],
    entry_points={
         'console_scripts': [
             'sm-api = sm_api.cmd.api:main'
         ]}
)
