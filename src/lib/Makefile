#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

NVIDIA ?= 0

.PHONY: common mem_manage stream_engine route_controller iddma function_base toe

all: mem_manage route_controller iddma function_base

mem_manage:
	@+make -C mem_manage
	@+if [ $(NVIDIA) -ne 0 ]; then \
		make -C mem_manage NVIDIA=1 EXT=_nv; \
	fi

stream_engine: common
	@+make -C stream_engine

route_controller: common
	@+make -C route_controller

common:
	@+make -C common

iddma: toe stream_engine
	@+make -C iddma

function_base:
	@+make -C function_base

toe:
	@+make -C toe

clean:
	@make -C toe clean
	@make -C function_base clean
	@make -C iddma clean
	@make -C route_controller clean
	@make -C stream_engine clean
	@make -C mem_manage clean
	@make -C mem_manage NVIDIA=1 EXT=_nv clean
	@make -C common clean

