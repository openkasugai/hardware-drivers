#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the GPL-2.0  License, see LICENSE for details.
# SPDX-License-Identifier: GPL-2.0-or-later
#=================================================

SRCS := libxse.c \
	device_info.c \
	xdma.c \
	devmem.c \
	stream_engine.c \
	route_controller.c \
	toe.c \
	cms.c \
	function.c 

OBJS := $(SRCS:%.c=%.o)
TARGET_MODULE := xse

$(TARGET_MODULE)-objs := $(OBJS)
obj-m := $(TARGET_MODULE).o

#ccflags-y += -std=c99
#-Wall -Wno-declaration-after-statement

MEM_MANAGE_DIR ?= $(PWD)/../mem_manage/
CUR_CFLAGS := -I$(PWD) -I$(MEM_MANAGE_DIR) -I$(PWD)/../../../include/stream_engine
CUR_CFLAGS += -I$(PWD)/../../../include/route_controller
CUR_CFLAGS += -I$(PWD)/../../../include/devmem
CUR_CFLAGS += -I$(PWD)/../../../include/toe
ccflags-y += $(CUR_CFLAGS)

KBUILD_EXTRA_SYMBOLS := $(MEM_MANAGE_DIR)/Module.symvers

all: build

build:
	@echo $(PWD)
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

install: build
	sudo insmod $(TARGET_MODULE).ko

uninstall:
	sudo rmmod $(TARGET_MODULE)

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	@rm -f *~
