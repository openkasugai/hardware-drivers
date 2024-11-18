/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfunction_filter_resize_err.h>
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


int fpga_filter_resize_get_check_err(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *err_det
) {
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !err_det || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), err_det(%#lx))\n", __func__, dev_id, lane, (uintptr_t)err_det);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), err_det(%#lx))\n", __func__, dev_id, lane, (uintptr_t)err_det);

  if (pread(dev->fd, err_det, len, XPCIE_FPGA_FRFUNC_DETECT_FAULT(lane)) != len) goto failed;

  return 0;

failed:
  llf_err(FAILURE_READ, "Failed to get parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_READ;
}


int fpga_filter_resize_get_err_prot(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fr_id,
  uint32_t dir,
  fpga_func_err_prot_t *func_err_prot
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !func_err_prot || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, (uintptr_t)func_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, (uintptr_t)func_err_prot);

  if (dir == FRFUNC_DIR_INGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_0(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_1(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else if (dir == FRFUNC_DIR_EGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_0(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_1(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else {
    // Do Nothing
    llf_err(INVALID_ARGUMENT, "dir(%u) is not the expected value.\n", dir);
    return -INVALID_ARGUMENT;
  }

  func_err_prot->prot_ch               = (uint8_t)(value  & 0x00000001);
  func_err_prot->prot_len              = (uint8_t)((value & 0x00000002) >> 1);
  func_err_prot->prot_sof              = (uint8_t)((value & 0x00000004) >> 2);
  func_err_prot->prot_eof              = (uint8_t)((value & 0x00000008) >> 3);
  func_err_prot->prot_reqresp          = (uint8_t)((value & 0x00000010) >> 4);
  func_err_prot->prot_datanum          = (uint8_t)((value & 0x00000020) >> 5);
  func_err_prot->prot_req_outstanding  = (uint8_t)((value & 0x00000040) >> 6);
  func_err_prot->prot_resp_outstanding = (uint8_t)((value & 0x00000080) >> 7);
  func_err_prot->prot_max_datanum      = (uint8_t)((value & 0x00000100) >> 8);
  func_err_prot->prot_reqlen           = (uint8_t)((value & 0x00001000) >> 12);
  func_err_prot->prot_reqresplen       = (uint8_t)((value & 0x00002000) >> 13);

  return 0;

failed:
  llf_err(FAILURE_READ, "Failed to get parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_READ;
}


int fpga_filter_resize_set_err_prot_clear(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fr_id,
  uint32_t dir,
  fpga_func_err_prot_t func_err_prot
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, func_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, func_err_prot);

  value =  ((uint32_t)func_err_prot.prot_ch               & 0x01);
  value |= ((uint32_t)func_err_prot.prot_len              & 0x01) << 1;
  value |= ((uint32_t)func_err_prot.prot_sof              & 0x01) << 2;
  value |= ((uint32_t)func_err_prot.prot_eof              & 0x01) << 3;
  value |= ((uint32_t)func_err_prot.prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)func_err_prot.prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)func_err_prot.prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)func_err_prot.prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)func_err_prot.prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)func_err_prot.prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)func_err_prot.prot_reqresplen       & 0x01) << 13;

  if (dir == FRFUNC_DIR_INGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_0(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_1(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else if (dir == FRFUNC_DIR_EGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_0(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_1(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else {
    // Do Nothing
    llf_err(INVALID_ARGUMENT, "dir(%u) is not the expected value.\n", dir);
    return -INVALID_ARGUMENT;
  }

  return 0;

failed:
  llf_err(FAILURE_WRITE, "Failed to set parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_WRITE, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_WRITE;
}


int fpga_filter_resize_set_err_prot_mask(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fr_id,
  uint32_t dir,
  fpga_func_err_prot_t func_err_prot
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, func_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, func_err_prot);

  value =  ((uint32_t)func_err_prot.prot_ch               & 0x01);
  value |= ((uint32_t)func_err_prot.prot_len              & 0x01) << 1;
  value |= ((uint32_t)func_err_prot.prot_sof              & 0x01) << 2;
  value |= ((uint32_t)func_err_prot.prot_eof              & 0x01) << 3;
  value |= ((uint32_t)func_err_prot.prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)func_err_prot.prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)func_err_prot.prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)func_err_prot.prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)func_err_prot.prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)func_err_prot.prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)func_err_prot.prot_reqresplen       & 0x01) << 13;

  if (dir == FRFUNC_DIR_INGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_0_MASK(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_1_MASK(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else if (dir == FRFUNC_DIR_EGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_0_MASK(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_1_MASK(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }  } else {
    // Do Nothing
    llf_err(INVALID_ARGUMENT, "dir(%u) is not the expected value.\n", dir);
    return -INVALID_ARGUMENT;
  }

  return 0;

failed:
  llf_err(FAILURE_WRITE, "Failed to set parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_WRITE, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_WRITE;
}


int fpga_filter_resize_get_err_prot_mask(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fr_id,
  uint32_t dir,
  fpga_func_err_prot_t *func_err_prot
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !func_err_prot || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, (uintptr_t)func_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, (uintptr_t)func_err_prot);

  if (dir == FRFUNC_DIR_INGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_0_MASK(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_1_MASK(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else if (dir == FRFUNC_DIR_EGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_0_MASK(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_1_MASK(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else {
    // Do Nothing
    llf_err(INVALID_ARGUMENT, "dir(%u) is not the expected value.\n", dir);
    return -INVALID_ARGUMENT;
  }

  func_err_prot->prot_ch               = (uint8_t)(value  & 0x00000001);
  func_err_prot->prot_len              = (uint8_t)((value & 0x00000002) >> 1);
  func_err_prot->prot_sof              = (uint8_t)((value & 0x00000004) >> 2);
  func_err_prot->prot_eof              = (uint8_t)((value & 0x00000008) >> 3);
  func_err_prot->prot_reqresp          = (uint8_t)((value & 0x00000010) >> 4);
  func_err_prot->prot_datanum          = (uint8_t)((value & 0x00000020) >> 5);
  func_err_prot->prot_req_outstanding  = (uint8_t)((value & 0x00000040) >> 6);
  func_err_prot->prot_resp_outstanding = (uint8_t)((value & 0x00000080) >> 7);
  func_err_prot->prot_max_datanum      = (uint8_t)((value & 0x00000100) >> 8);
  func_err_prot->prot_reqlen           = (uint8_t)((value & 0x00001000) >> 12);
  func_err_prot->prot_reqresplen       = (uint8_t)((value & 0x00002000) >> 13);

  return 0;

failed:
  llf_err(FAILURE_READ, "Failed to get parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_READ;
}


int  fpga_filter_resize_set_err_prot_force(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fr_id,
  uint32_t dir,
  fpga_func_err_prot_t func_err_prot
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n",
      __func__, dev_id, lane, fr_id, dir, func_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n",
    __func__, dev_id, lane, fr_id, dir, func_err_prot);

  value =  ((uint32_t)func_err_prot.prot_ch               & 0x01);
  value |= ((uint32_t)func_err_prot.prot_len              & 0x01) << 1;
  value |= ((uint32_t)func_err_prot.prot_sof              & 0x01) << 2;
  value |= ((uint32_t)func_err_prot.prot_eof              & 0x01) << 3;
  value |= ((uint32_t)func_err_prot.prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)func_err_prot.prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)func_err_prot.prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)func_err_prot.prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)func_err_prot.prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)func_err_prot.prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)func_err_prot.prot_reqresplen       & 0x01) << 13;

  if (dir == FRFUNC_DIR_INGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_0_FORCE(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_1_FORCE(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else if (dir == FRFUNC_DIR_EGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_0_FORCE(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_1_FORCE(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }  } else {
    // Do Nothing
    llf_err(INVALID_ARGUMENT, "dir(%u) is not the expected value.\n", dir);
    return -INVALID_ARGUMENT;
  }

  return 0;

failed:
  llf_err(FAILURE_WRITE, "Failed to set parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_WRITE, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_WRITE;
}


int fpga_filter_resize_get_err_prot_force(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fr_id,
  uint32_t dir,
  fpga_func_err_prot_t *func_err_prot
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !func_err_prot || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, (uintptr_t)func_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, (uintptr_t)func_err_prot);

  if (dir == FRFUNC_DIR_INGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_0_FORCE(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_PROTOCOL_FAULT_1_FORCE(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else if (dir == FRFUNC_DIR_EGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_0_FORCE(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_PROTOCOL_FAULT_1_FORCE(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else {
    // Do Nothing
    llf_err(INVALID_ARGUMENT, "dir(%u) is not the expected value.\n", dir);
    return -INVALID_ARGUMENT;
  }

  func_err_prot->prot_ch               = (uint8_t)(value  & 0x00000001);
  func_err_prot->prot_len              = (uint8_t)((value & 0x00000002) >> 1);
  func_err_prot->prot_sof              = (uint8_t)((value & 0x00000004) >> 2);
  func_err_prot->prot_eof              = (uint8_t)((value & 0x00000008) >> 3);
  func_err_prot->prot_reqresp          = (uint8_t)((value & 0x00000010) >> 4);
  func_err_prot->prot_datanum          = (uint8_t)((value & 0x00000020) >> 5);
  func_err_prot->prot_req_outstanding  = (uint8_t)((value & 0x00000040) >> 6);
  func_err_prot->prot_resp_outstanding = (uint8_t)((value & 0x00000080) >> 7);
  func_err_prot->prot_max_datanum      = (uint8_t)((value & 0x00000100) >> 8);
  func_err_prot->prot_reqlen           = (uint8_t)((value & 0x00001000) >> 12);
  func_err_prot->prot_reqresplen       = (uint8_t)((value & 0x00002000) >> 13);

  return 0;

failed:
  llf_err(FAILURE_READ, "Failed to get parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_READ;
}


int fpga_filter_resize_get_err_stif(
  uint32_t dev_id,
  uint32_t lane,
  fpga_fr_err_stif_t *fr_err_stif
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !fr_err_stif || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_err_stif(%#lx))\n", __func__, dev_id, lane, (uintptr_t)fr_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_err_stif(%#lx))\n", __func__, dev_id, lane, (uintptr_t)fr_err_stif);

  if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_STREAMIF_STALL(lane)) != len) goto failed;

  fr_err_stif->ingress0_rcv_req  = (uint8_t)(value & 0x00000001);
  fr_err_stif->ingress0_rcv_resp = (uint8_t)((value & 0x00000002) >> 1);
  fr_err_stif->ingress0_rcv_data = (uint8_t)((value & 0x00000004) >> 2);
  fr_err_stif->ingress1_rcv_req  = (uint8_t)((value & 0x00000008) >> 3);
  fr_err_stif->ingress1_rcv_resp = (uint8_t)((value & 0x00000010) >> 4);
  fr_err_stif->ingress1_rcv_data = (uint8_t)((value & 0x00000020) >> 5);
  fr_err_stif->egress0_snd_req   = (uint8_t)((value & 0x00000040) >> 6);
  fr_err_stif->egress0_snd_resp  = (uint8_t)((value & 0x00000080) >> 7);
  fr_err_stif->egress0_snd_data  = (uint8_t)((value & 0x00000100) >> 8);
  fr_err_stif->egress1_snd_req   = (uint8_t)((value & 0x00000200) >> 9);
  fr_err_stif->egress1_snd_resp  = (uint8_t)((value & 0x00000400) >> 10);
  fr_err_stif->egress1_snd_data  = (uint8_t)((value & 0x00000800) >> 11);

  return 0;

failed:
  llf_err(FAILURE_READ, "Failed to get parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_READ;
}


int fpga_filter_resize_set_err_stif_mask(
  uint32_t dev_id,
  uint32_t lane,
  fpga_fr_err_stif_t fr_err_stif
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_err_stif(%#lx))\n", __func__, dev_id, lane, fr_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_err_stif(%#lx))\n", __func__, dev_id, lane, fr_err_stif);

  value =  ((uint32_t)fr_err_stif.ingress0_rcv_req  & 0x01);
  value |= ((uint32_t)fr_err_stif.ingress0_rcv_resp & 0x01) << 1;
  value |= ((uint32_t)fr_err_stif.ingress0_rcv_data & 0x01) << 2;
  value |= ((uint32_t)fr_err_stif.ingress1_rcv_req  & 0x01) << 3;
  value |= ((uint32_t)fr_err_stif.ingress1_rcv_resp & 0x01) << 4;
  value |= ((uint32_t)fr_err_stif.ingress1_rcv_data & 0x01) << 5;
  value |= ((uint32_t)fr_err_stif.egress0_snd_req   & 0x01) << 6;
  value |= ((uint32_t)fr_err_stif.egress0_snd_resp  & 0x01) << 7;
  value |= ((uint32_t)fr_err_stif.egress0_snd_data  & 0x01) << 8;
  value |= ((uint32_t)fr_err_stif.egress1_snd_req   & 0x01) << 9;
  value |= ((uint32_t)fr_err_stif.egress1_snd_resp  & 0x01) << 10;
  value |= ((uint32_t)fr_err_stif.egress1_snd_data  & 0x01) << 11;

  if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_STREAMIF_STALL_MASK(lane)) != len) goto failed;

  return 0;

failed:
  llf_err(FAILURE_WRITE, "Failed to set parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_WRITE, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_WRITE;
}


int fpga_filter_resize_get_err_stif_mask(
  uint32_t dev_id,
  uint32_t lane,
  fpga_fr_err_stif_t *fr_err_stif
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !fr_err_stif || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_err_stif(%#lx))\n", __func__, dev_id, lane, (uintptr_t)fr_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_err_stif(%#lx))\n", __func__, dev_id, lane, (uintptr_t)fr_err_stif);

  if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_STREAMIF_STALL_MASK(lane)) != len) goto failed;

  fr_err_stif->ingress0_rcv_req  = (uint8_t)(value & 0x00000001);
  fr_err_stif->ingress0_rcv_resp = (uint8_t)((value & 0x00000002) >> 1);
  fr_err_stif->ingress0_rcv_data = (uint8_t)((value & 0x00000004) >> 2);
  fr_err_stif->ingress1_rcv_req  = (uint8_t)((value & 0x00000008) >> 3);
  fr_err_stif->ingress1_rcv_resp = (uint8_t)((value & 0x00000010) >> 4);
  fr_err_stif->ingress1_rcv_data = (uint8_t)((value & 0x00000020) >> 5);
  fr_err_stif->egress0_snd_req   = (uint8_t)((value & 0x00000040) >> 6);
  fr_err_stif->egress0_snd_resp  = (uint8_t)((value & 0x00000080) >> 7);
  fr_err_stif->egress0_snd_data  = (uint8_t)((value & 0x00000100) >> 8);
  fr_err_stif->egress1_snd_req   = (uint8_t)((value & 0x00000200) >> 9);
  fr_err_stif->egress1_snd_resp  = (uint8_t)((value & 0x00000400) >> 10);
  fr_err_stif->egress1_snd_data  = (uint8_t)((value & 0x00000800) >> 11);

  return 0;

failed:
  llf_err(FAILURE_READ, "Failed to get parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_READ;
}


int fpga_filter_resize_set_err_stif_force(
  uint32_t dev_id,
  uint32_t lane,
  fpga_fr_err_stif_t fr_err_stif
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_err_stif(%#lx))\n", __func__, dev_id, lane, fr_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_err_stif(%#lx))\n", __func__, dev_id, lane, fr_err_stif);

  value =  ((uint32_t)fr_err_stif.ingress0_rcv_req  & 0x01);
  value |= ((uint32_t)fr_err_stif.ingress0_rcv_resp & 0x01) << 1;
  value |= ((uint32_t)fr_err_stif.ingress0_rcv_data & 0x01) << 2;
  value |= ((uint32_t)fr_err_stif.ingress1_rcv_req  & 0x01) << 3;
  value |= ((uint32_t)fr_err_stif.ingress1_rcv_resp & 0x01) << 4;
  value |= ((uint32_t)fr_err_stif.ingress1_rcv_data & 0x01) << 5;
  value |= ((uint32_t)fr_err_stif.egress0_snd_req   & 0x01) << 6;
  value |= ((uint32_t)fr_err_stif.egress0_snd_resp  & 0x01) << 7;
  value |= ((uint32_t)fr_err_stif.egress0_snd_data  & 0x01) << 8;
  value |= ((uint32_t)fr_err_stif.egress1_snd_req   & 0x01) << 9;
  value |= ((uint32_t)fr_err_stif.egress1_snd_resp  & 0x01) << 10;
  value |= ((uint32_t)fr_err_stif.egress1_snd_data  & 0x01) << 11;

  if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_STREAMIF_STALL_FORCE(lane)) != len) goto failed;

  return 0;

failed:
  llf_err(FAILURE_WRITE, "Failed to set parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_WRITE, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_WRITE;
}


int fpga_filter_resize_get_err_stif_force(
  uint32_t dev_id,
  uint32_t lane,
  fpga_fr_err_stif_t *fr_err_stif
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !fr_err_stif || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_err_stif(%#lx))\n", __func__, dev_id, lane, (uintptr_t)fr_err_stif);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_err_stif(%#lx))\n", __func__, dev_id, lane, (uintptr_t)fr_err_stif);

  if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_STREAMIF_STALL_FORCE(lane)) != len) goto failed;

  fr_err_stif->ingress0_rcv_req  = (uint8_t)(value & 0x00000001);
  fr_err_stif->ingress0_rcv_resp = (uint8_t)((value & 0x00000002) >> 1);
  fr_err_stif->ingress0_rcv_data = (uint8_t)((value & 0x00000004) >> 2);
  fr_err_stif->ingress1_rcv_req  = (uint8_t)((value & 0x00000008) >> 3);
  fr_err_stif->ingress1_rcv_resp = (uint8_t)((value & 0x00000010) >> 4);
  fr_err_stif->ingress1_rcv_data = (uint8_t)((value & 0x00000020) >> 5);
  fr_err_stif->egress0_snd_req   = (uint8_t)((value & 0x00000040) >> 6);
  fr_err_stif->egress0_snd_resp  = (uint8_t)((value & 0x00000080) >> 7);
  fr_err_stif->egress0_snd_data  = (uint8_t)((value & 0x00000100) >> 8);
  fr_err_stif->egress1_snd_req   = (uint8_t)((value & 0x00000200) >> 9);
  fr_err_stif->egress1_snd_resp  = (uint8_t)((value & 0x00000400) >> 10);
  fr_err_stif->egress1_snd_data  = (uint8_t)((value & 0x00000800) >> 11);

  return 0;

failed:
  llf_err(FAILURE_READ, "Failed to get parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_READ;
}


int fpga_filter_resize_err_prot_ins(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fr_id,
  uint32_t dir,
  fpga_func_err_prot_t func_err_prot
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n",
      __func__, dev_id, lane, fr_id, dir, func_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n",
    __func__, dev_id, lane, fr_id, dir, func_err_prot);

  value =  ((uint32_t)func_err_prot.prot_ch               & 0x01);
  value |= ((uint32_t)func_err_prot.prot_len              & 0x01) << 1;
  value |= ((uint32_t)func_err_prot.prot_sof              & 0x01) << 2;
  value |= ((uint32_t)func_err_prot.prot_eof              & 0x01) << 3;
  value |= ((uint32_t)func_err_prot.prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)func_err_prot.prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)func_err_prot.prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)func_err_prot.prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)func_err_prot.prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)func_err_prot.prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)func_err_prot.prot_reqresplen       & 0x01) << 13;

  if (dir == FRFUNC_DIR_INGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_INSERT_PROTOCOL_FAULT_0(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_INSERT_PROTOCOL_FAULT_1(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else if (dir == FRFUNC_DIR_EGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_INSERT_PROTOCOL_FAULT_0(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_INSERT_PROTOCOL_FAULT_1(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }  } else {
    // Do Nothing
    llf_err(INVALID_ARGUMENT, "dir(%u) is not the expected value.\n", dir);
    return -INVALID_ARGUMENT;
  }

  return 0;

failed:
  llf_err(FAILURE_WRITE, "Failed to set parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_WRITE, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_WRITE;
}


int fpga_filter_resize_err_prot_get_ins(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t fr_id,
  uint32_t dir,
  fpga_func_err_prot_t *func_err_prot
) {
  uint32_t value;
  size_t len = sizeof(uint32_t);

  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !func_err_prot || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, (uintptr_t)func_err_prot);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), fr_id(%u), dir(%u), func_err_prot(%#lx))\n", __func__, dev_id, lane, fr_id, dir, (uintptr_t)func_err_prot);

  if (dir == FRFUNC_DIR_INGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_INSERT_PROTOCOL_FAULT_0(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_INGR_RCV_INSERT_PROTOCOL_FAULT_1(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else if (dir == FRFUNC_DIR_EGRESS) {
    if (fr_id == FRFUNC_FUNC_NUMBER_0) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_INSERT_PROTOCOL_FAULT_0(lane)) != len) goto failed;
    } else if (fr_id == FRFUNC_FUNC_NUMBER_1) {
      if (pread(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_EGR_SND_INSERT_PROTOCOL_FAULT_1(lane)) != len) goto failed;
    } else {
      // Do Nothing
      llf_err(INVALID_ARGUMENT, "fr_id(%u) is not the expected value.\n", fr_id);
      return -INVALID_ARGUMENT;
    }
  } else {
    // Do Nothing
    llf_err(INVALID_ARGUMENT, "dir(%u) is not the expected value.\n", dir);
    return -INVALID_ARGUMENT;
  }

  func_err_prot->prot_ch               = (uint8_t)(value  & 0x00000001);
  func_err_prot->prot_len              = (uint8_t)((value & 0x00000002) >> 1);
  func_err_prot->prot_sof              = (uint8_t)((value & 0x00000004) >> 2);
  func_err_prot->prot_eof              = (uint8_t)((value & 0x00000008) >> 3);
  func_err_prot->prot_reqresp          = (uint8_t)((value & 0x00000010) >> 4);
  func_err_prot->prot_datanum          = (uint8_t)((value & 0x00000020) >> 5);
  func_err_prot->prot_req_outstanding  = (uint8_t)((value & 0x00000040) >> 6);
  func_err_prot->prot_resp_outstanding = (uint8_t)((value & 0x00000080) >> 7);
  func_err_prot->prot_max_datanum      = (uint8_t)((value & 0x00000100) >> 8);
  func_err_prot->prot_reqlen           = (uint8_t)((value & 0x00001000) >> 12);
  func_err_prot->prot_reqresplen       = (uint8_t)((value & 0x00002000) >> 13);

  return 0;

failed:
  llf_err(FAILURE_READ, "Failed to get parameter.\n");
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "Invalid operation: Maybe FPGA registers are locked yet.\n");
  return -FAILURE_READ;
}
