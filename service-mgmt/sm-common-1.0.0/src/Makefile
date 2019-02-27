#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

INCLUDES =-I$(STAGING_DIR)/usr/include/glib-2.0
INCLUDES+=-I$(STAGING_DIR)/usr/lib64/glib-2.0/include -I.

SRCS=sm_types.c
SRCS+=sm_debug.c
SRCS+=sm_debug_thread.c
SRCS+=sm_trap.c
SRCS+=sm_trap_thread.c
SRCS+=sm_thread_health.c
SRCS+=sm_utils.c
SRCS+=sm_node_utils.c
SRCS+=sm_node_stats.c
SRCS+=sm_selobj.c
SRCS+=sm_time.c
SRCS+=sm_timer.c
SRCS+=sm_netlink.c
SRCS+=sm_hw.c
SRCS+=sm_uuid.c
SRCS+=sm_sha512.c
SRCS+=sm_eru_db.c
SRCS+=sm_util_types.c

OBJS = $(SRCS:.c=.o)
CCFLAGS= -fPIC -g -O2 -Wall -Werror -std=c++11
EXTRACCFLAGS= -D__STDC_FORMAT_MACROS -DSW_VERSION=\"$(SW_VERSION)\"
EXTRACCFLAGS+= -Wformat -Wformat-security
LDLIBS= -lsqlite3 -lglib-2.0 -lgmodule-2.0 -luuid -lrt -lpthread
LDFLAGS = -shared -rdynamic

build: libsm_common.so libsm_watchdog_nfs.so sm_watchdog sm_eru sm_eru_dump

.c.o:
	$(CXX) $(INCLUDES) $(CCFLAGS) $(EXTRACCFLAGS) -c $< -o $@

libsm_common.so: libsm_common.so.$(VER_MJR)
	ln -sf $^ $@

libsm_common.so.$(VER_MJR): libsm_common.so.$(VER)
	ln -sf $^ $@

libsm_common.so.$(VER): ${OBJS}
	$(CXX) ${LDFLAGS} -Wl,--start-group $(LDLIBS) -Wl,-soname,libsm_common.so.$(VER_MJR) -o $@ $^

libsm_watchdog_nfs.so: libsm_watchdog_nfs.so.$(VER_MJR)
	ln -sf $^ $@

libsm_watchdog_nfs.so.$(VER_MJR): libsm_watchdog_nfs.so.$(VER)
	ln -sf $^ $@

libsm_watchdog_nfs.so.$(VER): libsm_common.so.$(VER)
	$(CXX) $(INCLUDES) $(CCFLAGS) $(EXTRACCFLAGS) sm_watchdog_nfs.c ${LDFLAGS} $(LDLIBS) -L./ -lsm_common -Wl,-soname,libsm_watchdog_nfs.so.$(VER_MJR) -o $@

sm_watchdog: libsm_common.so
	$(CXX) $(INCLUDES) $(CCFLAGS) $(EXTRACCFLAGS) $(OBJS) sm_watchdog_module.c sm_watchdog_process.c sm_watchdog_main.c $(LDLIBS) -L./ -lsm_common -o sm_watchdog

sm_eru: libsm_common.so
	$(CXX) $(INCLUDES) $(CCFLAGS) $(EXTRACCFLAGS) $(OBJS) sm_eru_process.c sm_eru_main.c $(LDLIBS) -L./ -lsm_common -o sm_eru

sm_eru_dump: libsm_common.so
	$(CXX) $(INCLUDES) $(CCFLAGS) $(EXTRACCFLAGS) $(OBJS) sm_eru_dump.c $(LDLIBS) -L./ -lsm_common -o sm_eru_dump

install:
	# install of these 3 are in the .spec file so that they can be
	# renamed with '-' like they are in the bitbake file.
	#
	# install -d $(DEST_DIR)$(BIN_DIR)
	# install sm_watchdog sm_eru sm_eru_dump $(DEST_DIR)$(BIN_DIR)
	install -d $(DEST_DIR)$(LIB_DIR)
	install libsm_common.so.${VER} $(DEST_DIR)$(LIB_DIR)
	cp -P libsm_common.so libsm_common.so.$(VER_MJR) $(DEST_DIR)$(LIB_DIR)
	install -d $(DEST_DIR)$(INC_DIR)
	install -m 644 *.h $(DEST_DIR)$(INC_DIR)
	install -d $(DEST_DIR)/var/lib/sm/watchdog/modules
	install libsm_watchdog_nfs.so.${VER} $(DEST_DIR)/var/lib/sm/watchdog/modules
	cp -P libsm_watchdog_nfs.so libsm_watchdog_nfs.so.${VER_MJR}  $(DEST_DIR)/var/lib/sm/watchdog/modules

clean:
	rm -f *.o *.so *.so.*
