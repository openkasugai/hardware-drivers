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
SW_TOP ?= $(BASE_TOP)/../../..
UTIL_TOP := $(BASE_TOP)/util
LIB_PATH ?= $(SW_TOP)/lib

DEBUG_FLAGS :=
ifeq ($(DEBUG),1)
	DEBUG_FLAGS += -D__KTP_DEBUG__ -D_DEBUG
endif

CUDA_UTIL_SRCS := $(UTIL_TOP)/iddma_cuda_util.cpp

MM_INCLUDE_FLAGS := -I$(SW_TOP)/include/mem_manage
MM_LD_FLAGS := -L$(SW_TOP)/lib -lmem_manage
# end of CUDA settings

SRCS := $(BASE_TOP)/iddma.cpp \
		$(BASE_TOP)/iddma_common.cpp \
		$(UTIL_TOP)/ftok_util.cpp \
		$(UTIL_TOP)/iddma_memory.cpp \
		$(UTIL_TOP)/iddma_memory_map.cpp \
		$(UTIL_TOP)/tcp_cmd.cpp \
		$(UTIL_TOP)/tcp_ctrl_tunnel.cpp \
		$(UTIL_TOP)/shmem_ctrl_tunnel.cpp \
		$(BASE_TOP)/iddma_tcp.cpp \
		iddma_cpu.cpp

DEFS ?= -D__USE_CPU__

CC := g++
INCLUDE_FLAGS := -I. \
				 -I$(BASE_TOP) \
				 -I$(UTIL_TOP) \
				 -I$(SW_TOP)/include/iddma
C_FLAGS := $(DEFS) $(INCLUDE_FLAGS) $(MM_INCLUDE_FLAGS) -fPIC $(DEBUG_FLAGS)
LD_FLAGS := -Wl,-rpath,'$$ORIGIN' $(MM_LD_FLAGS)

SRCS += $(CUDA_UTIL_SRCS)

LIB_TYPE := cpu

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
