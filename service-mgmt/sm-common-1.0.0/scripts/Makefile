install_non_bb:
	install -d $(DEST_DIR)$(UNIT_DIR)
	install -m 644 *.service $(DEST_DIR)$(UNIT_DIR) 
	install -d $(DEST_DIR)/etc/init.d
	install sm-watchdog sm-eru $(DEST_DIR)/etc/init.d
	install -d $(DEST_DIR)/etc/pmon.d
	install *.conf $(DEST_DIR)/etc/pmon.d

