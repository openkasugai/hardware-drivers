#=================================================
# Copyright 2024 NTT Corporation, FUJITSU LIMITED
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

# all source are stored in SRCS-y
SRCS-y    := src/cppfunc.cpp

# object name
OBJS      := cppfunc.o

# Build using pkg-config variables
export PKG_CONFIG_PATH=/usr/local/opencv-3.4.3/lib/pkgconfig
PKGCONFIG := `pkg-config --cflags opencv pkg-config --libs opencv`

INCLUDE   := -I./include
INCLUDE   += -I$(XILINX_VITIS_HLS)/include

CPPFLAGS  := -O3 $(PKGCONFIG)


.PHONY: default clean

$(OBJS): $(SRCS-y)
	$(CXX) $(CPPFLAGS) $(SRCS-y) $(INCLUDE) $(LDFLAGS) $(LIBS) -c

default: $(OBJS)

clean:
	$(RM) $(OBJS)
