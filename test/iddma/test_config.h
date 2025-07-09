/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _TEST_CONFIG_H__
#define _TEST_CONFIG_H__

//#define _DEBUG
#define ENABLE_NOP
#define NOP_FUNC_ID 0

//#define ENABLE_NOP_DUAL // ONLY 2-FPGA ENVIRONMENT

#define ENABLE_NOP_MMAPPED
#define NOP_MMAPPED_ID 8

//#define ENABLE_VEC_FP_INC
#define VEC_FP_INC_ID 1

#define ENABLE_TCP_NOP
#define ENABLE_TCP_NOP_CP
#endif // _TEST_CONFIG_H__
