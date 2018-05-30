#!/usr/bin/env python
# vim: tabstop=4 shiftwidth=4 softtabstop=4

# Copyright 2013 Red Hat, Inc.
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
# Copyright (c) 2013-2014 Wind River Systems, Inc.
#


import pecan
from pecan import rest
import wsme
from wsme import types as wtypes
import wsmeext.pecan as wsme_pecan

from sm_api.api.controllers.v1 import base
from sm_api.api.controllers.v1 import smc_api
from sm_api.openstack.common import log

LOG = log.getLogger(__name__)

ERR_CODE_SUCCESS = "0"
ERR_CODE_HOST_NOT_FOUND = "-1000"
ERR_CODE_ACTION_FAILED = "-1001"
ERR_CODE_NO_HOST_TO_SWACT_TO = "-1002"

SM_NODE_STATE_UNKNOWN = "unknown"
SM_NODE_ADMIN_LOCKED = "locked"
SM_NODE_ADMIN_UNLOCKED = "unlocked"
SM_NODE_OPER_ENABLED = "enabled"
SM_NODE_OPER_DISABLED = "disabled"
SM_NODE_AVAIL_AVAILABLE = "available"
SM_NODE_AVAIL_DEGRADED = "degraded"
SM_NODE_AVAIL_FAILED = "failed"

# sm_types.c
SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_NIL = "nil"
SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_UNKNOWN = "unknown"
SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_NONE = "none"
SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N = "N"
SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_PLUS_M = "N + M"
SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_TO_1 = "N to 1"
SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_TO_N = "N to N"

# sm_types.c
SM_SERVICE_GROUP_STATE_NIL = "nil"
SM_SERVICE_GROUP_STATE_NA = "not-applicable"
SM_SERVICE_GROUP_STATE_INITIAL = "initial"
SM_SERVICE_GROUP_STATE_UNKNOWN = "unknown"
SM_SERVICE_GROUP_STATE_STANDBY = "standby"
SM_SERVICE_GROUP_STATE_GO_STANDBY = "go-standby"
SM_SERVICE_GROUP_STATE_GO_ACTIVE = "go-active"
SM_SERVICE_GROUP_STATE_ACTIVE = "active"
SM_SERVICE_GROUP_STATE_DISABLING = "disabling"
SM_SERVICE_GROUP_STATE_DISABLED = "disabled"
SM_SERVICE_GROUP_STATE_SHUTDOWN = "shutdown"

# sm_types.c
SM_SERVICE_GROUP_STATUS_NIL = "nil"
SM_SERVICE_GROUP_STATUS_NONE = ""
SM_SERVICE_GROUP_STATUS_WARN = "warn"
SM_SERVICE_GROUP_STATUS_DEGRADED = "degraded"
SM_SERVICE_GROUP_STATUS_FAILED = "failed"

# sm_types.c
SM_SERVICE_GROUP_CONDITION_NIL = "nil"
SM_SERVICE_GROUP_CONDITION_NONE = ""
SM_SERVICE_GROUP_CONDITION_DATA_INCONSISTENT = "data-inconsistent"
SM_SERVICE_GROUP_CONDITION_DATA_OUTDATED = "data-outdated"
SM_SERVICE_GROUP_CONDITION_DATA_CONSISTENT = "data-consistent"
SM_SERVICE_GROUP_CONDITION_DATA_SYNC = "data-syncing"
SM_SERVICE_GROUP_CONDITION_DATA_STANDALONE = "data-standalone"
SM_SERVICE_GROUP_CONDITION_RECOVERY_FAILURE = "recovery-failure"
SM_SERVICE_GROUP_CONDITION_ACTION_FAILURE = "action-failure"
SM_SERVICE_GROUP_CONDITION_FATAL_FAILURE = "fatal-failure"


class ServiceNodeCommand(base.APIBase):
    origin = wtypes.text
    action = wtypes.text  # swact | swact-force | unlock | lock | event
    admin = wtypes.text   # locked | unlocked
    oper = wtypes.text    # enabled | disabled
    avail = wtypes.text   # none | ...


class ServiceNodeCommandResult(base.APIBase):
    # Origin and Host Information
    origin = wtypes.text  # e.g. "mtce" or "sm"
    hostname = wtypes.text
    # Command
    action = wtypes.text
    admin = wtypes.text
    oper = wtypes.text
    avail = wtypes.text
    # Result
    error_code = wtypes.text
    error_details = wtypes.text


class ServiceNode(base.APIBase):
    origin = wtypes.text
    hostname = wtypes.text
    admin = wtypes.text
    oper = wtypes.text
    avail = wtypes.text
    active_services = wtypes.text
    swactable_services = wtypes.text


class ServiceNodeController(rest.RestController):

    def __init__(self, from_isystem=False):
        self._seqno = 0

    def _seqno_incr_get(self):
        self._seqno += 1
        return self._seqno

    def _seqno_get(self):
        return self._seqno

    def _get_current_sm_sdas(self):
        sm_sdas = pecan.request.dbapi.sm_sda_get_list()
        for sm in sm_sdas:
            LOG.debug("sm-api sm_sdas= %s" % sm.as_dict())

        return sm_sdas

    def _sm_sdm_get(self, server, service_group_name):
        return pecan.request.dbapi.sm_sdm_get(server, service_group_name)

    def _smc_node_exists(self, hostname):
        # check whether hostname exists in nodes table
        node_exists = False
        sm_nodes = pecan.request.dbapi.sm_node_get_by_name(hostname)
        for sm_node in sm_nodes:
            node_exists = True

        return node_exists

    def _get_sm_node_state(self, hostname):
        sm_nodes = pecan.request.dbapi.sm_node_get_by_name(hostname)

        # default values
        node_state = {'hostname': hostname,
                      'admin': SM_NODE_STATE_UNKNOWN,
                      'oper': SM_NODE_STATE_UNKNOWN,
                      'avail': SM_NODE_STATE_UNKNOWN}

        for sm_node in sm_nodes:
            node_state = {'hostname': hostname,
                          'admin': sm_node.administrative_state,
                          'oper': sm_node.operational_state,
                          'avail': sm_node.availability_status}
            break

        LOG.debug("sm-api get_sm_node_state hostname: %s" % (node_state))
        return node_state

    def _have_active_sm_services(self, hostname, sm_sdas):
        # check db service_domain_assignments for any "active"
        # in either state or desired state:
        active_sm_services = False

        # active: current or transition state
        active_attr_list = [SM_SERVICE_GROUP_STATE_ACTIVE,
                            SM_SERVICE_GROUP_STATE_GO_ACTIVE,
                            SM_SERVICE_GROUP_STATE_GO_STANDBY,
                            SM_SERVICE_GROUP_STATE_DISABLING,
                            SM_SERVICE_GROUP_STATE_UNKNOWN]

        for sm_sda in sm_sdas:
            if sm_sda.node_name == hostname:
                for aa in active_attr_list:
                    if sm_sda.state == aa or sm_sda.desired_state == aa:
                        active_sm_services = True
                        LOG.debug("sm-api have_active_sm_services True")
                        return active_sm_services

        LOG.debug("sm-api have_active_sm_services: False")
        return active_sm_services

    def _have_swactable_sm_services(self, hostname, sm_sdas):
        # check db service_domain_assignments for any "active"
        # in either state or desired state:
        swactable_sm_services = False

        # active: current or transition state
        active_attr_list = [SM_SERVICE_GROUP_STATE_ACTIVE,
                            SM_SERVICE_GROUP_STATE_GO_ACTIVE,
                            SM_SERVICE_GROUP_STATE_GO_STANDBY,
                            SM_SERVICE_GROUP_STATE_DISABLING,
                            SM_SERVICE_GROUP_STATE_UNKNOWN]

        for sm_sda in sm_sdas:
            if sm_sda.node_name == hostname:
                for aa in active_attr_list:
                    if sm_sda.state == aa or sm_sda.desired_state == aa:
                        sdm = self._sm_sdm_get(sm_sda.name,
                                               sm_sda.service_group_name)
                        if sdm.redundancy_model == \
                           SM_SERVICE_DOMAIN_MEMBER_REDUNDANCY_MODEL_N_PLUS_M:
                            swactable_sm_services = True
                            LOG.debug("sm-api have_swactable_sm_services True")
                            return swactable_sm_services

        LOG.debug("sm-api have_active_sm_services: False")
        return swactable_sm_services

    def _swact_pre_check(self, hostname):
        # run pre-swact checks, verify that services are in the right state
        # to accept service
        have_destination = False
        check_result = None

        sm_sdas = pecan.request.dbapi.sm_sda_get_list(None, None,
                                                      sort_key='name',
                                                      sort_dir='asc')

        origin_state = self._collect_svc_state(sm_sdas, hostname)

        for sm_sda in sm_sdas:
            if sm_sda.node_name != hostname:
                have_destination = True

                # Verify that target host state is unlocked-enabled
                node_state = self._get_sm_node_state(sm_sda.node_name)
                if SM_NODE_ADMIN_LOCKED == node_state['admin']:
                    check_result = ("%s is not ready to take service, "
                                    "%s is locked"
                                    % (sm_sda.node_name, sm_sda.node_name))
                    break

                if SM_NODE_OPER_DISABLED == node_state['oper']:
                    check_result = ("%s is not ready to take service, "
                                    "%s is disabled"
                                    % (sm_sda.node_name, sm_sda.node_name))
                    break

                # Verify that 
                # all the services are in the standby or active
                #   state on the other host
                # or service only provisioned in the other host
                # or service state are the same on both hosts 
                if SM_SERVICE_GROUP_STATE_ACTIVE != sm_sda.state \
                        and SM_SERVICE_GROUP_STATE_STANDBY != sm_sda.state \
                        and origin_state.has_key(sm_sda.service_group_name) \
                        and origin_state[sm_sda.service_group_name] != sm_sda.state:
                    check_result = (
                        "%s on %s is not ready to take service, "
                        "service not in the active or standby "
                        "state" % (sm_sda.service_group_name,
                        sm_sda.node_name))
                    break

                # Verify that all the services are in the desired state on
                # the other host
                if sm_sda.desired_state != sm_sda.state:
                    check_result = ("%s on %s is not ready to take service, "
                                    "services transitioning state"
                                    % (sm_sda.service_group_name,
                                       sm_sda.node_name))
                    break

                # Verify that all the services are ready to accept service
                # i.e. not failed or syncing data
                if SM_SERVICE_GROUP_STATUS_FAILED == sm_sda.status:
                    check_result = ("%s on %s is not ready to take service, "
                                    "service is failed"
                                    % (sm_sda.service_group_name,
                                       sm_sda.node_name))
                    break

                elif SM_SERVICE_GROUP_STATUS_DEGRADED == sm_sda.status:
                    degraded_conditions \
                        = [SM_SERVICE_GROUP_CONDITION_DATA_INCONSISTENT,
                           SM_SERVICE_GROUP_CONDITION_DATA_OUTDATED,
                           SM_SERVICE_GROUP_CONDITION_DATA_CONSISTENT,
                           SM_SERVICE_GROUP_CONDITION_DATA_STANDALONE]

                    if sm_sda.condition == SM_SERVICE_GROUP_CONDITION_DATA_SYNC:
                        check_result = ("%s on %s is not ready to take "
                                        "service, service is syncing data"
                                        % (sm_sda.service_group_name,
                                           sm_sda.node_name))
                        break

                    elif sm_sda.condition in degraded_conditions:
                        check_result = ("%s on %s is not ready to take "
                                        "service, service is degraded, %s"
                                        % (sm_sda.service_group_name,
                                           sm_sda.node_name, sm_sda.condition))
                        break
                    else:
                        check_result = ("%s on %s is not ready to take "
                                        "service, service is degraded"
                                        % (sm_sda.service_group_name,
                                           sm_sda.node_name))
                        break

        if check_result is None and not have_destination:
            check_result = "no peer available"

        if check_result is not None:
            LOG.info("swact pre-check failed host %s, reason=%s."
                     % (hostname, check_result))

        return check_result

    @staticmethod
    def _collect_svc_state(sm_sdas, hostname):
        sm_state_ht = {}
        for sm_sda in sm_sdas:
            if sm_sda.node_name == hostname:
                sm_state_ht[sm_sda.service_group_name] = sm_sda.state
        LOG.info("%s" % sm_state_ht)
        return sm_state_ht

    def _do_modify_command(self, hostname, command):

        if command.action == smc_api.SM_NODE_ACTION_SWACT_PRE_CHECK or \
           command.action == smc_api.SM_NODE_ACTION_SWACT:
            check_result = self._swact_pre_check(hostname)
            if check_result is not None:
                result = ServiceNodeCommandResult(
                    origin="sm", hostname=hostname, action=command.action,
                    admin=command.admin, oper=command.oper,
                    avail=command.avail, error_code=ERR_CODE_ACTION_FAILED,
                    error_details=check_result)

                if command.action == smc_api.SM_NODE_ACTION_SWACT_PRE_CHECK:
                    return wsme.api.Response(result, status_code=200)
                return wsme.api.Response(result, status_code=400)

            elif command.action == smc_api.SM_NODE_ACTION_SWACT_PRE_CHECK:
                result = ServiceNodeCommandResult(
                    origin="sm", hostname=hostname, action=command.action,
                    admin=command.admin, oper=command.oper,
                    avail=command.avail, error_code=ERR_CODE_SUCCESS,
                    error_details=check_result)
                return wsme.api.Response(result, status_code=200)

        if command.action == smc_api.SM_NODE_ACTION_UNLOCK or \
           command.action == smc_api.SM_NODE_ACTION_LOCK or \
           command.action == smc_api.SM_NODE_ACTION_SWACT or \
           command.action == smc_api.SM_NODE_ACTION_SWACT_FORCE or \
           command.action == smc_api.SM_NODE_ACTION_EVENT:

            sm_ack_dict = smc_api.sm_api_set_node_state(command.origin,
                                                        hostname,
                                                        command.action,
                                                        command.admin,
                                                        command.avail,
                                                        command.oper,
                                                        self._seqno_incr_get())

            ack_admin = sm_ack_dict['SM_API_MSG_NODE_ADMIN'].lower()
            ack_oper = sm_ack_dict['SM_API_MSG_NODE_OPER'].lower()
            ack_avail = sm_ack_dict['SM_API_MSG_NODE_AVAIL'].lower()

            LOG.debug("sm-api _do_modify_command sm_ack_dict: %s ACK admin: "
                      "%s oper: %s avail: %s." % (sm_ack_dict, ack_admin,
                                                  ack_oper, ack_avail))

            # loose check on admin and oper only
            if (command.admin == ack_admin) and (command.oper == ack_oper):
                return ServiceNodeCommandResult(
                              origin=sm_ack_dict['SM_API_MSG_ORIGIN'],
                              hostname=sm_ack_dict['SM_API_MSG_NODE_NAME'],
                              action=sm_ack_dict['SM_API_MSG_NODE_ACTION'],
                              admin=ack_admin,
                              oper=ack_oper,
                              avail=ack_avail,
                              error_code=ERR_CODE_SUCCESS,
                              error_msg="success")
            else:
                result = ServiceNodeCommandResult(
                                origin="sm",
                                hostname=hostname,
                                action=sm_ack_dict['SM_API_MSG_NODE_ACTION'],
                                admin=ack_admin,
                                oper=ack_oper,
                                avail=ack_avail,
                                error_code=ERR_CODE_ACTION_FAILED,
                                error_details="action failed")

                return wsme.api.Response(result, status_code=500)
        else:
            raise wsme.exc.InvalidInput('action', command.action, "unknown")

    @wsme_pecan.wsexpose(ServiceNode, unicode)
    def get_one(self, hostname):

        try:
            data = self._get_sm_node_state(hostname)
        except:
            LOG.exception("No entry in database for %s:" % hostname)
            return ServiceNode(origin="sm",
                               hostname=hostname,
                               admin=SM_NODE_STATE_UNKNOWN,
                               oper=SM_NODE_STATE_UNKNOWN,
                               avail=SM_NODE_STATE_UNKNOWN,
                               active_services="unknown",
                               swactable_services="unknown")

        sm_sdas = self._get_current_sm_sdas()

        if self._have_active_sm_services(hostname, sm_sdas):
            active_services = "yes"
        else:
            active_services = "no"

        if self._have_swactable_sm_services(hostname, sm_sdas):
            swactable_services = "yes"
        else:
            swactable_services = "no"

        return ServiceNode(origin="sm",
                           hostname=data['hostname'],
                           admin=data['admin'],
                           oper=data['oper'],
                           avail=data['avail'],
                           active_services=active_services,
                           swactable_services=swactable_services)

    @wsme_pecan.wsexpose(ServiceNodeCommandResult, unicode,
                         body=ServiceNodeCommand)
    def patch(self, hostname, command):

        if command.origin != "mtce" and command.origin != "sysinv":
            LOG.warn("sm-api unexpected origin: %s.  Continuing."
                     % command.origin)

        return self._do_modify_command(hostname, command)
