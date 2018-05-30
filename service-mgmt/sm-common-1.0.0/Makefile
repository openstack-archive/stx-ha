#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
build:
	@(cd src; make build VER=$(VER) VER_MJR=$(VER_MJR))

install_non_bb:
	@(cd src; make DEST_DIR=$(DEST_DIR) BIN_DIR=$(BIN_DIR) LIB_DIR=$(LIB_DIR) INC_DIR=$(INC_DIR) VER=$(VER) VER_MJR=$(VER_MJR) install_non_bb)
	@(cd scripts; make DEST_DIR=$(DEST_DIR) UNIT_DIR=$(UNIT_DIR) install_non_bb)

clean:
	@( cd src; make clean )
