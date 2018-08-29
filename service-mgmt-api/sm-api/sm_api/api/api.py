#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
import os
import sys
import argparse
import ConfigParser
import eventlet
from wsgiref import simple_server

from sm_api.common import config
from sm_api.common import log
from sm_api import app

os.environ['EVENTLET_NO_GREENDNS'] = 'yes'
eventlet.monkey_patch(os=False)


def get_handler_cls():
    cls = simple_server.WSGIRequestHandler

    # old-style class doesn't support super
    class MyHandler(cls, object):
        def address_string(self):
            # In the future, we could provide a config option to allow reverse DNS lookup
            return self.client_address[0]

    return MyHandler


def main():
    try:
        parser = argparse.ArgumentParser()
        parser.add_argument('-c', '--config', required=True,
                            help='configuration file')
        args = parser.parse_args()

        config.load(args.config)

        if not config.CONF:
            print ("Error: configuration not available.")
            sys.exit(-1)

        log.configure(config.CONF)

        wsgi = simple_server.make_server('0.0.0.0', 7777, app.Application(),
                                         handler_class=get_handler_cls())
        wsgi.serve_forever()

    except ConfigParser.NoOptionError as e:
        print (e)
        sys.exit(-2)

    except ConfigParser.NoSectionError as e:
        print (e)
        sys.exit(-3)

    except KeyboardInterrupt:
        sys.exit()

    except Exception as e:
        print (e)
        sys.exit(-4)

main()
