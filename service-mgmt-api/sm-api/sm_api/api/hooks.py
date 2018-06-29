#
# Copyright (c) 2014-2018 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

"""
Hooks
"""

import sqlite3
from pecan import hooks

from sm_api.common import context
from sm_api.common import utils

from sm_api.common import config
from sm_api.db import api as dbapi
from sm_api.openstack.common import policy


class ConfigHook(hooks.PecanHook):
    def __init__(self):
        super(ConfigHook, self).__init__()

    def before(self, state):
        state.request.config = config.CONF


class DatabaseHook(hooks.PecanHook):
    # JKUNG def __init__(self):
    #     super(DatabaseHook, self).__init__()
    #     self.database = sqlite3.connect(config.CONF['database']['database'])

    def before(self, state):
        # state.request.database = self.database
        state.request.dbapi = dbapi.get_instance()

    # def __del__(self):
    #     self.database.close()


class ContextHook(hooks.PecanHook):
    """Configures a request context and attaches it to the request.

    The following HTTP request headers are used:

    X-User-Id or X-User:
        Used for context.user_id.

    X-Tenant-Id or X-Tenant:
        Used for context.tenant.

    X-Auth-Token:
        Used for context.auth_token.

    X-Roles:
        Used for setting context.is_admin flag to either True or False.
        The flag is set to True, if X-Roles contains either an administrator
        or admin substring. Otherwise it is set to False.

    """
    def __init__(self, public_api_routes):
        self.public_api_routes = public_api_routes
        super(ContextHook, self).__init__()

    def before(self, state):
        user_id = state.request.headers.get('X-User-Id')
        user_id = state.request.headers.get('X-User', user_id)
        tenant = state.request.headers.get('X-Tenant-Id')
        tenant = state.request.headers.get('X-Tenant', tenant)
        domain_id = state.request.headers.get('X-User-Domain-Id')
        domain_name = state.request.headers.get('X-User-Domain-Name')
        auth_token = state.request.headers.get('X-Auth-Token', None)
        creds = {'roles': state.request.headers.get('X-Roles', '').split(',')}

        is_admin = policy.check('admin', state.request.headers, creds)

        path = utils.safe_rstrip(state.request.path, '/')
        is_public_api = path in self.public_api_routes

        state.request.context = context.RequestContext(
            auth_token=auth_token,
            user=user_id,
            tenant=tenant,
            domain_id=domain_id,
            domain_name=domain_name,
            is_admin=is_admin,
            is_public_api=is_public_api)


class RPCHook(hooks.PecanHook):
    """Attach the rpcapi object to the request so controllers can get to it."""

    def before(self, state):
        state.request.rpcapi = rpcapi.ConductorAPI()
