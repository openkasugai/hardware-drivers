#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

.PHONY: all clean

TOP := ../../..
TEST_TOP := ../..
CUDA_TOP ?= /usr/local/cuda
PCM_POWER ?= $(TOP)/../../pcm/build/bin/pcm-power
CPU_POWER_MIN ?= 45
XSE ?= 1
TEST_CONFIG ?=

IDDMA_INCLUDE_PATH := $(TOP)/include/iddma
IDDMA_LIB_PATH := $(TOP)/lib

MM_LIB_PATH := $(TOP)/lib

GTEST_INCLUDE_PATH := $(TEST_TOP)/3rdparty/googletest/googletest/include
GTEST_LIB_PATH := $(TEST_TOP)/3rdparty/googletest/build/lib

SET_ENV ?= LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:./cpu/$(IDDMA_LIB_PATH):./cpu/$(GTEST_LIB_PATH):./cpu/$(MM_LIB_PATH)
FPGA_TCP_CFG ?= CPU_CP_IP=$(XSE_IP) CPU_CP_PORT_BASE=$(XSE_RX_PORT_BASE) \
	   CPU_PORT_BASE=$(CPU_PORT_BASE)
TCP_CFG ?= CPU_CP_IP=$(CPU_CP_IP) CPU_CP_PORT_BASE=$(CPU_CP_PORT_BASE) \
	   CPU_PORT_BASE=$(CPU_PORT_BASE)
TCP_CP_CFG ?= CPU_IP=$(CPU_IP) CPU_PORT_BASE=$(CPU_PORT_BASE) \
	      CPU_CP_PORT_BASE=$(CPU_CP_PORT_BASE)

include ../config.make

vpath %.cpp $(TOP)/src/lib/iddma/util

SRCS := test.cpp test_cpu.cpp test_cpu_tcp.cpp
SRCS += tcp_cmd.cpp
OBJS := $(SRCS:%.cpp=%.o)
DEFS ?= -D__USE_CPU__

CC := g++
CC_FLAG := -I. -I.. -fopenmp

LDFLAGS := -L$(IDDMA_LIB_PATH) -liddma_cpu
LDFLAGS += -L$(MM_LIB_PATH) -lmem_manage
LDFLAGS += -L$(GTEST_LIB_PATH) -lgtest -lgtest_main
LDFLAGS += -lpthread

INCLUDEFLAGS := -I$(IDDMA_INCLUDE_PATH)
INCLUDEFLAGS += -I$(GTEST_INCLUDE_PATH)
INCLUDEFLAGS += $(TEST_CONFIG)
INCLUDEFLAGS += -I$(TOP)/src/lib/iddma/util
#INCLUDEFLAGS += -I../gpu

ifeq ($(DEBUG), 1)
	CC_FLAG += -D__KTP_DEBUG__ -D_DEBUG
endif

EXEC := test_cpu_exec

vpath %.cpp ..

all: $(EXEC)

test: $(EXEC)
	cd .. && $(SET_ENV) ./cpu/$(EXEC) --gtest_filter=-*Tcp*

test_tcp_cp: $(EXEC)
	cd .. && $(SET_ENV) $(TCP_CP_CFG) ./cpu/$(EXEC) --gtest_filter=*Tcp*Cp*

test_tcp: $(EXEC)
	cd .. && $(SET_ENV) $(TCP_CFG) ./cpu/$(EXEC) --gtest_filter=*Tcp*:-*Cp*

test_tcp_for_xse: $(EXEC)
	cd .. && $(SET_ENV) $(FPGA_TCP_CFG) ./cpu/$(EXEC) --gtest_filter=*Tcp*:-*Cp*

eval_power:
	@echo $(PCM_POWER)
	@sudo $(PCM_POWER) 2> /dev/null | grep --line-buffered Consumed | awk -F ' ' \
		'BEGIN{core=0; core2=0; mem=0; cnt=0; flag=0;} {gsub(";","",$$10); if ($$3=="energy" && $$10>$(CPU_POWER_MIN)) {core=core+$$10; core2=core2+$$10*$$10; flag=1;}; if (flag==1 && $$3=="DRAM") {mem=mem+$$13; flag=0; cnt=cnt+1;}} END{if (cnt>0) {ave=core/cnt; var=core2/cnt-ave*ave; print("CPU Power: ", ave, sqrt(var), cnt);}}'

$(EXEC): $(OBJS)
	$(CC) $(CC_FLAG)  -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CC_FLAG) $(DEFS) -o $@ -c $< $(INCLUDEFLAGS)

clean:
	@rm -f *.o
	@rm -f $(EXEC) ../test.o
