# Initial source of lib script
source $DEST/stx-ha/devstack/lib/stx-ha

# check for service enabled
if is_service_enabled stx-ha; then
    if [[ "$1" == "stack" && "$2" == "install" ]]; then
	# Perform installation of service source
	echo_summary "Installing stx-ha"
        install_sm_common
	install_sm_db
	install_sm
	install_sm_api
	install_sm_client
	install_sm_tools

    elif [[ "$1" == "stack" && "$2" == "post-config" ]]; then
	# Configure after the other layer 1 and 2 services have been configured
	echo_summary "Configuring stx-ha"
	#enable_sm //disabled for now
    fi

fi

if [[ "$1" == "unstack" ]]; then
    :
fi

if [[ "$1" == "clean" ]]; then
    echo_summary "Unstack clean-ha"
    clean_sm_common
    clean_sm_db
    clean_sm
fi
