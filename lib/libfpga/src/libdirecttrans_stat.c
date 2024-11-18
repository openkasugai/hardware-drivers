/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libdirecttrans_stat.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>

// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBDIRECTTRANS


int fpga_direct_get_stat_bytes(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t reg_id,
  uint64_t *byte_num
) {
  fpga_ioctl_direct_bytenum_t ioctl_direct_bytenum;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !byte_num || (lane >= KERNEL_NUM_DIRECT(dev)) || (fchid > FUNCTION_CHAIN_ID_MAX) || (reg_id > DIRECT_STAT_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), byte_num(%#lx))\n",
      __func__, dev_id, lane, fchid, reg_id, (uintptr_t)byte_num);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), byte_num(%#lx))\n",
      __func__, dev_id, lane, fchid, reg_id, (uintptr_t)byte_num);

  ioctl_direct_bytenum.lane   = (int)lane;  // NOLINT
  ioctl_direct_bytenum.fchid  = (uint16_t)(fchid  & 0x0000FFFF);
  ioctl_direct_bytenum.reg_id = (uint16_t)(reg_id & 0x0000FFFF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_GET_BYTES, &ioctl_direct_bytenum) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_GET_BYTES(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *byte_num = ioctl_direct_bytenum.byte_num;

  return 0;
}


int fpga_direct_get_stat_frames(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t reg_id,
  uint32_t *frame_num
) {
  fpga_ioctl_direct_framenum_t ioctl_direct_framenum;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !frame_num || (lane >= KERNEL_NUM_DIRECT(dev)) || (fchid > FUNCTION_CHAIN_ID_MAX) || (reg_id > DIRECT_STAT_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), frame_num(%#lx))\n",
      __func__, dev_id, lane, fchid, reg_id, (uintptr_t)frame_num);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), frame_num(%#lx))\n",
      __func__, dev_id, lane, fchid, reg_id, (uintptr_t)frame_num);

  ioctl_direct_framenum.lane   = (int)lane;  // NOLINT
  ioctl_direct_framenum.fchid  = (uint16_t)(fchid  & 0x0000FFFF);
  ioctl_direct_framenum.reg_id = (uint16_t)(reg_id & 0x0000FFFF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_GET_FRAMES, &ioctl_direct_framenum) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_GET_FRAMES(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *frame_num = ioctl_direct_framenum.frame_num;

  return 0;
}
