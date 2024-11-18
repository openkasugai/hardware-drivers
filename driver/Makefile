#=================================================
# Copyright 2024 NTT Corporation, FUJITSU LIMITED
# Licensed under the GPL-2.0  License, see LICENSE for details.
# SPDX-License-Identifier: GPL-2.0-or-later
#=================================================

##########################################################
# Check Commands
ifeq ($(filter dev-id, $(MAKECMDGOALS)),dev-id)
DISABLE_SERIAL_ID_PREFIX := 1
endif
ifeq ($(filter rw-nolock, $(MAKECMDGOALS)),rw-nolock)
DISABLE_REGISTER_WRRD := 1
endif

ifeq ($(filter extif-inv, $(MAKECMDGOALS)),extif-inv)
ENABLE_EXTIF_INV := 1
endif
ifeq ($(filter wo-direct, $(MAKECMDGOALS)),wo-direct)
ENABLE_MODULE_DIRECT := 0
endif
ifeq ($(filter wo-nw, $(MAKECMDGOALS)),wo-nw)
ENABLE_MODULE_PTU := 0
endif

# depricated Commands
ifeq ($(filter debug_autoclose, $(MAKECMDGOALS)),debug_autoclose)
$(error MAKE COMMAND 'debug_autoclose' was REMOVED)
endif
ifeq ($(filter non-sleep, $(MAKECMDGOALS)),non-sleep)
$(error MAKE COMMAND 'non-sleep' was REMOVED)
endif

##########################################################
# Setup
SHELL = /bin/bash

ENABLE_SETTING_IN_DRIVER      ?= 1

DISABLE_REGISTER_WRRD         ?= 0
DISABLE_SERIAL_ID_PREFIX      ?= 0
ENABLE_EXTIF_INV              ?= 0

ENABLE_MODULE_GLOBAL          ?= 1
ENABLE_MODULE_CHAIN           ?= 1
ENABLE_MODULE_DIRECT          ?= 1
ENABLE_MODULE_LLDMA           ?= 1
ENABLE_MODULE_PTU             ?= 1
ENABLE_MODULE_FUNC            ?= 1
ENABLE_MODULE_CONV            ?= 1
ENABLE_MODULE_CMS             ?= 1

ENABLE_REFCOUNT_GLOBAL        ?= 1
ENABLE_REFCOUNT_CHAIN         ?= 0
ENABLE_REFCOUNT_DIRECT        ?= 0
ENABLE_REFCOUNT_LLDMA         ?= 0
ENABLE_REFCOUNT_PTU           ?= 0
ENABLE_REFCOUNT_FUNC          ?= 0
ENABLE_REFCOUNT_CONV          ?= 0
ENABLE_REFCOUNT_CMS           ?= 0


DRIVER_TYPE_DEFAULT := 0      # 0b00000000 0x00
DRIVER_TYPE_NODRAIN := 1      # 0b00000001 0x01
DRIVER_TYPE_DEBUG   := 128    # 0b10000000 0x80
DRIVER_TYPE         := $(DRIVER_TYPE_DEFAULT)

# v(Major.Minor.Revision-Patch)
DRIVER_MAJOR_VER := 1 # [31:24]
DRIVER_MINOR_VER := 0 # [23:16]
DRIVER_REVISION  := 0 # [15:8]
DRIVER_PATCH     := 0 # [7:0]


# Build Options
XPCIE_HOME ?= $(shell pwd)
XPCIE_KVER := $(shell uname -r)
XPCIE_BUILD_OPTIONS ?= \
	XPCIE_HOME=$(XPCIE_HOME) \
	ENABLE_SETTING_IN_DRIVER=$(ENABLE_SETTING_IN_DRIVER) \
	DISABLE_REGISTER_WRRD=$(DISABLE_REGISTER_WRRD) \
	DISABLE_SERIAL_ID_PREFIX=$(DISABLE_SERIAL_ID_PREFIX) \
	ENABLE_EXTIF_INV=$(ENABLE_EXTIF_INV) \
	ENABLE_MODULE_GLOBAL=$(ENABLE_MODULE_GLOBAL) \
	ENABLE_MODULE_CHAIN=$(ENABLE_MODULE_CHAIN) \
	ENABLE_MODULE_DIRECT=$(ENABLE_MODULE_DIRECT) \
	ENABLE_MODULE_LLDMA=$(ENABLE_MODULE_LLDMA) \
	ENABLE_MODULE_PTU=$(ENABLE_MODULE_PTU) \
	ENABLE_MODULE_FUNC=$(ENABLE_MODULE_FUNC) \
	ENABLE_MODULE_CONV=$(ENABLE_MODULE_CONV) \
	ENABLE_MODULE_CMS=$(ENABLE_MODULE_CMS) \
	ENABLE_REFCOUNT_GLOBAL=$(ENABLE_REFCOUNT_GLOBAL) \
	ENABLE_REFCOUNT_CHAIN=$(ENABLE_REFCOUNT_CHAIN) \
	ENABLE_REFCOUNT_DIRECT=$(ENABLE_REFCOUNT_DIRECT) \
	ENABLE_REFCOUNT_LLDMA=$(ENABLE_REFCOUNT_LLDMA) \
	ENABLE_REFCOUNT_PTU=$(ENABLE_REFCOUNT_PTU) \
	ENABLE_REFCOUNT_FUNC=$(ENABLE_REFCOUNT_FUNC) \
	ENABLE_REFCOUNT_CONV=$(ENABLE_REFCOUNT_CONV) \
	ENABLE_REFCOUNT_CMS=$(ENABLE_REFCOUNT_CMS)

TARGET_MODULE := xpcie
obj-m := $(TARGET_MODULE).o
$(TARGET_MODULE)-objs := \
	xpcie_device.o \
	xpcie_device_fops.o \
	libxpcie.o

ifeq ($(ENABLE_MODULE_GLOBAL),1)
$(TARGET_MODULE)-objs += \
	global/xpcie_device_fops_global.o \
	global/libxpcie_global.o
KCPPFLAGS += -DENABLE_MODULE_GLOBAL
ifeq ($(ENABLE_REFCOUNT_GLOBAL),1)
KCPPFLAGS += -DENABLE_REFCOUNT_GLOBAL
endif
else
ENABLE_REFCOUNT_GLOBAL := 0
endif

ifeq ($(ENABLE_MODULE_CHAIN),1)
$(TARGET_MODULE)-objs += \
	chain/xpcie_device_fops_chain.o \
	chain/libxpcie_chain.o
KCPPFLAGS += -DENABLE_MODULE_CHAIN
ifeq ($(ENABLE_REFCOUNT_CHAIN),1)
KCPPFLAGS += -DENABLE_REFCOUNT_CHAIN
endif
else
ENABLE_REFCOUNT_CHAIN := 0
endif

ifeq ($(ENABLE_MODULE_DIRECT),1)
$(TARGET_MODULE)-objs += \
	direct/xpcie_device_fops_direct.o \
	direct/libxpcie_direct.o
KCPPFLAGS += -DENABLE_MODULE_DIRECT
ifeq ($(ENABLE_REFCOUNT_DIRECT),1)
KCPPFLAGS += -DENABLE_REFCOUNT_DIRECT
endif
else
# KCPPFLAGS += -DWO_DIRECT
ENABLE_REFCOUNT_DIRECT := 0
endif

ifeq ($(ENABLE_MODULE_LLDMA),1)
$(TARGET_MODULE)-objs += \
	lldma/xpcie_device_fops_lldma.o \
	lldma/libxpcie_lldma.o
KCPPFLAGS += -DENABLE_MODULE_LLDMA
ifeq ($(ENABLE_REFCOUNT_LLDMA),1)
KCPPFLAGS += -DENABLE_REFCOUNT_LLDMA
endif
else
ENABLE_REFCOUNT_LLDMA := 0
endif

ifeq ($(ENABLE_MODULE_PTU),1)
$(TARGET_MODULE)-objs += \
	ptu/libxpcie_ptu.o
KCPPFLAGS += -DENABLE_MODULE_PTU
ifeq ($(ENABLE_REFCOUNT_PTU),1)
KCPPFLAGS += -DENABLE_REFCOUNT_PTU
endif
else
# KCPPFLAGS += -DWO_NW
ENABLE_REFCOUNT_PTU := 0
endif

ifeq ($(ENABLE_MODULE_CONV),1)
$(TARGET_MODULE)-objs += \
	conv/libxpcie_conv.o
KCPPFLAGS += -DENABLE_MODULE_CONV
ifeq ($(ENABLE_REFCOUNT_CONV),1)
KCPPFLAGS += -DENABLE_REFCOUNT_CONV
endif
else
ENABLE_REFCOUNT_CONV := 0
endif

ifeq ($(ENABLE_MODULE_FUNC),1)
$(TARGET_MODULE)-objs += \
	func/libxpcie_func.o
KCPPFLAGS += -DENABLE_MODULE_FUNC
ifeq ($(ENABLE_REFCOUNT_FUNC),1)
KCPPFLAGS += -DENABLE_REFCOUNT_FUNC
endif
else
ENABLE_REFCOUNT_FUNC := 0
endif

ifeq ($(ENABLE_MODULE_CMS),1)
$(TARGET_MODULE)-objs += \
	cms/xpcie_device_fops_cms.o \
	cms/libxpcie_cms.o
KCPPFLAGS += -DENABLE_MODULE_CMS
ifeq ($(ENABLE_REFCOUNT_CMS),1)
KCPPFLAGS += -DENABLE_REFCOUNT_CMS
endif
else
ENABLE_REFCOUNT_CMS := 0
endif

ifeq ($(ENABLE_SETTING_IN_DRIVER),1)
KCPPFLAGS += -DENABLE_SETTING_IN_DRIVER
endif

ifeq ($(DISABLE_SERIAL_ID_PREFIX),1)
KCPPFLAGS += -DXPCIE_UNUSE_SERIAL_ID
endif

ifeq ($(DISABLE_REGISTER_WRRD),1)
KCPPFLAGS += -DXPCIE_REGISTER_NO_LOCK
endif

ifeq ($(ENABLE_EXTIF_INV),1)
KCPPFLAGS += -DEXTIFINV
endif

BUILDSYSTEM_DIR := /lib/modules/$(shell uname -r)/build
KERNEL_BUILDCOMPILER := $(shell \
	cat /lib/modules/`uname -r`/build/.config \
	| grep CONFIG_CC_VERSION_TEXT \
	| cut -d"\"" -f2 \
	| cut -d" " -f1)
ifneq ($(strip $(KERNEL_BUILDCOMPILER)),)
CC=$(KERNEL_BUILDCOMPILER)
endif

MAKE=@make

KCPPFLAGS += -Wall
KCPPFLAGS += -I$(XPCIE_HOME)

# KCPPFLAGS += -DXPCIE_REGISTER_LOG_SUPPRESS_CHECK_REALLY_WRITE

# Default target
.PHONY: default
default: normal


##########################################################
# Build Driver

## Pipe drain driver
##  Now, this driver only drain, without pipe forced clear...
# =========================================================
## Normal driver
.PHONY:normal
normal:\
	print-log-default\
	build

# =========================================================
## debug driver
.PHONY:debug
debug:\
	print-log-default\
	print-log-register\
	print-log-func-trace\
	build

# =========================================================
## trace driver
.PHONY:trace
trace:\
	print-log-default\
	print-log-func-trace\
	build

# =========================================================
## build driver exactly
build: print-driver-tvr
	$(MAKE) KCPPFLAGS+='$(KCPPFLAGS)' \
	-C $(BUILDSYSTEM_DIR) \
	CC=$(CC) \
	XPCIE_BUILD_OPTIONS=$(XPCIE_BUILD_OPTIONS) \
	M=$(shell pwd) \
	modules

# =========================================================
## clean driver files
.PHONY:clean
clean:
	@echo "###"
	@echo "### Cleaning $(TARGET_MODULE).ko..."
	@echo "###"
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(shell pwd) clean
	@rm -rf *.o.ur-safe

# =========================================================
.PHONY:all
all: clean default

# =========================================================
DUMMY_CMDS := rw-nolock dev-id non-sleep extif-inv wo-direct wo-nw
.PHONY: $(DUMMY_CMDS)
$(DUMMY_CMDS):
	@echo -n ""

# =========================================================
.PHONY: print
print: print-driver-tvr
print-driver-tvr: \
	print-device-file-name-type \
	print-read-write-lock \
	print-fpga-type \
	print-setting-parameter-in-driver \
	print-refcount-module-in-driver
	@echo "###"
	@printf "### Driver Type           : 0x%02x\n" $(DRIVER_TYPE)
	@printf "### Driver Version(major) : 0x%02x\n" $(DRIVER_MAJOR_VER)
	@printf "### Driver Version(minor) : 0x%02x\n" $(DRIVER_MINOR_VER)
	@printf "### Driver Revision       : 0x%02x\n" $(DRIVER_REVISION)
	@printf "### Driver Patch          : 0x%02x\n" $(DRIVER_PATCH)
	@echo "###"
	@echo "### Building $(TARGET_MODULE).ko..."
	@echo "###"
	$(eval KCPPFLAGS += -DDRIVER_TYPE=$(DRIVER_TYPE) )
	$(eval KCPPFLAGS += -DDRIVER_MAJOR_VER=$(DRIVER_MAJOR_VER) )
	$(eval KCPPFLAGS += -DDRIVER_MINOR_VER=$(DRIVER_MINOR_VER) )
	$(eval KCPPFLAGS += -DDRIVER_REVISION=$(DRIVER_REVISION) )
	$(eval KCPPFLAGS += -DDRIVER_PATCH=$(DRIVER_PATCH) )

print-log-default:
	@echo "###"
	@echo "### About LOG into syslog"
	@echo "###  - Default"
print-log-register:
	@echo "###  - Register Log"
	$(eval KCPPFLAGS += -DXPCIE_REGISTER_LOG )
print-log-func-trace:
	@echo "###  - Function Trace Log"
	$(eval KCPPFLAGS += -DXPCIE_TRACE_LOG )

print-device-file-name-type:
	@echo "###"
ifeq ($(DISABLE_SERIAL_ID_PREFIX),1)
	@echo "### Device file name : /dev/xpcie<dev_id>"
else
	@echo "### Device file name : /dev/xpcie_<serial_id>"
endif

print-read-write-lock:
	@echo "###"
ifeq ($(DISABLE_REGISTER_WRRD),1)
	@echo "### read()/write() : NO-LOCKED"
else
	@echo "### read()/write() : LOCKED"
endif

print-setting-parameter-in-driver:
	@echo "###"
ifeq ($(ENABLE_SETTING_IN_DRIVER),1)
	@echo "### Setting parameters when insmod : True"
	@echo "###  - LLDMA DDR4 buffer address"
	@echo "###  - LLDMA DDR4 buffer size"
	@echo "###  - CMS reset"
else
	@echo "### Setting parameters when insmod : False"
endif

print-refcount-module-in-driver:
	@echo "###"
	@echo "### Modules which have refcount"
ifeq ($(ENABLE_REFCOUNT_GLOBAL),1)
	@echo "###   * GLOBAL"
endif
ifeq ($(ENABLE_REFCOUNT_CHAIN),1)
	@echo "###   * CHAIN"
endif
ifeq ($(ENABLE_REFCOUNT_DIRECT),1)
	@echo "###   * DIRECT"
endif
ifeq ($(ENABLE_REFCOUNT_LLDMA),1)
	@echo "###   * LLDMA"
endif
ifeq ($(ENABLE_REFCOUNT_PTU),1)
	@echo "###   * PTU"
endif
ifeq ($(ENABLE_REFCOUNT_CONV),1)
	@echo "###   * CONV"
endif
ifeq ($(ENABLE_REFCOUNT_FUNC),1)
	@echo "###   * FUNC"
endif
ifeq ($(ENABLE_REFCOUNT_CMS),1)
	@echo "###   * CMS"
endif

print-fpga-type:
	@echo "###"
	@echo "### SUPPORT FPGA TYPE"
	@echo "###  Enabled feature"
ifeq ($(ENABLE_EXTIF_INV),1)
	@echo "###   * EXTIF-INV"
endif
ifeq ($(ENABLE_MODULE_GLOBAL),1)
	@echo "###   * module : GLOBAL"
endif
ifeq ($(ENABLE_MODULE_CHAIN),1)
	@echo "###   * module : CHAIN"
endif
ifeq ($(ENABLE_MODULE_DIRECT),1)
	@echo "###   * module : DIRECT"
endif
ifeq ($(ENABLE_MODULE_LLDMA),1)
	@echo "###   * module : LLDMA"
endif
ifeq ($(ENABLE_MODULE_PTU),1)
	@echo "###   * module : PTU"
endif
ifeq ($(ENABLE_MODULE_CONV),1)
	@echo "###   * module : CONV"
endif
ifeq ($(ENABLE_MODULE_FUNC),1)
	@echo "###   * module : FUNC"
endif
ifeq ($(ENABLE_MODULE_CMS),1)
	@echo "###   * module : CMS"
endif
