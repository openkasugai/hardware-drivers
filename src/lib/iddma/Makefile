#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

CUDA_TOP ?= /usr/local/cuda
DEBUG ?= 0
LATENCY ?= 0
FDMA ?= 1

.PHONY: all lib cpu gpu fpga clean

all: lib

lib: cpu fpga

cpu:
	@+make -C cpu DEBUG=$(DEBUG) LATENCY=$(LATENCY) CUDA_TOP=$(CUDA_TOP)

gpu:
	@+make -C gpu DEBUG=$(DEBUG) LATENCY=$(LATENCY) CUDA_TOP=$(CUDA_TOP)

fpga:
ifeq ($(FDMA), 1)
	@+make -C fpga DEBUG=$(DEBUG) LATENCY=$(LATENCY) CUDA_TOP=$(CUDA_TOP)
endif

clean:
	@make -C cpu clean
	@make -C gpu clean
	@make -C fpga clean
