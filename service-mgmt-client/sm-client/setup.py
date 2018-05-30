#
# Copyright (c) 2013-2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

import setuptools

setuptools.setup(
    name='sm_client',
    description='Service Management Client and CLI',
    version='1.0.0',
    license='Apache-2.0',
    packages=['sm_client', 'sm_client.v1', 'sm_client.openstack',
              'sm_client.openstack.common',
              'sm_client.openstack.common.config',
              'sm_client.openstack.common.rootwrap',
              'sm_client.common'],
    # entry_points={
    #      'console_scripts': [
    #          'smc = sm_client.shell:main'
    #      ]}
)
