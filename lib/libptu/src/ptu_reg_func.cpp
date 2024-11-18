/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <ptu_reg_func.hpp>

#include <stdint.h>
#include <unistd.h>


void ptu_reg_write(int fd, uint32_t base, uint32_t reg_idx, uint32_t value) {
  pwrite(fd, &value, sizeof(uint32_t), base + reg_idx * 4);
}

uint32_t ptu_reg_read(int fd, uint32_t base, uint32_t reg_idx) {
  uint32_t value;
  pread(fd, &value, sizeof(uint32_t), base + reg_idx * 4);
  return value;
}
