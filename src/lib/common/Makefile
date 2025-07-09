#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

SRCS := module_base.cpp \
	libutil.cpp

OBJS := $(SRCS:%.cpp=%.o)
NOGPU_OBJS := $(SRCS:%.cpp=%.nogpu.o)

CFLAGS := -I. -I../../../include/common -fPIC
LDFLAGS := -fPIC

LIBNAME := xsecommon
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
