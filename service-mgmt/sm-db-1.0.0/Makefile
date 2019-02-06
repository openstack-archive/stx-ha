#
# Copyright (c) 2014 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#
build:
	@(cd src; make build VER=$(VER) VER_MJR=$(VER_MJR))

install:
	@(cd src; make $@)
	@(cd database; make $@)

clean:
	@( cd src; make clean )
