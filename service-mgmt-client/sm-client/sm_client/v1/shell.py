#
# Copyright (c) 2013-2018 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

#


from sm_client.common import utils
from sm_client.v1 import smc_service_shell
from sm_client.v1 import smc_service_node_shell
from sm_client.v1 import smc_servicegroup_shell

COMMAND_MODULES = [
    smc_service_shell,
    smc_service_node_shell,
    smc_servicegroup_shell,
 ]


def enhance_parser(parser, subparsers, cmd_mapper):
    '''Take a basic (nonversioned) parser and enhance it with
    commands and options specific for this version of API.

    :param parser: top level parser :param subparsers: top level
        parser's subparsers collection where subcommands will go
    '''
    for command_module in COMMAND_MODULES:
        utils.define_commands_from_module(subparsers, command_module,
                                          cmd_mapper)
