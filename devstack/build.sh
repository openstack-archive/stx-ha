#!/bin/bash

# devstack/build.sh
# Test DevStack plugin builds without needing an entire DevStack job

set -o xtrace
set -o errexit

unset LANG
unset LANGUAGE
LC_ALL=en_US.utf8
export LC_ALL

# Keep track of the DevStack plugin directory
PLUGIN_DIR=$(cd $(dirname "$0")/.. && pwd)
PLUGIN_NAME=$(basename $PLUGIN_DIR)

# Keep plugin happy
declare -a GITDIR
GITDIR[$PLUGIN_NAME]=$PLUGIN_DIR

# Dummy function to keep plugin happy
function get_python_exec_prefix {
    echo ""
}

function is_service_enabled {
    return 0
}

# Get the build functions
source $PLUGIN_DIR/devstack/lib/stx-ha

# Call builds
build_sm_common
install_sm_common_libs
build_sm_db
build_sm
build_sm_api
