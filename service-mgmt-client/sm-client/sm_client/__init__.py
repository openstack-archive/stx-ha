#
# Copyright (c) 2013-2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

# All Rights Reserved.
#

# import pbr.version

try:
    import sm_client.client
    Client = sm_client.client.Client
except ImportError:
    import warnings
    warnings.warn("Could not import sm_client.client", ImportWarning)


__version__ = "1.0"
# __version__ = pbr.version.VersionInfo('python-sm_client').version_string()
