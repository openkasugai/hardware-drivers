#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

SRCS := libstream_engine.cpp \
	stream_engine.cpp

OBJS := $(SRCS:%.cpp=%.o)
NOGPU_OBJS := $(SRCS:%.cpp=%.nogpu.o)

CFLAGS := -I. -I../../../include/stream_engine -I../../../include/common -fPIC
CFLAGS += -I../common
LDFLAGS := -L../../../lib -lxsecommon -lpthread -fPIC

LIBNAME := stream_engine
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
