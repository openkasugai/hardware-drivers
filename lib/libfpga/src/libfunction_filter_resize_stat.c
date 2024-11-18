/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfunction_filter_resize_stat.h>
#include <liblogging.h>

#include <libfpga_internal/libfpga_json.h>
#include <libfpga_internal/libfpgautil.h>
#include <libfpga_internal/libfunction_regmap.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBFUNCTION


int fpga_filter_resize_get_stat_bytes(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t reg_id,
  uint64_t *byte_num
) {
  uint32_t value_l, value_h;
  uint64_t addr_l, addr_h;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !byte_num || (lane >= KERNEL_NUM_FUNC(dev)) || (fchid > XPCIE_FUNCTION_CHAIN_ID_MAX) || (reg_id > FR_STAT_EGR_SND1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), byte_num(%#lx))\n", __func__, dev_id, lane, fchid, reg_id, (uintptr_t)byte_num);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), byte_num(%#lx))\n", __func__, dev_id, lane, fchid, reg_id, (uintptr_t)byte_num);

  if (pwrite(dev->fd, &fchid, len, XPCIE_FPGA_FRFUNC_STAT_SEL_CHANNEL(lane)) != len) goto failed_W;

  switch (reg_id) {
    case FR_STAT_INGR_RCV0:
      addr_l = XPCIE_FPGA_FRFUNC_STAT_INGR_RCV_DATA_0_VALUE_L(lane);
      addr_h = XPCIE_FPGA_FRFUNC_STAT_INGR_RCV_DATA_0_VALUE_H(lane);
      break;

    case FR_STAT_INGR_RCV1:
      addr_l = XPCIE_FPGA_FRFUNC_STAT_INGR_RCV_DATA_1_VALUE_L(lane);
      addr_h = XPCIE_FPGA_FRFUNC_STAT_INGR_RCV_DATA_1_VALUE_H(lane);
      break;

    case FR_STAT_EGR_SND0:
      addr_l = XPCIE_FPGA_FRFUNC_STAT_EGR_SND_DATA_0_VALUE_L(lane);
      addr_h = XPCIE_FPGA_FRFUNC_STAT_EGR_SND_DATA_0_VALUE_H(lane);
      break;

    case FR_STAT_EGR_SND1:
      addr_l = XPCIE_FPGA_FRFUNC_STAT_EGR_SND_DATA_1_VALUE_L(lane);
      addr_h = XPCIE_FPGA_FRFUNC_STAT_EGR_SND_DATA_1_VALUE_H(lane);
      break;

    default:
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "reg_id(%u) is not the expected value.\n", reg_id);
      return -INVALID_ARGUMENT;
  }

  if (pread(dev->fd, &value_l, len, addr_l) != len) goto failed_R;
  if (pread(dev->fd, &value_h, len, addr_h) != len) goto failed_R;
  *byte_num = (uint64_t)value_l;
  *byte_num |= ((uint64_t)value_h & 0xFFFFFFFF) << 32;

  return 0;

failed_W:
  llf_err(FAILURE_WRITE, "Failed to set parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_WRITE, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_WRITE;

failed_R:
  llf_err(FAILURE_READ, "Failed to get parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_READ;
}


int fpga_filter_resize_get_stat_frames(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t reg_id,
  uint32_t *frame_num
) {
  uint32_t value;
  uint64_t addr;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !frame_num || (lane >= KERNEL_NUM_FUNC(dev)) || (fchid > XPCIE_FUNCTION_CHAIN_ID_MAX) || (reg_id > FR_STAT_EGR_SND1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), frame_num(%#lx))\n", __func__, dev_id, lane, fchid, reg_id, (uintptr_t)frame_num);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), frame_num(%#lx))\n", __func__, dev_id, lane, fchid, reg_id, (uintptr_t)frame_num);

  if (pwrite(dev->fd, &fchid, len, XPCIE_FPGA_FRFUNC_STAT_SEL_CHANNEL(lane)) != len) goto failed_W;

  switch (reg_id) {
    case FR_STAT_INGR_RCV0:
      addr = XPCIE_FPGA_FRFUNC_STAT_INGR_RCV_FRAME_0_VALUE(lane);
      break;

    case FR_STAT_INGR_RCV1:
      addr = XPCIE_FPGA_FRFUNC_STAT_INGR_RCV_FRAME_1_VALUE(lane);
      break;

    case FR_STAT_EGR_SND0:
      addr = XPCIE_FPGA_FRFUNC_STAT_EGR_SND_FRAME_0_VALUE(lane);
      break;

    case FR_STAT_EGR_SND1:
      addr = XPCIE_FPGA_FRFUNC_STAT_EGR_SND_FRAME_1_VALUE(lane);
      break;

    default:
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "reg_id(%u) is not the expected value.\n", reg_id);
      return -INVALID_ARGUMENT;
  }

  if (pread(dev->fd, &value, len, addr) != len) goto failed_R;
  *frame_num = value;

  return 0;

failed_W:
  llf_err(FAILURE_WRITE, "Failed to set parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_WRITE, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_WRITE;

failed_R:
  llf_err(FAILURE_READ, "Failed to get parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_READ;
}
