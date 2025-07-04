#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

SRCS := toe_config.cpp

OBJS := $(SRCS:%.cpp=%.o)

CFLAGS := -O2 -I. -I../../../include/common -I../../../include/toe
LDFLAGS :=

BIN_DIR := ../../../bin
EXENAME := $(BIN_DIR)/xse_toe_config

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
