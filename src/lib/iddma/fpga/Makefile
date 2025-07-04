#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================
# CPU Makefile

.PHONY: all clean

DEBUG ?= 1
LATENCY ?= 0

BASE_TOP := ..
SW_TOP := $(BASE_TOP)/../../..
UTIL_TOP := $(BASE_TOP)/util
LIB_PATH ?= $(SW_TOP)/lib

DEBUG_FLAGS :=
ifeq ($(DEBUG),1)
	DEBUG_FLAGS += -D__KTP_DEBUG__ -D_DEBUG
endif

ifeq ($(LATENCY),1)
	DEBUG_FLAGS += -D__DEBUG_LATENCY__
endif

# CUDA settings
CUDA_TOP ?= /usr/local/cuda
CUDA_LD_FLAGS := -L$(CUDA_TOP)/lib64 -lcudart -lcuda
CUDA_INCLUDE_FLAGS := -I$(CUDA_TOP)/include/
CUDA_UTIL_SRCS := $(UTIL_TOP)/iddma_cuda_util.cpp

NVCC := $(CUDA_TOP)/bin/nvcc
NV_C_FLAGS := -gencode arch=compute_70,code=sm_70 -gencode arch=compute_80,code=sm_80 -gencode arch=compute_80,code=compute_80 -rdc=true -Xcompiler -fPIC

MM_TOP ?= $(BASE_TOP)/../../mem_manage
MM_INCLUDE_FLAGS := -I$(SW_TOP)/include/mem_manage
MM_LD_FLAGS := -L$(SW_TOP)/lib -lmem_manage
# end of CUDA settings

# XSE settings
LIBCOMMON_INC_PATH := $(SW_TOP)/include/common
STREAM_ENGINE_INC_PATH := $(SW_TOP)/include/stream_engine
ROUTE_CONTROLLER_INC_PATH = $(SW_TOP)/include/route_controller
TOE_INC_PATH = $(SW_TOP)/include/toe
XSE_LDFLAGS := -lstream_engine -ltoe
# end of XSE settings

SRCS := $(BASE_TOP)/iddma.cpp \
		$(BASE_TOP)/iddma_common.cpp \
		$(UTIL_TOP)/ftok_util.cpp \
		$(UTIL_TOP)/iddma_memory.cpp \
		$(UTIL_TOP)/iddma_memory_map.cpp \
		$(UTIL_TOP)/tcp_cmd.cpp \
		$(UTIL_TOP)/tcp_ctrl_tunnel.cpp \
		$(UTIL_TOP)/shmem_ctrl_tunnel.cpp \
		$(CUDA_UTIL_SRCS) \
		iddma_fpga.cpp \
		iddma_fpga_xse.cpp \
		tcp_xse_ctrl_tunnel.cpp

DEFS ?= -D__USE_FPGA_XSE__ 
WITH_NV := $(findstring _GPU_NV__, $(DEFS))

CC := g++
INCLUDE_FLAGS := -I. \
				 -I$(BASE_TOP) \
				 -I$(UTIL_TOP) \
				 -I$(SW_TOP)/include/iddma \
				 -I$(LIBCOMMON_INC_PATH) \
				 -I$(STREAM_ENGINE_INC_PATH) \
				 -I$(TOE_INC_PATH)
C_FLAGS := $(DEFS) $(INCLUDE_FLAGS) -fPIC $(DEBUG_FLAGS)
LD_FLAGS := -Wl,-rpath,'$$ORIGIN'
# LD_FLAGS += -L$(FDMA_LIB_PATH) -lfpga $(DPDK_LD_FLAGS)

ifneq ($(WITH_NV),)
C_FLAGS += $(CUDA_INCLUDE_FLAGS) $(MM_INCLUDE_FLAGS)
LD_FLAGS += $(CUDA_LD_FLAGS) $(MM_LD_FLAGS) $(XSE_LDFLAGS)
SRCS += $(CUDA_UTIL_SRCS)
else
C_FLAGS += $(MM_INCLUDE_FLAGS)
LD_FLAGS += $(MM_LD_FLAGS) $(XSE_LDFLAGS)
SRCS += $(CUDA_UTIL_SRCS)
endif

NV_C_FLAGS += $(INCLUDE_FLAGS) $(CUDA_INCLUDE_FLAGS) $(MM_INCLUDE_FLAGS)

LIB_TYPE := xse

OBJS := $(SRCS:%.cpp=%.$(LIB_TYPE).o)

SO := $(LIB_PATH)/libiddma_$(LIB_TYPE).so

all: $(SO)

$(SO): $(OBJS)
	@mkdir -p $(LIB_PATH)
	$(CC) --shared -o $@ $^ $(LD_FLAGS)

%.$(LIB_TYPE).o: %.cpp
	$(CC) $(C_FLAGS) -o $@ -c $<

clean:
	@rm -f $(SO) $(OBJS) *.$(LIB_TYPE).o
	@rm -f *~;
