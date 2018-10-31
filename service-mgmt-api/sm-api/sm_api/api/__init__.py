#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
from oslo_config import cfg

API_SERVICE_OPTS = [
    cfg.StrOpt('sm_api_bind_ip',
               default='0.0.0.0',
               help='IP for the Service Management API server to bind to',
               ),
    cfg.IntOpt('sm_api_port',
               default=7777,
               help='The port for the Service Management API server',
               ),
    cfg.IntOpt('api_limit_max',
               default=1000,
               help='the maximum number of items returned in a single '
               'response from a collection resource'),
]

CONF = cfg.CONF
opt_group = cfg.OptGroup(name='api',
                         title='Options for the sm-api service')
CONF.register_group(opt_group)
CONF.register_opts(API_SERVICE_OPTS)
