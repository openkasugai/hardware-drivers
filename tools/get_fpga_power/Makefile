#=================================================
# Copyright 2024 NTT Corporation, FUJITSU LIMITED
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

# Should be absolute path
LIB_DIR := ../../lib
LIB_BUILD_DIR=$(shell cd $(LIB_DIR);pwd)/build

APP_VERSION := \"1.0.0\"

# binary name
APP = get_fpga_power

# ====================================================
# all source are stored in SRCS
SRCS := main.c

CC := @$(CC)

PKGCONF ?= env PKG_CONFIG_PATH=$(LIB_BUILD_DIR)/pkgconfig pkg-config

CFLAGS += -DAPP_VERSION=$(APP_VERSION)
CFLAGS += $(shell $(PKGCONF) --cflags libfpga)
LDFLAGS += $(shell $(PKGCONF) --static --libs libfpga)

# ====================================================
# COMMAND
# ====================================================

# APP remake command
.PHONY: all
all: clean static

# static APP make command
.PHONY: static
static: $(SRCS) | $(LIB_BUILD_DIR)
	$(CC) $^ $(LDFLAGS) $(CFLAGS) -o $(APP)
	@echo build APP[$(APP)]

# APP delete command
.PHONY: clean
clean:
	rm $(APP) -f

$(LIB_BUILD_DIR):
	@make -C $(LIB_DIR) dpdk
	@make -C $(LIB_DIR) mcap
	@make -C $(LIB_DIR) json
	@make -C $(LIB_DIR)
