/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file ptu_reg_func.hpp
 * @brief Header file for PTU register access APIs
 */

#pragma once

#include <stdint.h>

// reg func
void ptu_reg_write(int fd, uint32_t base, uint32_t reg_idx, uint32_t value);
uint32_t ptu_reg_read(int fd, uint32_t base, uint32_t reg_idx);
