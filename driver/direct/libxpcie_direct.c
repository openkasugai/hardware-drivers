/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxpcie_direct.h"
#include "xpcie_regs_direct.h"


int
xpcie_fpga_common_get_direct_module_info(
  fpga_dev_info_t *dev)
{
  fpga_module_info_t *info = &dev->mods.direct;

  // Set Module base address
  info->base = XPCIE_FPGA_DIRECT_OFFSET;

  // Set Module length per 1 lane
  info->len = XPCIE_FPGA_DIRECT_SIZE;

  // Get Module num
  for (info->num = 0; info->num < XPCIE_KERNEL_LANE_MAX; info->num++) {
    // Read Module ID
    uint32_t value = reg_read32(dev, info->base + info->num * info->len
      + XPCIE_FPGA_DIRECT_MODULE_ID);
    switch (value) {
    case XPCIE_FPGA_DIRECT_MODULE_ID_VALUE:
      // do nothing
      break;
    default:
      // To break for loop, set zero
      value = 0;
      break;
    }
    if (!value)
      break; // Module which is not Conv found
  }

  if (info->num == 0)
    return -ENODEV;

  return 0;
}


void
xpcie_fpga_start_direct_module(
  fpga_dev_info_t *dev,
  uint32_t kernel_lane)
{
  xpcie_trace("%s: lane(%d)", __func__, kernel_lane);
  direct_reg_write(dev, XPCIE_FPGA_DIRECT_CONTROL, kernel_lane, XPCIE_FPGA_START_MODULE);
}


void
xpcie_fpga_stop_direct_module(
  fpga_dev_info_t *dev,
  uint32_t kernel_lane)
{
  xpcie_trace("%s: lane(%d)", __func__, kernel_lane);
  direct_reg_write(dev, XPCIE_FPGA_DIRECT_CONTROL, kernel_lane, XPCIE_FPGA_STOP_MODULE);
}


void
xpcie_fpga_get_direct_ctrl(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_ctrl_t *direct_ctrl)
{
  direct_ctrl->value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_CONTROL, direct_ctrl->lane);
}


void
xpcie_fpga_get_direct_module_id(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_ctrl_t *direct_ctrl)
{
  direct_ctrl->value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_MODULE_ID, direct_ctrl->lane);
}


void
xpcie_fpga_get_direct_bytes(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_bytenum_t *bytenum)
{
  uint32_t value_l, value_h;
  uint64_t addr_l, addr_h;

  switch(bytenum->reg_id) {
    case DIRECT_STAT_INGR_RCV:
      {
        addr_l = XPCIE_FPGA_DIRECT_STAT_INGR_RCV_DATA_VALUE_L;
        addr_h = XPCIE_FPGA_DIRECT_STAT_INGR_RCV_DATA_VALUE_H;
      }
      break;
    case DIRECT_STAT_INGR_SND:
      {
        addr_l = XPCIE_FPGA_DIRECT_STAT_INGR_SND_DATA_VALUE_L;
        addr_h = XPCIE_FPGA_DIRECT_STAT_INGR_SND_DATA_VALUE_H;
      }
      break;
    case DIRECT_STAT_EGR_RCV:
      {
        addr_l = XPCIE_FPGA_DIRECT_STAT_EGR_RCV_DATA_VALUE_L;
        addr_h = XPCIE_FPGA_DIRECT_STAT_EGR_RCV_DATA_VALUE_H;
      }
      break;
    case DIRECT_STAT_EGR_SND:
      {
        addr_l = XPCIE_FPGA_DIRECT_STAT_EGR_SND_DATA_VALUE_L;
        addr_h = XPCIE_FPGA_DIRECT_STAT_EGR_SND_DATA_VALUE_H;
      }
      break;
    default:
      // Do Nothing
      xpcie_err("reg_id(%u) is not the expected value.\n", bytenum->reg_id);
      return;
  }

  direct_reg_write(dev, XPCIE_FPGA_DIRECT_STAT_SEL_CHANNEL, bytenum->lane, bytenum->fchid);
  value_l = direct_reg_read(dev, addr_l, bytenum->lane);
  value_h = direct_reg_read(dev, addr_h, bytenum->lane);
  bytenum->byte_num = (uint64_t)value_l;
  bytenum->byte_num |= ((uint64_t)value_h & 0xFFFFFFFF) << 32;
}


void
xpcie_fpga_get_direct_frames(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_framenum_t *framenum)
{
  uint64_t addr;

  switch(framenum->reg_id) {
    case DIRECT_STAT_INGR_RCV:
      {
        addr = XPCIE_FPGA_DIRECT_STAT_INGR_RCV_FRAME_VALUE;
      }
      break;
    case DIRECT_STAT_INGR_SND:
      {
        addr = XPCIE_FPGA_DIRECT_STAT_INGR_SND_FRAME_VALUE;
      }
      break;
    case DIRECT_STAT_EGR_RCV:
      {
        addr = XPCIE_FPGA_DIRECT_STAT_EGR_RCV_FRAME_VALUE;
      }
      break;
    case DIRECT_STAT_EGR_SND:
      {
        addr = XPCIE_FPGA_DIRECT_STAT_EGR_SND_FRAME_VALUE;
      }
      break;
    default:
      // Do Nothing
      xpcie_err("reg_id(%u) is not the expected value.\n", framenum->reg_id);
      break;
  }

  direct_reg_write(dev, XPCIE_FPGA_DIRECT_STAT_SEL_CHANNEL, framenum->lane, framenum->fchid);
  framenum->frame_num = direct_reg_read(dev, addr, framenum->lane);
}


void
xpcie_fpga_check_direct_err(
  fpga_dev_info_t *dev,
  fpga_ioctl_err_all_t *err)
{
  err->err_all = direct_reg_read(dev, XPCIE_FPGA_DIRECT_DETECT_FAULT, err->lane);
}


void
xpcie_fpga_detect_direct_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_prot_t *direct_err_prot)
{
  uint32_t value;

  switch(direct_err_prot->dir_type) {
    case DIRECT_DIR_INGR_RCV:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_INGR_RCV_PROTOCOL_FAULT, direct_err_prot->lane);
      break;
    case DIRECT_DIR_INGR_SND:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_INGR_SND_PROTOCOL_FAULT, direct_err_prot->lane);
      break;
    case DIRECT_DIR_EGR_RCV:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_EGR_RCV_PROTOCOL_FAULT, direct_err_prot->lane);
      break;
    case DIRECT_DIR_EGR_SND:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_EGR_SND_PROTOCOL_FAULT, direct_err_prot->lane);
      break;
    default:
      /* Do Nothing */
      xpcie_err("dir_type(%u) is not the expected value.\n", direct_err_prot->dir_type);
      return;
  }

  direct_err_prot->prot_ch                = (uint8_t)(value  & 0x00000001);
  direct_err_prot->prot_len               = (uint8_t)((value & 0x00000002) >> 1);
  direct_err_prot->prot_sof               = (uint8_t)((value & 0x00000004) >> 2);
  direct_err_prot->prot_eof               = (uint8_t)((value & 0x00000008) >> 3);
  direct_err_prot->prot_reqresp           = (uint8_t)((value & 0x00000010) >> 4);
  direct_err_prot->prot_datanum           = (uint8_t)((value & 0x00000020) >> 5);
  direct_err_prot->prot_req_outstanding   = (uint8_t)((value & 0x00000040) >> 6);
  direct_err_prot->prot_resp_outstanding  = (uint8_t)((value & 0x00000080) >> 7);
  direct_err_prot->prot_max_datanum       = (uint8_t)((value & 0x00000100) >> 8);
  direct_err_prot->prot_reqlen            = (uint8_t)((value & 0x00001000) >> 12);
  direct_err_prot->prot_reqresplen        = (uint8_t)((value & 0x00002000) >> 13);
}


void
xpcie_fpga_clear_direct_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_prot_t *direct_err_prot)
{
  uint32_t value;

  value =  ((uint32_t)direct_err_prot->prot_ch               & 0x01);
  value |= ((uint32_t)direct_err_prot->prot_len              & 0x01) << 1;
  value |= ((uint32_t)direct_err_prot->prot_sof              & 0x01) << 2;
  value |= ((uint32_t)direct_err_prot->prot_eof              & 0x01) << 3;
  value |= ((uint32_t)direct_err_prot->prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)direct_err_prot->prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)direct_err_prot->prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)direct_err_prot->prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)direct_err_prot->prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)direct_err_prot->prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)direct_err_prot->prot_reqresplen       & 0x01) << 13;

  switch(direct_err_prot->dir_type) {
    case DIRECT_DIR_INGR_RCV:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_INGR_RCV_PROTOCOL_FAULT, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_INGR_SND:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_INGR_SND_PROTOCOL_FAULT, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_EGR_RCV:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_EGR_RCV_PROTOCOL_FAULT, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_EGR_SND:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_EGR_SND_PROTOCOL_FAULT, direct_err_prot->lane, value);
      break;
    default:
      /* Do Nothing */
      xpcie_err("dir_type(%u) is not the expected value.\n", direct_err_prot->dir_type);
      break;
  }
}


void
xpcie_fpga_mask_direct_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_prot_t *direct_err_prot)
{
  uint32_t value;

  value =  ((uint32_t)direct_err_prot->prot_ch               & 0x01);
  value |= ((uint32_t)direct_err_prot->prot_len              & 0x01) << 1;
  value |= ((uint32_t)direct_err_prot->prot_sof              & 0x01) << 2;
  value |= ((uint32_t)direct_err_prot->prot_eof              & 0x01) << 3;
  value |= ((uint32_t)direct_err_prot->prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)direct_err_prot->prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)direct_err_prot->prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)direct_err_prot->prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)direct_err_prot->prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)direct_err_prot->prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)direct_err_prot->prot_reqresplen       & 0x01) << 13;

  switch(direct_err_prot->dir_type) {
    case DIRECT_DIR_INGR_RCV:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_INGR_RCV_PROTOCOL_FAULT_MASK, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_INGR_SND:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_INGR_SND_PROTOCOL_FAULT_MASK, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_EGR_RCV:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_EGR_RCV_PROTOCOL_FAULT_MASK, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_EGR_SND:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_EGR_SND_PROTOCOL_FAULT_MASK, direct_err_prot->lane, value);
      break;
    default:
      /* Do Nothing */
      xpcie_err("dir_type(%u) is not the expected value.\n", direct_err_prot->dir_type);
      break;
  }
}


void
xpcie_fpga_get_mask_direct_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_prot_t *direct_err_prot)
{
  uint32_t value;

  switch(direct_err_prot->dir_type) {
    case DIRECT_DIR_INGR_RCV:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_INGR_RCV_PROTOCOL_FAULT_MASK, direct_err_prot->lane);
      break;
    case DIRECT_DIR_INGR_SND:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_INGR_SND_PROTOCOL_FAULT_MASK, direct_err_prot->lane);
      break;
    case DIRECT_DIR_EGR_RCV:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_EGR_RCV_PROTOCOL_FAULT_MASK, direct_err_prot->lane);
      break;
    case DIRECT_DIR_EGR_SND:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_EGR_SND_PROTOCOL_FAULT_MASK, direct_err_prot->lane);
      break;
    default:
      /* Do Nothing */
      xpcie_err("dir_type(%u) is not the expected value.\n", direct_err_prot->dir_type);
      return;
  }

  direct_err_prot->prot_ch                = (uint8_t)(value  & 0x00000001);
  direct_err_prot->prot_len               = (uint8_t)((value & 0x00000002) >> 1);
  direct_err_prot->prot_sof               = (uint8_t)((value & 0x00000004) >> 2);
  direct_err_prot->prot_eof               = (uint8_t)((value & 0x00000008) >> 3);
  direct_err_prot->prot_reqresp           = (uint8_t)((value & 0x00000010) >> 4);
  direct_err_prot->prot_datanum           = (uint8_t)((value & 0x00000020) >> 5);
  direct_err_prot->prot_req_outstanding   = (uint8_t)((value & 0x00000040) >> 6);
  direct_err_prot->prot_resp_outstanding  = (uint8_t)((value & 0x00000080) >> 7);
  direct_err_prot->prot_max_datanum       = (uint8_t)((value & 0x00000100) >> 8);
  direct_err_prot->prot_reqlen            = (uint8_t)((value & 0x00001000) >> 12);
  direct_err_prot->prot_reqresplen        = (uint8_t)((value & 0x00002000) >> 13);
}


void
xpcie_fpga_force_direct_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_prot_t *direct_err_prot)
{
  uint32_t value;

  value =  ((uint32_t)direct_err_prot->prot_ch               & 0x01);
  value |= ((uint32_t)direct_err_prot->prot_len              & 0x01) << 1;
  value |= ((uint32_t)direct_err_prot->prot_sof              & 0x01) << 2;
  value |= ((uint32_t)direct_err_prot->prot_eof              & 0x01) << 3;
  value |= ((uint32_t)direct_err_prot->prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)direct_err_prot->prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)direct_err_prot->prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)direct_err_prot->prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)direct_err_prot->prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)direct_err_prot->prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)direct_err_prot->prot_reqresplen       & 0x01) << 13;

  switch(direct_err_prot->dir_type) {
    case DIRECT_DIR_INGR_RCV:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_INGR_RCV_PROTOCOL_FAULT_FORCE, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_INGR_SND:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_INGR_SND_PROTOCOL_FAULT_FORCE, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_EGR_RCV:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_EGR_RCV_PROTOCOL_FAULT_FORCE, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_EGR_SND:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_EGR_SND_PROTOCOL_FAULT_FORCE, direct_err_prot->lane, value);
      break;
    default:
      /* Do Nothing */
      xpcie_err("dir_type(%u) is not the expected value.\n", direct_err_prot->dir_type);
      break;
  }
}


void
xpcie_fpga_get_force_direct_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_prot_t *direct_err_prot)
{
  uint32_t value;

  switch(direct_err_prot->dir_type) {
    case DIRECT_DIR_INGR_RCV:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_INGR_RCV_PROTOCOL_FAULT_FORCE, direct_err_prot->lane);
      break;
    case DIRECT_DIR_INGR_SND:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_INGR_SND_PROTOCOL_FAULT_FORCE, direct_err_prot->lane);
      break;
    case DIRECT_DIR_EGR_RCV:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_EGR_RCV_PROTOCOL_FAULT_FORCE, direct_err_prot->lane);
      break;
    case DIRECT_DIR_EGR_SND:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_EGR_SND_PROTOCOL_FAULT_FORCE, direct_err_prot->lane);
      break;
    default:
      /* Do Nothing */
      xpcie_err("dir_type(%u) is not the expected value.\n", direct_err_prot->dir_type);
      return;
  }

  direct_err_prot->prot_ch                = (uint8_t)(value  & 0x00000001);
  direct_err_prot->prot_len               = (uint8_t)((value & 0x00000002) >> 1);
  direct_err_prot->prot_sof               = (uint8_t)((value & 0x00000004) >> 2);
  direct_err_prot->prot_eof               = (uint8_t)((value & 0x00000008) >> 3);
  direct_err_prot->prot_reqresp           = (uint8_t)((value & 0x00000010) >> 4);
  direct_err_prot->prot_datanum           = (uint8_t)((value & 0x00000020) >> 5);
  direct_err_prot->prot_req_outstanding   = (uint8_t)((value & 0x00000040) >> 6);
  direct_err_prot->prot_resp_outstanding  = (uint8_t)((value & 0x00000080) >> 7);
  direct_err_prot->prot_max_datanum       = (uint8_t)((value & 0x00000100) >> 8);
  direct_err_prot->prot_reqlen            = (uint8_t)((value & 0x00001000) >> 12);
  direct_err_prot->prot_reqresplen        = (uint8_t)((value & 0x00002000) >> 13);
}


void
xpcie_fpga_ins_direct_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_prot_t *direct_err_prot)
{
  uint32_t value;

  value =  ((uint32_t)direct_err_prot->prot_ch               & 0x01);
  value |= ((uint32_t)direct_err_prot->prot_len              & 0x01) << 1;
  value |= ((uint32_t)direct_err_prot->prot_sof              & 0x01) << 2;
  value |= ((uint32_t)direct_err_prot->prot_eof              & 0x01) << 3;
  value |= ((uint32_t)direct_err_prot->prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)direct_err_prot->prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)direct_err_prot->prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)direct_err_prot->prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)direct_err_prot->prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)direct_err_prot->prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)direct_err_prot->prot_reqresplen       & 0x01) << 13;

  switch(direct_err_prot->dir_type) {
    case DIRECT_DIR_INGR_RCV:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_INGR_RCV_PROTOCOL_FAULT_INS, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_INGR_SND:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_INGR_SND_PROTOCOL_FAULT_INS, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_EGR_RCV:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_EGR_RCV_PROTOCOL_FAULT_INS, direct_err_prot->lane, value);
      break;
    case DIRECT_DIR_EGR_SND:
      direct_reg_write(dev, XPCIE_FPGA_DIRECT_EGR_SND_PROTOCOL_FAULT_INS, direct_err_prot->lane, value);
      break;
    default:
      /* Do Nothing */
      xpcie_err("dir_type(%u) is not the expected value.\n", direct_err_prot->dir_type);
      break;
  }
}


void
xpcie_fpga_get_ins_direct_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_prot_t *direct_err_prot)
{
  uint32_t value;

  switch(direct_err_prot->dir_type) {
    case DIRECT_DIR_INGR_RCV:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_INGR_RCV_PROTOCOL_FAULT_INS, direct_err_prot->lane);
      break;
    case DIRECT_DIR_INGR_SND:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_INGR_SND_PROTOCOL_FAULT_INS, direct_err_prot->lane);
      break;
    case DIRECT_DIR_EGR_RCV:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_EGR_RCV_PROTOCOL_FAULT_INS, direct_err_prot->lane);
      break;
    case DIRECT_DIR_EGR_SND:
      value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_EGR_SND_PROTOCOL_FAULT_INS, direct_err_prot->lane);
      break;
    default:
      /* Do Nothing */
      xpcie_err("dir_type(%u) is not the expected value.\n", direct_err_prot->dir_type);
      return;
  }

  direct_err_prot->prot_ch                = (uint8_t)(value  & 0x00000001);
  direct_err_prot->prot_len               = (uint8_t)((value & 0x00000002) >> 1);
  direct_err_prot->prot_sof               = (uint8_t)((value & 0x00000004) >> 2);
  direct_err_prot->prot_eof               = (uint8_t)((value & 0x00000008) >> 3);
  direct_err_prot->prot_reqresp           = (uint8_t)((value & 0x00000010) >> 4);
  direct_err_prot->prot_datanum           = (uint8_t)((value & 0x00000020) >> 5);
  direct_err_prot->prot_req_outstanding   = (uint8_t)((value & 0x00000040) >> 6);
  direct_err_prot->prot_resp_outstanding  = (uint8_t)((value & 0x00000080) >> 7);
  direct_err_prot->prot_max_datanum       = (uint8_t)((value & 0x00000100) >> 8);
  direct_err_prot->prot_reqlen            = (uint8_t)((value & 0x00001000) >> 12);
  direct_err_prot->prot_reqresplen        = (uint8_t)((value & 0x00002000) >> 13);
}


void
xpcie_fpga_detect_direct_err_stif(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_stif_t *direct_err_stif)
{
  uint32_t value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_STREAMIF_STALL, direct_err_stif->lane);

  direct_err_stif->ingress_rcv_req  = (uint8_t)(value  & 0x00000001);
  direct_err_stif->ingress_rcv_resp = (uint8_t)((value & 0x00000002) >> 1);
  direct_err_stif->ingress_rcv_data = (uint8_t)((value & 0x00000004) >> 2);
  direct_err_stif->ingress_snd_req  = (uint8_t)((value & 0x00000008) >> 3);
  direct_err_stif->ingress_snd_resp = (uint8_t)((value & 0x00000010) >> 4);
  direct_err_stif->ingress_snd_data = (uint8_t)((value & 0x00000020) >> 5);
  direct_err_stif->egress_rcv_req   = (uint8_t)((value & 0x00000040) >> 6);
  direct_err_stif->egress_rcv_resp  = (uint8_t)((value & 0x00000080) >> 7);
  direct_err_stif->egress_rcv_data  = (uint8_t)((value & 0x00000100) >> 8);
  direct_err_stif->egress_snd_req   = (uint8_t)((value & 0x00000200) >> 9);
  direct_err_stif->egress_snd_resp  = (uint8_t)((value & 0x00000400) >> 10);
  direct_err_stif->egress_snd_data  = (uint8_t)((value & 0x00000800) >> 11);
}


void
xpcie_fpga_mask_direct_err_stif(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_stif_t *direct_err_stif)
{
  uint32_t value;

  value =  ((uint32_t)direct_err_stif->ingress_rcv_req  & 0x01);
  value |= ((uint32_t)direct_err_stif->ingress_rcv_resp & 0x01) << 1;
  value |= ((uint32_t)direct_err_stif->ingress_rcv_data & 0x01) << 2;
  value |= ((uint32_t)direct_err_stif->ingress_snd_req  & 0x01) << 3;
  value |= ((uint32_t)direct_err_stif->ingress_snd_resp & 0x01) << 4;
  value |= ((uint32_t)direct_err_stif->ingress_snd_data & 0x01) << 5;
  value |= ((uint32_t)direct_err_stif->egress_rcv_req   & 0x01) << 6;
  value |= ((uint32_t)direct_err_stif->egress_rcv_resp  & 0x01) << 7;
  value |= ((uint32_t)direct_err_stif->egress_rcv_data  & 0x01) << 8;
  value |= ((uint32_t)direct_err_stif->egress_snd_req   & 0x01) << 9;
  value |= ((uint32_t)direct_err_stif->egress_snd_resp  & 0x01) << 10;
  value |= ((uint32_t)direct_err_stif->egress_snd_data  & 0x01) << 11;

  direct_reg_write(dev, XPCIE_FPGA_DIRECT_STREAMIF_STALL_MASK, direct_err_stif->lane, value);
}


void
xpcie_fpga_get_mask_direct_err_stif(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_stif_t *direct_err_stif)
{
  uint32_t value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_STREAMIF_STALL_MASK, direct_err_stif->lane);

  direct_err_stif->ingress_rcv_req  = (uint8_t)(value  & 0x00000001);
  direct_err_stif->ingress_rcv_resp = (uint8_t)((value & 0x00000002) >> 1);
  direct_err_stif->ingress_rcv_data = (uint8_t)((value & 0x00000004) >> 2);
  direct_err_stif->ingress_snd_req  = (uint8_t)((value & 0x00000008) >> 3);
  direct_err_stif->ingress_snd_resp = (uint8_t)((value & 0x00000010) >> 4);
  direct_err_stif->ingress_snd_data = (uint8_t)((value & 0x00000020) >> 5);
  direct_err_stif->egress_rcv_req   = (uint8_t)((value & 0x00000040) >> 6);
  direct_err_stif->egress_rcv_resp  = (uint8_t)((value & 0x00000080) >> 7);
  direct_err_stif->egress_rcv_data  = (uint8_t)((value & 0x00000100) >> 8);
  direct_err_stif->egress_snd_req   = (uint8_t)((value & 0x00000200) >> 9);
  direct_err_stif->egress_snd_resp  = (uint8_t)((value & 0x00000400) >> 10);
  direct_err_stif->egress_snd_data  = (uint8_t)((value & 0x00000800) >> 11);
}


void
xpcie_fpga_force_direct_err_stif(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_stif_t *direct_err_stif)
{
  uint32_t value;

  value =  ((uint32_t)direct_err_stif->ingress_rcv_req  & 0x01);
  value |= ((uint32_t)direct_err_stif->ingress_rcv_resp & 0x01) << 1;
  value |= ((uint32_t)direct_err_stif->ingress_rcv_data & 0x01) << 2;
  value |= ((uint32_t)direct_err_stif->ingress_snd_req  & 0x01) << 3;
  value |= ((uint32_t)direct_err_stif->ingress_snd_resp & 0x01) << 4;
  value |= ((uint32_t)direct_err_stif->ingress_snd_data & 0x01) << 5;
  value |= ((uint32_t)direct_err_stif->egress_rcv_req   & 0x01) << 6;
  value |= ((uint32_t)direct_err_stif->egress_rcv_resp  & 0x01) << 7;
  value |= ((uint32_t)direct_err_stif->egress_rcv_data  & 0x01) << 8;
  value |= ((uint32_t)direct_err_stif->egress_snd_req   & 0x01) << 9;
  value |= ((uint32_t)direct_err_stif->egress_snd_resp  & 0x01) << 10;
  value |= ((uint32_t)direct_err_stif->egress_snd_data  & 0x01) << 11;

  direct_reg_write(dev, XPCIE_FPGA_DIRECT_STREAMIF_STALL_FORCE, direct_err_stif->lane, value);
}


void
xpcie_fpga_get_force_direct_err_stif(
  fpga_dev_info_t *dev,
  fpga_ioctl_direct_err_stif_t *direct_err_stif)
{
  uint32_t value = direct_reg_read(dev, XPCIE_FPGA_DIRECT_STREAMIF_STALL_FORCE, direct_err_stif->lane);

  direct_err_stif->ingress_rcv_req  = (uint8_t)(value  & 0x00000001);
  direct_err_stif->ingress_rcv_resp = (uint8_t)((value & 0x00000002) >> 1);
  direct_err_stif->ingress_rcv_data = (uint8_t)((value & 0x00000004) >> 2);
  direct_err_stif->ingress_snd_req  = (uint8_t)((value & 0x00000008) >> 3);
  direct_err_stif->ingress_snd_resp = (uint8_t)((value & 0x00000010) >> 4);
  direct_err_stif->ingress_snd_data = (uint8_t)((value & 0x00000020) >> 5);
  direct_err_stif->egress_rcv_req   = (uint8_t)((value & 0x00000040) >> 6);
  direct_err_stif->egress_rcv_resp  = (uint8_t)((value & 0x00000080) >> 7);
  direct_err_stif->egress_rcv_data  = (uint8_t)((value & 0x00000100) >> 8);
  direct_err_stif->egress_snd_req   = (uint8_t)((value & 0x00000200) >> 9);
  direct_err_stif->egress_snd_resp  = (uint8_t)((value & 0x00000400) >> 10);
  direct_err_stif->egress_snd_data  = (uint8_t)((value & 0x00000800) >> 11);
}
