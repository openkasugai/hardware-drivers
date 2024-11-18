/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfunction_conv_stat.h>
#include <liblogging.h>

#include <libfpga_internal/libfpga_json.h>
#include <libfpga_internal/libfpgautil.h>

/**
 * Register map for modulized fpga
 */
#include <libfpga_internal/libfunction_regmap.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBFUNCTION


int fpga_conv_get_stat_bytes(
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
  if (!dev || !byte_num || (lane >= KERNEL_NUM_CONV(dev)) || (fchid > XPCIE_FUNCTION_CHAIN_ID_MAX) || (reg_id > CONV_STAT_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), byte_num(%#lx))\n", __func__, dev_id, lane, fchid, reg_id, (uintptr_t)byte_num);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), byte_num(%#lx))\n", __func__, dev_id, lane, fchid, reg_id, (uintptr_t)byte_num);

  if (pwrite(dev->fd, &fchid, len, XPCIE_FPGA_CONV_STAT_SEL_CHANNEL(lane)) != len) goto failed_W;

  switch (reg_id) {
    case CONV_STAT_INGR_RCV:
      addr_l = XPCIE_FPGA_CONV_STAT_INGR_RCV_DATA_VALUE_L(lane);
      addr_h = XPCIE_FPGA_CONV_STAT_INGR_RCV_DATA_VALUE_H(lane);
      break;

    case CONV_STAT_INGR_SND0:
      addr_l = XPCIE_FPGA_CONV_STAT_INGR_SND_DATA_0_VALUE_L(lane);
      addr_h = XPCIE_FPGA_CONV_STAT_INGR_SND_DATA_0_VALUE_H(lane);
      break;

    case CONV_STAT_INGR_SND1:
      addr_l = XPCIE_FPGA_CONV_STAT_INGR_SND_DATA_1_VALUE_L(lane);
      addr_h = XPCIE_FPGA_CONV_STAT_INGR_SND_DATA_1_VALUE_H(lane);
      break;

    case CONV_STAT_EGR_RCV0:
      addr_l = XPCIE_FPGA_CONV_STAT_EGR_RCV_DATA_0_VALUE_L(lane);
      addr_h = XPCIE_FPGA_CONV_STAT_EGR_RCV_DATA_0_VALUE_H(lane);
      break;

    case CONV_STAT_EGR_RCV1:
      addr_l = XPCIE_FPGA_CONV_STAT_EGR_RCV_DATA_1_VALUE_L(lane);
      addr_h = XPCIE_FPGA_CONV_STAT_EGR_RCV_DATA_1_VALUE_H(lane);
      break;

    case CONV_STAT_EGR_SND:
      addr_l = XPCIE_FPGA_CONV_STAT_EGR_SND_DATA_VALUE_L(lane);
      addr_h = XPCIE_FPGA_CONV_STAT_EGR_SND_DATA_VALUE_H(lane);
      break;

    default:
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "reg_id(%u) is not the expected value.\n", reg_id);
      return -INVALID_ARGUMENT;
  }

  if (pread(dev->fd, &value_l, len, addr_l) != len) goto failed_R;
  if (pread(dev->fd, &value_h, len, addr_h) != len) goto failed_R;
  *byte_num =  (uint64_t)value_l;
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


int fpga_conv_get_stat_frames(
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
  if (!dev || !frame_num || (lane >= KERNEL_NUM_CONV(dev)) || (fchid > XPCIE_FUNCTION_CHAIN_ID_MAX) || (reg_id > CONV_STAT_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), frame_num(%#lx))\n", __func__, dev_id, lane, fchid, reg_id, (uintptr_t)frame_num);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), frame_num(%#lx))\n", __func__, dev_id, lane, fchid, reg_id, (uintptr_t)frame_num);

  if (pwrite(dev->fd, &fchid, len, XPCIE_FPGA_CONV_STAT_SEL_CHANNEL(lane)) != len) goto failed_W;

  switch (reg_id) {
    case CONV_STAT_INGR_RCV:
      addr = XPCIE_FPGA_CONV_STAT_INGR_RCV_FRAME_VALUE(lane);
      break;

    case CONV_STAT_INGR_SND0:
      addr = XPCIE_FPGA_CONV_STAT_INGR_SND_FRAME_0_VALUE(lane);
      break;

    case CONV_STAT_INGR_SND1:
      addr = XPCIE_FPGA_CONV_STAT_INGR_SND_FRAME_1_VALUE(lane);
      break;

    case CONV_STAT_EGR_RCV0:
      addr = XPCIE_FPGA_CONV_STAT_EGR_RCV_FRAME_0_VALUE(lane);
      break;

    case CONV_STAT_EGR_RCV1:
      addr = XPCIE_FPGA_CONV_STAT_EGR_RCV_FRAME_1_VALUE(lane);
      break;

    case CONV_STAT_EGR_SND:
      addr = XPCIE_FPGA_CONV_STAT_EGR_SND_FRAME_VALUE(lane);
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


int fpga_conv_get_stat_ovf(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *ovf_result
) {
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !ovf_result || (lane >= KERNEL_NUM_CONV(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), ovf_result(%#lx))\n", __func__, dev_id, lane, (uintptr_t)ovf_result);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), ovf_result(%#lx))\n", __func__, dev_id, lane, (uintptr_t)ovf_result);

  if (pread(dev->fd, ovf_result, len, XPCIE_FPGA_CONV_STAT_INGR_FRAME_BUFFER_OVERFLOW(lane)) != len) goto failed;

  return 0;

failed:
  llf_err(FAILURE_READ, "Failed to get parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_READ;
}


int fpga_conv_set_stat_ovf_clear(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t ovf_result
) {
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CONV(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), ovf_result(%#lx))\n", __func__, dev_id, lane, ovf_result);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), ovf_result(%#lx))\n", __func__, dev_id, lane, ovf_result);

  if (pwrite(dev->fd, &ovf_result, len, XPCIE_FPGA_CONV_STAT_INGR_FRAME_BUFFER_OVERFLOW(lane)) != len) goto failed;

  return 0;

failed:
  llf_err(FAILURE_WRITE, "Failed to set parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_WRITE, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_WRITE;
}


int fpga_conv_get_stat_buff_usage(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t *usage_num
) {
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !usage_num || (lane >= KERNEL_NUM_CONV(dev)) || (fchid > XPCIE_FUNCTION_CHAIN_ID_MAX)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), usage_num(%#lx))\n", __func__, dev_id, lane, fchid, (uintptr_t)usage_num);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), usage_num(%#lx))\n", __func__, dev_id, lane, fchid, (uintptr_t)usage_num);

  if (pwrite(dev->fd, &fchid, len, XPCIE_FPGA_CONV_STAT_SEL_CHANNEL(lane)) != len) goto failed_W;
  if (pread(dev->fd, usage_num, len, XPCIE_FPGA_CONV_STAT_INGR_FRAME_BUFFER_USAGE(lane)) != len) goto failed_R;

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
