#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

"""
Application
"""

from oslo_config import cfg
import pecan

from sm_api.api import config
from sm_api.api import hooks

from sm_api.api import acl
from sm_api.api import middleware


auth_opts = [
    cfg.StrOpt('auth_strategy',
        default='noauth',
        help='Method to use for auth: noauth or keystone.'),
    ]

CONF = cfg.CONF
CONF.register_opts(auth_opts)


def get_pecan_config():
    filename = config.__file__.replace('.pyc', '.py')
    return pecan.configuration.conf_from_file(filename)


def create_app():
    pecan_conf = get_pecan_config()
    app_hooks = [hooks.ConfigHook(),
                 hooks.DatabaseHook()]

    pecan.configuration.set_config(dict(pecan_conf), overwrite=True)

    app = pecan.make_app(
        pecan_conf.app.root,
        static_root=pecan_conf.app.static_root,
        debug=False,
        force_canonical=getattr(pecan_conf.app, 'force_canonical', True),
        hooks=app_hooks
    )

    return app


def setup_app(pecan_config=None, extra_hooks=None):
    app_hooks = [hooks.ConfigHook(),
                 hooks.DatabaseHook(),
                 hooks.ContextHook(pecan_config.app.acl_public_routes),
                 ]
    #  hooks.RPCHook()
    if extra_hooks:
        app_hooks.extend(extra_hooks)

    if not pecan_config:
        pecan_config = get_pecan_config()

    if pecan_config.app.enable_acl:
        app_hooks.append(acl.AdminAuthHook())

    pecan.configuration.set_config(dict(pecan_config), overwrite=True)

    app = pecan.make_app(
        pecan_config.app.root,
        static_root=pecan_config.app.static_root,
        debug=CONF.debug,
        force_canonical=getattr(pecan_config.app, 'force_canonical', True),
        hooks=app_hooks,
        wrap_app=middleware.ParsableErrorMiddleware,
    )

    if pecan_config.app.enable_acl:
        return acl.install(app, cfg.CONF, pecan_config.app.acl_public_routes)

    return app


class Application(object):
    def __init__(self):
        self.v1 = create_app()

    @classmethod
    def unsupported_version(cls, start_response):
        start_response('404 Not Found', [])
        return []

    def __call__(self, environ, start_response):
        if environ['PATH_INFO'].startswith("/v1/"):
            return self.v1(environ, start_response)

        return Application.unsupported_version(start_response)


class VersionSelectorApplication(object):
    def __init__(self):
        pc = get_pecan_config()
        pc.app.enable_acl = (CONF.auth_strategy == 'keystone')
        self.v1 = setup_app(pecan_config=pc)

    def __call__(self, environ, start_response):
        return self.v1(environ, start_response)
