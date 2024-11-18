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
APP-r := reg32r
APP-w := reg32w

# ====================================================
SRCS-r := reg32r.c
SRCS-w := reg32w.c

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
all: clean read write

# reg32r make command
.PHONY: read
read: $(SRCS-r) | $(LIB_BUILD_DIR)
	$(CC) $^ $(LDFLAGS) $(CFLAGS) -o $(APP-r)
	@echo build APP[$(APP-r)]

# reg32w make command
.PHONY: write
write: $(SRCS-w) | $(LIB_BUILD_DIR)
	$(CC) $^ $(LDFLAGS) $(CFLAGS) -o $(APP-w)
	@echo build APP[$(APP-w)]

# APP delete command
.PHONY: clean
clean:
	rm $(APP-r) $(APP-w) -f

$(LIB_BUILD_DIR):
	@make -C $(LIB_DIR) dpdk
	@make -C $(LIB_DIR) mcap
	@make -C $(LIB_DIR) json
	@make -C $(LIB_DIR)
