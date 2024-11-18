/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfpgactl.h>
#include <libfpgactl_err.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>
#include <libfpga_internal/libfpga_json.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBFPGACTL


int fpga_dev_get_check_err(
  uint32_t dev_id,
  uint32_t *check_err
) {
  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !check_err) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), check_err(%#lx))\n", __func__, dev_id, (uintptr_t)check_err);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), check_err(%#lx))\n", __func__, dev_id, (uintptr_t)check_err);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_CHK_ERR, check_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_CHK_ERR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  return 0;
}


int fpga_dev_get_clk_dwn(
  uint32_t dev_id,
  fpga_clkdwn_t *clk_dwn
) {
  fpga_ioctl_clkdown_t ioctl_clkdwn;
  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !clk_dwn) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), clk_dwn(%#lx))\n", __func__, dev_id, (uintptr_t)clk_dwn);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), clk_dwn(%#lx))\n", __func__, dev_id, (uintptr_t)clk_dwn);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_CLKDOWN, &ioctl_clkdwn) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_CLKDOWN(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  clk_dwn->user_clk  = (ioctl_clkdwn.user_clk  & 0x01);
  clk_dwn->ddr4_clk0 = (ioctl_clkdwn.ddr4_clk0 & 0x01);
  clk_dwn->ddr4_clk1 = (ioctl_clkdwn.ddr4_clk1 & 0x01);
  clk_dwn->ddr4_clk2 = (ioctl_clkdwn.ddr4_clk2 & 0x01);
  clk_dwn->ddr4_clk3 = (ioctl_clkdwn.ddr4_clk3 & 0x01);
  clk_dwn->qsfp_clk0 = (ioctl_clkdwn.qsfp_clk0 & 0x01);
  clk_dwn->qsfp_clk1 = (ioctl_clkdwn.qsfp_clk1 & 0x01);
  return 0;
}


int fpga_dev_set_clk_dwn_clear(
  uint32_t dev_id,
  fpga_clkdwn_t clk_dwn
) {
  fpga_ioctl_clkdown_t ioctl_clkdwn;
  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }

  ioctl_clkdwn.user_clk  = (clk_dwn.user_clk  & 0x01);
  ioctl_clkdwn.ddr4_clk0 = (clk_dwn.ddr4_clk0 & 0x01);
  ioctl_clkdwn.ddr4_clk1 = (clk_dwn.ddr4_clk1 & 0x01);
  ioctl_clkdwn.ddr4_clk2 = (clk_dwn.ddr4_clk2 & 0x01);
  ioctl_clkdwn.ddr4_clk3 = (clk_dwn.ddr4_clk3 & 0x01);
  ioctl_clkdwn.qsfp_clk0 = (clk_dwn.qsfp_clk0 & 0x01);
  ioctl_clkdwn.qsfp_clk1 = (clk_dwn.qsfp_clk1 & 0x01);

  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_SET_CLKDOWN_CLR, &ioctl_clkdwn) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_SET_CLKDOWN_CLR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  return 0;
}


int fpga_dev_get_clk_dwn_raw(
  uint32_t dev_id,
  fpga_clkdwn_t *clk_dwn
) {
  fpga_ioctl_clkdown_t ioctl_clkdwn;
  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !clk_dwn) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), clk_dwn(%#lx))\n", __func__, dev_id, (uintptr_t)clk_dwn);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), clk_dwn(%#lx))\n", __func__, dev_id, (uintptr_t)clk_dwn);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_CLKDOWN_RAW, &ioctl_clkdwn) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_CLKDOWN_RAW(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  clk_dwn->user_clk  = (ioctl_clkdwn.user_clk  & 0x01);
  clk_dwn->ddr4_clk0 = (ioctl_clkdwn.ddr4_clk0 & 0x01);
  clk_dwn->ddr4_clk1 = (ioctl_clkdwn.ddr4_clk1 & 0x01);
  clk_dwn->ddr4_clk2 = (ioctl_clkdwn.ddr4_clk2 & 0x01);
  clk_dwn->ddr4_clk3 = (ioctl_clkdwn.ddr4_clk3 & 0x01);
  clk_dwn->qsfp_clk0 = (ioctl_clkdwn.qsfp_clk0 & 0x01);
  clk_dwn->qsfp_clk1 = (ioctl_clkdwn.qsfp_clk1 & 0x01);

  return 0;
}


int fpga_dev_set_clk_dwn_mask(
  uint32_t dev_id,
  fpga_clkdwn_t clk_dwn
) {
  fpga_ioctl_clkdown_t ioctl_clkdwn;
  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }

  ioctl_clkdwn.user_clk  = (clk_dwn.user_clk  & 0x01);
  ioctl_clkdwn.ddr4_clk0 = (clk_dwn.ddr4_clk0 & 0x01);
  ioctl_clkdwn.ddr4_clk1 = (clk_dwn.ddr4_clk1 & 0x01);
  ioctl_clkdwn.ddr4_clk2 = (clk_dwn.ddr4_clk2 & 0x01);
  ioctl_clkdwn.ddr4_clk3 = (clk_dwn.ddr4_clk3 & 0x01);
  ioctl_clkdwn.qsfp_clk0 = (clk_dwn.qsfp_clk0 & 0x01);
  ioctl_clkdwn.qsfp_clk1 = (clk_dwn.qsfp_clk1 & 0x01);

  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_SET_CLKDOWN_MASK, &ioctl_clkdwn) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_SET_CLKDOWN_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_dev_get_clk_dwn_mask(
  uint32_t dev_id,
  fpga_clkdwn_t *clk_dwn
) {
  fpga_ioctl_clkdown_t ioctl_clkdwn;
  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !clk_dwn) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), clk_dwn(%#lx))\n", __func__, dev_id, (uintptr_t)clk_dwn);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), clk_dwn(%#lx))\n", __func__, dev_id, (uintptr_t)clk_dwn);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_CLKDOWN_MASK, &ioctl_clkdwn) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_CLKDOWN_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  clk_dwn->user_clk  = (ioctl_clkdwn.user_clk  & 0x01);
  clk_dwn->ddr4_clk0 = (ioctl_clkdwn.ddr4_clk0 & 0x01);
  clk_dwn->ddr4_clk1 = (ioctl_clkdwn.ddr4_clk1 & 0x01);
  clk_dwn->ddr4_clk2 = (ioctl_clkdwn.ddr4_clk2 & 0x01);
  clk_dwn->ddr4_clk3 = (ioctl_clkdwn.ddr4_clk3 & 0x01);
  clk_dwn->qsfp_clk0 = (ioctl_clkdwn.qsfp_clk0 & 0x01);
  clk_dwn->qsfp_clk1 = (ioctl_clkdwn.qsfp_clk1 & 0x01);

  return 0;
}


int fpga_dev_set_clk_dwn_force(
  uint32_t dev_id,
  fpga_clkdwn_t clk_dwn
) {
  fpga_ioctl_clkdown_t ioctl_clkdwn;
  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }

  ioctl_clkdwn.user_clk  = (clk_dwn.user_clk  & 0x01);
  ioctl_clkdwn.ddr4_clk0 = (clk_dwn.ddr4_clk0 & 0x01);
  ioctl_clkdwn.ddr4_clk1 = (clk_dwn.ddr4_clk1 & 0x01);
  ioctl_clkdwn.ddr4_clk2 = (clk_dwn.ddr4_clk2 & 0x01);
  ioctl_clkdwn.ddr4_clk3 = (clk_dwn.ddr4_clk3 & 0x01);
  ioctl_clkdwn.qsfp_clk0 = (clk_dwn.qsfp_clk0 & 0x01);
  ioctl_clkdwn.qsfp_clk1 = (clk_dwn.qsfp_clk1 & 0x01);

  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_SET_CLKDOWN_FORCE, &ioctl_clkdwn) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_SET_CLKDOWN_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_dev_get_clk_dwn_force(
  uint32_t dev_id,
  fpga_clkdwn_t *clk_dwn
) {
  fpga_ioctl_clkdown_t ioctl_clkdwn;
  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !clk_dwn) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), clk_dwn(%#lx))\n", __func__, dev_id, (uintptr_t)clk_dwn);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), clk_dwn(%#lx))\n", __func__, dev_id, (uintptr_t)clk_dwn);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_CLKDOWN_FORCE, &ioctl_clkdwn) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_CLKDOWN_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  clk_dwn->user_clk  = (ioctl_clkdwn.user_clk  & 0x01);
  clk_dwn->ddr4_clk0 = (ioctl_clkdwn.ddr4_clk0 & 0x01);
  clk_dwn->ddr4_clk1 = (ioctl_clkdwn.ddr4_clk1 & 0x01);
  clk_dwn->ddr4_clk2 = (ioctl_clkdwn.ddr4_clk2 & 0x01);
  clk_dwn->ddr4_clk3 = (ioctl_clkdwn.ddr4_clk3 & 0x01);
  clk_dwn->qsfp_clk0 = (ioctl_clkdwn.qsfp_clk0 & 0x01);
  clk_dwn->qsfp_clk1 = (ioctl_clkdwn.qsfp_clk1 & 0x01);

  return 0;
}


int fpga_dev_get_ecc_err(
  uint32_t dev_id,
  fpga_eccerr_t *eccerr
) {
  fpga_ioctl_eccerr_t ioctl_eccerr;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !eccerr) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), eccerr(%#lx))\n", __func__, dev_id, (uintptr_t)eccerr);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), eccerr(%#lx))\n", __func__, dev_id, (uintptr_t)eccerr);

  // single
  ioctl_eccerr.type = ECCERR_TYPE_SINGLE;
  ioctl_eccerr.eccerr = 0;
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_ECCERR, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_ECCERR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  eccerr->ddr4_single0 = (uint8_t)(ioctl_eccerr.eccerr  & 0x000000FF);
  eccerr->ddr4_single1 = (uint8_t)((ioctl_eccerr.eccerr & 0x0000FF00) >> 8);
  eccerr->ddr4_single2 = (uint8_t)((ioctl_eccerr.eccerr & 0x00FF0000) >> 16);
  eccerr->ddr4_single3 = (uint8_t)((ioctl_eccerr.eccerr & 0xFF000000) >> 24);

  // multi
  ioctl_eccerr.type = ECCERR_TYPE_MULTI;
  ioctl_eccerr.eccerr = 0;
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_ECCERR, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_ECCERR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  eccerr->ddr4_multi0 = (uint8_t)(ioctl_eccerr.eccerr  & 0x000000FF);
  eccerr->ddr4_multi1 = (uint8_t)((ioctl_eccerr.eccerr & 0x0000FF00) >> 8);
  eccerr->ddr4_multi2 = (uint8_t)((ioctl_eccerr.eccerr & 0x00FF0000) >> 16);
  eccerr->ddr4_multi3 = (uint8_t)((ioctl_eccerr.eccerr & 0xFF000000) >> 24);

  return 0;
}


int fpga_dev_set_ecc_err_clear(
  uint32_t dev_id,
  fpga_eccerr_t eccerr
) {
  fpga_ioctl_eccerr_t ioctl_eccerr;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  // single
  ioctl_eccerr.type = ECCERR_TYPE_SINGLE;
  ioctl_eccerr.eccerr = 0;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single3 & 0xFF) << 24;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single2 & 0xFF) << 16;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single1 & 0xFF) << 8;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single0 & 0xFF);
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_SET_ECCERR_CLR, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_SET_ECCERR_CLR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  // multi
  ioctl_eccerr.type = ECCERR_TYPE_MULTI;
  ioctl_eccerr.eccerr = 0;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi3 & 0xFF) << 24;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi2 & 0xFF) << 16;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi1 & 0xFF) << 8;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi0 & 0xFF);
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_SET_ECCERR_CLR, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_SET_ECCERR_CLR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_dev_get_ecc_err_raw(
  uint32_t dev_id,
  fpga_eccerr_t *eccerr
) {
  fpga_ioctl_eccerr_t ioctl_eccerr;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !eccerr) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), eccerr(%#lx))\n", __func__, dev_id, (uintptr_t)eccerr);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), eccerr(%#lx))\n", __func__, dev_id, (uintptr_t)eccerr);

  // single
  ioctl_eccerr.type = ECCERR_TYPE_SINGLE;
  ioctl_eccerr.eccerr = 0;
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_ECCERR_RAW, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_ECCERR_RAW(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  eccerr->ddr4_single0 = (uint8_t)(ioctl_eccerr.eccerr  & 0x000000FF);
  eccerr->ddr4_single1 = (uint8_t)((ioctl_eccerr.eccerr & 0x0000FF00) >> 8);
  eccerr->ddr4_single2 = (uint8_t)((ioctl_eccerr.eccerr & 0x00FF0000) >> 16);
  eccerr->ddr4_single3 = (uint8_t)((ioctl_eccerr.eccerr & 0xFF000000) >> 24);

  // multi
  ioctl_eccerr.type = ECCERR_TYPE_MULTI;
  ioctl_eccerr.eccerr = 0;
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_ECCERR_RAW, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_ECCERR_RAW(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  eccerr->ddr4_multi0 = (uint8_t)(ioctl_eccerr.eccerr  & 0x000000FF);
  eccerr->ddr4_multi1 = (uint8_t)((ioctl_eccerr.eccerr & 0x0000FF00) >> 8);
  eccerr->ddr4_multi2 = (uint8_t)((ioctl_eccerr.eccerr & 0x00FF0000) >> 16);
  eccerr->ddr4_multi3 = (uint8_t)((ioctl_eccerr.eccerr & 0xFF000000) >> 24);

  return 0;
}


int fpga_dev_set_ecc_err_mask(
  uint32_t dev_id,
  fpga_eccerr_t eccerr
) {
  fpga_ioctl_eccerr_t ioctl_eccerr;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id, eccerr);

  // single
  ioctl_eccerr.type = ECCERR_TYPE_SINGLE;
  ioctl_eccerr.eccerr = 0;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single3 & 0xFF) << 24;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single2 & 0xFF) << 16;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single1 & 0xFF) << 8;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single0 & 0xFF);
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_SET_ECCERR_MASK, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_SET_ECCERR_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  // multi
  ioctl_eccerr.type = ECCERR_TYPE_MULTI;
  ioctl_eccerr.eccerr = 0;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi3 & 0xFF) << 24;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi2 & 0xFF) << 16;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi1 & 0xFF) << 8;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi0 & 0xFF);
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_SET_ECCERR_MASK, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_SET_ECCERR_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_dev_get_ecc_err_mask(
  uint32_t dev_id,
  fpga_eccerr_t *eccerr
) {
  fpga_ioctl_eccerr_t ioctl_eccerr;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !eccerr) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), eccerr(%#lx))\n", __func__, dev_id, (uintptr_t)eccerr);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), eccerr(%#lx))\n", __func__, dev_id, (uintptr_t)eccerr);

  // single
  ioctl_eccerr.type = ECCERR_TYPE_SINGLE;
  ioctl_eccerr.eccerr = 0;
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_ECCERR_MASK, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_ECCERR_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  eccerr->ddr4_single0 = (uint8_t)(ioctl_eccerr.eccerr  & 0x000000FF);
  eccerr->ddr4_single1 = (uint8_t)((ioctl_eccerr.eccerr & 0x0000FF00) >> 8);
  eccerr->ddr4_single2 = (uint8_t)((ioctl_eccerr.eccerr & 0x00FF0000) >> 16);
  eccerr->ddr4_single3 = (uint8_t)((ioctl_eccerr.eccerr & 0xFF000000) >> 24);

  // multi
  ioctl_eccerr.type = ECCERR_TYPE_MULTI;
  ioctl_eccerr.eccerr = 0;
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_ECCERR_MASK, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_ECCERR_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  eccerr->ddr4_multi0 = (uint8_t)(ioctl_eccerr.eccerr  & 0x000000FF);
  eccerr->ddr4_multi1 = (uint8_t)((ioctl_eccerr.eccerr & 0x0000FF00) >> 8);
  eccerr->ddr4_multi2 = (uint8_t)((ioctl_eccerr.eccerr & 0x00FF0000) >> 16);
  eccerr->ddr4_multi3 = (uint8_t)((ioctl_eccerr.eccerr & 0xFF000000) >> 24);

  return 0;
}


int fpga_dev_set_ecc_err_force(
  uint32_t dev_id,
  fpga_eccerr_t eccerr
) {
  fpga_ioctl_eccerr_t ioctl_eccerr;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  // single
  ioctl_eccerr.type = ECCERR_TYPE_SINGLE;
  ioctl_eccerr.eccerr = 0;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single3 & 0xFF) << 24;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single2 & 0xFF) << 16;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single1 & 0xFF) << 8;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_single0 & 0xFF);
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_SET_ECCERR_FORCE, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_SET_ECCERR_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  // multi
  ioctl_eccerr.type = ECCERR_TYPE_MULTI;
  ioctl_eccerr.eccerr = 0;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi3 & 0xFF) << 24;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi2 & 0xFF) << 16;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi1 & 0xFF) << 8;
  ioctl_eccerr.eccerr |= ((uint32_t)eccerr.ddr4_multi0 & 0xFF);
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_SET_ECCERR_FORCE, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_SET_ECCERR_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_dev_get_ecc_err_force(
  uint32_t dev_id,
  fpga_eccerr_t *eccerr
) {
  fpga_ioctl_eccerr_t ioctl_eccerr;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !eccerr) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), eccerr(%#lx))\n", __func__, dev_id, (uintptr_t)eccerr);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), eccerr(%#lx))\n", __func__, dev_id, (uintptr_t)eccerr);

  // single
  ioctl_eccerr.type = ECCERR_TYPE_SINGLE;
  ioctl_eccerr.eccerr = 0;
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_ECCERR_FORCE, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_ECCERR_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  eccerr->ddr4_single0 = (uint8_t)(ioctl_eccerr.eccerr  & 0x000000FF);
  eccerr->ddr4_single1 = (uint8_t)((ioctl_eccerr.eccerr & 0x0000FF00) >> 8);
  eccerr->ddr4_single2 = (uint8_t)((ioctl_eccerr.eccerr & 0x00FF0000) >> 16);
  eccerr->ddr4_single3 = (uint8_t)((ioctl_eccerr.eccerr & 0xFF000000) >> 24);

  // multi
  ioctl_eccerr.type = ECCERR_TYPE_MULTI;
  ioctl_eccerr.eccerr = 0;
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_GLOBAL_GET_ECCERR_FORCE, &ioctl_eccerr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_GLOBAL_GET_ECCERR_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  eccerr->ddr4_multi0 = (uint8_t)(ioctl_eccerr.eccerr  & 0x000000FF);
  eccerr->ddr4_multi1 = (uint8_t)((ioctl_eccerr.eccerr & 0x0000FF00) >> 8);
  eccerr->ddr4_multi2 = (uint8_t)((ioctl_eccerr.eccerr & 0x00FF0000) >> 16);
  eccerr->ddr4_multi3 = (uint8_t)((ioctl_eccerr.eccerr & 0xFF000000) >> 24);

  return 0;
}
