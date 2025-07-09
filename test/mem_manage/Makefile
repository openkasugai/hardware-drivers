#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

ENABLE_CUDA ?= 1
SRCS := test_mem_manage.cpp

OBJS := $(SRCS:%.cpp=%.o)

EXEC := test_mem_manage

GTEST_DIR ?= ../3rdparty
LIBGTEST := gtest
LIBGTESTMAIN := gtest_main

CUDA_INCDIR ?= /usr/local/cuda/include
CUDA_LIBDIR ?= /usr/local/cuda/lib64

MEMMANAGE_DIR := ../../src/lib/mem_manage
INCDIR := ../../include/mem_manage
LIBDIR := ../../lib
LIBMEMMANAGE := mem_manage

CFLAGS := -O2 -g -std=c++14 -I. -I$(INCDIR) -I$(GTEST_DIR)/googletest/googletest/include
LDFLAGS := -g -L$(LIBDIR) -l$(LIBMEMMANAGE) -L$(GTEST_DIR)/googletest/build/lib -l$(LIBGTEST) -l$(LIBGTESTMAIN)
ifneq ($(ENABLE_CUDA),0)
CFLAGS += -DENABLE_CUDA -I$(CUDA_INCDIR)
LDFLAGS += -L$(CUDA_LIBDIR) -lcudart
endif
LDFLAGS += -lpthread

.PHONY: gtest mem_manage

all: gtest mem_manage
	@make $(EXEC)

test: all
	LD_LIBRARY_PATH=$(LIBDIR) ./$(EXEC)

$(EXEC): $(OBJS)
	g++ -o $@ $^ $(LDFLAGS)

%.o : %.cpp
	g++ -c -o $@ $< $(CFLAGS)

gtest:
	@make -C $(GTEST_DIR) googletest

mem_manage:
	@make -C $(MEMMANAGE_DIR)

clean:
	@rm -f $(OBJS) $(EXEC)
