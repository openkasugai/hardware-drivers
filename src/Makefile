#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

.PHONY: drivers lib tools

all: drivers lib tools

drivers:
	@+make -C drivers PWD=$(PWD)/drivers

lib:
	@+make -C lib

tools:
	@+make -C tools

install-drivers:
	@make -C drivers install PWD=$(PWD)/drivers

uninstall-drivers:
	@make -C drivers uninstall PWD=$(PWD)/drivers

clean:
	@make -C drivers clean
	@make -C lib clean

