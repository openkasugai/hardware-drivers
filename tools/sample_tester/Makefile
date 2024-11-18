#=================================================
# Copyright 2024 NTT Corporation, FUJITSU LIMITED
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

# binary name
APP = sample_tester

# all source are stored in SRCS-y

SRCS-y := \
src/main.c \
src/param_tables.c \
src/bcdbg.c \
src/bcdbg_thread.c \
src/common_func.c \
src/tp_func.c \
src/tp_host_host.c \
src/tp_d2d_d_host_host.c \
src/tp_d2d_h_host_host.c
INCLUDE := -I./include
LIBS := -lpciaccess

# cppfunc
OBJS := cppfunc/cppfunc.o
INCLUDE += -I./cppfunc/include
LIBS += -lstdc++ -lm -lopencv_core -lopencv_imgcodecs -lopencv_videoio -lopencv_imgproc
LDFLAGS := -L/usr/local/opencv-3.4.3/lib

# driver
INCLUDE += -I../../driver

# libfpga
LIBFPGADIR = ../../lib/libfpga
SRCS-y += \
$(LIBFPGADIR)/src/libchain.c \
$(LIBFPGADIR)/src/libchain_stat.c \
$(LIBFPGADIR)/src/libchain_err.c \
$(LIBFPGADIR)/src/libdirecttrans.c \
$(LIBFPGADIR)/src/libdirecttrans_stat.c \
$(LIBFPGADIR)/src/libdirecttrans_err.c \
$(LIBFPGADIR)/src/liblldma.c \
$(LIBFPGADIR)/src/libshmem.c \
$(LIBFPGADIR)/src/libdma.c \
$(LIBFPGADIR)/src/libdpdkutil.c \
$(LIBFPGADIR)/src/libfpga_json.c \
$(LIBFPGADIR)/src/libfpgacommon.c \
$(LIBFPGADIR)/src/libfpgabs.c \
$(LIBFPGADIR)/src/libfpgactl.c \
$(LIBFPGADIR)/src/libfpgactl_err.c \
$(LIBFPGADIR)/src/liblogging.c \
$(LIBFPGADIR)/src/libshmem_controller.c \
$(LIBFPGADIR)/src/libshmem_manager.c \
$(LIBFPGADIR)/src/libshmem_mmap.cpp \
$(LIBFPGADIR)/src/libshmem_socket.c \
$(LIBFPGADIR)/src/libfpgautil.c \
$(LIBFPGADIR)/src/libpower.c \
$(LIBFPGADIR)/src/libtemp.c \
$(LIBFPGADIR)/src/libfunction.c \
$(LIBFPGADIR)/src/libfunction_filter_resize.c \
$(LIBFPGADIR)/src/libfunction_filter_resize_stat.c \
$(LIBFPGADIR)/src/libfunction_filter_resize_err.c \
$(LIBFPGADIR)/src/libfunction_conv.c \
$(LIBFPGADIR)/src/libfunction_conv_stat.c \
$(LIBFPGADIR)/src/libfunction_conv_err.c

INCLUDE += -I$(LIBFPGADIR)/include

# JSON
JSONDIR = ../../lib/JSON
SRCS-y += $(JSONDIR)/parson/parson.c
INCLUDE += -I$(JSONDIR)/parson

# DPDK
PKGCONF ?= env PKG_CONFIG_PATH=`cd ../../; pwd`/lib/DPDK/dpdk/lib/x86_64-linux-gnu/pkgconfig pkg-config

# Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

all: static
.PHONY: shared static
shared: build/$(APP)-shared
	ln -sf $(APP)-shared build/$(APP)
static: build/$(APP)-static
	ln -sf $(APP)-static build/$(APP)

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk)
# Add flag to allow experimental API as l2fwd uses rte_ethdev_set_ptype API
CFLAGS += -DALLOW_EXPERIMENTAL_API -I../include $(INCLUDE)
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)
LDFLAGS_STATIC = $(shell $(PKGCONF) --static --libs libdpdk)

ifeq ($(MAKECMDGOALS),static)
# check for broken pkg-config
ifeq ($(shell echo $(LDFLAGS_STATIC) | grep 'whole-archive.*l:lib.*no-whole-archive'),)
$(warning "pkg-config output list does not contain drivers between 'whole-archive'/'no-whole-archive' flags.")
$(error "Cannot generate statically-linked binaries with this version of pkg-config")
endif
endif

build/$(APP)-shared: $(SRCS-y) $(OBJS) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) $(OBJS) $(LIBS) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build/$(APP)-static: $(SRCS-y) $(OBJS) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) $(OBJS) $(LIBS) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC)

build:
	@mkdir -p $@

$(OBJS):
	make -C cppfunc

.PHONY: clean
clean:
	rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	test -d build && rmdir -p build || true
	make clean -C cppfunc

