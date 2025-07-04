#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

NVIDIA ?= 0
AMD ?= 0
EXT ?=
SRCS := mem_manage.cpp \
	mem_manage_c.cpp

OBJS := $(SRCS:%.cpp=%.o)
NOGPU_OBJS := $(SRCS:%.cpp=%.nogpu.o)

CFLAGS := -I. -I../../../include/mem_manage -fPIC
LDFLAGS := -lpthread -fPIC

ifeq ($(NVIDIA), 1)
	CFLAGS += -DNVIDIA_PLATFORM -I/usr/local/cuda/include
	LDFLAGS += -L/usr/local/cuda/lib64 -lcuda
endif

ifeq ($(AMD), 1)
	CFLAGS += -DAMD_PLATFORM
endif

LIBNAME := mem_manage
SO := ../../../lib/lib$(LIBNAME)$(EXT).so

all: $(SO)

$(SO): $(OBJS)
	@if [ ! -d ../../../lib ]; then mkdir ../../../lib; fi
	g++ --shared -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	g++ -o $@ -c $< $(CFLAGS)

clean:
	@rm -f $(SO) $(OBJS)
	@rm -f *.so
	@rm -f *~
