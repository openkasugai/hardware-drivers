#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the GPL-2.0  License, see LICENSE for details.
# SPDX-License-Identifier: GPL-2.0-or-later
#=================================================

.PHONY: xse mem_manage

all: xse_cpu_only
	
mem_manage_cpu_only:
	@+make -C mem_manage PWD=$(PWD)/mem_manage

mem_manage_nv:
	@+make -C mem_manage NVIDIA=1 EXT=_nv PWD=$(PWD)/mem_manage

xse_cpu_only : mem_manage_cpu_only
	@+make -C xse PWD=$(PWD)/xse

xse_nv : mem_manage_nv
	@+make -C xse PWD=$(PWD)/xse

install: install_cpu_only

install_cpu_only: xse_cpu_only
	@make -C mem_manage PWD=$(PWD)/mem_manage install
	@make -C xse PWD=$(PWD)/xse install

uninstall: uninstall_cpu_only

uninstall_cpu_only:
	@make -C xse PWD=$(PWD)/xse uninstall
	@make -C mem_manage PWD=$(PWD)/mem_manage uninstall

clean:
	@make -C xse clean
	@make -C mem_manage clean
	@make -C mem_manage EXT=_nv clean
