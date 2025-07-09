#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

.PHONY: googletest

all: googletest

googletest:
	@if [ ! -d googletest ]; then \
		git clone https://github.com/google/googletest.git && \
		mkdir googletest/build && \
		cd googletest/build && cmake .. && make; \
	fi

clean:
	@rm -rf googletest
