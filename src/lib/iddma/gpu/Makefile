#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================
# CPU Makefile

.PHONY: all clean

DEBUG ?= 0
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
NV_C_FLAGS := -gencode arch=compute_70,code=sm_70 -gencode arch=compute_80,code=sm_80 -gencode arch=compute_80,code=compute_80 -Xcompiler -fPIC

MM_TOP ?= $(BASE_TOP)/../../mem_manage
MM_INCLUDE_FLAGS := -I$(SW_TOP)/include/mem_manage
MM_LD_FLAGS := -L$(SW_TOP)/lib -lmem_manage_nv
# end of CUDA settings

SRCS := $(BASE_TOP)/iddma.cpp \
		$(BASE_TOP)/iddma_common.cpp \
		$(UTIL_TOP)/ftok_util.cpp \
		$(UTIL_TOP)/iddma_memory.cpp \
		$(UTIL_TOP)/iddma_memory_map.cpp \
		$(UTIL_TOP)/tcp_cmd.cpp \
		$(UTIL_TOP)/tcp_ctrl_tunnel.cpp \
		$(UTIL_TOP)/shmem_ctrl_tunnel.cpp \
		iddma_nv_dma.cpp \
		$(CUDA_UTIL_SRCS)
CU_SRCS := iddma_gpu.cu

DEFS ?= -D__USE_GPU_NV__

CC := g++
INCLUDE_FLAGS := -I. \
				 -I$(BASE_TOP) \
				 -I$(UTIL_TOP) \
				 -I$(SW_TOP)/include/iddma
C_FLAGS := $(DEFS) $(INCLUDE_FLAGS) -fPIC $(DEBUG_FLAGS)
LD_FLAGS := -gencode arch=compute_70,code=sm_70 -gencode arch=compute_80,code=sm_80 -gencode arch=compute_80,code=compute_80
LD_FLAGS += -Xlinker -rpath='$$ORIGIN' $(CUDA_LD_FLAGS) $(MM_LD_FLAGS)

C_FLAGS += $(CUDA_INCLUDE_FLAGS) $(MM_INCLUDE_FLAGS)
NV_C_FLAGS += $(INCLUDE_FLAGS) $(CUDA_INCLUDE_FLAGS) $(MM_INCLUDE_FLAGS)

LIB_TYPE := nv

OBJS := $(SRCS:%.cpp=%.$(LIB_TYPE).o) $(CU_SRCS:%.cu=%.$(LIB_TYPE).o)

SO := $(LIB_PATH)/libiddma_$(LIB_TYPE).so

all: $(SO)

$(SO): $(OBJS)
	@mkdir -p $(LIB_PATH)
	$(NVCC) --shared -o $@ $^ $(LD_FLAGS)

%.$(LIB_TYPE).o: %.cpp
	$(CC) $(C_FLAGS) -o $@ -c $<

%.$(LIB_TYPE).o: %.cu
	$(NVCC) $(NV_C_FLAGS) -o $@ -c $<

clean:
	@rm -f $(SO) $(OBJS) *.$(LIB_TYPE).o
	@rm -f *~;
