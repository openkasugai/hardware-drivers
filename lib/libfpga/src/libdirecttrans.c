/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libdirecttrans.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>

// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBDIRECTTRANS


int fpga_direct_start(
  uint32_t dev_id,
  uint32_t lane
) {
  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_DIRECT(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u))\n",
      __func__, dev_id, lane);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u))\n",
      __func__, dev_id, lane);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_START_MODULE, &lane) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_START_MODULE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  return 0;
}


int fpga_direct_stop(
  uint32_t dev_id,
  uint32_t lane
) {
  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_DIRECT(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u))\n",
      __func__, dev_id, lane);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u))\n",
      __func__, dev_id, lane);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_STOP_MODULE, &lane) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_STOP_MODULE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  return 0;
}


int fpga_direct_get_control(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *control
) {
  fpga_ioctl_direct_ctrl_t ioctl_direct_ctrl;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !control || (lane >= KERNEL_NUM_DIRECT(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), control(%#lx))\n", __func__, dev_id, lane, (uintptr_t)control);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), control(%#lx))\n", __func__, dev_id, lane, (uintptr_t)control);

  ioctl_direct_ctrl.lane  = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_GET_MODULE, &ioctl_direct_ctrl) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_GET_MODULE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *control = ioctl_direct_ctrl.value;

  return 0;
}


int fpga_direct_get_module_id(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *module_id
) {
  fpga_ioctl_direct_ctrl_t ioctl_direct_ctrl;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !module_id || (lane >= KERNEL_NUM_DIRECT(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), module_id(%#lx))\n", __func__, dev_id, lane, (uintptr_t)module_id);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), module_id(%#lx))\n", __func__, dev_id, lane, (uintptr_t)module_id);

  ioctl_direct_ctrl.lane  = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_GET_MODULE_ID, &ioctl_direct_ctrl) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_GET_MODULE_ID(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *module_id = ioctl_direct_ctrl.value;

  return 0;
}
