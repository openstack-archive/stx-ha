#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

"""
Configuration
"""

import ConfigParser

CONF = None


class Config(ConfigParser.ConfigParser):
    def as_dict(self):
        d = dict(self._sections)
        for key in d:
            d[key] = dict(self._defaults, **d[key])
            d[key].pop('__name__', None)
        return d


def load(config_file):
    """ Load the power management configuration

        Parameters: configuration file
    """
    global CONF

    config = Config()
    config.read(config_file)
    CONF = config.as_dict()
