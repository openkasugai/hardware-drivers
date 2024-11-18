#=================================================
# Copyright 2024 NTT Corporation, FUJITSU LIMITED
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

# ====================================================
# User Definition
# ====================================================
# must be absolute path
# target directory
BUILD_DIR ?= $(shell pwd)/build

# suppress command line text, if you need text remove '@'
MAKE  := @make --no-print-directory
MKDIR := @mkdir
CP    := @cp
RM    := @rm
SED   := @sed
CC    := @$(CC)
CXX   := @$(CXX)
AR    := @$(AR)

# ====================================================
# Library Definition
# ====================================================
# library name
LIB := libfpga

# libfpga description
LIBRARY_TYPE_DEFAULT := 0

# v(Major.Minor.Revision-Patch)
LIBRARY_NAME       := $(LIB)
LIBRARY_TYPE       := $(LIBRARY_TYPE_DEFAULT)
LIBRARY_VERSION1   :=1
LIBRARY_VERSION2   :=0
LIBRARY_REVISION_H :=0
LIBRARY_REVISION_L :=0
LIBFPGA_VERSION    := type:$(LIBRARY_TYPE) v.$(LIBRARY_VERSION1).$(LIBRARY_VERSION2).$(LIBRARY_REVISION_H)-$(LIBRARY_REVISION_L)

# library name for each purpose
LIBFPGACTL  := libfpgactl
LIBCHAIN    := libchain
LIBDIRECT   := libdirecttrans
LIBCONV     := libfunction_conv
LIBFUNCTION := libfunction
LIBFPGABS   := libfpgabs
LIBLLDMA    := liblldma
LIBDMA      := libdma
LIBSHMEM    := libshmem
LIBSHMEMSYS := libshmem_controller
LIBLOGGING  := liblogging
LIBCMSCTL   := libcmsctl
LIBPTU      := libptu

# library name for a specific function
LIBFUNCTION_FILTER_RESIZE := libfunction_filter_resize

# libraries used as default functions by libfunction
LIBFUNCTION_DEFAULT := \
       $(LIBFUNCTION_FILTER_RESIZE)

# libraries NOT used as default functions by libfunction
LIBFUNCTION_EXTERNAL :=

# default libfpga libraries
LIBDEFAULT := $(LIBFPGACTL)\
              $(LIBCHAIN)\
              $(LIBDIRECT)\
              $(LIBLLDMA)\
              $(LIBDMA)\
              $(LIBCMSCTL)\
              $(LIBPTU)\
              $(LIBSHMEM)\
              $(LIBSHMEMSYS)\
              $(LIBLOGGING)\
              $(LIBFPGABS)\
              $(LIBFUNCTION)\
              $(LIBFUNCTION_DEFAULT)\
              $(LIBCONV)

# external libfpga libraries
LIBEXT := $(LIBFUNCTION_EXTERNAL)

# all libfpga libraries
LIBALL := $(LIBDEFAULT) $(LIBEXT)

# DPDK version
DPDK_VERSION := 23.11.1

# pkg-config file name for libfpga
LIBFPGAPC := libfpga.pc

# set color for 'echo'
ifeq ($(SHELL),/bin/bash)
DECORATE := -e
endif
DECORATE_B := \e[33;1m# 33:yellow;1:bold
DECORATE_E := \e[m

FPGA_UNUSE_SERIAL_ID ?= 0


# ====================================================
# Source files
# ====================================================
# Set directory for source files
SRCDFPGA  := $(shell cd libfpga;pwd)
SRCDPTU   := $(shell cd libptu;pwd)
DRIVERDIR := $(shell cd ../driver;pwd)
DPDK_DIR  := $(shell cd DPDK;pwd)
JSON_DIR  := $(shell cd JSON;pwd)
MCAP_DIR  := $(shell cd MCAP;pwd)
LD_CONF_DIR := /etc/ld.so.conf.d
INC_CONF_DIR := /etc/profile.d

# ----------------------------------------------------
# header files
# ----------------------------------------------------
LIBFPGA_HEADERS :=\
	$(wildcard $(SRCDFPGA)/include/*.h*)\
	$(wildcard $(SRCDPTU)/include/*.h*)
XPCIE_DRIVER_HEADERS :=\
	$(DRIVERDIR)/xpcie_device.h
ALL_HEADERS := $(LIBFPGA_HEADERS) $(XPCIE_DRIVER_HEADERS)
LINK_HEADERS := $(addprefix $(INSTALL_INC_DIR)/,$(notdir $(ALL_HEADERS)))

# ----------------------------------------------------
# source files
#   Set all source files for <library-name> into SRCS-<library-name>
#   Set all source files into SRCS
# ----------------------------------------------------
# Set strange name rule files at first
SRCS-libfunction   := $(SRCDFPGA)/src/libfunction.c
SRCS-libcmsctl     := $(SRCDFPGA)/src/libpower.c\
                      $(SRCDFPGA)/src/libtemp.c
SRCS-libptu        := $(SRCDPTU)/src/libptu.cpp\
                      $(SRCDPTU)/src/ptu_dev.cpp\
                      $(SRCDPTU)/src/ptu_reg_func.cpp
SRCS-libshmem      := $(SRCDFPGA)/src/libshmem.c\
                      $(SRCDFPGA)/src/libshmem_manager.c\
                      $(SRCDFPGA)/src/libshmem_mmap.cpp
SRCS-libshmem_controller :=\
                      $(SRCDFPGA)/src/libshmem_controller.c\
                      $(SRCDFPGA)/src/libshmem_socket.c
SRCS-libfpgacommon := $(SRCDFPGA)/src/libfpgautil.c\
                      $(SRCDFPGA)/src/libfpgacommon.c\
                      $(SRCDFPGA)/src/libfpga_json.c\
                      $(JSON_DIR)/parson/parson.c
SRCS-libshmemcommon:= $(SRCDFPGA)/src/libdpdkutil.c

# Set default name rule files into SRCS-<library name>,
#  when not set as strange name rule files
define srcs-each-libs
ifndef SRCS-${1}
SRCS-${1} += $(wildcard $(SRCDFPGA)/src/${1}*.c) $(wildcard $(SRCDFPGA)/src/${1}*.cpp)
endif
endef
$(foreach lib,$(LIBALL),$(eval $(call srcs-each-libs,$(lib))))

# Set common source files into SRCS-libfpgactl/SRCS-libshmem
SRCS-libfpgactl += $(SRCS-libfpgacommon)
SRCS-libshmem   += $(SRCS-libshmemcommon)

# Set all source files for default libraries into SRCS_DEFAULT
# Set all source files for external libraries into SRCS_EXT
define srcs-all-libs
${2} += $(SRCS-${1})
endef
$(foreach lib,$(LIBDEFAULT),$(eval $(call srcs-all-libs,$(lib),SRCS_DEFAULT)))
$(foreach lib,$(LIBEXT),$(eval $(call srcs-all-libs,$(lib),SRCS_EXT)))

# Set all source files into SRCS
SRCS = $(SRCS_DEFAULT) $(SRCS_EXT)

# ----------------------------------------------------
# object files
#   Set all object files for <library-name> into OBJS-<library-name>
#   Set all object files into OBJS
# ----------------------------------------------------
# Set all object files for <library-name> into OBJS-<library-name>
define objs-each-libs
OBJS-${1} = $(addsuffix .o,$(SRCS-${1}))
endef
$(foreach lib,$(LIBALL),$(eval $(call objs-each-libs,$(lib))))

# Set all object files for default libraries into OBJS_DEFAULT
# Set all object files for external libraries into OBJS_EXT
# store all source file into OBJS
define objs-all-libs
${2} += $(OBJS-${1})
endef
$(foreach lib,$(LIBDEFAULT),$(eval $(call objs-all-libs,$(lib),OBJS_DEFAULT)))
$(foreach lib,$(LIBEXT),$(eval $(call objs-all-libs,$(lib),OBJS_EXT)))

# Set all object files into OBJS
OBJS = $(OBJS_DEFAULT) $(OBJS_EXT)

# ----------------------------------------------------
# library file
#   set direcotry and suffix
# ----------------------------------------------------
# Set all static library files for default libraries into LIBDEFAULT
LIBS_STATIC := $(patsubst %,$(BUILD_DIR)/%.a,$(LIBDEFAULT))

# Set all shared library files for default libraries into LIBDEFAULT
LIBS_SHARED := $(patsubst %,$(BUILD_DIR)/%.so,$(LIBDEFAULT))

# Set all shared library files for external libraries into LIBEXT
LIBS_SHARED_EXT := $(patsubst %,$(BUILD_DIR)/%.so,$(LIBEXT))

# Libraries for DPDK
LIBDPDK := $(basename $(notdir $(wildcard $(DPDK_DIR)/dpdk/lib/x86_64-linux-gnu/*.a)))


# ====================================================
# C(XX)FLAGS
# ====================================================
# Set -g flag for gdb if need
ifeq ($(MAKECMDGOALS),debug)
COV_OPT := -g --coverage
endif
COV_OPT ?= 

ENABLE_MOCK_TEST?=1

# Set library info as macro of C
CFLAGS+= -DLIBRARY_NAME=\"$(LIBRARY_NAME)\"
CFLAGS+= -DLIBRARY_TYPE=$(LIBRARY_TYPE)
CFLAGS+= -DLIBRARY_VERSION1=$(LIBRARY_VERSION1)
CFLAGS+= -DLIBRARY_VERSION2=$(LIBRARY_VERSION2)
CFLAGS+= -DLIBRARY_REVISION_H=$(LIBRARY_REVISION_H)
CFLAGS+= -DLIBRARY_REVISION_L=$(LIBRARY_REVISION_L)

# Set include path for gcc
CFLAGS += -O3 -Wall $(COV_OPT)\
	-I$(SRCDFPGA)/include \
	-I$(SRCDPTU)/include \
	-I$(SRCDPTU)/src \
	-I$(DRIVERDIR) 

# Avoid issuing benign warnings for gcc
CFLAGS += -Wno-address-of-packed-member

# Set include path for g++
CXXFLAGS += \
	$(COV_OPT) \
	-I../driver \
	-I./libfpga/include \
	-I./libptu/include \
	-I./libptu/src \
	-I./DPDK/dpdk/include

# Enable external function register
CFLAGS += $(patsubst %,-Denable_external_%,$(LIBFUNCTION_EXTERNAL))

# Decide FPGA's character device file's name
ifeq ($(FPGA_UNUSE_SERIAL_ID),1)
$(info ### Activate Flag : FPGA_UNUSE_SERIAL_ID)
CFLAGS += -DFPGA_UNUSE_SERIAL_ID
CXXFLAGS += -DFPGA_UNUSE_SERIAL_ID
endif


# ====================================================
# COMMAND
# ====================================================
.PHONY: all default static shared debug clean
.PHONY: dpdk mcap json
.PHONY: dpdk-uninstall mcap-uninstall json-uninstall


COMMAND-NO_NEED_BUILD := clean dpdk mcap json\
                         dpdk-uninstall mcap-uninstall json-uninstall

# ----------------------------------------------------
# Command for default library
default: all

# Command for remake library
all: clean static

# Command for build <static default libraries> and <shared external libraries>
static:\
	print-build-directory\
	$(BUILD_DIR)/$(LIB).a\
	$(LIBS_SHARED_EXT)\
	$(BUILD_DIR)/pkgconfig/$(LIBFPGAPC)
	@echo "[$(LIB)]Build library($(LIBFPGA_VERSION)) Succeed!!"

# Command for build <shared default libraries> and <shared external libraries>
shared:\
	print-build-directory\
	shared-cflags-enable\
	$(BUILD_DIR)/$(LIB).so\
	$(LIBS_SHARED_EXT)\
	$(BUILD_DIR)/pkgconfig/$(LIBFPGAPC)
	@echo "[$(LIB)]Build Shared library($(LIBFPGA_VERSION)) Succeed!!"

# Command for build <static> with -g option
debug: debug-cflags-enable static

# Command for clean build things
clean:
	@$(call clean-foreach,$(OBJS),object file,$(LIB))
	@$(call clean-foreach,$(LIBS_STATIC),static library,$(LIB))
	@$(call clean-element,$(BUILD_DIR)/$(LIB).a,static library,$(LIB))
	@$(call clean-foreach,$(LIBS_SHARED),shared library,$(LIB))
	@$(call clean-foreach,$(LIBS_SHARED_EXT),shared library,$(LIB))
	@$(call clean-element,$(BUILD_DIR)/$(LIB).so,shared library,$(LIB))
	@$(call clean-element,$(BUILD_DIR)/include,include files,$(LIB))
	@$(call clean-element,$(BUILD_DIR)/pkgconfig,pkgconfig files,$(LIB))
	@test -d $(BUILD_DIR)\
		&& echo "[$(LIB)]delete library directory[$(BUILD_DIR)]"\
		&& rmdir $(BUILD_DIR) || true


# =======================================================
# CREATE pkgconfig file
# =======================================================
# Set paths
PC_PREFIX=$(BUILD_DIR)
PC_INCDIR=$(PC_PREFIX)/include
PC_LIBDIR=$(PC_PREFIX)

# Create pkgconfig file from .in file
$(BUILD_DIR)/pkgconfig/$(LIBFPGAPC): $(LIBFPGAPC).in $(PC_INCDIR)/libfpga | $(BUILD_DIR)/pkgconfig
	$(SED) <$< >$@ -e 's,@PREFIX@,$(PC_PREFIX),' \
		-e 's,@INCDIR@,$(PC_INCDIR),' \
		-e 's,@LIBDIR@,$(PC_LIBDIR),' \
		-e 's,@VERSION@,$(LIBFPGA_VERSION),' \
		-e 's,@LDLIBS@,$(LDLIBS),' \
		-e 's,@WITH_LIBS@,$(WITH_LIBS),'
	@echo "[$(LIB)]create $@"

# Create direcotry for include files and,
# Copy include files into the directory
#PC_INCS: $(PC_INCDIR)/libfpga $(PC_INCDIR)/driver
$(PC_INCDIR)/libfpga: $(ALL_HEADERS) | $(PC_INCDIR)
	$(MKDIR) -p $@
	$(CP) $^ $@
	@touch $@
	@echo "[$(LIB)]copy header file from $(subst $(shell cd ..;pwd)/,fpga-software/,$(SRCDFPGA)/include/) into $@/"
	@echo "[$(LIB)]copy header file from $(subst $(shell cd ..;pwd)/,fpga-software/,$(SRCDPTU)/include/) into $@/"
	@echo "[$(LIB)]copy header file from $(subst $(shell cd ..;pwd)/,fpga-software/,$(DRIVERDIR)/xpcie_device.h) into $@/"
$(PC_INCDIR): | $(PC_PREFIX)
	$(MKDIR) -p $@

# =======================================================
# build other library
# =======================================================
# Set path for dpdk
INSTALL_PREFIX:=$(DPDK_DIR)/dpdk
DPDK_COMPRESSED := $(DPDK_DIR)/dpdk-$(DPDK_VERSION).tar.xz
DPDK_DECOMPRESSED := $(DPDK_DIR)/dpdk-stable-$(DPDK_VERSION)
DPDK_MESON_BUILD_OPTIONS := \
	--prefix=$(INSTALL_PREFIX) \
	-Dplatform=generic \
	-Ddisable_drivers=*/* \
	-Ddisable_libs=* \
	-Ddisable_apps=*

# Command for get/build DPDK
dpdk: dpdk-get dpdk-install

# wget dpdk from web
dpdk-get: $(DPDK_DECOMPRESSED)
$(DPDK_DECOMPRESSED): $(DPDK_COMPRESSED) | $(DPDK_DIR)
	cd $(DPDK_DIR) && tar Jxvf $(DPDK_COMPRESSED)
$(DPDK_COMPRESSED): | $(DPDK_DIR)
	cd $(DPDK_DIR) && wget http://fast.dpdk.org/rel/$(notdir $(DPDK_COMPRESSED))

# build dpdk
dpdk-install: $(INSTALL_PREFIX)
$(INSTALL_PREFIX):
	@cd $(DPDK_DECOMPRESSED) && meson $(DPDK_MESON_BUILD_OPTIONS) build
	@cd $(DPDK_DECOMPRESSED)/build && ninja && ninja install

# clean built dpdk
dpdk-uninstall:
	cd $(DPDK_DIR) && rm -rf dpdk*

$(DPDK_DIR):
	$(MKDIR) -p $@

# copy pkgconfig file of DPDK
$(BUILD_DIR)/pkgconfig/libdpdk.pc: $(PC_FILE) | $(BUILD_DIR)/pkgconfig
	$(CP) $(DPDK_DIR)/dpdk/lib/x86_64-linux-gnu/pkgconfig/*.pc $(BUILD_DIR)/pkgconfig
	@echo [$(LIB)]copy DPDK\'s pc file into $@

# -------------------------------------------------------
# Set path for MCAP
PCIUTILS_REPO:=https://github.com/pciutils/pciutils.git
PCIUTILS_VER:=v3.13.0
PCIUTILS_SRCDIR:=$(MCAP_DIR)/pciutils
PCIUTILS_LIB:=$(PCIUTILS_SRCDIR)/lib/libpci.a
MCAP_REPO:=https://github.com/Xilinx/embeddedsw.git
MCAP_VER:=xilinx_v2023.1
MCAP_SRCDIR:=$(MCAP_DIR)/embeddedsw/mcap/linux
MCAP_BIN:=mcap

# Command for get/build MCAP
mcap: mcap-get mcap-install

# Clone mcap from web
mcap-get: $(MCAP_SRCDIR)
$(MCAP_SRCDIR): | $(MCAP_DIR)
	cd $(MCAP_DIR) && git clone $(MCAP_REPO) -b $(MCAP_VER) --depth 1

# Command for build MCAP
mcap-install: $(MCAP_DIR)/$(MCAP_BIN)
$(MCAP_DIR)/$(MCAP_BIN): $(PCIUTILS_LIB) $(MCAP_SRCDIR)
	@patch $(MCAP_SRCDIR)/mcap.c $(MCAP_DIR)/patch/mcap.c.patch -o $(MCAP_DIR)/mcap.c
	@patch $(MCAP_SRCDIR)/mcap_lib.c $(MCAP_DIR)/patch/mcap_lib.c.patch -o $(MCAP_DIR)/mcap_lib.c
	@patch $(MCAP_SRCDIR)/mcap_lib.h $(MCAP_DIR)/patch/mcap_lib.h.patch -o $(MCAP_DIR)/mcap_lib.h
	@cp $(MCAP_SRCDIR)/Makefile $(MCAP_DIR)/Makefile
	$(MAKE) -C $(MCAP_DIR) PCIUTILS_PATH=$(PCIUTILS_SRCDIR)
	@echo $(DECORATE) \
		"$(DECORATE_B)"\
		' * Please set path of mcap into 'PATH', or mv mcap to the path included in PATH for sudo.\n' \
		' * Examples:\n' \
		' *  $$ mv $(MCAP_DIR)/$(MCAP_BIN) /usr/local/bin' \
		"$(DECORATE_E)"

# clean built MCAP
mcap-uninstall:
	test -e $(MCAP_DIR)/Makefile && make -C $(MCAP_DIR) clean || true
	cd $(MCAP_DIR)/ && rm -rf embeddedsw *.c *.h Makefile $(PCIUTILS_SRCDIR)

# get pciutils
$(PCIUTILS_SRCDIR): | $(MCAP_DIR)
	cd $(MCAP_DIR) && git clone $(PCIUTILS_REPO) -b $(PCIUTILS_VER) --depth 1

$(PCIUTILS_LIB): $(PCIUTILS_SRCDIR)
	$(MAKE) -C $(PCIUTILS_SRCDIR) HWDB=no

# -------------------------------------------------------
# Command for get JSON PARSER library(parson) file 
json: | $(JSON_DIR)/parson/parson.c

# get parson.c parson.h
$(JSON_DIR)/parson/parson.c:
	cd $(JSON_DIR) && rm -rf parson && git clone https://github.com/kgabis/parson.git parson

json-uninstall:
	cd $(JSON_DIR) && rm -rf parson

# ====================================================
# Check PKG-CONFIG and get GFLAGS to use DPDK/parson
# ====================================================
# must be absolute path
# set direcotry where DPDK were built
PKGCONF_PATH := $(DPDK_DIR)/dpdk/lib/x86_64-linux-gnu/pkgconfig:

# command for get CFLAGS for DPDK
PKGCONF ?= env PKG_CONFIG_PATH="$(PKGCONF_PATH)" pkg-config

# ignore when No need to build libfpga
# Check and Get pkg-config flags
ifeq ($(MAKECMDGOALS),)
NEED_BUILD:=1
else
ifneq ($(filter-out $(COMMAND-NO_NEED_BUILD),$(MAKECMDGOALS)),)
NEED_BUILD:=1
endif
endif

# Check and get pkg-config
ifeq ($(NEED_BUILD),1)
# ----------------------------------------------------
# DPDK
# Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif
# ----------------------------------------------------
# CFLAGS for build libfpga with other libraries
CFLAGS+=$(shell $(PKGCONF) --cflags libdpdk)
CFLAGS+=-I./JSON/parson/
endif


# ====================================================
# Rules for library
# ====================================================
# information
print-build-directory:
	@echo $(DECORATE) "[$(LIB)]build direcotry: $(DECORATE_B)$(BUILD_DIR)$(DECORATE_E)"

# ----------------------------------------------------
# Set CFLAGS if need
shared-cflags-enable:
	$(eval SHARED-FLAGS += -fPIC)
	@echo $(DECORATE) "[$(LIB)]build option: Activate $(DECORATE_B)shared option(-fPIC)$(DECORATE_E)"

debug-cflags-enable:
	$(eval CFLAGS += -g)
	@echo $(DECORATE) "[$(LIB)]build option: Activate $(DECORATE_B)debug option(-g)$(DECORATE_E)"

# ----------------------------------------------------
# Build object files
$(OBJS_DEFAULT): $(ALL_HEADERS) $(SRCS_DEFAULT)
$(OBJS_EXT): $(ALL_HEADERS) $(SRCS_EXT)

%.c.o: %.c
	@echo "[$(LIB)]build $@"
	$(CC) -c $(CFLAGS) $(SHARED-FLAGS) -o $@ $<

%.cpp.o: %.cpp
	@echo "[$(LIB)]build $@"
	$(CXX) -c $(CXXFLAGS) $(SHARED-FLAGS) -o $@ $<

%.o: %.cpp
	@echo "[$(LIB)]build $@"
	$(CXX) -c $(CXXFLAGS) $(SHARED-FLAGS) -o $@ $<

# ----------------------------------------------------
# no-meaning parameter, just for switch template's $$ or $
defined-process := 1

# template for build static library
build-static-library-template =\
	$(AR) rcs $(BUILD_DIR)/${1}.a ${2} ;\
	$(if ${3}\
	,$$(eval LDLIBS+=-l:${1}.a)\
	 $$(eval WITH_LIBS+=-$(patsubst lib%,l%,${1}))\
	,$(eval LDLIBS+=-l:${1}.a)\
	 $(eval WITH_LIBS+=-$(patsubst lib%,l%,${1}))\
	)\
	echo $(DECORATE) "[$(LIB)]build static library: $(DECORATE_B)${1}.a$(DECORATE_E)"

# template for build shared library
build-shared-library-template =\
	$(CC) -shared $(CFLAGS) -o $(BUILD_DIR)/${1}.so ${2} ;\
	$(if ${3}\
	,$$(eval WITH_LIBS+=-$(patsubst lib%,l%,${1}))\
	,$(eval WITH_LIBS+=-$(patsubst lib%,l%,${1}))\
	)\
	echo $(DECORATE) "[$(LIB)]build shared library: $(DECORATE_B)${1}.so$(DECORATE_E)"\

# build libfpga.a
$(BUILD_DIR)/$(LIB).a: pkgconfig-libs $(OBJS_DEFAULT) Makefile
	$(call build-static-library-template,$(LIB),$(OBJS_DEFAULT))

# build libfpga.so
$(BUILD_DIR)/$(LIB).so: shared-cflags-enable pkgconfig-libs $(OBJS_DEFAULT) Makefile
	$(call build-shared-library-template,$(LIB),$(OBJS_DEFAULT))

# build each library
define build-library-template
$(BUILD_DIR)/${1}.a: pkgconfig-libs $(OBJS-${1}) Makefile
	$(call build-static-library-template,${1},$(OBJS-${1}),defined-process)

$(BUILD_DIR)/${1}.so: shared-cflags-enable pkgconfig-libs $(OBJS-${1}) Makefile
	$(call build-shared-library-template,${1},$(OBJS-${1}),defined-process)

endef
$(foreach lib,$(LIBALL),$(eval $(call build-library-template,$(lib))))

# ----------------------------------------------------
# get other libraries' pkg-config files for libfpga user
pkgconfig-libs:  | \
	$(BUILD_DIR) \
	$(BUILD_DIR)/pkgconfig/libdpdk.pc

# ----------------------------------------------------
# Make direcotry for libfpga
$(BUILD_DIR):
	$(MKDIR) -p $@
$(BUILD_DIR)/pkgconfig: | $(BUILD_DIR)
	$(MKDIR) -p $@

# ----------------------------------------------------
# Rule clean files and echo file name

# template text
CLEAN_MSG = echo '[${3}]delete ${2}[${1}]'

# clean if exist
define clean-element
	test -e ${1} && rm -rf ${1} && $(call CLEAN_MSG,${1},${2},${3}) || true

endef

# clean list
define clean-foreach
	$(foreach obj,${1},$(call clean-element,$(obj),${2},${3}))

endef

