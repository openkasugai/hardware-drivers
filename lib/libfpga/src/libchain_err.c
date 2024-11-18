/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libchain.h>
#include <libchain_err.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBCHAIN


int fpga_chain_get_check_err(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *err_det
) {
  fpga_ioctl_err_all_t ioctl_err_all;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !err_det || (lane >= KERNEL_NUM_CHAIN(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), err_det(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)err_det);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), err_det(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)err_det);

  ioctl_err_all.lane = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_CHK_ERR, &ioctl_err_all) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_CHK_ERR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *err_det = ioctl_err_all.err_all;

  return 0;
}


int fpga_chain_get_err(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t extif_id,
  uint32_t cid,
  uint32_t dir,
  fpga_chain_err_t *chain_err
) {
  fpga_ioctl_chain_err_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), cid(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, cid, dir, (uintptr_t)chain_err);
    return -INVALID_ARGUMENT;
  }
  if (cid < CID_MIN || cid > CID_MAX) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), cid(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, cid, dir, chain_err);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), cid(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, cid, dir, (uintptr_t)chain_err);

  ioctl_chain_err.lane      = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id  = extif_id;
  ioctl_chain_err.dir       = (uint8_t)(dir & 0x000000FF);
  ioctl_chain_err.cid_fchid = (uint16_t)(cid & 0x0000FFFF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err->header_marker         = (ioctl_chain_err.header_marker         & 0x01);
  chain_err->payload_len           = (ioctl_chain_err.payload_len           & 0x01);
  chain_err->header_len            = (ioctl_chain_err.header_len            & 0x01);
  chain_err->header_chksum         = (ioctl_chain_err.header_chksum         & 0x01);
  chain_err->header_stat           = ioctl_chain_err.header_stat;
  chain_err->pointer_table_miss    = (ioctl_chain_err.pointer_table_miss    & 0x01);
  chain_err->payload_table_miss    = (ioctl_chain_err.payload_table_miss    & 0x01);
  chain_err->pointer_table_invalid = (ioctl_chain_err.pointer_table_invalid & 0x01);
  chain_err->payload_table_invalid = (ioctl_chain_err.payload_table_invalid & 0x01);

  return 0;
}


int fpga_chain_set_err_mask(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t extif_id,
  uint32_t dir,
  fpga_chain_err_t chain_err
) {
  fpga_ioctl_chain_err_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, chain_err);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, chain_err);

  ioctl_chain_err.lane                  = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id              = extif_id;
  ioctl_chain_err.dir                   = (uint8_t)(dir & 0x000000FF);
  ioctl_chain_err.header_marker         = (chain_err.header_marker         & 0x01);
  ioctl_chain_err.payload_len           = (chain_err.payload_len           & 0x01);
  ioctl_chain_err.header_len            = (chain_err.header_len            & 0x01);
  ioctl_chain_err.header_chksum         = (chain_err.header_chksum         & 0x01);
  ioctl_chain_err.header_stat           = chain_err.header_stat;
  ioctl_chain_err.pointer_table_miss    = (chain_err.pointer_table_miss    & 0x01);
  ioctl_chain_err.payload_table_miss    = (chain_err.payload_table_miss    & 0x01);
  ioctl_chain_err.pointer_table_invalid = (chain_err.pointer_table_invalid & 0x01);
  ioctl_chain_err.payload_table_invalid = (chain_err.payload_table_invalid & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_MASK, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_get_err_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_t *chain_err
) {
  fpga_ioctl_chain_err_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, (uintptr_t)chain_err);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, (uintptr_t)chain_err);

  ioctl_chain_err.lane      = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id  = extif_id;
  ioctl_chain_err.dir       = (uint8_t)(dir & 0x000000FF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_MASK, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err->header_marker         = (ioctl_chain_err.header_marker         & 0x01);
  chain_err->payload_len           = (ioctl_chain_err.payload_len           & 0x01);
  chain_err->header_len            = (ioctl_chain_err.header_len            & 0x01);
  chain_err->header_chksum         = (ioctl_chain_err.header_chksum         & 0x01);
  chain_err->header_stat           = ioctl_chain_err.header_stat;
  chain_err->pointer_table_miss    = (ioctl_chain_err.pointer_table_miss    & 0x01);
  chain_err->payload_table_miss    = (ioctl_chain_err.payload_table_miss    & 0x01);
  chain_err->pointer_table_invalid = (ioctl_chain_err.pointer_table_invalid & 0x01);
  chain_err->payload_table_invalid = (ioctl_chain_err.payload_table_invalid & 0x01);

  return 0;
}


int fpga_chain_set_err_force(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t extif_id,
  uint32_t dir,
  fpga_chain_err_t chain_err
) {
  fpga_ioctl_chain_err_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, chain_err);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, chain_err);

  ioctl_chain_err.lane                  = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id              = extif_id;
  ioctl_chain_err.dir                   = (uint8_t)(dir & 0x000000FF);
  ioctl_chain_err.header_marker         = (chain_err.header_marker         & 0x01);
  ioctl_chain_err.payload_len           = (chain_err.payload_len           & 0x01);
  ioctl_chain_err.header_len            = (chain_err.header_len            & 0x01);
  ioctl_chain_err.header_chksum         = (chain_err.header_chksum         & 0x01);
  ioctl_chain_err.header_stat           = chain_err.header_stat;
  ioctl_chain_err.pointer_table_miss    = (chain_err.pointer_table_miss    & 0x01);
  ioctl_chain_err.payload_table_miss    = (chain_err.payload_table_miss    & 0x01);
  ioctl_chain_err.pointer_table_invalid = (chain_err.pointer_table_invalid & 0x01);
  ioctl_chain_err.payload_table_invalid = (chain_err.payload_table_invalid & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_FORCE, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_get_err_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_t *chain_err
) {
  fpga_ioctl_chain_err_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, (uintptr_t)chain_err);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, (uintptr_t)chain_err);

  ioctl_chain_err.lane      = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id  = extif_id;
  ioctl_chain_err.dir       = (uint8_t)(dir & 0x000000FF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_FORCE, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err->header_marker         = (ioctl_chain_err.header_marker         & 0x01);
  chain_err->payload_len           = (ioctl_chain_err.payload_len           & 0x01);
  chain_err->header_len            = (ioctl_chain_err.header_len            & 0x01);
  chain_err->header_chksum         = (ioctl_chain_err.header_chksum         & 0x01);
  chain_err->header_stat           = ioctl_chain_err.header_stat;
  chain_err->pointer_table_miss    = (ioctl_chain_err.pointer_table_miss    & 0x01);
  chain_err->payload_table_miss    = (ioctl_chain_err.payload_table_miss    & 0x01);
  chain_err->pointer_table_invalid = (ioctl_chain_err.pointer_table_invalid & 0x01);
  chain_err->payload_table_invalid = (ioctl_chain_err.payload_table_invalid & 0x01);

  return 0;
}


int fpga_chain_err_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_t chain_err
) {
  fpga_ioctl_chain_err_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, chain_err);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, chain_err);

  ioctl_chain_err.lane                  = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id              = extif_id;
  ioctl_chain_err.dir                   = (uint8_t)(dir & 0x000000FF);

  if (dir == FPGA_CID_KIND_INGRESS) {
    ioctl_chain_err.header_marker         = 0; /* invalid */
    ioctl_chain_err.payload_len           = 0; /* invalid */
    ioctl_chain_err.header_len            = 0; /* invalid */
    ioctl_chain_err.header_chksum         = 0; /* invalid */
    ioctl_chain_err.header_stat           = 0; /* invalid */
    ioctl_chain_err.pointer_table_miss    = 0; /* invalid */
    ioctl_chain_err.payload_table_miss    = 0; /* invalid */
    ioctl_chain_err.con_table_miss        = 0; /* invalid */
    ioctl_chain_err.pointer_table_invalid = (chain_err.pointer_table_invalid & 0x01);
    ioctl_chain_err.payload_table_invalid = (chain_err.payload_table_invalid & 0x01);
    ioctl_chain_err.con_table_invalid     = (chain_err.con_table_invalid     & 0x01);
  } else if (dir == FPGA_CID_KIND_EGRESS) {
    ioctl_chain_err.header_marker         = (chain_err.header_marker         & 0x01);
    ioctl_chain_err.payload_len           = (chain_err.payload_len           & 0x01);
    ioctl_chain_err.header_len            = (chain_err.header_len            & 0x01);
    ioctl_chain_err.header_chksum         = (chain_err.header_chksum         & 0x01);
    ioctl_chain_err.header_stat           = chain_err.header_stat;
    ioctl_chain_err.pointer_table_miss    = 0; /* invalid */
    ioctl_chain_err.payload_table_miss    = 0; /* invalid */
    ioctl_chain_err.con_table_miss        = 0; /* invalid */
    ioctl_chain_err.pointer_table_invalid = (chain_err.pointer_table_invalid & 0x01);
    ioctl_chain_err.payload_table_invalid = (chain_err.payload_table_invalid & 0x01);
    ioctl_chain_err.con_table_invalid     = (chain_err.con_table_invalid     & 0x01);
  }

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_ERR_INS, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_ERR_INS(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_err_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_t *chain_err
) {
  fpga_ioctl_chain_err_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, (uintptr_t)chain_err);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, (uintptr_t)chain_err);

  ioctl_chain_err.lane      = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id  = extif_id;
  ioctl_chain_err.dir       = (uint8_t)(dir & 0x000000FF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_ERR_GET_INS, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_ERR_GET_INS(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  if (dir == FPGA_CID_KIND_INGRESS) {
    chain_err->header_marker         = 0; /* invalid */
    chain_err->payload_len           = 0; /* invalid */
    chain_err->header_len            = 0; /* invalid */
    chain_err->header_chksum         = 0; /* invalid */
    chain_err->header_stat           = 0; /* invalid */
    chain_err->pointer_table_miss    = 0; /* invalid */
    chain_err->payload_table_miss    = 0; /* invalid */
    chain_err->con_table_miss        = 0; /* invalid */
    chain_err->pointer_table_invalid = (ioctl_chain_err.pointer_table_invalid & 0x01);
    chain_err->payload_table_invalid = (ioctl_chain_err.payload_table_invalid & 0x01);
    chain_err->con_table_invalid     = (ioctl_chain_err.con_table_invalid     & 0x01);
  } else if (dir == FPGA_CID_KIND_EGRESS) {
    chain_err->header_marker         = (ioctl_chain_err.header_marker         & 0x01);
    chain_err->payload_len           = (ioctl_chain_err.payload_len           & 0x01);
    chain_err->header_len            = (ioctl_chain_err.header_len            & 0x01);
    chain_err->header_chksum         = (ioctl_chain_err.header_chksum         & 0x01);
    chain_err->header_stat           = ioctl_chain_err.header_stat;
    chain_err->pointer_table_miss    = 0; /* invalid */
    chain_err->payload_table_miss    = 0; /* invalid */
    chain_err->con_table_miss        = 0; /* invalid */
    chain_err->pointer_table_invalid = (ioctl_chain_err.pointer_table_invalid & 0x01);
    chain_err->payload_table_invalid = (ioctl_chain_err.payload_table_invalid & 0x01);
    chain_err->con_table_invalid     = (ioctl_chain_err.con_table_invalid     & 0x01);
  }
  return 0;
}


int fpga_chain_get_err_table(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t extif_id,
  uint32_t cid_fchid,
  uint32_t dir,
  fpga_chain_err_table_t *chain_err
) {
  fpga_ioctl_chain_err_table_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    goto invalid_arg;
  }
  if (dir == FPGA_CID_KIND_INGRESS) {
    if (cid_fchid < CID_MIN || cid_fchid > CID_MAX) {
      goto invalid_arg;
    }
    if (extif_id > FPGA_EXTIF_NUMBER_1) {
      goto invalid_arg;
    }
  } else if (dir == FPGA_CID_KIND_EGRESS) {
    if (cid_fchid < FUNCTION_CHAIN_ID_MIN || cid_fchid > FUNCTION_CHAIN_ID_MAX) {
      goto invalid_arg;
    }
  } else {
    goto invalid_arg;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), cid_fchid(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, cid_fchid, dir, (uintptr_t)chain_err);

  ioctl_chain_err.lane      = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id  = extif_id;
  ioctl_chain_err.dir       = (uint8_t)(dir & 0x000000FF);
  ioctl_chain_err.cid_fchid = (uint16_t)(cid_fchid & 0x0000FFFF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_TBL, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_TBL(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err->con_table_miss    = (ioctl_chain_err.con_table_miss    & 0x01);
  chain_err->con_table_invalid = (ioctl_chain_err.con_table_invalid & 0x01);

  return 0;

invalid_arg:
  llf_err(INVALID_ARGUMENT,
    "%s(dev_id(%u), lane(%u), extif_id(%u), cid_fchid(%u), dir(%u), chain_err(%#lx))\n",
    __func__, dev_id, lane, extif_id, cid_fchid, dir, chain_err);
  return -INVALID_ARGUMENT;
}


int fpga_chain_set_err_table_mask(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t extif_id,
  uint32_t dir,
  fpga_chain_err_table_t chain_err
) {
  fpga_ioctl_chain_err_table_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    goto invalid_arg;
  }
  if (dir == FPGA_CID_KIND_INGRESS) {
    if (extif_id > FPGA_EXTIF_NUMBER_1) {
      goto invalid_arg;
    }
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, chain_err);

  ioctl_chain_err.lane                  = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id              = extif_id;
  ioctl_chain_err.dir                   = (uint8_t)(dir & 0x000000FF);
  ioctl_chain_err.con_table_miss        = (chain_err.con_table_miss        & 0x01);
  ioctl_chain_err.con_table_invalid     = (chain_err.con_table_invalid     & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_TBL_MASK, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_TBL_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;

invalid_arg:
  llf_err(INVALID_ARGUMENT,
    "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
    __func__, dev_id, lane, extif_id, dir, chain_err);
  return -INVALID_ARGUMENT;
}


int fpga_chain_get_err_table_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_table_t *chain_err
) {
  fpga_ioctl_chain_err_table_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    goto invalid_arg;
  }
  if (dir == FPGA_CID_KIND_INGRESS) {
    if (extif_id > FPGA_EXTIF_NUMBER_1) {
      goto invalid_arg;
    }
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, (uintptr_t)chain_err);

  ioctl_chain_err.lane      = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id  = extif_id;
  ioctl_chain_err.dir       = (uint8_t)(dir & 0x000000FF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_TBL_MASK, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_TBL_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err->con_table_miss        = (ioctl_chain_err.con_table_miss        & 0x01);
  chain_err->con_table_invalid     = (ioctl_chain_err.con_table_invalid     & 0x01);

  return 0;

invalid_arg:
  llf_err(INVALID_ARGUMENT,
    "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
    __func__, dev_id, lane, extif_id, dir, (uintptr_t)chain_err);
  return -INVALID_ARGUMENT;
}


int fpga_chain_set_err_table_force(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t extif_id,
  uint32_t dir,
  fpga_chain_err_table_t chain_err
) {
  fpga_ioctl_chain_err_table_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    goto invalid_arg;
  }
  if (dir == FPGA_CID_KIND_INGRESS) {
    if (extif_id > FPGA_EXTIF_NUMBER_1) {
      goto invalid_arg;
    }
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, chain_err);

  ioctl_chain_err.lane                  = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id              = extif_id;
  ioctl_chain_err.dir                   = (uint8_t)(dir & 0x000000FF);
  ioctl_chain_err.con_table_miss        = (chain_err.con_table_miss        & 0x01);
  ioctl_chain_err.con_table_invalid     = (chain_err.con_table_invalid     & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_TBL_FORCE, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_TBL_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;

invalid_arg:
  llf_err(INVALID_ARGUMENT,
    "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
    __func__, dev_id, lane, extif_id, dir, chain_err);
  return -INVALID_ARGUMENT;
}


int fpga_chain_get_err_table_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_table_t *chain_err
) {
  fpga_ioctl_chain_err_table_t ioctl_chain_err;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    goto invalid_arg;
  }
  if (dir == FPGA_CID_KIND_INGRESS) {
    if (extif_id > FPGA_EXTIF_NUMBER_1) {
      goto invalid_arg;
    }
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, (uintptr_t)chain_err);

  ioctl_chain_err.lane      = (int)lane;  // NOLINT
  ioctl_chain_err.extif_id  = extif_id;
  ioctl_chain_err.dir       = (uint8_t)(dir & 0x000000FF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_TBL_FORCE, &ioctl_chain_err) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_TBL_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err->con_table_miss        = (ioctl_chain_err.con_table_miss        & 0x01);
  chain_err->con_table_invalid     = (ioctl_chain_err.con_table_invalid     & 0x01);

  return 0;

invalid_arg:
  llf_err(INVALID_ARGUMENT,
    "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), chain_err(%#lx))\n",
    __func__, dev_id, lane, extif_id, dir, (uintptr_t)chain_err);
  return -INVALID_ARGUMENT;
}


int fpga_chain_get_err_prot(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t dir,
  fpga_chain_err_prot_t *chain_err_prot
) {
  fpga_ioctl_chain_err_prot_t ioctl_chain_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err_prot || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)chain_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)chain_err_prot);

  ioctl_chain_err_prot.lane = (int)lane;  // NOLINT
  ioctl_chain_err_prot.dir  = dir;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_PROT, &ioctl_chain_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_PROT(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err_prot->prot_ch                = (ioctl_chain_err_prot.prot_ch                & 0x01);
  chain_err_prot->prot_len               = (ioctl_chain_err_prot.prot_len               & 0x01);
  chain_err_prot->prot_sof               = (ioctl_chain_err_prot.prot_sof               & 0x01);
  chain_err_prot->prot_eof               = (ioctl_chain_err_prot.prot_eof               & 0x01);
  chain_err_prot->prot_reqresp           = (ioctl_chain_err_prot.prot_reqresp           & 0x01);
  chain_err_prot->prot_datanum           = (ioctl_chain_err_prot.prot_datanum           & 0x01);
  chain_err_prot->prot_req_outstanding   = (ioctl_chain_err_prot.prot_req_outstanding   & 0x01);
  chain_err_prot->prot_resp_outstanding  = (ioctl_chain_err_prot.prot_resp_outstanding  & 0x01);
  chain_err_prot->prot_max_datanum       = (ioctl_chain_err_prot.prot_max_datanum       & 0x01);
  chain_err_prot->prot_reqlen            = (ioctl_chain_err_prot.prot_reqlen            & 0x01);
  chain_err_prot->prot_reqresplen        = (ioctl_chain_err_prot.prot_reqresplen        & 0x01);

  return 0;
}


int fpga_chain_set_err_prot_clear(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t dir,
  fpga_chain_err_prot_t chain_err_prot
) {
  fpga_ioctl_chain_err_prot_t ioctl_chain_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, chain_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, chain_err_prot);

  ioctl_chain_err_prot.lane                  = (int)lane;  // NOLINT
  ioctl_chain_err_prot.dir                   = dir;
  ioctl_chain_err_prot.prot_ch               = (chain_err_prot.prot_ch                & 0x01);
  ioctl_chain_err_prot.prot_len              = (chain_err_prot.prot_len               & 0x01);
  ioctl_chain_err_prot.prot_sof              = (chain_err_prot.prot_sof               & 0x01);
  ioctl_chain_err_prot.prot_eof              = (chain_err_prot.prot_eof               & 0x01);
  ioctl_chain_err_prot.prot_reqresp          = (chain_err_prot.prot_reqresp           & 0x01);
  ioctl_chain_err_prot.prot_datanum          = (chain_err_prot.prot_datanum           & 0x01);
  ioctl_chain_err_prot.prot_req_outstanding  = (chain_err_prot.prot_req_outstanding   & 0x01);
  ioctl_chain_err_prot.prot_resp_outstanding = (chain_err_prot.prot_resp_outstanding  & 0x01);
  ioctl_chain_err_prot.prot_max_datanum      = (chain_err_prot.prot_max_datanum       & 0x01);
  ioctl_chain_err_prot.prot_reqlen           = (chain_err_prot.prot_reqlen            & 0x01);
  ioctl_chain_err_prot.prot_reqresplen       = (chain_err_prot.prot_reqresplen        & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_PROT_CLR, &ioctl_chain_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_PROT_CLR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_set_err_prot_mask(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t dir,
  fpga_chain_err_prot_t chain_err_prot
) {
  fpga_ioctl_chain_err_prot_t ioctl_chain_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, chain_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, chain_err_prot);

  ioctl_chain_err_prot.lane                  = (int)lane;  // NOLINT
  ioctl_chain_err_prot.dir                   = dir;
  ioctl_chain_err_prot.prot_ch               = (chain_err_prot.prot_ch                & 0x01);
  ioctl_chain_err_prot.prot_len              = (chain_err_prot.prot_len               & 0x01);
  ioctl_chain_err_prot.prot_sof              = (chain_err_prot.prot_sof               & 0x01);
  ioctl_chain_err_prot.prot_eof              = (chain_err_prot.prot_eof               & 0x01);
  ioctl_chain_err_prot.prot_reqresp          = (chain_err_prot.prot_reqresp           & 0x01);
  ioctl_chain_err_prot.prot_datanum          = (chain_err_prot.prot_datanum           & 0x01);
  ioctl_chain_err_prot.prot_req_outstanding  = (chain_err_prot.prot_req_outstanding   & 0x01);
  ioctl_chain_err_prot.prot_resp_outstanding = (chain_err_prot.prot_resp_outstanding  & 0x01);
  ioctl_chain_err_prot.prot_max_datanum      = (chain_err_prot.prot_max_datanum       & 0x01);
  ioctl_chain_err_prot.prot_reqlen           = (chain_err_prot.prot_reqlen            & 0x01);
  ioctl_chain_err_prot.prot_reqresplen       = (chain_err_prot.prot_reqresplen        & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_PROT_MASK, &ioctl_chain_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_PROT_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_get_err_prot_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t *chain_err_prot
) {
  fpga_ioctl_chain_err_prot_t ioctl_chain_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err_prot || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)chain_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)chain_err_prot);

  ioctl_chain_err_prot.lane = (int)lane;  // NOLINT
  ioctl_chain_err_prot.dir  = dir;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_PROT_MASK, &ioctl_chain_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_PROT_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  chain_err_prot->prot_ch                = (ioctl_chain_err_prot.prot_ch                & 0x01);
  chain_err_prot->prot_len               = (ioctl_chain_err_prot.prot_len               & 0x01);
  chain_err_prot->prot_sof               = (ioctl_chain_err_prot.prot_sof               & 0x01);
  chain_err_prot->prot_eof               = (ioctl_chain_err_prot.prot_eof               & 0x01);
  chain_err_prot->prot_reqresp           = (ioctl_chain_err_prot.prot_reqresp           & 0x01);
  chain_err_prot->prot_datanum           = (ioctl_chain_err_prot.prot_datanum           & 0x01);
  chain_err_prot->prot_req_outstanding   = (ioctl_chain_err_prot.prot_req_outstanding   & 0x01);
  chain_err_prot->prot_resp_outstanding  = (ioctl_chain_err_prot.prot_resp_outstanding  & 0x01);
  chain_err_prot->prot_max_datanum       = (ioctl_chain_err_prot.prot_max_datanum       & 0x01);
  chain_err_prot->prot_reqlen            = (ioctl_chain_err_prot.prot_reqlen            & 0x01);
  chain_err_prot->prot_reqresplen        = (ioctl_chain_err_prot.prot_reqresplen        & 0x01);

  return 0;
}


int fpga_chain_set_err_prot_force(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t dir,
  fpga_chain_err_prot_t chain_err_prot
) {
  fpga_ioctl_chain_err_prot_t ioctl_chain_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, chain_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, chain_err_prot);

  ioctl_chain_err_prot.lane                  = (int)lane;  // NOLINT
  ioctl_chain_err_prot.dir                   = dir;
  ioctl_chain_err_prot.prot_ch               = (chain_err_prot.prot_ch                & 0x01);
  ioctl_chain_err_prot.prot_len              = (chain_err_prot.prot_len               & 0x01);
  ioctl_chain_err_prot.prot_sof              = (chain_err_prot.prot_sof               & 0x01);
  ioctl_chain_err_prot.prot_eof              = (chain_err_prot.prot_eof               & 0x01);
  ioctl_chain_err_prot.prot_reqresp          = (chain_err_prot.prot_reqresp           & 0x01);
  ioctl_chain_err_prot.prot_datanum          = (chain_err_prot.prot_datanum           & 0x01);
  ioctl_chain_err_prot.prot_req_outstanding  = (chain_err_prot.prot_req_outstanding   & 0x01);
  ioctl_chain_err_prot.prot_resp_outstanding = (chain_err_prot.prot_resp_outstanding  & 0x01);
  ioctl_chain_err_prot.prot_max_datanum      = (chain_err_prot.prot_max_datanum       & 0x01);
  ioctl_chain_err_prot.prot_reqlen           = (chain_err_prot.prot_reqlen            & 0x01);
  ioctl_chain_err_prot.prot_reqresplen       = (chain_err_prot.prot_reqresplen        & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_PROT_FORCE, &ioctl_chain_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_PROT_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_get_err_prot_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t *chain_err_prot
) {
  fpga_ioctl_chain_err_prot_t ioctl_chain_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err_prot || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)chain_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)chain_err_prot);

  ioctl_chain_err_prot.lane = (int)lane;  // NOLINT
  ioctl_chain_err_prot.dir  = dir;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_PROT_FORCE, &ioctl_chain_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_PROT_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err_prot->prot_ch                = (ioctl_chain_err_prot.prot_ch                & 0x01);
  chain_err_prot->prot_len               = (ioctl_chain_err_prot.prot_len               & 0x01);
  chain_err_prot->prot_sof               = (ioctl_chain_err_prot.prot_sof               & 0x01);
  chain_err_prot->prot_eof               = (ioctl_chain_err_prot.prot_eof               & 0x01);
  chain_err_prot->prot_reqresp           = (ioctl_chain_err_prot.prot_reqresp           & 0x01);
  chain_err_prot->prot_datanum           = (ioctl_chain_err_prot.prot_datanum           & 0x01);
  chain_err_prot->prot_req_outstanding   = (ioctl_chain_err_prot.prot_req_outstanding   & 0x01);
  chain_err_prot->prot_resp_outstanding  = (ioctl_chain_err_prot.prot_resp_outstanding  & 0x01);
  chain_err_prot->prot_max_datanum       = (ioctl_chain_err_prot.prot_max_datanum       & 0x01);
  chain_err_prot->prot_reqlen            = (ioctl_chain_err_prot.prot_reqlen            & 0x01);
  chain_err_prot->prot_reqresplen        = (ioctl_chain_err_prot.prot_reqresplen        & 0x01);

  return 0;
}


int fpga_chain_err_prot_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t chain_err_prot
) {
  fpga_ioctl_chain_err_prot_t ioctl_chain_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, chain_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, chain_err_prot);

  ioctl_chain_err_prot.lane                  = (int)lane;  // NOLINT
  ioctl_chain_err_prot.dir                   = dir;
  ioctl_chain_err_prot.prot_ch               = (chain_err_prot.prot_ch                & 0x01);
  ioctl_chain_err_prot.prot_len              = (chain_err_prot.prot_len               & 0x01);
  ioctl_chain_err_prot.prot_sof              = (chain_err_prot.prot_sof               & 0x01);
  ioctl_chain_err_prot.prot_eof              = (chain_err_prot.prot_eof               & 0x01);
  ioctl_chain_err_prot.prot_reqresp          = (chain_err_prot.prot_reqresp           & 0x01);
  ioctl_chain_err_prot.prot_datanum          = (chain_err_prot.prot_datanum           & 0x01);
  ioctl_chain_err_prot.prot_req_outstanding  = (chain_err_prot.prot_req_outstanding   & 0x01);
  ioctl_chain_err_prot.prot_resp_outstanding = (chain_err_prot.prot_resp_outstanding  & 0x01);
  ioctl_chain_err_prot.prot_max_datanum      = (chain_err_prot.prot_max_datanum       & 0x01);
  ioctl_chain_err_prot.prot_reqlen           = (chain_err_prot.prot_reqlen            & 0x01);
  ioctl_chain_err_prot.prot_reqresplen       = (chain_err_prot.prot_reqresplen        & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_ERR_PROT_INS, &ioctl_chain_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_ERR_PROT_INS(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_err_prot_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t *chain_err_prot
) {
  fpga_ioctl_chain_err_prot_t ioctl_chain_err_prot;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err_prot || (lane >= KERNEL_NUM_CHAIN(dev)) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)chain_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), dir(%u), chain_err_prot(%#lx))\n",
      __func__, dev_id, lane, dir, (uintptr_t)chain_err_prot);

  ioctl_chain_err_prot.lane = (int)lane;  // NOLINT
  ioctl_chain_err_prot.dir  = dir;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_ERR_PROT_GET_INS, &ioctl_chain_err_prot) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_ERR_PROT_GET_INS(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err_prot->prot_ch                = (ioctl_chain_err_prot.prot_ch                & 0x01);
  chain_err_prot->prot_len               = (ioctl_chain_err_prot.prot_len               & 0x01);
  chain_err_prot->prot_sof               = (ioctl_chain_err_prot.prot_sof               & 0x01);
  chain_err_prot->prot_eof               = (ioctl_chain_err_prot.prot_eof               & 0x01);
  chain_err_prot->prot_reqresp           = (ioctl_chain_err_prot.prot_reqresp           & 0x01);
  chain_err_prot->prot_datanum           = (ioctl_chain_err_prot.prot_datanum           & 0x01);
  chain_err_prot->prot_req_outstanding   = (ioctl_chain_err_prot.prot_req_outstanding   & 0x01);
  chain_err_prot->prot_resp_outstanding  = (ioctl_chain_err_prot.prot_resp_outstanding  & 0x01);
  chain_err_prot->prot_max_datanum       = (ioctl_chain_err_prot.prot_max_datanum       & 0x01);
  chain_err_prot->prot_reqlen            = (ioctl_chain_err_prot.prot_reqlen            & 0x01);
  chain_err_prot->prot_reqresplen        = (ioctl_chain_err_prot.prot_reqresplen        & 0x01);

  return 0;
}


int fpga_chain_get_err_evt(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t extif_id,
  fpga_chain_err_evt_t *chain_err_evt
) {
  fpga_ioctl_chain_err_evt_t ioctl_chain_err_evt;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err_evt || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, (uintptr_t)chain_err_evt);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, (uintptr_t)chain_err_evt);

  ioctl_chain_err_evt.lane     = (int)lane;  // NOLINT
  ioctl_chain_err_evt.extif_id = extif_id & 0xFF;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_EVT, &ioctl_chain_err_evt) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_EVT(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err_evt->established      = (ioctl_chain_err_evt.established      & 0x01);
  chain_err_evt->close_wait       = (ioctl_chain_err_evt.close_wait       & 0x01);
  chain_err_evt->erased           = (ioctl_chain_err_evt.erased           & 0x01);
  chain_err_evt->syn_timeout      = (ioctl_chain_err_evt.syn_timeout      & 0x01);
  chain_err_evt->syn_ack_timeout  = (ioctl_chain_err_evt.syn_ack_timeout  & 0x01);
  chain_err_evt->timeout          = (ioctl_chain_err_evt.timeout          & 0x01);
  chain_err_evt->recv_data        = (ioctl_chain_err_evt.recv_data        & 0x01);
  chain_err_evt->send_data        = (ioctl_chain_err_evt.send_data        & 0x01);
  chain_err_evt->recv_urgent_data = (ioctl_chain_err_evt.recv_urgent_data & 0x01);
  chain_err_evt->recv_rst         = (ioctl_chain_err_evt.recv_rst         & 0x01);

  return 0;
}


int fpga_chain_set_err_evt_clear(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t extif_id,
  fpga_chain_err_evt_t chain_err_evt
) {
  fpga_ioctl_chain_err_evt_t ioctl_chain_err_evt;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, chain_err_evt);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, chain_err_evt);

  ioctl_chain_err_evt.lane             = (int)lane;  // NOLINT
  ioctl_chain_err_evt.extif_id         = extif_id;
  ioctl_chain_err_evt.established      = (chain_err_evt.established      & 0x01);
  ioctl_chain_err_evt.close_wait       = (chain_err_evt.close_wait       & 0x01);
  ioctl_chain_err_evt.erased           = (chain_err_evt.erased           & 0x01);
  ioctl_chain_err_evt.syn_timeout      = (chain_err_evt.syn_timeout      & 0x01);
  ioctl_chain_err_evt.syn_ack_timeout  = (chain_err_evt.syn_ack_timeout  & 0x01);
  ioctl_chain_err_evt.timeout          = (chain_err_evt.timeout          & 0x01);
  ioctl_chain_err_evt.recv_data        = (chain_err_evt.recv_data        & 0x01);
  ioctl_chain_err_evt.send_data        = (chain_err_evt.send_data        & 0x01);
  ioctl_chain_err_evt.recv_urgent_data = (chain_err_evt.recv_urgent_data & 0x01);
  ioctl_chain_err_evt.recv_rst         = (chain_err_evt.recv_rst         & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_EVT_CLR, &ioctl_chain_err_evt) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_EVT_CLR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_set_err_evt_mask(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t extif_id,
  fpga_chain_err_evt_t chain_err_evt
) {
  fpga_ioctl_chain_err_evt_t ioctl_chain_err_evt;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, chain_err_evt);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, chain_err_evt);

  ioctl_chain_err_evt.lane             = (int)lane;  // NOLINT
  ioctl_chain_err_evt.extif_id         = extif_id;
  ioctl_chain_err_evt.established      = (chain_err_evt.established      & 0x01);
  ioctl_chain_err_evt.close_wait       = (chain_err_evt.close_wait       & 0x01);
  ioctl_chain_err_evt.erased           = (chain_err_evt.erased           & 0x01);
  ioctl_chain_err_evt.syn_timeout      = (chain_err_evt.syn_timeout      & 0x01);
  ioctl_chain_err_evt.syn_ack_timeout  = (chain_err_evt.syn_ack_timeout  & 0x01);
  ioctl_chain_err_evt.timeout          = (chain_err_evt.timeout          & 0x01);
  ioctl_chain_err_evt.recv_data        = (chain_err_evt.recv_data        & 0x01);
  ioctl_chain_err_evt.send_data        = (chain_err_evt.send_data        & 0x01);
  ioctl_chain_err_evt.recv_urgent_data = (chain_err_evt.recv_urgent_data & 0x01);
  ioctl_chain_err_evt.recv_rst         = (chain_err_evt.recv_rst         & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_EVT_MASK, &ioctl_chain_err_evt) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_EVT_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_get_err_evt_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        fpga_chain_err_evt_t *chain_err_evt
) {
  fpga_ioctl_chain_err_evt_t ioctl_chain_err_evt;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err_evt || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, (uintptr_t)chain_err_evt);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, (uintptr_t)chain_err_evt);

  ioctl_chain_err_evt.lane     = (int)lane;  // NOLINT
  ioctl_chain_err_evt.extif_id = extif_id & 0xFF;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_EVT_MASK, &ioctl_chain_err_evt) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_EVT_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err_evt->established      = (ioctl_chain_err_evt.established      & 0x01);
  chain_err_evt->close_wait       = (ioctl_chain_err_evt.close_wait       & 0x01);
  chain_err_evt->erased           = (ioctl_chain_err_evt.erased           & 0x01);
  chain_err_evt->syn_timeout      = (ioctl_chain_err_evt.syn_timeout      & 0x01);
  chain_err_evt->syn_ack_timeout  = (ioctl_chain_err_evt.syn_ack_timeout  & 0x01);
  chain_err_evt->timeout          = (ioctl_chain_err_evt.timeout          & 0x01);
  chain_err_evt->recv_data        = (ioctl_chain_err_evt.recv_data        & 0x01);
  chain_err_evt->send_data        = (ioctl_chain_err_evt.send_data        & 0x01);
  chain_err_evt->recv_urgent_data = (ioctl_chain_err_evt.recv_urgent_data & 0x01);
  chain_err_evt->recv_rst         = (ioctl_chain_err_evt.recv_rst         & 0x01);

  return 0;
}


int fpga_chain_set_err_evt_force(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t extif_id,
  fpga_chain_err_evt_t chain_err_evt
) {
  fpga_ioctl_chain_err_evt_t ioctl_chain_err_evt;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, chain_err_evt);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, chain_err_evt);

  ioctl_chain_err_evt.lane             = (int)lane;  // NOLINT
  ioctl_chain_err_evt.extif_id         = extif_id;
  ioctl_chain_err_evt.established      = (chain_err_evt.established      & 0x01);
  ioctl_chain_err_evt.close_wait       = (chain_err_evt.close_wait       & 0x01);
  ioctl_chain_err_evt.erased           = (chain_err_evt.erased           & 0x01);
  ioctl_chain_err_evt.syn_timeout      = (chain_err_evt.syn_timeout      & 0x01);
  ioctl_chain_err_evt.syn_ack_timeout  = (chain_err_evt.syn_ack_timeout  & 0x01);
  ioctl_chain_err_evt.timeout          = (chain_err_evt.timeout          & 0x01);
  ioctl_chain_err_evt.recv_data        = (chain_err_evt.recv_data        & 0x01);
  ioctl_chain_err_evt.send_data        = (chain_err_evt.send_data        & 0x01);
  ioctl_chain_err_evt.recv_urgent_data = (chain_err_evt.recv_urgent_data & 0x01);
  ioctl_chain_err_evt.recv_rst         = (chain_err_evt.recv_rst         & 0x01);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_EVT_FORCE, &ioctl_chain_err_evt) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_EVT_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_get_err_evt_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        fpga_chain_err_evt_t *chain_err_evt
) {
  fpga_ioctl_chain_err_evt_t ioctl_chain_err_evt;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err_evt || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, (uintptr_t)chain_err_evt);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), chain_err_evt(%#lx))\n",
      __func__, dev_id, lane, extif_id, (uintptr_t)chain_err_evt);

  ioctl_chain_err_evt.lane     = (int)lane;  // NOLINT
  ioctl_chain_err_evt.extif_id = extif_id & 0xFF;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_EVT_FORCE, &ioctl_chain_err_evt) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_EVT_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err_evt->established      = (ioctl_chain_err_evt.established      & 0x01);
  chain_err_evt->close_wait       = (ioctl_chain_err_evt.close_wait       & 0x01);
  chain_err_evt->erased           = (ioctl_chain_err_evt.erased           & 0x01);
  chain_err_evt->syn_timeout      = (ioctl_chain_err_evt.syn_timeout      & 0x01);
  chain_err_evt->syn_ack_timeout  = (ioctl_chain_err_evt.syn_ack_timeout  & 0x01);
  chain_err_evt->timeout          = (ioctl_chain_err_evt.timeout          & 0x01);
  chain_err_evt->recv_data        = (ioctl_chain_err_evt.recv_data        & 0x01);
  chain_err_evt->send_data        = (ioctl_chain_err_evt.send_data        & 0x01);
  chain_err_evt->recv_urgent_data = (ioctl_chain_err_evt.recv_urgent_data & 0x01);
  chain_err_evt->recv_rst         = (ioctl_chain_err_evt.recv_rst         & 0x01);

  return 0;
}


int fpga_chain_get_err_stif(
  uint32_t dev_id,
  uint32_t lane,
  fpga_chain_err_stif_t *chain_err_stif
) {
  fpga_ioctl_chain_err_stif_t ioctl_chain_err_stif;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err_stif || (lane >= KERNEL_NUM_CHAIN(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), chain_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)chain_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), chain_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)chain_err_stif);

  ioctl_chain_err_stif.lane = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_STIF, &ioctl_chain_err_stif) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_STIF(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err_stif->ingress_req   = (ioctl_chain_err_stif.ingress_req  & 0x01);
  chain_err_stif->ingress_resp  = (ioctl_chain_err_stif.ingress_resp & 0x01);
  chain_err_stif->ingress_data  = (ioctl_chain_err_stif.ingress_data & 0x01);
  chain_err_stif->egress_req    = (ioctl_chain_err_stif.egress_req   & 0x01);
  chain_err_stif->egress_resp   = (ioctl_chain_err_stif.egress_resp  & 0x01);
  chain_err_stif->egress_data   = (ioctl_chain_err_stif.egress_data  & 0x01);
  chain_err_stif->extif_event   = ioctl_chain_err_stif.extif_event;
  chain_err_stif->extif_command = ioctl_chain_err_stif.extif_command;

  return 0;
}


int fpga_chain_set_err_stif_mask(
  uint32_t dev_id,
  uint32_t lane,
  fpga_chain_err_stif_t chain_err_stif
) {
  fpga_ioctl_chain_err_stif_t ioctl_chain_err_stif;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), chain_err_stif(%#lx))\n",
      __func__, dev_id, lane, chain_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), chain_err_stif(%#lx))\n",
      __func__, dev_id, lane, chain_err_stif);

  ioctl_chain_err_stif.lane          = (int)lane;  // NOLINT
  ioctl_chain_err_stif.ingress_req   = (chain_err_stif.ingress_req  & 0x01);
  ioctl_chain_err_stif.ingress_resp  = (chain_err_stif.ingress_resp & 0x01);
  ioctl_chain_err_stif.ingress_data  = (chain_err_stif.ingress_data & 0x01);
  ioctl_chain_err_stif.egress_req    = (chain_err_stif.egress_req   & 0x01);
  ioctl_chain_err_stif.egress_resp   = (chain_err_stif.egress_resp  & 0x01);
  ioctl_chain_err_stif.egress_data   = (chain_err_stif.egress_data  & 0x01);
  ioctl_chain_err_stif.extif_event   = chain_err_stif.extif_event;
  ioctl_chain_err_stif.extif_command = chain_err_stif.extif_command;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_STIF_MASK, &ioctl_chain_err_stif) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_STIF_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_get_err_stif_mask(
        uint32_t dev_id,
        uint32_t lane,
        fpga_chain_err_stif_t *chain_err_stif
) {
  fpga_ioctl_chain_err_stif_t ioctl_chain_err_stif;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err_stif || (lane >= KERNEL_NUM_CHAIN(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), chain_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)chain_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), chain_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)chain_err_stif);

  ioctl_chain_err_stif.lane = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_STIF_MASK, &ioctl_chain_err_stif) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_STIF_MASK(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err_stif->ingress_req   = (ioctl_chain_err_stif.ingress_req  & 0x01);
  chain_err_stif->ingress_resp  = (ioctl_chain_err_stif.ingress_resp & 0x01);
  chain_err_stif->ingress_data  = (ioctl_chain_err_stif.ingress_data & 0x01);
  chain_err_stif->egress_req    = (ioctl_chain_err_stif.egress_req   & 0x01);
  chain_err_stif->egress_resp   = (ioctl_chain_err_stif.egress_resp  & 0x01);
  chain_err_stif->egress_data   = (ioctl_chain_err_stif.egress_data  & 0x01);
  chain_err_stif->extif_event   = ioctl_chain_err_stif.extif_event;
  chain_err_stif->extif_command = ioctl_chain_err_stif.extif_command;

  return 0;
}


int fpga_chain_set_err_stif_force(
  uint32_t dev_id,
  uint32_t lane,
  fpga_chain_err_stif_t chain_err_stif
) {
  fpga_ioctl_chain_err_stif_t ioctl_chain_err_stif;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), chain_err_stif(%#lx))\n",
      __func__, dev_id, lane, chain_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), chain_err_stif(%#lx))\n",
      __func__, dev_id, lane, chain_err_stif);

  ioctl_chain_err_stif.lane          = (int)lane;  // NOLINT
  ioctl_chain_err_stif.ingress_req   = (chain_err_stif.ingress_req  & 0x01);
  ioctl_chain_err_stif.ingress_resp  = (chain_err_stif.ingress_resp & 0x01);
  ioctl_chain_err_stif.ingress_data  = (chain_err_stif.ingress_data & 0x01);
  ioctl_chain_err_stif.egress_req    = (chain_err_stif.egress_req   & 0x01);
  ioctl_chain_err_stif.egress_resp   = (chain_err_stif.egress_resp  & 0x01);
  ioctl_chain_err_stif.egress_data   = (chain_err_stif.egress_data  & 0x01);
  ioctl_chain_err_stif.extif_event   = chain_err_stif.extif_event;
  ioctl_chain_err_stif.extif_command = chain_err_stif.extif_command;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_ERR_STIF_FORCE, &ioctl_chain_err_stif) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_ERR_STIF_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_get_err_stif_force(
        uint32_t dev_id,
        uint32_t lane,
        fpga_chain_err_stif_t *chain_err_stif
) {
  fpga_ioctl_chain_err_stif_t ioctl_chain_err_stif;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_err_stif || (lane >= KERNEL_NUM_CHAIN(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), chain_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)chain_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), chain_err_stif(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)chain_err_stif);

  ioctl_chain_err_stif.lane = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_ERR_STIF_FORCE, &ioctl_chain_err_stif) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_ERR_STIF_FORCE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_err_stif->ingress_req   = (ioctl_chain_err_stif.ingress_req  & 0x01);
  chain_err_stif->ingress_resp  = (ioctl_chain_err_stif.ingress_resp & 0x01);
  chain_err_stif->ingress_data  = (ioctl_chain_err_stif.ingress_data & 0x01);
  chain_err_stif->egress_req    = (ioctl_chain_err_stif.egress_req   & 0x01);
  chain_err_stif->egress_resp   = (ioctl_chain_err_stif.egress_resp  & 0x01);
  chain_err_stif->egress_data   = (ioctl_chain_err_stif.egress_data  & 0x01);
  chain_err_stif->extif_event   = ioctl_chain_err_stif.extif_event;
  chain_err_stif->extif_command = ioctl_chain_err_stif.extif_command;

  return 0;
}


int fpga_chain_err_cmdfault_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint16_t ins_enable,
        uint16_t cid
) {
  fpga_ioctl_chain_err_cmdfault_t ioctl_chain_err_cmdfault;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1) || (ins_enable > 1) || (cid > CID_MAX)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), ins_enable(%u), cid(%u))\n",
      __func__, dev_id, lane, extif_id, ins_enable, cid);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), ins_enable(%u), cid(%u))\n",
      __func__, dev_id, lane, extif_id, ins_enable, cid);

  ioctl_chain_err_cmdfault.lane     = (int)lane;  // NOLINT
  ioctl_chain_err_cmdfault.extif_id = extif_id;
  ioctl_chain_err_cmdfault.enable   = ins_enable;
  ioctl_chain_err_cmdfault.cid      = cid;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_ERR_CMDFAULT_INS, &ioctl_chain_err_cmdfault) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_ERR_CMDFAULT_INS(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_err_cmdfault_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint16_t *ins_enable,
        uint16_t *cid
) {
  fpga_ioctl_chain_err_cmdfault_t ioctl_chain_err_cmdfault;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !ins_enable || !cid || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), ins_enable(%#lx), cid(%#lx))\n",
      __func__, dev_id, lane, extif_id, (uintptr_t)ins_enable, (uintptr_t)cid);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), ins_enable(%#lx), cid(%#lx))\n",
      __func__, dev_id, lane, extif_id, (uintptr_t)ins_enable, (uintptr_t)cid);

  ioctl_chain_err_cmdfault.lane     = (int)lane;  // NOLINT
  ioctl_chain_err_cmdfault.extif_id = extif_id & 0xFF;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_ERR_CMDFAULT_GET_INS, &ioctl_chain_err_cmdfault) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_ERR_CMDFAULT_GET_INS(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *ins_enable = ioctl_chain_err_cmdfault.enable;
  *cid        = ioctl_chain_err_cmdfault.cid;

  return 0;
}
