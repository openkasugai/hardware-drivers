#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

.PHONY: all clean

TOP := ../../..
TEST_TOP := ../..

XSE ?= 1
PCM_POWER ?= ~/work/pcm/build/bin/pcm-power
TEST_CONFIG ?=

IDDMA_INCLUDE_PATH := $(TOP)/include/iddma
IDDMA_LIB_PATH := $(TOP)/lib

CUDA_TOP ?= /usr/local/cuda
CUDA_INCLUDE_PATH := $(CUDA_TOP)/include
CUDA_LIB_PATH := $(CUDA_TOP)/lib64

MM_LIB_PATH := $(TOP)/lib
FUNCTION_BASE_INC_PATH := $(TOP)/include/function_base
LIBCOMMON_INC_PATH := $(TOP)/include/common
ROUTE_CONTROLLER_INC_PATH := $(TOP)/include/route_controller
XSE_LDFLAGS := -lstream_engine -lroute_controller -lfunction_base -lxsecommon

GTEST_INCLUDE_PATH := $(TEST_TOP)/3rdparty/googletest/googletest/include
GTEST_LIB_PATH := $(TEST_TOP)/3rdparty/googletest/build/lib

CC := g++
OPT ?=

SET_FENV ?= LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./fpga/$(IDDMA_LIB_PATH):./fpga/$(MM_LIB_PATH)
SET_TCP_FENV ?= $(SET_FENV) \
		XSE_RX_PORT_BASE=$(XSE_RX_PORT_BASE) XSE_TX_PORT_BASE=$(CPU_PORT_BASE) \
		XSE_TX_TARGET_IP=$(CPU_IP)

include ../config.make

vpath %.cpp $(TOP)/src/lib/iddma/util

SRCS_XSE := test_xse.cpp test_xse_tcp.cpp test.cpp
SRCS_XSE += tcp_cmd.cpp
OBJS_XSE := $(notdir $(SRCS_XSE:%.cpp=%.o))
#DEFS ?= -D__SUPPORT_GPU_NV__ -D__USE_FPGA_XSE__
DEFS ?= -D__USE_FPGA_XSE__

CC_FLAG := -I. -I.. -I../../../src/util -I../gpu
CC_FLAG += -I$(CUDA_INCLUDE_PATH)

LDFLAGS := -L$(MM_LIB_PATH) -lmem_manage
LDFLAGS += -L$(IDDMA_LIB_PATH) -liddma_xse $(XSE_LDFLAGS)
LDFLAGS += -L$(GTEST_LIB_PATH) -lgtest -lgtest_main
LDFLAGS += -lpthread
INCLUDEFLAGS := -I$(IDDMA_INCLUDE_PATH)
INCLUDEFLAGS += -I$(GTEST_INCLUDE_PATH)
INCLUDEFLAGS += -I$(LIBCOMMON_INC_PATH)
INCLUDEFLAGS += -I$(ROUTE_CONTROLLER_INC_PATH)
INCLUDEFLAGS += -I$(FUNCTION_BASE_INC_PATH)
INCLUDEFLAGS += -I$(TOP)/src/lib/iddma/util
INCLUDEFLAGS += $(TEST_CONFIG)

ifeq ($(DEBUG), 1)
	CC_FLAG += -D__KTP_DEBUG__ -D_DEBUG
endif

ifeq ($(XSE), 1)
	EXEC_XSE := test_xse_exec
endif

vpath %.cpp ..

all: $(EXEC_XSE)

test: $(EXEC_XSE)
	cd .. && sudo $(SET_FENV) ./fpga/$(EXEC_XSE) --gtest_filter=-*TestTcp*

test_tcp: $(EXEC_XSE)
	cd .. && sudo $(SET_TCP_FENV) ./fpga/$(EXEC_XSE) --gtest_filter=*TestTcp*

test_xse_nop: $(EXEC_XSE)
	cd .. && sudo $(SET_FENV) ./fpga/$(EXEC_XSE) --gtest_filter=*.TestNop*

$(EXEC_XSE): $(OBJS_XSE)
	echo $^
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: ../%.cpp
	$(CC) $(CC_FLAG) $(DEFS) -o $@ -c $< $(INCLUDEFLAGS)

%.o: ../gpu/%.cpp
	$(CC) $(CC_FLAG) $(DEFS) -o $@ -c $< $(INCLUDEFLAGS)

%.o: ../../../src/util/%.cpp
	$(CC) $(CC_FLAG) $(DEFS) -o $@ -c $< $(INCLUDEFLAGS)

%.o: %.cpp
	$(CC) $(CC_FLAG) $(DEFS) -o $@ -c $< $(INCLUDEFLAGS)

clean:
	@rm -f *.o
	@rm -f $(EXEC_XSE) ../test.o
