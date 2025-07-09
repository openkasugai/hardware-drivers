#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

SRCS := function_base.cpp \
	libfunction_base.cpp

OBJS := $(SRCS:%.cpp=%.o)
NOGPU_OBJS := $(SRCS:%.cpp=%.nogpu.o)

CFLAGS := -I. -I../../../include/function_base -I../../../include/common -fPIC
CFLAGS += -I../common
LDFLAGS := -L../../../lib -lxsecommon -lpthread -fPIC

LIBNAME := function_base
SO := ../../../lib/lib$(LIBNAME).so

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
