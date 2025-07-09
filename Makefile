#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

PWD = $(abspath .)

.PHONY: src test

all: src test

src:
	@+make -C src PWD=$(PWD)/src

test:
	@+make -C test

install-drivers: src
	@make -C src install-drivers PWD=$(PWD)/src

uninstall-drivers: src
	@make -C src uninstall-drivers PWD=$(PWD)/src

setup-toe: src
	@if [ ! -z "$(FPGA_ETH_IP)" ] && [ ! -z "$(FPGA_ETH_MASK)" ] && [ ! -z "$(FPGA_ETH_GATEWAY)" ]; then \
		sudo bin/xse_toe_config -i $(FPGA_ETH_IP) -m $(FPGA_ETH_MASK) -g $(FPGA_ETH_GATEWAY); \
	fi
	@if [ ! -z "$(FPGA_ETH2_IP)" ] && [ ! -z "$(FPGA_ETH2_MASK)" ] && [ ! -z "$(FPGA_ETH2_GATEWAY)" ]; then \
		sudo bin/xse_toe_config -d 1 -i $(FPGA_ETH2_IP) -m $(FPGA_ETH2_MASK) -g $(FPGA_ETH2_GATEWAY); \
	fi

test_iddma:
	@make -C test test_iddma

test_mem_manage:
	@make -C test test_mem_manage

clean:
	@make -C src clean
	@make -C test clean

