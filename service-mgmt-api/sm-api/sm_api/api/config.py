#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

# Server Configuration
server = {'host': '0.0.0.0', 'port': '7777'}

# Pecan Application Configurations
app = {'root': 'sm_api.api.controllers.root.RootController',
       'modules': ['sm_api'],
       'static_root': '',
       'debug': False,
       'enable_acl': False,
       'acl_public_routes': ['/', '/v1']
      }
