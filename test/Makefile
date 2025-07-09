#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

.PHONY: 3rdparty iddma mem_manage

all: iddma

iddma: 3rdparty
	@+make -C iddma

mem_manage: 3rdparty
	@+make -C mem_manage

3rdparty:
	@make -C 3rdparty

test_iddma:
	@make -C iddma test

test_mem_manage:
	@make -C mem_manage test

clean:
	@make -C iddma clean
	@make -C mem_manage clean
	@make -C 3rdparty clean

