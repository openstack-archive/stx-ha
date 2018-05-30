#
# Copyright (c) 2013-2014, 2016 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

import setuptools

setuptools.setup(
    name='sm_tools',
    description='Service Management Tools',
    version='1.0.0',
    license='Apache-2.0',
    packages=['sm_tools'],
    entry_points={
        'console_scripts': [
            'sm-configure = sm_tools.sm_configure:main',
            'sm-provision = sm_tools.sm_provision:main',
            'sm-deprovision = sm_tools.sm_provision:main ',
            'sm-dump = sm_tools.sm_dump:main',
            'sm-query = sm_tools.sm_query:main',
            'sm-patch = sm_tools.sm_patch:main',
            'sm-manage = sm_tools.sm_action:main',
            'sm-unmanage = sm_tools.sm_action:main ',
            'sm-restart-safe = sm_tools.sm_action:main ',
            'sm-restart = sm_tools.sm_action:main ',
            'sm-iface-state = sm_tools.sm_domain_interface_set_state:main '
        ]}
)
