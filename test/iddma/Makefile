#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

TEST_CONFIG ?=

include ./config.make

.PHONY: fpga cpu

all: fpga cpu

fpga:
	@+make -C fpga TEST_CONFIG=$(TEST_CONFIG)

cpu:
	@+make -C cpu TEST_CONFIG=$(TEST_CONFIG)

test:
	@make -j2 test_cpu_fpga_

test_cpu_tcp:
	@make -j2 test_cpu_tcp_

test_tcp:
	@make -j2 test_tcp_

test_cpu_fpga_: test_cpu test_fpga

test_cpu_tcp_: test_cpu_tcp_main test_cpu_tcp_cp

test_tcp_: test_cpu_tcp_for_fpga test_fpga_tcp

test_cpu:
	@make -C cpu test

test_fpga:
	@make -C fpga test

test_fpga_tcp:
	@make -C fpga test_tcp

test_cpu_tcp_for_fpga:
	@make -C cpu test_tcp_for_xse

test_cpu_tcp_main:
	@make -C cpu test_tcp

test_cpu_tcp_cp:
	@make -C cpu test_tcp_cp

clean:
	@make -C fpga clean
	@make -C cpu clean

