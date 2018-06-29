# Copyright 2012 OpenStack LLC.
# All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License"); you may
#    not use this file except in compliance with the License. You may obtain
#    a copy of the License at
#
#         http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
#    WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
#    License for the specific language governing permissions and limitations
#    under the License.
#
# Copyright (c) 2013-2018 Wind River Systems, Inc.
#


from sm_client.common import http
from sm_client.v1 import sm_sda
from sm_client.v1 import sm_nodes
from sm_client.v1 import smc_service
from sm_client.v1 import smc_service_node
from sm_client.v1 import smc_servicegroup


class Client(http.HTTPClient):
    """Client for the Smc v1 API.

    :param string endpoint: A user-supplied endpoint URL for the smc
                            service.
    :param function token: Provides token for authentication.
    :param integer timeout: Allows customization of the timeout for client
                            http requests. (optional)
    """

    def __init__(self, *args, **kwargs):
        """Initialize a new client for the Smc v1 API."""
        super(Client, self).__init__(*args, **kwargs)
        self.sm_sda = sm_sda.Sm_SdaManager(self)
        self.sm_nodes = sm_nodes.Sm_NodesManager(self)
        self.smc_service = smc_service.SmcServiceManager(self)
        self.smc_service_node = smc_service_node.SmcNodeManager(self)
        self.smc_servicegroup = smc_servicegroup.SmcServiceGroupManager(self)
