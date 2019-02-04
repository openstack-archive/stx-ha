install:
	install -d ${DEST_DIR}/etc/init.d
	install sm ${DEST_DIR}/etc/init.d/sm
	install sm.shutdown ${DEST_DIR}/etc/init.d/sm-shutdown
	install -d ${DEST_DIR}/etc/pmon.d
	install -m 644 sm.conf ${DEST_DIR}/etc/pmon.d/sm.conf
	install -d ${DEST_DIR}/etc/logrotate.d
	install -m 644 sm.logrotate ${DEST_DIR}/etc/logrotate.d/sm.logrotate
	install -d 755 ${DEST_DIR}/usr/local/sbin
	install sm.notify ${DEST_DIR}/usr/local/sbin/sm-notify
	install sm.troubleshoot ${DEST_DIR}/usr/local/sbin/sm-troubleshoot
	install sm.notification ${DEST_DIR}/usr/local/sbin/sm-notification
	install -d $(DEST_DIR)$(UNIT_DIR)
	install -m 644 *.service $(DEST_DIR)$(UNIT_DIR) 

