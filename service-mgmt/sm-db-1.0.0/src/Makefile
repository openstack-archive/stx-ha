#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
INCLUDES =-I$(STAGING_DIR)/usr/include/glib-2.0
INCLUDES+=-I$(STAGING_DIR)/usr/lib64/glib-2.0/include

SRCS=sm_db.c
SRCS+=sm_db_iterator.c
SRCS+=sm_db_foreach.c
SRCS+=sm_db_nodes.c
SRCS+=sm_db_node_history.c
SRCS+=sm_db_service_domains.c
SRCS+=sm_db_service_domain_interfaces.c
SRCS+=sm_db_service_domain_members.c
SRCS+=sm_db_service_domain_neighbors.c
SRCS+=sm_db_service_domain_assignments.c
SRCS+=sm_db_service_groups.c
SRCS+=sm_db_service_group_members.c
SRCS+=sm_db_services.c
SRCS+=sm_db_service_heartbeat.c
SRCS+=sm_db_service_dependency.c
SRCS+=sm_db_service_instances.c
SRCS+=sm_db_service_actions.c
SRCS+=sm_db_service_action_results.c
SRCS+=sm_db_build.c
SRCS+=sm_db_configuration.c

OBJS = $(SRCS:.c=.o)
CCFLAGS= -fPIC -g -O2 -Wall -Werror -std=c++11 -Wformat -Wformat-security
EXTRACCFLAGS= -D__STDC_FORMAT_MACROS
LDLIBS= -lsqlite3 -lglib-2.0 -luuid -lrt -lsm_common
LDFLAGS = -shared -rdynamic

build: libsm_db.so sm_db_build

.c.o:
	$(CXX) $(INCLUDES) $(CCFLAGS) $(EXTRACCFLAGS) -c $< -o $@

libsm_db.so: libsm_db.so.$(VER_MJR)
	ln -sf $^ $@

libsm_db.so.$(VER_MJR): libsm_db.so.$(VER)
	ln -sf $^ $@

libsm_db.so.$(VER): ${OBJS}
	$(CXX) ${LDFLAGS} $(LDLIBS) -Wl,-soname,libsm_db.so.$(VER_MJR) -o $@ $^

sm_db_build: ${OBJS}
	$(CXX) $(INCLUDES) $(CCFLAGS) $(EXTRACCFLAGS) $(OBJS) $(LDLIBS) -o sm_db_build

install:
	install -d ${DEST_DIR}/usr/lib64
	install libsm_db.so.${VER} $(DEST_DIR)/usr/lib64
	cp -P libsm_db.so libsm_db.so.$(VER_MJR) $(DEST_DIR)/usr/lib64
	install -d ${DEST_DIR}/usr/include
	install -m 0644 *.h ${DEST_DIR}/usr/include
	install -d 755 ${DEST_DIR}/usr/bin

clean:
	@rm -f *.o *.a *.so *.so.*
