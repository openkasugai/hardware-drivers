#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the GPL-2.0  License, see LICENSE for details.
# SPDX-License-Identifier: GPL-2.0-or-later
#=================================================

CFILES = mem_ops.c mem_manage.c
TARGET_MODULE := MemManage

NVIDIA_SRC ?= $(shell ls -d /usr/src/nvidia-[0-9]* | tail -1)
AMD_SRC ?= $(shell ls -d /usr/src/amdgpu-[0-9]* | tail -1)
KBUILD_EXTRA_SYMBOLS :=
CUR_CFLAGS := -I$(PWD) -I$(PWD)/../../../include/mem_manage

NVIDIA ?= 0
AMD ?= 0
USE_PROPRIETARY ?= 0

ifeq ($(NVIDIA),1)
	KBUILD_EXTRA_SYMBOLS := $(NVIDIA_SRC)/Module.symvers
	CUR_CFLAGS += -I$(NVIDIA_SRC)/nvidia -DNVIDIA_PLATFORM
endif
ifeq ($(AMD),1)
	CUR_CFLAGS += -I$(AMD_SRC)/include/drm/ -DAMD_PLATFORM
endif
ifeq ($(USE_PROPRIETARY),1)
	CUR_CFLAGS += -DUSE_PROPRIETARY
endif

obj-m := $(TARGET_MODULE).o
$(TARGET_MODULE)-objs := $(CFILES:%.c=%.o)

CFLAGS_mem_manage.o := $(CUR_CFLAGS)
CFLAGS_mem_ops.o := $(CUR_CFLAGS)

all:
	@if [ ! -e $(KBUILD_EXTRA_SYMBOLS) ]; then \
		echo need compile at $(KBUILD_EXTRA_SYMBOLS); \
		return 1; \
	else \
		make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules; \
	fi
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

install: all
	sudo insmod $(TARGET_MODULE).ko
	sudo mknod --mode=666 /dev/mem_manage0 c `grep mem_manage /proc/devices | awk '{print $$1;}'` 0

uninstall:
	sudo rmmod $(TARGET_MODULE)
	sudo rm /dev/mem_manage0

