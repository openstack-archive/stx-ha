#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
#

"""
Logging
"""

import logging
import logging.handlers

LOG_FILE_NAME = "sm.log"
LOG_MAX_BYTES = 10485760
LOG_BACKUP_COUNT = 5

_log_to_console = False
_log_to_syslog = False
_log_to_file = False

_syslog_facility = None

_loggers = {}


def _setup_logger(logger):
    formatter = logging.Formatter("%(asctime)s %(threadName)s[%(process)d] "
                                  "%(name)s.%(lineno)d - %(levelname)s "
                                  "%(message)s")

    if _log_to_console:
        handler = logging.StreamHandler()
        handler.setFormatter(formatter)
        logger.addHandler(handler)

    if _log_to_syslog:
        handler = logging.handlers.SysLogHandler(address='/dev/log',
                                                 facility=_syslog_facility)
        handler.setFormatter(formatter)
        logger.addHandler(handler)

    if _log_to_file:
        handler = logging.handlers.RotatingFileHandler(LOG_FILE_NAME, "a+",
                                                       LOG_MAX_BYTES,
                                                       LOG_BACKUP_COUNT)
        handler.setFormatter(formatter)
        logger.addHandler(handler)

    logger.setLevel(logging.DEBUG)


def get_logger(name):
    """ Get a logger or create one .
    """

    global _loggers

    _loggers[name] = logging.getLogger(name)
    return _loggers[name]


def configure(conf):
    """ Setup logging.
    """

    global _loggers
    global _log_to_syslog
    global _syslog_facility

    if conf['logging']['use_syslog']:
        _log_to_syslog = True
        _syslog_facility = conf['logging']['log_facility']

    for logger in _loggers:
        _setup_logger(_loggers[logger])
