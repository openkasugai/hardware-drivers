/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libdirecttrans_err.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>

// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBDIRECTTRANS


int fpga_direct_get_check_err(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *err_det
) {
  fpga_ioctl_err_all_t ioctl_err_all;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !err_det || (lane >= KERNEL_NUM_DIRECT(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), err_det(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)err_det);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), err_det(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)err_det);

  ioctl_err_all.lane = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_GET_ERR_ALL, &ioctl_err_all) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_GET_ERR_ALL(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *err_det = ioctl_err_all.err_all;

  return 0;
}


int fpga_direct_get_err_prot(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t dir,
  fpga_direct_err_prot_t *direct_err_prot
) {
  fpga_ioctl_direct_err_prot_t ioctl_direct_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !direct_err_prot || (lane >= KERNEL_NUM_DIRECT(dev)) || (dir > DIRECT_DIR_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)direct_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)direct_err_prot);

	ioctl_direct_err_prot.lane     = (int)lane;  // NOLINT
  ioctl_direct_err_prot.dir_type = dir;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_GET_ERR_PROT, &ioctl_direct_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_GET_ERR_PROT(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  direct_err_prot->prot_ch                = (ioctl_direct_err_prot.prot_ch                & 0x01);
  direct_err_prot->prot_len               = (ioctl_direct_err_prot.prot_len               & 0x01);
  direct_err_prot->prot_sof               = (ioctl_direct_err_prot.prot_sof               & 0x01);
  direct_err_prot->prot_eof               = (ioctl_direct_err_prot.prot_eof               & 0x01);
  direct_err_prot->prot_reqresp           = (ioctl_direct_err_prot.prot_reqresp           & 0x01);
  direct_err_prot->prot_datanum           = (ioctl_direct_err_prot.prot_datanum           & 0x01);
  direct_err_prot->prot_req_outstanding   = (ioctl_direct_err_prot.prot_req_outstanding   & 0x01);
  direct_err_prot->prot_resp_outstanding  = (ioctl_direct_err_prot.prot_resp_outstanding  & 0x01);
  direct_err_prot->prot_max_datanum       = (ioctl_direct_err_prot.prot_max_datanum       & 0x01);
  direct_err_prot->prot_reqlen            = (ioctl_direct_err_prot.prot_reqlen            & 0x01);
  direct_err_prot->prot_reqresplen        = (ioctl_direct_err_prot.prot_reqresplen        & 0x01);

  return 0;
}


int fpga_direct_set_err_prot_clear(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t dir,
  fpga_direct_err_prot_t direct_err_prot
) {
  fpga_ioctl_direct_err_prot_t ioctl_direct_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_DIRECT(dev)) || (dir > DIRECT_DIR_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, direct_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, direct_err_prot);

  ioctl_direct_err_prot.lane             = (int)lane;  // NOLINT
  ioctl_direct_err_prot.dir_type         = dir;
  ioctl_direct_err_prot.prot_ch                = (direct_err_prot.prot_ch                & 0x01);
  ioctl_direct_err_prot.prot_len               = (direct_err_prot.prot_len               & 0x01);
  ioctl_direct_err_prot.prot_sof               = (direct_err_prot.prot_sof               & 0x01);
  ioctl_direct_err_prot.prot_eof               = (direct_err_prot.prot_eof               & 0x01);
  ioctl_direct_err_prot.prot_reqresp           = (direct_err_prot.prot_reqresp           & 0x01);
  ioctl_direct_err_prot.prot_datanum           = (direct_err_prot.prot_datanum           & 0x01);
  ioctl_direct_err_prot.prot_req_outstanding   = (direct_err_prot.prot_req_outstanding   & 0x01);
  ioctl_direct_err_prot.prot_resp_outstanding  = (direct_err_prot.prot_resp_outstanding  & 0x01);
  ioctl_direct_err_prot.prot_max_datanum       = (direct_err_prot.prot_max_datanum       & 0x01);
  ioctl_direct_err_prot.prot_reqlen            = (direct_err_prot.prot_reqlen            & 0x01);
  ioctl_direct_err_prot.prot_reqresplen        = (direct_err_prot.prot_reqresplen        & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_SET_ERR_PROT_CLR, &ioctl_direct_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_SET_ERR_PROT_CLR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_direct_set_err_prot_mask(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t dir,
  fpga_direct_err_prot_t direct_err_prot
) {
  fpga_ioctl_direct_err_prot_t ioctl_direct_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_DIRECT(dev)) || (dir > DIRECT_DIR_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, direct_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, direct_err_prot);

  ioctl_direct_err_prot.lane                   = (int)lane;  // NOLINT
  ioctl_direct_err_prot.dir_type               = dir;
  ioctl_direct_err_prot.prot_ch                = (direct_err_prot.prot_ch                & 0x01);
  ioctl_direct_err_prot.prot_len               = (direct_err_prot.prot_len               & 0x01);
  ioctl_direct_err_prot.prot_sof               = (direct_err_prot.prot_sof               & 0x01);
  ioctl_direct_err_prot.prot_eof               = (direct_err_prot.prot_eof               & 0x01);
  ioctl_direct_err_prot.prot_reqresp           = (direct_err_prot.prot_reqresp           & 0x01);
  ioctl_direct_err_prot.prot_datanum           = (direct_err_prot.prot_datanum           & 0x01);
  ioctl_direct_err_prot.prot_req_outstanding   = (direct_err_prot.prot_req_outstanding   & 0x01);
  ioctl_direct_err_prot.prot_resp_outstanding  = (direct_err_prot.prot_resp_outstanding  & 0x01);
  ioctl_direct_err_prot.prot_max_datanum       = (direct_err_prot.prot_max_datanum       & 0x01);
  ioctl_direct_err_prot.prot_reqlen            = (direct_err_prot.prot_reqlen            & 0x01);
  ioctl_direct_err_prot.prot_reqresplen        = (direct_err_prot.prot_reqresplen        & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_SET_ERR_PROT_MASK, &ioctl_direct_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_SET_ERR_PROT_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_direct_get_err_prot_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_direct_err_prot_t *direct_err_prot
) {
  fpga_ioctl_direct_err_prot_t ioctl_direct_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !direct_err_prot || (lane >= KERNEL_NUM_DIRECT(dev)) || (dir > DIRECT_DIR_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)direct_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)direct_err_prot);

	ioctl_direct_err_prot.lane     = (int)lane;  // NOLINT
  ioctl_direct_err_prot.dir_type = dir;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_GET_ERR_PROT_MASK, &ioctl_direct_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_GET_ERR_PROT_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  direct_err_prot->prot_ch                = (ioctl_direct_err_prot.prot_ch                & 0x01);
  direct_err_prot->prot_len               = (ioctl_direct_err_prot.prot_len               & 0x01);
  direct_err_prot->prot_sof               = (ioctl_direct_err_prot.prot_sof               & 0x01);
  direct_err_prot->prot_eof               = (ioctl_direct_err_prot.prot_eof               & 0x01);
  direct_err_prot->prot_reqresp           = (ioctl_direct_err_prot.prot_reqresp           & 0x01);
  direct_err_prot->prot_datanum           = (ioctl_direct_err_prot.prot_datanum           & 0x01);
  direct_err_prot->prot_req_outstanding   = (ioctl_direct_err_prot.prot_req_outstanding   & 0x01);
  direct_err_prot->prot_resp_outstanding  = (ioctl_direct_err_prot.prot_resp_outstanding  & 0x01);
  direct_err_prot->prot_max_datanum       = (ioctl_direct_err_prot.prot_max_datanum       & 0x01);
  direct_err_prot->prot_reqlen            = (ioctl_direct_err_prot.prot_reqlen            & 0x01);
  direct_err_prot->prot_reqresplen        = (ioctl_direct_err_prot.prot_reqresplen        & 0x01);

  return 0;
}


int fpga_direct_set_err_prot_force(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t dir,
  fpga_direct_err_prot_t direct_err_prot
) {
  fpga_ioctl_direct_err_prot_t ioctl_direct_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_DIRECT(dev)) || (dir > DIRECT_DIR_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, direct_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, direct_err_prot);

  ioctl_direct_err_prot.lane                   = (int)lane;  // NOLINT
  ioctl_direct_err_prot.dir_type               = dir;
  ioctl_direct_err_prot.prot_ch                = (direct_err_prot.prot_ch                & 0x01);
  ioctl_direct_err_prot.prot_len               = (direct_err_prot.prot_len               & 0x01);
  ioctl_direct_err_prot.prot_sof               = (direct_err_prot.prot_sof               & 0x01);
  ioctl_direct_err_prot.prot_eof               = (direct_err_prot.prot_eof               & 0x01);
  ioctl_direct_err_prot.prot_reqresp           = (direct_err_prot.prot_reqresp           & 0x01);
  ioctl_direct_err_prot.prot_datanum           = (direct_err_prot.prot_datanum           & 0x01);
  ioctl_direct_err_prot.prot_req_outstanding   = (direct_err_prot.prot_req_outstanding   & 0x01);
  ioctl_direct_err_prot.prot_resp_outstanding  = (direct_err_prot.prot_resp_outstanding  & 0x01);
  ioctl_direct_err_prot.prot_max_datanum       = (direct_err_prot.prot_max_datanum       & 0x01);
  ioctl_direct_err_prot.prot_reqlen            = (direct_err_prot.prot_reqlen            & 0x01);
  ioctl_direct_err_prot.prot_reqresplen        = (direct_err_prot.prot_reqresplen        & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_SET_ERR_PROT_FORCE, &ioctl_direct_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_SET_ERR_PROT_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_direct_get_err_prot_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_direct_err_prot_t *direct_err_prot
) {
  fpga_ioctl_direct_err_prot_t ioctl_direct_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !direct_err_prot || (lane >= KERNEL_NUM_DIRECT(dev)) || (dir > DIRECT_DIR_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)direct_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)direct_err_prot);

	ioctl_direct_err_prot.lane     = (int)lane;  // NOLINT
  ioctl_direct_err_prot.dir_type = dir;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_GET_ERR_PROT_FORCE, &ioctl_direct_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_GET_ERR_PROT_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  direct_err_prot->prot_ch                = (ioctl_direct_err_prot.prot_ch                & 0x01);
  direct_err_prot->prot_len               = (ioctl_direct_err_prot.prot_len               & 0x01);
  direct_err_prot->prot_sof               = (ioctl_direct_err_prot.prot_sof               & 0x01);
  direct_err_prot->prot_eof               = (ioctl_direct_err_prot.prot_eof               & 0x01);
  direct_err_prot->prot_reqresp           = (ioctl_direct_err_prot.prot_reqresp           & 0x01);
  direct_err_prot->prot_datanum           = (ioctl_direct_err_prot.prot_datanum           & 0x01);
  direct_err_prot->prot_req_outstanding   = (ioctl_direct_err_prot.prot_req_outstanding   & 0x01);
  direct_err_prot->prot_resp_outstanding  = (ioctl_direct_err_prot.prot_resp_outstanding  & 0x01);
  direct_err_prot->prot_max_datanum       = (ioctl_direct_err_prot.prot_max_datanum       & 0x01);
  direct_err_prot->prot_reqlen            = (ioctl_direct_err_prot.prot_reqlen            & 0x01);
  direct_err_prot->prot_reqresplen        = (ioctl_direct_err_prot.prot_reqresplen        & 0x01);

  return 0;
}


int fpga_direct_err_prot_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir_type,
        fpga_direct_err_prot_t direct_err_prot
) {
  fpga_ioctl_direct_err_prot_t ioctl_direct_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_DIRECT(dev)) || (dir_type > DIRECT_DIR_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir_type(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir_type, direct_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir_type(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir_type, direct_err_prot);

  ioctl_direct_err_prot.lane                   = (int)lane;  // NOLINT
  ioctl_direct_err_prot.dir_type               = dir_type;
  ioctl_direct_err_prot.prot_ch                = (direct_err_prot.prot_ch                & 0x01);
  ioctl_direct_err_prot.prot_len               = (direct_err_prot.prot_len               & 0x01);
  ioctl_direct_err_prot.prot_sof               = (direct_err_prot.prot_sof               & 0x01);
  ioctl_direct_err_prot.prot_eof               = (direct_err_prot.prot_eof               & 0x01);
  ioctl_direct_err_prot.prot_reqresp           = (direct_err_prot.prot_reqresp           & 0x01);
  ioctl_direct_err_prot.prot_datanum           = (direct_err_prot.prot_datanum           & 0x01);
  ioctl_direct_err_prot.prot_req_outstanding   = (direct_err_prot.prot_req_outstanding   & 0x01);
  ioctl_direct_err_prot.prot_resp_outstanding  = (direct_err_prot.prot_resp_outstanding  & 0x01);
  ioctl_direct_err_prot.prot_max_datanum       = (direct_err_prot.prot_max_datanum       & 0x01);
  ioctl_direct_err_prot.prot_reqlen            = (direct_err_prot.prot_reqlen            & 0x01);
  ioctl_direct_err_prot.prot_reqresplen        = (direct_err_prot.prot_reqresplen        & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_ERR_PROT_INS, &ioctl_direct_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_ERR_PROT_INS(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_direct_err_prot_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir_type,
        fpga_direct_err_prot_t *direct_err_prot
) {
  fpga_ioctl_direct_err_prot_t ioctl_direct_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !direct_err_prot || (lane >= KERNEL_NUM_DIRECT(dev)) || (dir_type > DIRECT_DIR_EGR_SND)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir_type(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir_type, (uintptr_t)direct_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir_type(%u), direct_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir_type, (uintptr_t)direct_err_prot);

	ioctl_direct_err_prot.lane     = (int)lane;  // NOLINT
  ioctl_direct_err_prot.dir_type = dir_type;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_ERR_PROT_GET_INS, &ioctl_direct_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_ERR_PROT_GET_INS(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  direct_err_prot->prot_ch                = (ioctl_direct_err_prot.prot_ch                & 0x01);
  direct_err_prot->prot_len               = (ioctl_direct_err_prot.prot_len               & 0x01);
  direct_err_prot->prot_sof               = (ioctl_direct_err_prot.prot_sof               & 0x01);
  direct_err_prot->prot_eof               = (ioctl_direct_err_prot.prot_eof               & 0x01);
  direct_err_prot->prot_reqresp           = (ioctl_direct_err_prot.prot_reqresp           & 0x01);
  direct_err_prot->prot_datanum           = (ioctl_direct_err_prot.prot_datanum           & 0x01);
  direct_err_prot->prot_req_outstanding   = (ioctl_direct_err_prot.prot_req_outstanding   & 0x01);
  direct_err_prot->prot_resp_outstanding  = (ioctl_direct_err_prot.prot_resp_outstanding  & 0x01);
  direct_err_prot->prot_max_datanum       = (ioctl_direct_err_prot.prot_max_datanum       & 0x01);
  direct_err_prot->prot_reqlen            = (ioctl_direct_err_prot.prot_reqlen            & 0x01);
  direct_err_prot->prot_reqresplen        = (ioctl_direct_err_prot.prot_reqresplen        & 0x01);

  return 0;
}


int fpga_direct_get_err_stif(
  uint32_t dev_id,
  uint32_t lane,
  fpga_direct_err_stif_t *direct_err_stif
) {
  fpga_ioctl_direct_err_stif_t ioctl_direct_err_stif;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !direct_err_stif || (lane >= KERNEL_NUM_DIRECT(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), direct_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)direct_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), direct_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)direct_err_stif);

  ioctl_direct_err_stif.lane = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_GET_ERR_STIF, &ioctl_direct_err_stif) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_GET_ERR_STIF(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  direct_err_stif->ingress_rcv_req  = (ioctl_direct_err_stif.ingress_rcv_req  & 0x01);
  direct_err_stif->ingress_rcv_resp = (ioctl_direct_err_stif.ingress_rcv_resp & 0x01);
  direct_err_stif->ingress_rcv_data = (ioctl_direct_err_stif.ingress_rcv_data & 0x01);
  direct_err_stif->ingress_snd_req  = (ioctl_direct_err_stif.ingress_snd_req  & 0x01);
  direct_err_stif->ingress_snd_resp = (ioctl_direct_err_stif.ingress_snd_resp & 0x01);
  direct_err_stif->ingress_snd_data = (ioctl_direct_err_stif.ingress_snd_data & 0x01);
  direct_err_stif->egress_rcv_req   = (ioctl_direct_err_stif.egress_rcv_req   & 0x01);
  direct_err_stif->egress_rcv_resp  = (ioctl_direct_err_stif.egress_rcv_resp  & 0x01);
  direct_err_stif->egress_rcv_data  = (ioctl_direct_err_stif.egress_rcv_data  & 0x01);
  direct_err_stif->egress_snd_req   = (ioctl_direct_err_stif.egress_snd_req   & 0x01);
  direct_err_stif->egress_snd_resp  = (ioctl_direct_err_stif.egress_snd_resp  & 0x01);
  direct_err_stif->egress_snd_data  = (ioctl_direct_err_stif.egress_snd_data  & 0x01);

  return 0;
}


int fpga_direct_set_err_stif_mask(
  uint32_t dev_id,
  uint32_t lane,
  fpga_direct_err_stif_t direct_err_stif
) {
  fpga_ioctl_direct_err_stif_t ioctl_direct_err_stif;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_DIRECT(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), direct_err_stif(%#lx))\n",
      __func__, dev_id, lane, direct_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), direct_err_stif(%#lx))\n",
      __func__, dev_id, lane, direct_err_stif);

  ioctl_direct_err_stif.lane              = (int)lane;  // NOLINT
  ioctl_direct_err_stif.ingress_rcv_req   = (direct_err_stif.ingress_rcv_req  & 0x01);
  ioctl_direct_err_stif.ingress_rcv_resp  = (direct_err_stif.ingress_rcv_resp & 0x01);
  ioctl_direct_err_stif.ingress_rcv_data  = (direct_err_stif.ingress_rcv_data & 0x01);
  ioctl_direct_err_stif.ingress_snd_req   = (direct_err_stif.ingress_snd_req  & 0x01);
  ioctl_direct_err_stif.ingress_snd_resp  = (direct_err_stif.ingress_snd_resp & 0x01);
  ioctl_direct_err_stif.ingress_snd_data  = (direct_err_stif.ingress_snd_data & 0x01);
  ioctl_direct_err_stif.egress_rcv_req    = (direct_err_stif.egress_rcv_req   & 0x01);
  ioctl_direct_err_stif.egress_rcv_resp   = (direct_err_stif.egress_rcv_resp  & 0x01);
  ioctl_direct_err_stif.egress_rcv_data   = (direct_err_stif.egress_rcv_data  & 0x01);
  ioctl_direct_err_stif.egress_snd_req    = (direct_err_stif.egress_snd_req   & 0x01);
  ioctl_direct_err_stif.egress_snd_resp   = (direct_err_stif.egress_snd_resp  & 0x01);
  ioctl_direct_err_stif.egress_snd_data   = (direct_err_stif.egress_snd_data  & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_SET_ERR_STIF_MASK, &ioctl_direct_err_stif) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_SET_ERR_STIF_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_direct_get_err_stif_mask(
        uint32_t dev_id,
        uint32_t lane,
        fpga_direct_err_stif_t *direct_err_stif
) {
  fpga_ioctl_direct_err_stif_t ioctl_direct_err_stif;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !direct_err_stif || (lane >= KERNEL_NUM_DIRECT(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), direct_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)direct_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), direct_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)direct_err_stif);

  ioctl_direct_err_stif.lane = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_GET_ERR_STIF_MASK, &ioctl_direct_err_stif) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_GET_ERR_STIF_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  direct_err_stif->ingress_rcv_req  = (ioctl_direct_err_stif.ingress_rcv_req  & 0x01);
  direct_err_stif->ingress_rcv_resp = (ioctl_direct_err_stif.ingress_rcv_resp & 0x01);
  direct_err_stif->ingress_rcv_data = (ioctl_direct_err_stif.ingress_rcv_data & 0x01);
  direct_err_stif->ingress_snd_req  = (ioctl_direct_err_stif.ingress_snd_req  & 0x01);
  direct_err_stif->ingress_snd_resp = (ioctl_direct_err_stif.ingress_snd_resp & 0x01);
  direct_err_stif->ingress_snd_data = (ioctl_direct_err_stif.ingress_snd_data & 0x01);
  direct_err_stif->egress_rcv_req   = (ioctl_direct_err_stif.egress_rcv_req   & 0x01);
  direct_err_stif->egress_rcv_resp  = (ioctl_direct_err_stif.egress_rcv_resp  & 0x01);
  direct_err_stif->egress_rcv_data  = (ioctl_direct_err_stif.egress_rcv_data  & 0x01);
  direct_err_stif->egress_snd_req   = (ioctl_direct_err_stif.egress_snd_req   & 0x01);
  direct_err_stif->egress_snd_resp  = (ioctl_direct_err_stif.egress_snd_resp  & 0x01);
  direct_err_stif->egress_snd_data  = (ioctl_direct_err_stif.egress_snd_data  & 0x01);

  return 0;
}


int fpga_direct_set_err_stif_force(
  uint32_t dev_id,
  uint32_t lane,
  fpga_direct_err_stif_t direct_err_stif
) {
  fpga_ioctl_direct_err_stif_t ioctl_direct_err_stif;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_DIRECT(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), direct_err_stif(%#lx))\n",
      __func__, dev_id, lane, direct_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), direct_err_stif(%#lx))\n",
      __func__, dev_id, lane, direct_err_stif);

  ioctl_direct_err_stif.lane              = (int)lane;  // NOLINT
  ioctl_direct_err_stif.ingress_rcv_req   = (direct_err_stif.ingress_rcv_req  & 0x01);
  ioctl_direct_err_stif.ingress_rcv_resp  = (direct_err_stif.ingress_rcv_resp & 0x01);
  ioctl_direct_err_stif.ingress_rcv_data  = (direct_err_stif.ingress_rcv_data & 0x01);
  ioctl_direct_err_stif.ingress_snd_req   = (direct_err_stif.ingress_snd_req  & 0x01);
  ioctl_direct_err_stif.ingress_snd_resp  = (direct_err_stif.ingress_snd_resp & 0x01);
  ioctl_direct_err_stif.ingress_snd_data  = (direct_err_stif.ingress_snd_data & 0x01);
  ioctl_direct_err_stif.egress_rcv_req    = (direct_err_stif.egress_rcv_req   & 0x01);
  ioctl_direct_err_stif.egress_rcv_resp   = (direct_err_stif.egress_rcv_resp  & 0x01);
  ioctl_direct_err_stif.egress_rcv_data   = (direct_err_stif.egress_rcv_data  & 0x01);
  ioctl_direct_err_stif.egress_snd_req    = (direct_err_stif.egress_snd_req   & 0x01);
  ioctl_direct_err_stif.egress_snd_resp   = (direct_err_stif.egress_snd_resp  & 0x01);
  ioctl_direct_err_stif.egress_snd_data   = (direct_err_stif.egress_snd_data  & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_SET_ERR_STIF_FORCE, &ioctl_direct_err_stif) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_SET_ERR_STIF_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_direct_get_err_stif_force(
        uint32_t dev_id,
        uint32_t lane,
        fpga_direct_err_stif_t *direct_err_stif
) {
  fpga_ioctl_direct_err_stif_t ioctl_direct_err_stif;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !direct_err_stif || (lane >= KERNEL_NUM_DIRECT(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), direct_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)direct_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), direct_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)direct_err_stif);

  ioctl_direct_err_stif.lane = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_DIRECT_GET_ERR_STIF_FORCE, &ioctl_direct_err_stif) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DIRECT_GET_ERR_STIF_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  direct_err_stif->ingress_rcv_req  = (ioctl_direct_err_stif.ingress_rcv_req  & 0x01);
  direct_err_stif->ingress_rcv_resp = (ioctl_direct_err_stif.ingress_rcv_resp & 0x01);
  direct_err_stif->ingress_rcv_data = (ioctl_direct_err_stif.ingress_rcv_data & 0x01);
  direct_err_stif->ingress_snd_req  = (ioctl_direct_err_stif.ingress_snd_req  & 0x01);
  direct_err_stif->ingress_snd_resp = (ioctl_direct_err_stif.ingress_snd_resp & 0x01);
  direct_err_stif->ingress_snd_data = (ioctl_direct_err_stif.ingress_snd_data & 0x01);
  direct_err_stif->egress_rcv_req   = (ioctl_direct_err_stif.egress_rcv_req   & 0x01);
  direct_err_stif->egress_rcv_resp  = (ioctl_direct_err_stif.egress_rcv_resp  & 0x01);
  direct_err_stif->egress_rcv_data  = (ioctl_direct_err_stif.egress_rcv_data  & 0x01);
  direct_err_stif->egress_snd_req   = (ioctl_direct_err_stif.egress_snd_req   & 0x01);
  direct_err_stif->egress_snd_resp  = (ioctl_direct_err_stif.egress_snd_resp  & 0x01);
  direct_err_stif->egress_snd_data  = (ioctl_direct_err_stif.egress_snd_data  & 0x01);

  return 0;
}
