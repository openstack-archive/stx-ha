#!/bin/bash
#
# lib/stx-config
# Functions to control the configuration and operation of stx-ha

# ``plugin.sh`` calls the following entry points:
#
# - install_ha
# - configure_ha
# - init_ha
# - start_ha
# - stop_ha
# - cleanup_ha

_XTRACE_STX_HA=$(set +o | grep xtrace)
set -o xtrace

STX_SM_VERSION=${STX_SM_VERSION:="1.0.0"}
STX_SM_COMMON_VERSION=${STX_SM_COMMON_VERSION:=$STX_SM_VERSION}
STX_SM_DB_VERSION=${STX_SM_DB_VERSION:=$STX_SM_VERSION}

STX_HA_DIR=${GITDIR[$STX_HA_NAME]}
STX_SYSCONFDIR=${STX_SYSCONFDIR:-/etc}

STX_SM_DIR=$STX_HA_DIR/service-mgmt/sm-${STX_SM_VERSION}
STX_SM_CONF_DIR=$STX_SYSCONFDIR/sm
STX_SM_VAR_DIR=/var/lib/sm
STX_SM_API_CONF_DIR=$STX_SYSCONFDIR/sm-api

STX_BIN_DIR=$(get_python_exec_prefix)
PYTHON_SITE_DIR=$(python -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")

function cleanup_ha {
    stop_ha

    if is_service_enabled sm-api; then
        cleanup_sm_api
    fi
    if is_service_enabled sm-daemon; then
        cleanup_sm
    fi
    if is_service_enabled sm-db; then
        cleanup_sm_db
    fi
    if is_service_enabled sm-common; then
        cleanup_sm_common
    fi
}

function cleanup_sm {
    pushd $STX_HA_DIR/service-mgmt/sm-${STX_SM_COMMON_VERSION}

    local prefix=${STX_BIN_DIR%/*}

    sudo -E make \
        DEST_DIR=$prefix \
        BIN_DIR=/bin \
        LIB_DIR=/lib \
        INC_DIR=/include \
        clean

    sudo rm -f ${STX_BIN_DIR}/sm

    popd
}

function cleanup_sm_api {
    pushd ${GITDIR[$STX_HA_NAME]}/service-mgmt-api/sm-api

    sudo rm -f $STX_SYSCONFDIR/init.d/sm-api \
        $STX_SYSCONFDIR/systemd/system/sm-api.service \
        $STX_SYSCONFDIR/pmon.d/sm-api.conf

    sudo rm -rf $STX_SM_CONF_DIR $STX_SM_API_CONF_DIR

    popd
}

function cleanup_sm_common {
    pushd $STX_HA_DIR/service-mgmt/sm-common-${STX_SM_COMMON_VERSION}

    local prefix=${STX_BIN_DIR%/*}

    sudo -E make \
        VER=$STX_SM_COMMON_VERSION \
        VER_MJR=${STX_SM_COMMON_VERSION%%.*} \
        BIN_DIR=/bin \
        LIB_DIR=/lib \
        INC_DIR=/include \
        clean

    sudo rm -f \
        $prefix/include/sm_*.h \
        $prefix/lib/libsm_common.so.* \
        $STX_BIN_DIR/sm-eru \
        $STX_BIN_DIR/sm-eru-dump \
        $STX_BIN_DIR/sm-watchdog \
        $STX_SM_VAR_DIR/watchdog/modules/libsm_watchdog_nfs.so.* \
        $STX_SYSCONFDIR/systemd/system/sm-eru.service \
        $STX_SYSCONFDIR/systemd/system/sm-watchdog.service \
        $STX_SYSCONFDIR/pmon.d/sm-eru.conf \
        $STX_SYSCONFDIR/pmon.d/sm-watchdog.conf \
        $STX_SYSCONFDIR/init.d/sm-eru \
        $STX_SYSCONFDIR/init.d/sm-watchdog \
        /etc/ld.so.conf.d/stx-ha.conf

    popd
}

function cleanup_sm_db {
    pushd $STX_HA_DIR/service-mgmt/sm-db-${STX_SM_DB_VERSION}

    local prefix=${STX_BIN_DIR%/*}

    sudo -E make \
        VER=$STX_SM_DB_VERSION \
        VER_MJR=${STX_SM_DB_VERSION%%.*} \
        BIN_DIR=/bin \
        LIB_DIR=/lib \
        INC_DIR=/include \
        clean

    sudo rm -rf database/*.db \
        $prefix/include/sm_db_*.h \
        $prefix/lib/libsm_db.so* \
        $STX_SM_VAR_DIR

    popd
}

function configure_ha {
    :
}

function init_ha {
    :
}

function install_ha {
    if is_service_enabled sm-common; then
        install_sm_common
    fi
    if is_service_enabled sm-db; then
        install_sm_db
    fi
    if is_service_enabled sm-daemon; then
        install_sm
    fi
    if is_service_enabled sm-client; then
        install_sm_client
    fi
    if is_service_enabled sm-tools; then
        install_sm_tools
    fi
    if is_service_enabled sm-api; then
        install_sm_api
    fi
}

function install_sm {
    pushd $STX_HA_DIR/service-mgmt/sm-${STX_SM_VERSION}

    # On Xenial needed to remove -Werror and add -Wunused-result
    # CCFLAGS= -g -O2 -Wall -Werror -Wformat  -std=c++11
    make \
        CCFLAGS="-g -O2 -Wall -Wformat -Wunused-result -std=c++11" \
        INCLUDES="-I/usr/lib64/glib-2.0/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include" \
        build

    # Skip make install_non_bb, it hard-codes /usr/bin as the destination
    sudo install -m 755 src/sm ${STX_BIN_DIR}/sm

    popd
}

function install_sm_api {
    pushd ${GITDIR[$STX_HA_NAME]}/service-mgmt-api/sm-api

    sudo python setup.py install \
        --root=/ \
        --install-lib=$PYTHON_SITE_DIR \
        --prefix=/usr \
        --install-data=/usr/share

    sudo install -m 755 scripts/sm-api $STX_SYSCONFDIR/init.d
    sudo install -m 644 -D scripts/sm-api.service $STX_SYSCONFDIR/systemd/system
    sudo install -m 644 -D scripts/sm_api.ini $STX_SM_CONF_DIR
    sudo install -m 644 scripts/sm-api.conf $STX_SYSCONFDIR/pmon.d
    sudo install -m 644 -D etc/sm-api/policy.json $STX_SM_API_CONF_DIR

    popd
}

function install_sm_client {
    setup_install ${GITDIR[$STX_HA_NAME]}/service-mgmt-client/sm-client
    sudo install -m 755 ${GITDIR[$STX_HA_NAME]}/service-mgmt-client/sm-client/usr/bin/smc $STX_BIN_DIR
}

function install_sm_common {
    pushd $STX_HA_DIR/service-mgmt/sm-common-${STX_SM_COMMON_VERSION}

    local prefix=${STX_BIN_DIR%/*}

    # On Xenial needed to remove -O2
    # CCFLAGS= -fPIC -g -O2 -Wall -Werror
    make \
        VER=$STX_SM_COMMON_VERSION \
        VER_MJR=${STX_SM_COMMON_VERSION%%.*} \
        CCFLAGS="-fPIC -g -Wall -Werror" \
        INCLUDES="-I/usr/lib64/glib-2.0/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include" \
        build

    # NOTE: We don't use the make install_non_bb target as it doesn't
    #       properly support $PREFIX.
    sudo install -m 0644 -p src/*.h $prefix/include
    sudo install -m 0755 -p src/libsm_common.so.${STX_SM_COMMON_VERSION} $prefix/lib
    sudo cp -P src/libsm_common.so src/libsm_common.so.${STX_SM_COMMON_VERSION%%.*} $prefix/lib
    sudo install -m 0750 src/sm_watchdog src/sm_eru src/sm_eru_dump $STX_BIN_DIR
    sudo install -m 0755 -p -D -t $STX_SM_VAR_DIR/watchdog/modules src/libsm_watchdog_nfs.so.${STX_SM_COMMON_VERSION}
    sudo cp -P src/libsm_watchdog_nfs.so src/libsm_watchdog_nfs.so.${STX_SM_COMMON_VERSION%%.*} $STX_SM_VAR_DIR/watchdog/modules

    sudo install -m 644 -p -t $STX_SYSCONFDIR/systemd/system scripts/sm-eru.service scripts/sm-watchdog.service
    sudo install -m 750 -p -D -t $STX_SYSCONFDIR/init.d scripts/sm-eru scripts/sm-watchdog

    sudo install -m 750 -d $STX_SYSCONFDIR/pmon.d
    sudo install -m 640 -p -t $STX_SYSCONFDIR/pmon.d scripts/sm-eru.conf
    sudo install -m 640 -p -t $STX_SYSCONFDIR/pmon.d scripts/sm-watchdog.conf

    echo $prefix/lib | sudo tee /etc/ld.so.conf.d/stx-ha.conf
    sudo ldconfig

    popd
}

function install_sm_db {
    pushd $STX_HA_DIR/service-mgmt/sm-db-${STX_SM_DB_VERSION}

    local prefix=${STX_BIN_DIR%/*}

    # INCLUDES because we need /usr/lib/x86_64-linux-gnu/glib-2.0/include
    make \
        VER=$STX_SM_DB_VERSION \
        VER_MJR=${STX_SM_DB_VERSION%%.*} \
        INCLUDES="-I/usr/lib64/glib-2.0/include -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include" \
        build

    # NOTE: We don't use the make install_non_bb target as it doesn't
    #       properly support $PREFIX.
    sudo install -m 0644 -p src/*.h ${prefix}/include
    sudo install -m 0755 -p src/libsm_db.so.${STX_SM_DB_VERSION} $prefix/lib
    sudo cp -P src/libsm_db.so src/libsm_db.so.${STX_SM_DB_VERSION%%.*} $prefix/lib

    # NOTE: These belong in configure_sm_db but the Makefile insists they 
    #       be there for install_non_bb
    sqlite3 database/sm.db < database/create_sm_db.sql
    sqlite3 database/sm.hb.db < database/create_sm_hb_db.sql

    # Call database make directly, it works
    (cd database; \
        sudo -E make \
            DEST_DIR="" \
            install_non_bb; \
    )

    popd
}

function install_sm_tools {
    pushd ${GITDIR[$STX_HA_NAME]}/service-mgmt-tools/sm-tools

    sudo python setup.py install \
        --root=/ \
        --install-lib=$PYTHON_SITE_DIR \
        --prefix=/usr \
        --install-data=/usr/share

    popd
}

function start_ha {
    :
}

function stop_ha {
    :
}

$_XTRACE_STX_HA
