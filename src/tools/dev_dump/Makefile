#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

SRCS := dev_dump.cpp

OBJS := $(SRCS:%.cpp=%.o)

CFLAGS := -O2 -I. -I../../../include/common
LDFLAGS :=

BIN_DIR := ../../../bin
EXENAME := $(BIN_DIR)/xse_dev_dump

all: $(EXENAME)

$(EXENAME): $(OBJS)
	@if [ ! -d $(BIN_DIR) ]; then \
		mkdir $(BIN_DIR); \
	fi
	g++ -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	g++ -o $@ -c $< $(CFLAGS)

clean:
	@rm -f $(EXENAME) $(OBJS)
	@rm -f *~
