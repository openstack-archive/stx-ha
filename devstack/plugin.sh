#!/bin/bash
#
# lib/stx-ha
# Functions to control the configuration and operation of the **high availability** service

# check for service enabled
if is_service_enabled stx-ha; then
    if [[ "$1" == "stack" && "$2" == "install" ]]; then
	# Perform installation of service source
	echo_summary "Installing stx-ha"
	install_service_management

    elif [[ "$1" == "stack" && "$2" == "post-config" ]]; then
	# Configure after the other layer 1 and 2 services have been configured
	echo_summary "Configuring stx-ha"
	#enable_sm //disabled for now
	create_sm_accounts
	start_sm_api
    fi
    if [[ "$1" == "unstack" ]]; then
	stop_sm_api
    fi

    if [[ "$1" == "clean" ]]; then
	cleanup_sm
    fi
fi
