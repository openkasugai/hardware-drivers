#=================================================
# Copyright 2025 NTT Corporation
# Licensed under the 3-Clause BSD License, see LICENSE for details.
# SPDX-License-Identifier: BSD-3-Clause
#=================================================

FPGA_NUM := $(shell ls -1 /dev/xse?_xdma 2> /dev/null | wc -l)
VEC_FP_INC := $(shell ls -1 /dev/xse?_vec_fp_inc_wrapper_0 2> /dev/null | wc -l)

ifeq ($(TEST_CONFIG),)
TEST_CONFIGS :=
ifeq ($(FPGA_NUM),2)
        TEST_CONFIGS += -DENABLE_NOP_DUAL
endif
ifneq ($(VEC_FP_INC),0)
        TEST_CONFIGS += -DENABLE_VEC_FP_INC
endif
ifneq ($(TEST_CONFIGS),)
TEST_CONFIG = "$(TEST_CONFIGS)"
endif
endif


