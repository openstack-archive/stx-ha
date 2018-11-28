#!/bin/bash

# devstack/plugin.sh
# Dispatcher for functions to install and configure stx-ha components

echo_summary "stx-ha devstack plugin.sh called: $1/$2"

# check for service enabled
if is_service_enabled stx-ha; then
    if [[ "$1" == "stack" && "$2" == "install" ]]; then
        # Perform installation of source
        echo_summary "Install stx-ha"
        install_ha

    elif [[ "$1" == "stack" && "$2" == "post-config" ]]; then
        # Configure after the other layer 1 and 2 services have been configured
        echo_summary "Configure stx-ha"
        configure_ha
    elif [[ "$1" == "stack" && "$2" == "extra" ]]; then
        # Initialize and start the service
        echo_summary "Initialize and start stx-ha"
        init_ha
        start_ha
    elif [[ "$1" == "stack" && "$2" == "test-config" ]]; then
        # do sanity test
        echo_summary "do test-config"
    fi

    if [[ "$1" == "unstack" ]]; then
        # Shut down services
        echo_summary "Stop stx-ha services"
        stop_ha
    fi

    if [[ "$1" == "clean" ]]; then
        cleanup_ha
    fi
fi
