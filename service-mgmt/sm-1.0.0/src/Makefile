#
# Copyright (c) 2014-2018 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
INCLUDES =-I$(STAGING_DIR)/usr/include/glib-2.0
INCLUDES+=-I$(STAGING_DIR)/usr/lib64/glib-2.0/include

SRCS=main.c
SRCS+=sm_process.c
SRCS+=sm_process_death.c
SRCS+=sm_heartbeat.c
SRCS+=sm_heartbeat_msg.c
SRCS+=sm_heartbeat_thread.c
SRCS+=sm_log.c
SRCS+=sm_log_thread.c
SRCS+=sm_alarm.c
SRCS+=sm_alarm_thread.c
SRCS+=sm_troubleshoot.c
SRCS+=sm_api.c
SRCS+=sm_notify_api.c
SRCS+=sm_msg.c
SRCS+=sm_node_api.cpp
SRCS+=sm_node_fsm.c
SRCS+=sm_node_unknown_state.c
SRCS+=sm_node_enabled_state.c
SRCS+=sm_node_disabled_state.c
SRCS+=sm_service_domain_table.c
SRCS+=sm_service_domain_member_table.c
SRCS+=sm_service_domain_interface_table.c
SRCS+=sm_service_domain_neighbor_table.c
SRCS+=sm_service_domain_assignment_table.c
SRCS+=sm_service_domain_api.c
SRCS+=sm_service_domain_utils.c
SRCS+=sm_service_domain_fsm.c
SRCS+=sm_service_domain_initial_state.c
SRCS+=sm_service_domain_waiting_state.c
SRCS+=sm_service_domain_other_state.c
SRCS+=sm_service_domain_backup_state.c
SRCS+=sm_service_domain_leader_state.c
SRCS+=sm_service_domain_interface_api.c
SRCS+=sm_service_domain_interface_fsm.c
SRCS+=sm_service_domain_interface_unknown_state.c
SRCS+=sm_service_domain_interface_enabled_state.c
SRCS+=sm_service_domain_interface_disabled_state.c
SRCS+=sm_service_domain_neighbor_fsm.c
SRCS+=sm_service_domain_neighbor_down_state.c
SRCS+=sm_service_domain_neighbor_exchange_start_state.c
SRCS+=sm_service_domain_neighbor_exchange_state.c
SRCS+=sm_service_domain_neighbor_full_state.c
SRCS+=sm_service_domain_scheduler.c
SRCS+=sm_service_domain_filter.c
SRCS+=sm_service_domain_weight.c
SRCS+=sm_service_group_table.c
SRCS+=sm_service_group_member_table.c
SRCS+=sm_service_group_api.c
SRCS+=sm_service_group_health.c
SRCS+=sm_service_group_engine.c
SRCS+=sm_service_group_fsm.c
SRCS+=sm_service_group_initial_state.c
SRCS+=sm_service_group_active_state.c
SRCS+=sm_service_group_go_active_state.c
SRCS+=sm_service_group_go_standby_state.c
SRCS+=sm_service_group_standby_state.c
SRCS+=sm_service_group_disabling_state.c
SRCS+=sm_service_group_disabled_state.c
SRCS+=sm_service_group_shutdown_state.c
SRCS+=sm_service_group_enable.c
SRCS+=sm_service_group_go_active.c
SRCS+=sm_service_group_go_standby.c
SRCS+=sm_service_group_disable.c
SRCS+=sm_service_group_audit.c
SRCS+=sm_service_group_notification.c
SRCS+=sm_service_table.c
SRCS+=sm_service_dependency_table.c
SRCS+=sm_service_action_table.c
SRCS+=sm_service_action_result_table.c
SRCS+=sm_service_api.c
SRCS+=sm_service_dependency.c
SRCS+=sm_service_engine.c
SRCS+=sm_service_fsm.c
SRCS+=sm_service_initial_state.c
SRCS+=sm_service_unknown_state.c
SRCS+=sm_service_enabled_active_state.c
SRCS+=sm_service_enabled_go_active_state.c
SRCS+=sm_service_enabled_go_standby_state.c
SRCS+=sm_service_enabled_standby_state.c
SRCS+=sm_service_enabling_state.c
SRCS+=sm_service_enabling_throttle_state.c
SRCS+=sm_service_disabling_state.c
SRCS+=sm_service_disabled_state.c
SRCS+=sm_service_shutdown_state.c
SRCS+=sm_service_enable.c
SRCS+=sm_service_go_active.c
SRCS+=sm_service_go_standby.c
SRCS+=sm_service_disable.c
SRCS+=sm_service_audit.c
SRCS+=sm_service_action.c
SRCS+=sm_service_heartbeat.c
SRCS+=sm_service_heartbeat_api.c
SRCS+=sm_service_heartbeat_thread.c
SRCS+=sm_main_event_handler.c
SRCS+=fm_api_wrapper.c
SRCS+=sm_failover.c
SRCS+=sm_failover_thread.c
SRCS+=sm_swact_state.c
SRCS+=sm_worker_thread.cpp
SRCS+=sm_task_affining_thread.c
SRCS+=sm_node_swact_monitor.cpp
SRCS+=sm_failover_ss.c
SRCS+=sm_service_domain_interface_not_in_use_state.c
SRCS+=sm_configuration_table.c
SRCS+=sm_failover_utils.c

OBJS= $(SRCS:.c=.o)
CCFLAGS= -g -O2 -Wall -Werror -Wformat  -std=c++11
EXTRACCFLAGS= -D__STDC_FORMAT_MACROS
LDLIBS= -lsqlite3 -lglib-2.0 -luuid -lpthread -lrt -lsm_common -lsm_db -lfmcommon -ljson-c -lcrypto -lssl
LDFLAGS = -rdynamic

.c.o:
	$(CXX) $(INCLUDES) $(CCFLAGS) $(EXTRACCFLAGS) -c $< -o $@

build: $(OBJS)
	$(CXX) $(CCFLAGS) $(EXTRACCFLAGS) $(OBJS) ${LDFLAGS} $(LDLIBS) -o sm

install_non_bb:
	install -d 755 ${DEST_DIR}/usr/bin
	install -m 755 sm ${DEST_DIR}/usr/bin/sm

clean:
	@rm -f *.o *.a *.so
	@rm -f sm
