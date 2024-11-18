/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libchain.h>
#include <libchain_stat.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>
#include <libfpga_internal/libfpgacommon_internal.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>

// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBCHAIN


int fpga_chain_get_stat_latency_self(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t extif_id,
  uint8_t dir,
  uint32_t cid,
  uint32_t *latency
) {
  fpga_ioctl_chain_latency_t ioctl_chain_latency;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !latency || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1) || (dir > FPGA_CID_KIND_EGRESS)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), cid(%u), latency(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, cid, (uintptr_t)latency);
    return -INVALID_ARGUMENT;
  }
  if (cid < CID_MIN || cid > CID_MAX) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), cid(%u), latency(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, cid, (uintptr_t)latency);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), dir(%u), cid(%u), latency(%#lx))\n",
      __func__, dev_id, lane, extif_id, dir, cid, (uintptr_t)latency);

  ioctl_chain_latency.lane     = (int)lane;  // NOLINT
  ioctl_chain_latency.extif_id = extif_id;
  ioctl_chain_latency.dir      = dir;
  ioctl_chain_latency.cid      = (uint16_t)(cid & 0x0000FFFF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_LATENCY_CHAIN, &ioctl_chain_latency) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_LATENCY_CHAIN(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *latency = ioctl_chain_latency.latency;

  return 0;
}


int fpga_chain_get_stat_latency_func(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t *latency
) {
  fpga_ioctl_chain_func_latency_t ioctl_chain_func_latency;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !latency || (lane >= KERNEL_NUM_CHAIN(dev)) || (fchid < FUNCTION_CHAIN_ID_MIN || fchid > FUNCTION_CHAIN_ID_MAX)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), latency(%#lx))\n",
      __func__, dev_id, lane, fchid, (uintptr_t)latency);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), latency(%#lx))\n",
      __func__, dev_id, lane, fchid, (uintptr_t)latency);

  ioctl_chain_func_latency.lane = (int)lane;  // NOLINT
  ioctl_chain_func_latency.fchid = (uint16_t)(fchid & 0x0000FFFF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_LATENCY_FUNC, &ioctl_chain_func_latency) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_LATENCY_FUNC(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *latency = ioctl_chain_func_latency.latency;

  return 0;
}


int fpga_chain_get_stat_bytes(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t cid_fchid,
  uint32_t reg_id,
  uint64_t *byte_num
) {
  fpga_ioctl_chain_bytenum_t ioctl_chain_bytenum;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !byte_num || (lane >= KERNEL_NUM_CHAIN(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), cid_fchid(%u), reg_id(%u), byte_num(%#lx))\n",
      __func__, dev_id, lane, cid_fchid, reg_id, (uintptr_t)byte_num);
    return -INVALID_ARGUMENT;
  }
  if (reg_id == CHAIN_STAT_INGR_RCV0 || reg_id == CHAIN_STAT_INGR_RCV1 || reg_id == CHAIN_STAT_EGR_SND0 || reg_id == CHAIN_STAT_EGR_SND1) {
    /* cid */
    if (cid_fchid < CID_MIN || cid_fchid > CID_MAX) {
      llf_err(INVALID_ARGUMENT,
        "%s(dev_id(%u), lane(%u), cid_fchid(%u), reg_id(%u), byte_num(%#lx))\n",
        __func__, dev_id, lane, cid_fchid, reg_id, (uintptr_t)byte_num);
      return -INVALID_ARGUMENT;
    }
  } else if (reg_id == CHAIN_STAT_INGR_SND0 || reg_id == CHAIN_STAT_INGR_SND1 || reg_id == CHAIN_STAT_EGR_RCV0 || reg_id == CHAIN_STAT_EGR_RCV1) {
    /* fchid */
    if (cid_fchid < FUNCTION_CHAIN_ID_MIN || cid_fchid > FUNCTION_CHAIN_ID_MAX) {
      llf_err(INVALID_ARGUMENT,
        "%s(dev_id(%u), lane(%u), cid_fchid(%u), reg_id(%u), byte_num(%#lx))\n",
        __func__, dev_id, lane, cid_fchid, reg_id, (uintptr_t)byte_num);
      return -INVALID_ARGUMENT;
    }
  } else {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), cid_fchid(%u), reg_id(%u), byte_num(%#lx))\n",
      __func__, dev_id, lane, cid_fchid, reg_id, (uintptr_t)byte_num);
    return -INVALID_ARGUMENT;
  }


  llf_dbg("%s(dev_id(%u), lane(%u), cid_fchid(%u), reg_id(%u), byte_num(%#lx))\n",
      __func__, dev_id, lane, cid_fchid, reg_id, (uintptr_t)byte_num);

  ioctl_chain_bytenum.lane      = (int)lane;  // NOLINT
  ioctl_chain_bytenum.cid_fchid = (uint16_t)(cid_fchid & 0x0000FFFF);
  ioctl_chain_bytenum.reg_id    = (uint16_t)(reg_id & 0x0000FFFF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_CHAIN_BYTES, &ioctl_chain_bytenum) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_CHAIN_BYTES(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *byte_num = ioctl_chain_bytenum.byte_num;

  return 0;
}


int fpga_chain_get_stat_frames(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t reg_id,
  uint32_t *frame_num
) {
  fpga_ioctl_chain_framenum_t ioctl_chain_framenum;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !frame_num || (lane >= KERNEL_NUM_CHAIN(dev)) || (fchid < FUNCTION_CHAIN_ID_MIN || fchid > FUNCTION_CHAIN_ID_MAX) || (reg_id < CHAIN_STAT_INGR_SND0 || reg_id > CHAIN_STAT_EGR_RCV1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), frame_num(%#lx))\n",
      __func__, dev_id, lane, fchid, reg_id, (uintptr_t)frame_num);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), frame_num(%#lx))\n",
      __func__, dev_id, lane, fchid, reg_id, (uintptr_t)frame_num);

  ioctl_chain_framenum.lane   = (int)lane;  // NOLINT
  ioctl_chain_framenum.fchid  = (uint16_t)(fchid & 0x0000FFFF);
  ioctl_chain_framenum.reg_id = (uint16_t)(reg_id & 0x0000FFFF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_CHAIN_FRAMES, &ioctl_chain_framenum) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_CHAIN_FRAMES(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *frame_num = ioctl_chain_framenum.frame_num;

  return 0;
}


int fpga_chain_get_stat_discard_bytes(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t reg_id,
  uint64_t *byte_num
) {
  fpga_ioctl_chain_bytenum_t ioctl_chain_bytenum;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !byte_num || (lane >= KERNEL_NUM_CHAIN(dev)) || (fchid < FUNCTION_CHAIN_ID_MIN || fchid > FUNCTION_CHAIN_ID_MAX) || (reg_id < CHAIN_STAT_INGR_DISCARD0 || reg_id > CHAIN_STAT_EGR_DISCARD1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), byte_num(%#lx))\n",
      __func__, dev_id, lane, fchid, reg_id, (uintptr_t)byte_num);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), reg_id(%u), byte_num(%#lx))\n",
      __func__, dev_id, lane, fchid, reg_id, (uintptr_t)byte_num);

  ioctl_chain_bytenum.lane       = (int)lane;  // NOLINT
  ioctl_chain_bytenum.cid_fchid  = (uint16_t)(fchid & 0x0000FFFF);
  ioctl_chain_bytenum.reg_id     = (uint16_t)(reg_id & 0x0000FFFF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_CHAIN_BYTES, &ioctl_chain_bytenum) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_CHAIN_BYTES(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *byte_num = ioctl_chain_bytenum.byte_num;

  return 0;
}


int fpga_chain_get_stat_buff(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t *buff_num
) {
  fpga_ioctl_chain_framenum_t ioctl_chain_framenum;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !buff_num || (lane >= KERNEL_NUM_CHAIN(dev)) || (fchid < FUNCTION_CHAIN_ID_MIN || fchid > FUNCTION_CHAIN_ID_MAX)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), buff_num(%#lx))\n",
      __func__, dev_id, lane, fchid, (uintptr_t)buff_num);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), buff_num(%#lx))\n",
      __func__, dev_id, lane, fchid, (uintptr_t)buff_num);

  ioctl_chain_framenum.lane   = (int)lane;  // NOLINT
  ioctl_chain_framenum.fchid  = (uint16_t)(fchid & 0x0000FFFF);
  ioctl_chain_framenum.reg_id = 0;  // not used

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_CHAIN_BUFF, &ioctl_chain_framenum) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_CHAIN_BUFF(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *buff_num = ioctl_chain_framenum.frame_num;

  return 0;
}


int fpga_chain_get_stat_bp(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t *bp
) {
  fpga_ioctl_chain_framenum_t ioctl_chain_framenum;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !bp || (lane >= KERNEL_NUM_CHAIN(dev)) || (fchid < FUNCTION_CHAIN_ID_MIN || fchid > FUNCTION_CHAIN_ID_MAX)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), bp(%#lx))\n",
      __func__, dev_id, lane, fchid, (uintptr_t)bp);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), bp(%#lx))\n",
      __func__, dev_id, lane, fchid, (uintptr_t)bp);

  ioctl_chain_framenum.lane   = (int)lane;  // NOLINT
  ioctl_chain_framenum.fchid  = (uint16_t)(fchid & 0x0000FFFF);
  ioctl_chain_framenum.reg_id = 0;  // not used

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_CHAIN_BP, &ioctl_chain_framenum) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_CHAIN_BP(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *bp = ioctl_chain_framenum.frame_num;

  return 0;
}


int fpga_chain_set_stat_bp_clear(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t bp
) {
  fpga_ioctl_chain_framenum_t ioctl_chain_framenum;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (fchid < FUNCTION_CHAIN_ID_MIN || fchid > FUNCTION_CHAIN_ID_MAX)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u)\n",
      __func__, dev_id, lane, fchid);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), bp(%#lx))\n",
      __func__, dev_id, lane, fchid, bp);

  ioctl_chain_framenum.lane      = (int)lane;  // NOLINT
  ioctl_chain_framenum.fchid     = (uint16_t)(fchid & 0x0000FFFF);
  ioctl_chain_framenum.reg_id    = 0;  // not used
  ioctl_chain_framenum.frame_num = bp;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_CHAIN_BP_CLR, &ioctl_chain_framenum) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_CHAIN_BP_CLR(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_chain_get_stat_egr_busy(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t *busy
) {
  fpga_ioctl_chain_framenum_t ioctl_chain_framenum;

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !busy || (lane >= KERNEL_NUM_CHAIN(dev)) || (fchid < FUNCTION_CHAIN_ID_MIN || fchid > FUNCTION_CHAIN_ID_MAX)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), busy(%#lx))\n",
      __func__, dev_id, lane, fchid, (uintptr_t)busy);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), busy(%#lx))\n",
      __func__, dev_id, lane, fchid, (uintptr_t)busy);

  ioctl_chain_framenum.lane   = (int)lane;  // NOLINT
  ioctl_chain_framenum.fchid  = (uint16_t)(fchid & 0x0000FFFF);
  ioctl_chain_framenum.reg_id = 0;  // not used

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_EGR_BUSY, &ioctl_chain_framenum) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_EGR_BUSY(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *busy = ioctl_chain_framenum.frame_num;

  return 0;
}


/**
 * @struct fpga_chain_wait_stat_egr_free_struct
 * @brief Struct for __fpga_chain_wait_stat_egr_free_clb
 */
struct fpga_chain_wait_stat_egr_free_struct {
  uint32_t dev_id;  /**< Argument for fpga_chain_get_stat_egr_busy() */
  uint32_t lane;    /**< Argument for fpga_chain_get_stat_egr_busy() */
  uint32_t fchid;   /**< Argument for fpga_chain_get_stat_egr_busy() */
};


/**
 * @brief Function wrap fpga_chain_get_stat_egr_busy() to enable __fpga_common_polling()
 * @retval 0 Success
 * @retval (>0) Error(continue polling)
 * @retval (<0) Error(stop polling)
 */
static int __fpga_chain_wait_stat_egr_free_clb(
  void *arg
) {
  struct fpga_chain_wait_stat_egr_free_struct *argument = (struct fpga_chain_wait_stat_egr_free_struct*)arg;
  uint32_t busy;

  int ret = fpga_chain_get_stat_egr_busy(
    argument->dev_id,
    argument->lane,
    argument->fchid,
    &busy);

  if (ret)
    return ret;
  else
    return busy ? 1 : 0;
}


int fpga_chain_wait_stat_egr_free(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  const struct timeval *timeout,
  const struct timeval *interval,
  uint32_t *is_success
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !is_success || (lane >= KERNEL_NUM_CHAIN(dev)) || !IS_VALID_FUNCTION_CHAIN_ID(fchid)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), timeout(%#lx), interval(%#lx), is_success(%#lx))\n",
    __func__, dev_id, lane, fchid, (uintptr_t)timeout, (uintptr_t)interval, (uintptr_t)is_success);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), timeout(%#lx), interval(%#lx), is_success(%#lx))\n",
    __func__, dev_id, lane, fchid, (uintptr_t)timeout, (uintptr_t)interval, (uintptr_t)is_success);

  struct fpga_chain_wait_stat_egr_free_struct clb_argument = {
    .dev_id = dev_id,
    .lane = lane,
    .fchid = fchid};

  int ret = __fpga_common_polling(
    timeout,
    interval,
    __fpga_chain_wait_stat_egr_free_clb,
    (void*)&clb_argument);  // NOLINT

  if (ret < 0)
    return ret;

  if (!ret)
    *is_success = true;
  else
    *is_success = false;

  return 0;
}
