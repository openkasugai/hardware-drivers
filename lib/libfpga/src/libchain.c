/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libchain.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>
#include <libfpga_internal/libfpgacommon_internal.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBCHAIN


int fpga_chain_start(
  uint32_t dev_id,
  uint32_t lane
) {
  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u))\n", __func__, dev_id, lane);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u))\n", __func__, dev_id, lane);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_START_MODULE, &lane) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_START_MODULE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  return 0;
}


int fpga_chain_stop(
  uint32_t dev_id,
  uint32_t lane
) {
  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u))\n", __func__, dev_id, lane);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u))\n", __func__, dev_id, lane);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_STOP_MODULE, &lane) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_STOP_MODULE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  return 0;
}


int fpga_chain_set_ddr(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t extif_id
) {
  fpga_ioctl_extif_t ioctl_extif;

  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u))\n", __func__, dev_id, lane, extif_id);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u)))\n", __func__, dev_id, lane, extif_id);

  ioctl_extif.lane     = (int)lane;  // NOLINT
  ioctl_extif.extif_id = (uint8_t)(extif_id & 0x000000FF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_SET_DDR_OFFSET_FRAME, &ioctl_extif) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_SET_DDR_OFFSET_FRAME(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }
  return 0;
}


int fpga_chain_get_ddr(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t extif_id,
  fpga_chain_ddr_t *chain_ddr
) {
  fpga_ioctl_chain_ddr_t ioctl_chain_ddr;

  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !chain_ddr || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), chain_ddr(%#lx))\n", __func__, dev_id, lane, extif_id, (uintptr_t)chain_ddr);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), chain_ddr(%#lx))\n", __func__, dev_id, lane, extif_id, (uintptr_t)chain_ddr);

  ioctl_chain_ddr.lane      = (int)lane;  // NOLINT
  ioctl_chain_ddr.extif_id  = (uint8_t)(extif_id & 0x000000FF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_DDR_OFFSET_FRAME, &ioctl_chain_ddr) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_DDR_OFFSET_FRAME(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  chain_ddr->base      = ioctl_chain_ddr.base;
  chain_ddr->rx_offset = ioctl_chain_ddr.rx_offset;
  chain_ddr->rx_stride = ioctl_chain_ddr.rx_stride;
  chain_ddr->tx_offset = ioctl_chain_ddr.tx_offset;
  chain_ddr->tx_stride = ioctl_chain_ddr.tx_stride;
  chain_ddr->rx_size   = ioctl_chain_ddr.rx_size;
  chain_ddr->tx_size   = ioctl_chain_ddr.tx_size;

  return 0;
}


/**
 * @brief Function which set data into fpga_id_t
 */
static void __fpga_set_id_info(
  fpga_id_t *id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t extif_id,
  uint32_t cid,
  uint8_t enable_flag,
  uint8_t active_flag,
  uint8_t direct_flag,
  uint8_t virtual_flag,
  uint8_t blocking_flag
) {
  id->lane = lane;
  id->extif_id = extif_id;
  id->cid = (uint16_t)cid;
  id->fchid = (uint16_t)fchid;
  id->enable_flag = enable_flag;
  id->active_flag = active_flag;
  id->direct_flag = direct_flag;
  id->virtual_flag = virtual_flag;
  id->blocking_flag = blocking_flag;
}


/**
 * @brief Function which establish function chain connection
 */
static int __fpga_chain_connect(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t extif_id,
  uint32_t cid,
  int dir,
  uint8_t active_flag,
  uint8_t direct_flag,
  uint8_t virtual_flag,
  uint8_t blocking_flag
) {
  fpga_id_t ioctl_id;
  int ret = 0;

  fpga_device_t *dev = fpga_get_device(dev_id);

  // Check input
  if (!dev) {
    goto invalid_arg;
  }
  if (!((extif_id == FPGA_EXTIF_NUMBER_0) || (extif_id == FPGA_EXTIF_NUMBER_1))) {
    goto invalid_arg;
  }
  if (cid < CID_MIN || cid > CID_MAX) {
    goto invalid_arg;
  }
  if (fchid < FUNCTION_CHAIN_ID_MIN || fchid > FUNCTION_CHAIN_ID_MAX) {
    goto invalid_arg;
  }
  if (lane >= KERNEL_NUM_CHAIN(dev)) {
    goto invalid_arg;
  }
  if (active_flag > 0x1) {
    goto invalid_arg;
  }
  if (direct_flag > 0x1) {
    goto invalid_arg;
  }
  if (virtual_flag > 0x1) {
    goto invalid_arg;
  }
  if (blocking_flag > 0x1) {
    goto invalid_arg;
  }

  // enable_flag is always 1
  __fpga_set_id_info(&ioctl_id, lane, fchid, extif_id, cid, 1, active_flag, direct_flag, virtual_flag, blocking_flag);

  // Set function chain
  if (fpgautil_ioctl(dev->fd, FUNCTION_CHAIN_TABLE_UPDATE_CMD(dir), &ioctl_id)) {
    int err = errno;
    if (err == XPCIE_DEV_UPDATE_TIMEOUT) {
      llf_err(TABLE_UPDATE_TIMEOUT, "Error happened: Timeout of table update. XPCIE_DEV_UPDATE(%s)\n",
        dir == FUNCTION_CHAIN_DIR_INGRESS ? "ingress" : "egress");
      ret = -TABLE_UPDATE_TIMEOUT;
      goto failed;
    } else {
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_UPDATE(errno:%d)\n", err);
      ret = -FAILURE_IOCTL;
      goto failed;
    }
  }

failed:

  return ret;

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), fchid(%u), extif_id(%u), cid(%u), dir(%d),"
    " active_flag(%d), direct_flag(%d), virtual_flag(%d), blocking_flag(%d))\n",
    __func__, dev_id, lane, fchid, extif_id, cid, dir,
    active_flag, direct_flag, virtual_flag, blocking_flag);
  return -INVALID_ARGUMENT;
}


int fpga_chain_connect(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t ingress_extif_id,
  uint32_t ingress_cid,
  uint32_t egress_extif_id,
  uint32_t egress_cid,
  uint8_t ingress_active_flag,
  uint8_t egress_active_flag,
  uint8_t direct_flag,
  uint8_t egress_virtual_flag,
  uint8_t egress_blocking_flag
) {
  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev)
    goto invalid_arg;
  if ((ingress_cid < CID_MIN) || (ingress_cid > CID_MAX)) {
    goto invalid_arg;
  }
  if ((egress_cid < CID_MIN) || (egress_cid > CID_MAX)) {
    goto invalid_arg;
  }
  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), ingress_extif_id(%u), ingress_cid(%u), egress_extif_id(%u), egress_cid(%u))\n",
    __func__, dev_id, lane, fchid, ingress_extif_id, ingress_cid, egress_extif_id, egress_cid);

  // Set chain table
  int ret = fpga_chain_connect_ingress(
    dev_id,
    lane,
    fchid,
    ingress_extif_id,
    ingress_cid,
    ingress_active_flag,
    direct_flag);
  if (ret)
    return ret;

  ret = fpga_chain_connect_egress(
    dev_id,
    lane,
    fchid,
    egress_extif_id,
    egress_cid,
    egress_active_flag,
    egress_virtual_flag,
    egress_blocking_flag);
  if (ret) {
    // Delete ingress chain table when egress chain failed to establish
    fpga_chain_disconnect_ingress(
      dev_id,
      lane,
      fchid);
    return ret;
  }

  return 0;

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), fchid(%u), ingress_extif_id(%u), ingress_cid(%u), egress_extif_id(%u), egress_cid(%u))\n",
    __func__, dev_id, lane, fchid, ingress_extif_id, ingress_cid, egress_extif_id, egress_cid);
  return -INVALID_ARGUMENT;
}


int fpga_chain_connect_ingress(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t ingress_extif_id,
  uint32_t ingress_cid,
  uint8_t active_flag,
  uint8_t direct_flag
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev)
    goto invalid_arg;

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), ingress_extif_id(%u), ingress_cid(%u), active_flag(%u), direct_flag(%u))\n",
    __func__, dev_id, lane, fchid, ingress_extif_id, ingress_cid, active_flag, direct_flag);

  int ret;

  ret = __fpga_chain_connect(
    dev_id,
    lane,
    fchid,
    ingress_extif_id,
    ingress_cid,
    FUNCTION_CHAIN_DIR_INGRESS,
    active_flag,
    direct_flag,
    0,
    0);

  return ret;

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), fchid(%u), ingress_extif_id(%u), ingress_cid(%u), active_flag(%u), direct_flag(%u))\n",
    __func__, dev_id, lane, fchid, ingress_extif_id, ingress_cid, active_flag, direct_flag);
  return -INVALID_ARGUMENT;
}


int fpga_chain_connect_egress(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t egress_extif_id,
  uint32_t egress_cid,
  uint8_t active_flag,
  uint8_t virtual_flag,
  uint8_t blocking_flag
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev)
    goto invalid_arg;

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), egress_extif_id(%u),"
          " egress_cid(%u), active_flag(%u), virtual_flag(%u), blocking_flag(%u))\n",
          __func__, dev_id, lane, fchid, egress_extif_id,
          egress_cid, active_flag, virtual_flag, blocking_flag);

  int ret;

  ret = __fpga_chain_connect(
    dev_id,
    lane,
    fchid,
    egress_extif_id,
    egress_cid,
    FUNCTION_CHAIN_DIR_EGRESS,
    active_flag,
    0,
    virtual_flag,
    blocking_flag);

  return ret;

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), fchid(%u), egress_extif_id(%u),"
          " egress_cid(%u), active_flag(%u), virtual_flag(%u), blocking_flag(%u))\n",
          __func__, dev_id, lane, fchid, egress_extif_id,
          egress_cid, active_flag, virtual_flag, blocking_flag);
  return -INVALID_ARGUMENT;
}


/**
 * @brief Function which delete function chain connection
 */
static int __fpga_chain_disconnect(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  int dir
) {
  fpga_id_t ioctl_id;
  fpga_device_t *dev = fpga_get_device(dev_id);

  // Check input
  if (!dev) {
    goto invalid_arg;
  }
  if (fchid < FUNCTION_CHAIN_ID_MIN || fchid > FUNCTION_CHAIN_ID_MAX) {
    goto invalid_arg;
  }
  if (lane >= KERNEL_NUM_CHAIN(dev)) {
    goto invalid_arg;
  }


  // Delete function chain
  __fpga_set_id_info(&ioctl_id, lane, fchid, 0, 0, 0, 0, 0, 0, 0);
  if (fpgautil_ioctl(dev->fd, FUNCTION_CHAIN_TABLE_DELETE_CMD(dir), &ioctl_id)) {
    int err = errno;
    if (err == XPCIE_DEV_UPDATE_TIMEOUT) {
      llf_err(TABLE_UPDATE_TIMEOUT, "Error happened: Timeout of table update. XPCIE_DEV_DELETE(%s)\n",
        dir == FUNCTION_CHAIN_DIR_INGRESS ? "ingress" : "egress");
      return -TABLE_UPDATE_TIMEOUT;
    } else if (err == XPCIE_DEV_NO_CHAIN_FOUND) {
      llf_err(FUNC_CHAIN_ID_MISMATCH, "Error happened: No chain found. XPCIE_DEV_DELETE(%s)\n",
        dir == FUNCTION_CHAIN_DIR_INGRESS ? "ingress" : "egress");
      return -FUNC_CHAIN_ID_MISMATCH;
    } else {
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_DELETE(%s,errno:%d)\n",
        dir == FUNCTION_CHAIN_DIR_INGRESS ? "ingress" : "egress", err);
      return -FAILURE_IOCTL;
    }
  }

  return 0;

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), fchid(%u))\n",
    __func__, dev_id, lane, fchid);
  return -INVALID_ARGUMENT;
}


int fpga_chain_disconnect(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev)
    goto invalid_arg;

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u))\n",
    __func__, dev_id, lane, fchid);

  int ret;

  ret = fpga_chain_disconnect_ingress(
    dev_id,
    lane,
    fchid);
  if (ret)
    return ret;
  ret = fpga_chain_disconnect_egress(
    dev_id,
    lane,
    fchid);
  if (ret)
    return ret;
  return 0;

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), fchid(%u))\n",
    __func__, dev_id, lane, fchid);
  return -INVALID_ARGUMENT;
}


int fpga_chain_disconnect_ingress(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev)
    goto invalid_arg;

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u))\n",
    __func__, dev_id, lane, fchid);

  int ret;

  ret = __fpga_chain_disconnect(
    dev_id,
    lane,
    fchid,
    FUNCTION_CHAIN_DIR_INGRESS);

  return ret;

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), fchid(%u))\n",
    __func__, dev_id, lane, fchid);
  return -INVALID_ARGUMENT;
}


int fpga_chain_disconnect_egress(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev)
    goto invalid_arg;

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u))\n",
    __func__, dev_id, lane, fchid);

  int ret;

  ret = __fpga_chain_disconnect(
    dev_id,
    lane,
    fchid,
    FUNCTION_CHAIN_DIR_EGRESS);

  return ret;

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), fchid(%u))\n",
    __func__, dev_id, lane, fchid);
  return -INVALID_ARGUMENT;
}


int fpga_chain_read_table_ingress(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t ingress_extif_id,
  uint32_t ingress_cid,
  uint8_t *enable_flag,
  uint8_t *active_flag,
  uint8_t *direct_flag,
  uint32_t *fchid
) {
  fpga_id_t ioctl_fpga_id;

  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !enable_flag || !active_flag || !direct_flag || !fchid
    || (lane >= KERNEL_NUM_CHAIN(dev))
    || (ingress_cid < CID_MIN || ingress_cid > CID_MAX)
    || (ingress_extif_id > FPGA_EXTIF_NUMBER_1)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), ingress_extif_id(%u), ingress_cid(%u),"
      " enable_flag(%#lx), active_flag(%#lx), direct_flag(%#lx), fchid(%#lx))\n",
      __func__, dev_id, lane, ingress_extif_id, ingress_cid,
      (uintptr_t)enable_flag, (uintptr_t)active_flag, (uintptr_t)direct_flag, (uintptr_t)fchid);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), ingress_extif_id(%u), ingress_cid(%u),"
    " enable_flag(%#lx), active_flag(%#lx), direct_flag(%#lx), fchid(%#lx))\n",
    __func__, dev_id, lane, ingress_extif_id, ingress_cid,
    (uintptr_t)enable_flag, (uintptr_t)active_flag, (uintptr_t)direct_flag, (uintptr_t)fchid);

  ioctl_fpga_id.lane      = (int)lane;  // NOLINT
  ioctl_fpga_id.extif_id  = ingress_extif_id;
  ioctl_fpga_id.cid       = (uint16_t)(ingress_cid & 0x0000FFFF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_READ_TABLE_INGR, &ioctl_fpga_id) < 0) {
    int err = errno;
    if (err == XPCIE_DEV_UPDATE_TIMEOUT) {
      llf_err(TABLE_UPDATE_TIMEOUT, "Error happened: Timeout of table update. XPCIE_DEV_CHAIN_READ_TABLE_INGR\n");
      return -TABLE_UPDATE_TIMEOUT;
    } else if (err == XPCIE_DEV_NO_CHAIN_FOUND) {
      llf_err(FUNC_CHAIN_ID_MISMATCH, "Error happened: No chain found. XPCIE_DEV_CHAIN_READ_TABLE_INGR\n");
      return -FUNC_CHAIN_ID_MISMATCH;
    } else {
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_READ_TABLE_INGR(errno:%d)\n", err);
      return -FAILURE_IOCTL;
    }
  }

  *enable_flag = ioctl_fpga_id.enable_flag;
  *active_flag = ioctl_fpga_id.active_flag;
  *direct_flag = ioctl_fpga_id.direct_flag;
  *fchid       = (uint32_t)(ioctl_fpga_id.fchid & 0x0000FFFF);

  return 0;
}


int fpga_chain_read_table_egress(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint8_t *enable_flag,
  uint8_t *active_flag,
  uint8_t *virtual_flag,
  uint8_t *blocking_flag,
  uint32_t *egress_extif_id,
  uint32_t *egress_cid
) {
  fpga_id_t ioctl_fpga_id;

  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !enable_flag || !active_flag || !virtual_flag || !blocking_flag || !egress_extif_id || !egress_cid
    || (lane >= KERNEL_NUM_CHAIN(dev)) || (fchid < FUNCTION_CHAIN_ID_MIN || fchid > FUNCTION_CHAIN_ID_MAX)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u), enable_flag(%#lx), active_flag(%#lx),"
      " virtual_flag(%#lx), blocking_flag(%#lx), egress_extif_id(%#lx), egress_cid(%#lx))\n",
      __func__, dev_id, lane, fchid, (uintptr_t)enable_flag, (uintptr_t)active_flag,
      (uintptr_t)virtual_flag, (uintptr_t)blocking_flag, (uintptr_t)egress_extif_id, (uintptr_t)egress_cid);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u), enable_flag(%#lx), active_flag(%#lx), "
    "virtual_flag(%#lx), blocking_flag(%#lx), egress_extif_id(%#lx), egress_cid(%#lx))\n",
    __func__, dev_id, lane, fchid, (uintptr_t)enable_flag, (uintptr_t)active_flag,
    (uintptr_t)virtual_flag, (uintptr_t)blocking_flag, (uintptr_t)egress_extif_id, (uintptr_t)egress_cid);

  ioctl_fpga_id.lane = (int)lane;  // NOLINT
  ioctl_fpga_id.fchid = (uint16_t)(fchid & 0x0000FFFF);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_READ_TABLE_EGR, &ioctl_fpga_id) < 0) {
    int err = errno;
    if (err == XPCIE_DEV_UPDATE_TIMEOUT) {
      llf_err(TABLE_UPDATE_TIMEOUT, "Error happened: Timeout of table update. XPCIE_DEV_CHAIN_READ_TABLE_EGR\n");
      return -TABLE_UPDATE_TIMEOUT;
    } else if (err == XPCIE_DEV_NO_CHAIN_FOUND) {
      llf_err(FUNC_CHAIN_ID_MISMATCH, "Error happened: No chain found. XPCIE_DEV_CHAIN_READ_TABLE_EGR\n");
      return -FUNC_CHAIN_ID_MISMATCH;
    } else {
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_READ_TABLE_EGR(errno:%d)\n", err);
      return -FAILURE_IOCTL;
    }
  }

  *enable_flag      = ioctl_fpga_id.enable_flag;
  *active_flag      = ioctl_fpga_id.active_flag;
  *virtual_flag     = ioctl_fpga_id.virtual_flag;
  *blocking_flag    = ioctl_fpga_id.blocking_flag;
  *egress_cid       = (uint32_t)(ioctl_fpga_id.cid & 0x0000FFFF);
  *egress_extif_id  = (uint32_t)(ioctl_fpga_id.extif_id & 0x0000FFFF);

  return 0;
}


int fpga_chain_read_soft_table(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fchid,
  uint32_t *ingress_extif_id,
  uint32_t *ingress_cid,
  uint32_t *egress_extif_id,
  uint32_t *egress_cid
) {
  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev
    || (lane >= KERNEL_NUM_CHAIN(dev))
    || !IS_VALID_FUNCTION_CHAIN_ID(fchid)
    || (ingress_extif_id && !ingress_cid)
    || (!ingress_extif_id && ingress_cid)
    || (egress_extif_id && !egress_cid)
    || (!egress_extif_id && egress_cid)
    || (!ingress_extif_id && !ingress_cid && !egress_extif_id && !egress_cid)
    ) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fchid(%u),"
      " ingress_extif_id(%#lx), ingress_cid(%#lx), egress_extif_id(%#lx), egress_cid(%#lx))\n",
      __func__, dev_id, lane, fchid, (uintptr_t)ingress_extif_id, (uintptr_t)ingress_cid,
      (uintptr_t)egress_extif_id, (uintptr_t)egress_cid);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), lane(%u), fchid(%u),"
    " ingress_extif_id(%#lx), ingress_cid(%#lx), egress_extif_id(%#lx), egress_cid(%#lx))\n",
    __func__, dev_id, lane, fchid, (uintptr_t)ingress_extif_id, (uintptr_t)ingress_cid,
    (uintptr_t)egress_extif_id, (uintptr_t)egress_cid);

  fpga_ioctl_chain_ids_t ioctl_chains_ids;
  memset(&ioctl_chains_ids, 0, sizeof(ioctl_chains_ids));
  ioctl_chains_ids.lane = lane;
  ioctl_chains_ids.fchid = fchid;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_READ_SOFT_TABLE, &ioctl_chains_ids)) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_READ_SOFT_TABLE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  if (ingress_extif_id) *ingress_extif_id = ioctl_chains_ids.ingress_extif_id;
  if (ingress_cid)      *ingress_cid      = ioctl_chains_ids.ingress_cid;
  if (egress_extif_id)  *egress_extif_id  = ioctl_chains_ids.egress_extif_id;
  if (egress_cid)       *egress_cid       = ioctl_chains_ids.egress_cid;

  return 0;
}


/**
 * @struct fpga_chain_wait_connection_struct
 * @brief Struct for __fpga_chain_wait_connection_clb
 */
struct fpga_chain_wait_connection_struct {
  uint32_t dev_id;          /**< Argument for fpga_chain_read_soft_table */
  uint32_t lane;            /**< Argument for fpga_chain_read_soft_table */
  uint32_t fchid;           /**< Argument for fpga_chain_read_soft_table */
  bool     is_ingress;      /**< Argument for Callback function */
  bool     is_established;  /**< Argument for Callback function */
};


/**
 * @brief Function wrap fpga_chain_read_soft_table() to enable __fpga_common_polling()
 * @retval 0 Success
 * @retval (>0) Error(continue polling)
 * @retval (<0) Error(stop polling)
 */
static int __fpga_chain_wait_connection_clb(
  void *arg
) {
  struct fpga_chain_wait_connection_struct *argument = (struct fpga_chain_wait_connection_struct*)arg;
  uint32_t ingress_extif_id = -1;
  uint32_t ingress_cid      = -1;
  uint32_t egress_extif_id  = -1;
  uint32_t egress_cid       = -1;

  int ret = fpga_chain_read_soft_table(
    argument->dev_id,
    argument->lane,
    argument->fchid,
    &ingress_extif_id,
    &ingress_cid,
    &egress_extif_id,
    &egress_cid);

  if (ret) {
    // Failed to execute fpga_chain_read_soft_table()
    return ret;
  } else {
    if (argument->is_ingress) {
      if (argument->is_established)
        return (ingress_extif_id != -1 && ingress_cid != -1) ? 0 : 1;
      else
        return (ingress_extif_id == -1 && ingress_cid == -1) ? 0 : 1;
    } else {
      if (argument->is_established)
        return (egress_extif_id != -1 && egress_cid != -1) ? 0 : 1;
      else
        return (egress_extif_id == -1 && egress_cid == -1) ? 0 : 1;
    }
  }
}


int fpga_chain_wait_connection_ingress(
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

  struct fpga_chain_wait_connection_struct clb_argument = {
    .dev_id = dev_id,
    .lane = lane,
    .fchid = fchid,
    .is_ingress = true,
    .is_established = true};

  int ret = __fpga_common_polling(
    timeout,
    interval,
    __fpga_chain_wait_connection_clb,
    (void*)&clb_argument);  // NOLINT

  if (ret < 0)
    return ret;

  if (!ret)
    *is_success = true;
  else
    *is_success = false;

  return 0;
}


int fpga_chain_wait_connection_egress(
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

  struct fpga_chain_wait_connection_struct clb_argument = {
    .dev_id = dev_id,
    .lane = lane,
    .fchid = fchid,
    .is_ingress = false,
    .is_established = true};

  int ret = __fpga_common_polling(
    timeout,
    interval,
    __fpga_chain_wait_connection_clb,
    (void*)&clb_argument);  // NOLINT

  if (ret < 0)
    return ret;

  if (!ret)
    *is_success = true;
  else
    *is_success = false;

  return 0;
}


int fpga_chain_wait_disconnection_ingress(
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

  struct fpga_chain_wait_connection_struct clb_argument = {
    .dev_id = dev_id,
    .lane = lane,
    .fchid = fchid,
    .is_ingress = true,
    .is_established = false};

  int ret = __fpga_common_polling(
    timeout,
    interval,
    __fpga_chain_wait_connection_clb,
    (void*)&clb_argument);  // NOLINT

  if (ret < 0)
    return ret;

  if (!ret)
    *is_success = true;
  else
    *is_success = false;

  return 0;
}


int fpga_chain_wait_disconnection_egress(
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

  struct fpga_chain_wait_connection_struct clb_argument = {
    .dev_id = dev_id,
    .lane = lane,
    .fchid = fchid,
    .is_ingress = false,
    .is_established = false};

  int ret = __fpga_common_polling(
    timeout,
    interval,
    __fpga_chain_wait_connection_clb,
    (void*)&clb_argument);  // NOLINT

  if (ret < 0)
    return ret;

  if (!ret)
    *is_success = true;
  else
    *is_success = false;

  return 0;
}


int fpga_chain_get_control(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *control
) {
  fpga_ioctl_chain_ctrl_t ioctl_chain_ctrl;

  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !control || (lane >= KERNEL_NUM_CHAIN(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), control(%#lx))\n", __func__, dev_id, lane, (uintptr_t)control);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), control(%#lx))\n", __func__, dev_id, lane, (uintptr_t)control);

  ioctl_chain_ctrl.lane  = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_MODULE, &ioctl_chain_ctrl) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_MODULE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *control = ioctl_chain_ctrl.value;

  return 0;
}


int fpga_chain_get_module_id(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *module_id
) {
  fpga_ioctl_chain_ctrl_t ioctl_chain_ctrl;

  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !module_id || (lane >= KERNEL_NUM_CHAIN(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), module_id(%#lx))\n", __func__, dev_id, lane, (uintptr_t)module_id);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), module_id(%#lx))\n", __func__, dev_id, lane, (uintptr_t)module_id);

  ioctl_chain_ctrl.lane  = (int)lane;  // NOLINT

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_MODULE_ID, &ioctl_chain_ctrl) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_MODULE_ID(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *module_id = ioctl_chain_ctrl.value;

  return 0;
}


int fpga_chain_get_con_status(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t extif_id,
        uint32_t cid,
        uint32_t *status
) {
  fpga_ioctl_chain_con_status_t ioctl_chain_con_status;

  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !status || (lane >= KERNEL_NUM_CHAIN(dev)) || (extif_id > FPGA_EXTIF_NUMBER_1) || (cid < CID_MIN || cid > CID_MAX)) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), extif_id(%u), cid(%u), status(%#lx))\n", __func__, dev_id, lane, extif_id, cid, (uintptr_t)status);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), extif_id(%u), cid(%u), status(%#lx))\n", __func__, dev_id, lane, extif_id, cid, (uintptr_t)status);

  ioctl_chain_con_status.lane     = (int)lane;  // NOLINT
  ioctl_chain_con_status.extif_id = extif_id;
  ioctl_chain_con_status.cid      = cid;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CHAIN_GET_CONNECTION, &ioctl_chain_con_status) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CHAIN_GET_CONNECTION(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *status = ioctl_chain_con_status.value;

  return 0;
}
