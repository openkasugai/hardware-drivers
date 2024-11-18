/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include <asm/mwait.h>

#include "libxpcie_chain.h"
#include "xpcie_regs_chain.h"


int
xpcie_fpga_common_get_chain_module_info(
  fpga_dev_info_t *dev)
{
  fpga_module_info_t *info = &dev->mods.chain;

  // Set Module base address
  info->base = XPCIE_FPGA_CHAIN_OFFSET;

  // Set Module length per 1 lane
  info->len = XPCIE_FPGA_CHAIN_SIZE;

  // Get Module num
  for (info->num = 0; info->num < XPCIE_KERNEL_LANE_MAX; info->num++) {
    // Read Module ID
    uint32_t value = reg_read32(dev, info->base + info->num * info->len
      + XPCIE_FPGA_CHAIN_MODULE_ID);
    switch (value) {
    case XPCIE_FPGA_CHAIN_MODULE_ID_VALUE:
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
xpcie_fpga_get_chain_ctrl(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_ctrl_t *chain_ctrl)
{
  chain_ctrl->value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_CONTROL, chain_ctrl->lane);
}


void
xpcie_fpga_get_chain_module_id(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_ctrl_t *chain_ctrl)
{
  chain_ctrl->value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_MODULE_ID, chain_ctrl->lane);
}


int
xpcie_fpga_update_func_chain_table(
  fpga_dev_info_t *dev,
  fpga_id_t *id,
  uint32_t kind)
{
  xpcie_trace("%s: lane(%d), ingr/egr(%d), cid(%d), fchid(%d)",
    __func__, id->lane, kind, id->cid, id->fchid);
  {
    uint32_t *monitor_val;
    uint32_t value;
    int cnt;

    if (kind == FPGA_CID_KIND_INGRESS) {
      if ((id->cid >= XPCIE_CID_MIN && id->cid <= XPCIE_CID_MAX)
        && ((id->extif_id == FPGA_EXTIF_NUMBER_0) || (id->extif_id == FPGA_EXTIF_NUMBER_1))) {
        value = (uint32_t)id->cid;
        value |= ((uint32_t)id->extif_id & 0x00000001) << 9;
      } else {
        // Do Nothing
        xpcie_err("extif_id(%u) cid(%u) is not the expected value.\n", id->extif_id, id->cid);
        return -EINVAL;
      }
      // set request for function chain table update
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_FORWARD_UPDATE_REQ, id->lane, 0x0);
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_FORWARD_SESSION, id->lane, value);
      value = (uint32_t)id->fchid;
      value |= ((uint32_t)id->enable_flag & 0x00000001) << 16;
      value |= ((uint32_t)id->active_flag & 0x00000001) << 17;
      value |= ((uint32_t)id->direct_flag & 0x00000001) << 18;
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_FORWARD_CHANNEL, id->lane, value);
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_FORWARD_UPDATE_REQ, id->lane, 0x1);
      monitor_val = (uint32_t *)(
        dev->base_addr
        + dev->mods.chain.base
        + dev->mods.chain.len * id->lane
        + XPCIE_FPGA_CHAIN_INGR_FORWARD_UPDATE_RESP);
    } else if (kind == FPGA_CID_KIND_EGRESS) {
      if ((id->cid >= XPCIE_CID_MIN && id->cid <= XPCIE_CID_MAX)
        && ((id->extif_id == FPGA_EXTIF_NUMBER_0) || (id->extif_id == FPGA_EXTIF_NUMBER_1))) {
        value = (uint32_t)id->cid ;
        value |= ((uint32_t)id->extif_id & 0x00000001) << 9;
      } else {
        // Do Nothing
        xpcie_err("extif_id(%u) cid(%u) is not the expected value.\n", id->extif_id, id->cid);
        return -EINVAL;
      }
      // set request for function chain table update
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_FORWARD_UPDATE_REQ, id->lane, 0x0);
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_FORWARD_CHANNEL, id->lane, id->fchid);
      value |= ((uint32_t)id->enable_flag   & 0x00000001) << 16;
      value |= ((uint32_t)id->active_flag   & 0x00000001) << 17;
      value |= ((uint32_t)id->virtual_flag  & 0x00000001) << 19;
      value |= ((uint32_t)id->blocking_flag & 0x00000001) << 20;
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_FORWARD_SESSION, id->lane, value);
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_FORWARD_UPDATE_REQ, id->lane, 0x1);
      monitor_val = (uint32_t *)(
        dev->base_addr
        + dev->mods.chain.base
        + dev->mods.chain.len * id->lane
        + XPCIE_FPGA_CHAIN_EGR_FORWARD_UPDATE_RESP);
    } else {
      // Do Nothing
      xpcie_err("kind(%u) is not the expected value.\n", kind);
      return -EINVAL;
    }

    // monitor if table was updated
    for(cnt = 0; cnt < FPGA_UPDATE_POLLING_MAX; cnt++){
      __monitor(monitor_val, 0, 0);
      smp_mb();
      if (*(volatile uint32_t *)monitor_val == 0x1) {
        dev->fch_dev_table[id->lane][id->fchid][kind].extif_id = id->extif_id;
        dev->fch_dev_table[id->lane][id->fchid][kind].cid = id->cid;
        return 0;
      }
      __mwait(0, 0);
    }

    xpcie_warn("Chain update timeout...");
    return -XPCIE_DEV_UPDATE_TIMEOUT;
  }
}


int
xpcie_fpga_delete_func_chain_table(
  fpga_dev_info_t *dev,
  fpga_id_t *id,
  uint32_t kind)
{
  xpcie_trace("%s: lane(%d), ingr/egr(%d), fchid(%d)", __func__, id->lane, kind, id->fchid);
  {
  uint32_t *monitor_val;
  int cnt;

  if((dev->fch_dev_table[id->lane][id->fchid][kind].cid == -1)
    || (dev->fch_dev_table[id->lane][id->fchid][kind].extif_id == -1)){
    xpcie_warn("No chain found...");
    return -XPCIE_DEV_NO_CHAIN_FOUND;
  }

  if (kind == FPGA_CID_KIND_INGRESS) {
    uint32_t value;
    // set request for function chain table delete
    value = (uint32_t)dev->fch_dev_table[id->lane][id->fchid][kind].cid;
    value |= ((uint32_t)dev->fch_dev_table[id->lane][id->fchid][kind].extif_id & 0x00000001) << 9;
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_FORWARD_UPDATE_REQ, id->lane, 0x0);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_FORWARD_SESSION, id->lane, value);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_FORWARD_UPDATE_REQ, id->lane, 0x2);
    monitor_val = (uint32_t *)(
      dev->base_addr
      + dev->mods.chain.base
      + dev->mods.chain.len * id->lane
      + XPCIE_FPGA_CHAIN_INGR_FORWARD_UPDATE_RESP);
  } else if (kind == FPGA_CID_KIND_EGRESS) {
    // set request for function chain table delete
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_FORWARD_UPDATE_REQ, id->lane, 0x0);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_FORWARD_CHANNEL, id->lane, id->fchid);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_FORWARD_UPDATE_REQ, id->lane, 0x2);
    monitor_val = (uint32_t *)(
      dev->base_addr
      + dev->mods.chain.base
      + dev->mods.chain.len * id->lane
      + XPCIE_FPGA_CHAIN_EGR_FORWARD_UPDATE_RESP);
  } else {
    // Do Nothing
    xpcie_err("kind(%u) is not the expected value.\n", kind);
    return -EINVAL;
  }

  // monitor if table was updated
  for(cnt = 0; cnt < FPGA_UPDATE_POLLING_MAX; cnt++){
    __monitor(monitor_val, 0, 0);
    smp_mb();
    if (*(volatile uint32_t *)monitor_val == 0x1) {
      id->extif_id = (uint32_t)dev->fch_dev_table[id->lane][id->fchid][kind].extif_id;
      id->cid = (uint16_t)dev->fch_dev_table[id->lane][id->fchid][kind].cid;
      xpcie_trace("%s: delete extif_id(%d) cid(%d)", __func__, id->extif_id, id->cid);
      dev->fch_dev_table[id->lane][id->fchid][kind].extif_id = -1;
      dev->fch_dev_table[id->lane][id->fchid][kind].cid = -1;
      return 0;
    }
    __mwait(0, 0);
  }

  xpcie_warn("Chain delete timeout...");
  return -XPCIE_DEV_UPDATE_TIMEOUT;
  }
}


int
xpcie_fpga_read_func_chain_table(
  fpga_dev_info_t *dev,
  fpga_id_t *id,
  uint32_t kind)
{
  xpcie_trace("%s: lane(%d), ingr/egr(%d), fchid(%d), cid(%d)", __func__, id->lane, kind, id->fchid, id->cid);
  {
  uint32_t *monitor_val, read_val;
  uint64_t read_addr;
  int cnt;

  int i, j, k;
  bool flag = false;

  if (kind == FPGA_CID_KIND_INGRESS) {
    uint32_t value;
    for (i=0;i<XPCIE_KERNEL_LANE_MAX; i++) {
      int lane_ = i;
      for (j=XPCIE_FUNCTION_CHAIN_ID_MIN; j<XPCIE_FUNCTION_CHAIN_ID_MAX+1; j++) {
        uint16_t fchid_ = j;
        for (k=0; k<FPGA_CID_KIND_MAX; k++) { // 0:ig 1:eg
          uint32_t kind_ = k;
          if((dev->fch_dev_table[lane_][fchid_][kind_].cid == id->cid)
            && (dev->fch_dev_table[lane_][fchid_][kind_].extif_id == id->extif_id)){
            xpcie_trace("%s: extif_id(%d), cid(%d) ", __func__, id->extif_id, id->cid);
            flag = true;
          }
        }
      }
    }
    if (!flag) {
      xpcie_warn("Ingress No chain found...");
      return -XPCIE_DEV_NO_CHAIN_FOUND;
    }
    // set request for function chain table read
    value = (uint32_t)id->cid;
    value |= ((uint32_t)id->extif_id & 0x00000001) << 9;
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_FORWARD_UPDATE_REQ, id->lane, 0x0);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_FORWARD_SESSION, id->lane, value);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_FORWARD_UPDATE_REQ, id->lane, 0x3);
    monitor_val = (uint32_t *)(
      dev->base_addr
      + dev->mods.chain.base
      + dev->mods.chain.len * id->lane
      + XPCIE_FPGA_CHAIN_INGR_FORWARD_UPDATE_RESP);
    read_addr = XPCIE_FPGA_CHAIN_INGR_FORWARD_CHANNEL;
  } else if (kind == FPGA_CID_KIND_EGRESS) {
    if((dev->fch_dev_table[id->lane][id->fchid][kind].cid == -1)
      || (dev->fch_dev_table[id->lane][id->fchid][kind].extif_id == -1)){
      xpcie_warn("Egress No chain found...");
      return -XPCIE_DEV_NO_CHAIN_FOUND;
    }
    // set request for function chain table read
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_FORWARD_UPDATE_REQ, id->lane, 0x0);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_FORWARD_CHANNEL, id->lane, id->fchid);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_FORWARD_UPDATE_REQ, id->lane, 0x3);
    monitor_val = (uint32_t *)(
      dev->base_addr
      + dev->mods.chain.base
      + dev->mods.chain.len * id->lane
      + XPCIE_FPGA_CHAIN_EGR_FORWARD_UPDATE_RESP);
    read_addr = XPCIE_FPGA_CHAIN_EGR_FORWARD_SESSION;
  } else {
    // Do Nothing
    xpcie_err("kind(%u) is not the expected value.\n", kind);
    return -EINVAL;
  }

  // monitor if table was updated
  for(cnt = 0; cnt < FPGA_UPDATE_POLLING_MAX; cnt++){
    __monitor(monitor_val, 0, 0);
    smp_mb();
    if (*(volatile uint32_t *)monitor_val == 0x1) {
      read_val = chain_reg_read(dev, read_addr, id->lane);
      if (kind == FPGA_CID_KIND_INGRESS) {
        id->fchid       = (uint16_t)(read_val & 0x000001FF);
        id->enable_flag = (uint8_t)((read_val & 0x00010000) >> 16);
        id->active_flag = (uint8_t)((read_val & 0x00020000) >> 17);
        id->direct_flag = (uint8_t)((read_val & 0x00040000) >> 18);
      } else if (kind == FPGA_CID_KIND_EGRESS) {
        id->extif_id      = (uint32_t)((read_val & 0x00000200) >> 9);
        id->cid           = (uint16_t)(read_val & 0x000001FF);
        id->enable_flag   = (uint8_t)((read_val & 0x00010000) >> 16);
        id->active_flag   = (uint8_t)((read_val & 0x00020000) >> 17);
        id->virtual_flag  = (uint8_t)((read_val & 0x00080000) >> 19);
        id->blocking_flag = (uint8_t)((read_val & 0x00100000) >> 20);
      }
      xpcie_trace("%s: read cid(%d)", __func__, read_val);

      return 0;
    }
    __mwait(0, 0);
  }

  xpcie_warn("Chain read timeout...");
  return -XPCIE_DEV_UPDATE_TIMEOUT;
  }
}


void
xpcie_fpga_read_chain_soft_table(
  fpga_dev_info_t *dev,
  uint32_t lane,
  uint32_t fchid,
  uint32_t *ingress_extif_id,
  uint32_t *ingress_cid,
  uint32_t *egress_extif_id,
  uint32_t *egress_cid
)
{
  xpcie_trace("%s: lane(%u), fchid(%u)", __func__, lane, fchid);
  *ingress_extif_id = dev->fch_dev_table[lane][fchid][FPGA_CID_KIND_INGRESS].extif_id;
  *ingress_cid      = dev->fch_dev_table[lane][fchid][FPGA_CID_KIND_INGRESS].cid;
  *egress_extif_id  = dev->fch_dev_table[lane][fchid][FPGA_CID_KIND_EGRESS].extif_id;
  *egress_cid       = dev->fch_dev_table[lane][fchid][FPGA_CID_KIND_EGRESS].cid;
}


void
xpcie_fpga_reset_chain_soft_table(
  fpga_dev_info_t *dev
)
{
  xpcie_trace("%s: ", __func__);
  memset(dev->fch_dev_table, 0xFF, sizeof(dev->fch_dev_table));
}


void
xpcie_fpga_start_chain_module(
  fpga_dev_info_t *dev,
  uint32_t kernel_lane)
{
  xpcie_trace("%s: lane(%d)", __func__, kernel_lane);
  chain_reg_write(dev, XPCIE_FPGA_CHAIN_CONTROL, kernel_lane, XPCIE_FPGA_START_MODULE);
}


void
xpcie_fpga_stop_chain_module(
  fpga_dev_info_t *dev,
  uint32_t kernel_lane)
{
  xpcie_trace("%s: lane(%d)", __func__, kernel_lane);
  chain_reg_write(dev, XPCIE_FPGA_CHAIN_CONTROL, kernel_lane, XPCIE_FPGA_STOP_MODULE);
}


void
xpcie_fpga_set_ddr_offset_frame(
  fpga_dev_info_t *dev,
  fpga_ioctl_extif_t *extif
)
{
  if (extif->extif_id == FPGA_EXTIF_NUMBER_0) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_BASE_L,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF0_BUFFER_BASE_L);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_BASE_H,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF0_BUFFER_BASE_H(extif->lane));
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_RX_OFFSET_L,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF0_BUFFER_RX_OFFSET_L);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_RX_OFFSET_H,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF0_BUFFER_RX_OFFSET_H);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_RX_STRIDE,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF0_BUFFER_RX_STRIDE);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_RX_SIZE,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF0_BUFFER_RX_SIZE);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_TX_OFFSET_L,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF0_BUFFER_TX_OFFSET_L);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_TX_OFFSET_H,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF0_BUFFER_TX_OFFSET_H);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_TX_STRIDE,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF0_BUFFER_TX_STRIDE);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_TX_SIZE,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF0_BUFFER_TX_SIZE);
  } else if (extif->extif_id == FPGA_EXTIF_NUMBER_1) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_BASE_L,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF1_BUFFER_BASE_L);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_BASE_H,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF1_BUFFER_BASE_H(extif->lane));
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_RX_OFFSET_L,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF1_BUFFER_RX_OFFSET_L);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_RX_OFFSET_H,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF1_BUFFER_RX_OFFSET_H);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_RX_STRIDE,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF1_BUFFER_RX_STRIDE);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_RX_SIZE,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF1_BUFFER_RX_SIZE);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_TX_OFFSET_L,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF1_BUFFER_TX_OFFSET_L);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_TX_OFFSET_H,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF1_BUFFER_TX_OFFSET_H);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_TX_STRIDE,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF1_BUFFER_TX_STRIDE);
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_TX_SIZE,
            extif->lane, XPCIE_FPGA_DDR_VALUE_AXI_EXTIF1_BUFFER_TX_SIZE);
  } else {
    /* Do Nothing */
    xpcie_err("extif_id(%u) is not the expected value.\n", extif->extif_id);
  }
}


void
xpcie_fpga_get_ddr_offset_frame(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_ddr_t  *chain_ddr
)
{
  uint32_t value, value_l, value_h;

  if (chain_ddr->extif_id == FPGA_EXTIF_NUMBER_0) {
    value_l = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_BASE_L, chain_ddr->lane);
    value_h = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_BASE_H, chain_ddr->lane);
    chain_ddr->base =  ((uint64_t)value_l & 0xFFFFFFFF);
    chain_ddr->base |= ((uint64_t)value_h & 0xFFFFFFFF) << 32;

    value_l = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_RX_OFFSET_L, chain_ddr->lane);
    value_h = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_RX_OFFSET_H, chain_ddr->lane);
    chain_ddr->rx_offset =  ((uint64_t)value_l & 0xFFFFFFFF);
    chain_ddr->rx_offset |= ((uint64_t)value_h & 0xFFFFFFFF) << 32;

    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_RX_STRIDE, chain_ddr->lane);
    chain_ddr->rx_stride = value & 0xFFFFFFFF;

    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_RX_SIZE, chain_ddr->lane);
    chain_ddr->rx_size = (uint8_t)(value & 0x0000000F);

    value_l = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_TX_OFFSET_L, chain_ddr->lane);
    value_h = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_TX_OFFSET_H, chain_ddr->lane);
    chain_ddr->tx_offset =  ((uint64_t)value_l & 0xFFFFFFFF);
    chain_ddr->tx_offset |= ((uint64_t)value_h & 0xFFFFFFFF) << 32;

    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_TX_STRIDE, chain_ddr->lane);
    chain_ddr->tx_stride = value & 0xFFFFFFFF;

    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF0_BUFFER_TX_SIZE, chain_ddr->lane);
    chain_ddr->tx_size = (uint8_t)(value & 0x0000000F);
  } else if (chain_ddr->extif_id == FPGA_EXTIF_NUMBER_1) {
    value_l = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_BASE_L, chain_ddr->lane);
    value_h = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_BASE_H, chain_ddr->lane);
    chain_ddr->base =  ((uint64_t)value_l & 0xFFFFFFFF);
    chain_ddr->base |= ((uint64_t)value_h & 0xFFFFFFFF) << 32;

    value_l = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_RX_OFFSET_L, chain_ddr->lane);
    value_h = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_RX_OFFSET_H, chain_ddr->lane);
    chain_ddr->rx_offset =  ((uint64_t)value_l & 0xFFFFFFFF);
    chain_ddr->rx_offset |= ((uint64_t)value_h & 0xFFFFFFFF) << 32;

    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_RX_STRIDE, chain_ddr->lane);
    chain_ddr->rx_stride = value & 0xFFFFFFFF;

    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_RX_SIZE, chain_ddr->lane);
    chain_ddr->rx_size = (uint8_t)(value & 0x0000000F);

    value_l = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_TX_OFFSET_L, chain_ddr->lane);
    value_h = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_TX_OFFSET_H, chain_ddr->lane);
    chain_ddr->tx_offset =  ((uint64_t)value_l & 0xFFFFFFFF);
    chain_ddr->tx_offset |= ((uint64_t)value_h & 0xFFFFFFFF) << 32;

    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_TX_STRIDE, chain_ddr->lane);
    chain_ddr->tx_stride = value & 0xFFFFFFFF;

    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_AXI_EXTIF1_BUFFER_TX_SIZE, chain_ddr->lane);
    chain_ddr->tx_size = (uint8_t)(value & 0x0000000F);
  } else {
    /* Do Nothing */
    xpcie_err("extif_id(%u) is not the expected value.\n", chain_ddr->extif_id);
  }
}


void
xpcie_fpga_get_latency_chain(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_latency_t *latency)
{
  chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_SESSION, latency->lane, latency->cid);

  if (latency->dir == FPGA_CID_KIND_INGRESS) {
    if (latency->extif_id == FPGA_EXTIF_NUMBER_0) {
      latency->latency = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_LATENCY_0_VALUE, latency->lane);
    } else if (latency->extif_id == FPGA_EXTIF_NUMBER_1) {
      latency->latency = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_LATENCY_1_VALUE, latency->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", latency->extif_id);
    }
  }
  else if (latency->dir == FPGA_CID_KIND_EGRESS) {
    if (latency->extif_id == FPGA_EXTIF_NUMBER_0) {
      latency->latency = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_LATENCY_0_VALUE, latency->lane);
    } else if (latency->extif_id == FPGA_EXTIF_NUMBER_1) {
      latency->latency = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_LATENCY_1_VALUE, latency->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", latency->extif_id);
    }
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", latency->dir);
  }
}


void
xpcie_fpga_get_latency_func(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_func_latency_t *latency)
{
  chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL, latency->lane, latency->fchid);

  latency->latency = chain_reg_read(dev, XPCIE_FPGA_CHAIN_FUNC_LATENCY_VALUE, latency->lane);
}


void
xpcie_fpga_get_chain_bytes(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_bytenum_t *bytenum)
{
  uint32_t value_l, value_h;
  uint64_t addr_w, addr_l, addr_h;

  switch(bytenum->reg_id) {
    case CHAIN_STAT_INGR_RCV0:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_SESSION;
        addr_l = XPCIE_FPGA_CHAIN_STAT_INGR_RCV_DATA_0_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_INGR_RCV_DATA_0_VALUE_H;
      }
      break;
    case CHAIN_STAT_INGR_RCV1:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_SESSION;
        addr_l = XPCIE_FPGA_CHAIN_STAT_INGR_RCV_DATA_1_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_INGR_RCV_DATA_1_VALUE_H;
      }
      break;
    case CHAIN_STAT_INGR_SND0:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL;
        addr_l = XPCIE_FPGA_CHAIN_STAT_INGR_SND_DATA_0_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_INGR_SND_DATA_0_VALUE_H;
      }
      break;
    case CHAIN_STAT_INGR_SND1:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL;
        addr_l = XPCIE_FPGA_CHAIN_STAT_INGR_SND_DATA_1_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_INGR_SND_DATA_1_VALUE_H;
      }
      break;
    case CHAIN_STAT_EGR_RCV0:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL;
        addr_l = XPCIE_FPGA_CHAIN_STAT_EGR_RCV_DATA_0_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_EGR_RCV_DATA_0_VALUE_H;
      }
      break;
    case CHAIN_STAT_EGR_RCV1:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL;
        addr_l = XPCIE_FPGA_CHAIN_STAT_EGR_RCV_DATA_1_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_EGR_RCV_DATA_1_VALUE_H;
      }
      break;
    case CHAIN_STAT_EGR_SND0:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_SESSION;
        addr_l = XPCIE_FPGA_CHAIN_STAT_EGR_SND_DATA_0_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_EGR_SND_DATA_0_VALUE_H;
      }
      break;
    case CHAIN_STAT_EGR_SND1:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_SESSION;
        addr_l = XPCIE_FPGA_CHAIN_STAT_EGR_SND_DATA_1_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_EGR_SND_DATA_1_VALUE_H;
      }
      break;
    case CHAIN_STAT_INGR_DISCARD0:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL;
        addr_l = XPCIE_FPGA_CHAIN_STAT_INGR_DISCARD_DATA_0_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_INGR_DISCARD_DATA_0_VALUE_H;
      }
      break;
    case CHAIN_STAT_INGR_DISCARD1:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL;
        addr_l = XPCIE_FPGA_CHAIN_STAT_INGR_DISCARD_DATA_1_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_INGR_DISCARD_DATA_1_VALUE_H;
      }
      break;
    case CHAIN_STAT_EGR_DISCARD0:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL;
        addr_l = XPCIE_FPGA_CHAIN_STAT_EGR_DISCARD_DATA_0_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_EGR_DISCARD_DATA_0_VALUE_H;
      }
      break;
    case CHAIN_STAT_EGR_DISCARD1:
      {
        addr_w = XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL;
        addr_l = XPCIE_FPGA_CHAIN_STAT_EGR_DISCARD_DATA_1_VALUE_L;
        addr_h = XPCIE_FPGA_CHAIN_STAT_EGR_DISCARD_DATA_1_VALUE_H;
      }
      break;
    default:
      // Do Nothing
      xpcie_err("reg_id(%u) is not the expected value.\n", bytenum->reg_id);
      return;
  }

  chain_reg_write(dev, addr_w, bytenum->lane, bytenum->cid_fchid);
  value_l = chain_reg_read(dev, addr_l, bytenum->lane);
  value_h = chain_reg_read(dev, addr_h, bytenum->lane);
  bytenum->byte_num =  (uint64_t)value_l;
  bytenum->byte_num |= ((uint64_t)value_h & 0xFFFFFFFF) << 32;
}


void
xpcie_fpga_get_chain_frames(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_framenum_t *framenum)
{
  uint64_t addr;

  switch(framenum->reg_id) {
    case CHAIN_STAT_INGR_SND0:
      {
      	addr = XPCIE_FPGA_CHAIN_STAT_INGR_SND_FRAME_0_VALUE;
      }
      break;
    case CHAIN_STAT_INGR_SND1:
      {
        addr = XPCIE_FPGA_CHAIN_STAT_INGR_SND_FRAME_1_VALUE;
      }
      break;
    case CHAIN_STAT_EGR_RCV0:
      {
        addr = XPCIE_FPGA_CHAIN_STAT_EGR_RCV_FRAME_0_VALUE;
      }
      break;
    case CHAIN_STAT_EGR_RCV1:
      {
        addr = XPCIE_FPGA_CHAIN_STAT_EGR_RCV_FRAME_1_VALUE;
      }
      break;
    default:
      // Do Nothing
      xpcie_err("reg_id(%u) is not the expected value.\n", framenum->reg_id);
      return;
  }

  chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL, framenum->lane, framenum->fchid);
  framenum->frame_num = chain_reg_read(dev, addr, framenum->lane);
}


void
xpcie_fpga_get_chain_buff(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_framenum_t *framenum)
{
  chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL, framenum->lane, framenum->fchid);
  framenum->frame_num = chain_reg_read(dev, XPCIE_FPGA_CHAIN_STAT_HEADER_BUFF_STORED, framenum->lane);
}


void
xpcie_fpga_get_chain_bp(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_framenum_t *framenum)
{
  chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL, framenum->lane, framenum->fchid);
  framenum->frame_num = chain_reg_read(dev, XPCIE_FPGA_CHAIN_STAT_HEADER_BUFF_BP, framenum->lane);
}


void
xpcie_fpga_clear_chain_bp(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_framenum_t *framenum)
{
  chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL, framenum->lane, framenum->fchid);
  chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_HEADER_BUFF_BP, framenum->lane, framenum->frame_num);
}


void
xpcie_fpga_get_chain_busy(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_framenum_t *busy)
{
  chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL, busy->lane, busy->fchid);
  busy->frame_num = chain_reg_read(dev, XPCIE_FPGA_CHAIN_STAT_EGR_BUSY, busy->lane);
}


void
xpcie_fpga_check_chain_err(
  fpga_dev_info_t *dev,
  fpga_ioctl_err_all_t *err)
{
  err->err_all = chain_reg_read(dev, XPCIE_FPGA_CHAIN_DETECT_FAULT, err->lane);
}


void
xpcie_fpga_detect_chain_err(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_t *chain_err)
{
  uint32_t value;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_SESSION, chain_err->lane, chain_err->cid_fchid);

    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_0_VALUE, chain_err->lane);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_1_VALUE, chain_err->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_SESSION, chain_err->lane, chain_err->cid_fchid);

    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_0_VALUE, chain_err->lane);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_1_VALUE, chain_err->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }

  chain_err->header_marker         = (uint8_t)(value  & 0x00000001);
  chain_err->payload_len           = (uint8_t)((value & 0x00000002) >> 1);
  chain_err->header_len            = (uint8_t)((value & 0x00000004) >> 2);
  chain_err->header_chksum         = (uint8_t)((value & 0x00000008) >> 3);
  chain_err->header_stat           = (uint8_t)((value & 0x00000FF0) >> 4);
  chain_err->pointer_table_miss    = (uint8_t)((value & 0x00001000) >> 12);
  chain_err->payload_table_miss    = (uint8_t)((value & 0x00002000) >> 13);
  chain_err->pointer_table_invalid = (uint8_t)((value & 0x00010000) >> 16);
  chain_err->payload_table_invalid = (uint8_t)((value & 0x00020000) >> 17);
}


void
xpcie_fpga_mask_chain_err(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_t *chain_err)
{
  uint32_t value;

  value =  ((uint32_t)chain_err->header_marker         & 0x01);
  value |= ((uint32_t)chain_err->payload_len           & 0x01) << 1;
  value |= ((uint32_t)chain_err->header_len            & 0x01) << 2;
  value |= ((uint32_t)chain_err->header_chksum         & 0x01) << 3;
  value |= ((uint32_t)chain_err->header_stat           & 0xFF) << 4;
  value |= ((uint32_t)chain_err->pointer_table_miss    & 0x01) << 12;
  value |= ((uint32_t)chain_err->payload_table_miss    & 0x01) << 13;
  value |= ((uint32_t)chain_err->pointer_table_invalid & 0x01) << 16;
  value |= ((uint32_t)chain_err->payload_table_invalid & 0x01) << 17;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_0_MASK, chain_err->lane, value);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_1_MASK, chain_err->lane, value);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_0_MASK, chain_err->lane, value);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_1_MASK, chain_err->lane, value);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }
}


void
xpcie_fpga_get_mask_chain_err(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_t *chain_err)
{
  uint32_t value;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_0_MASK, chain_err->lane);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_1_MASK, chain_err->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_0_MASK, chain_err->lane);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_1_MASK, chain_err->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }

  chain_err->header_marker         = (uint8_t)(value  & 0x00000001);
  chain_err->payload_len           = (uint8_t)((value & 0x00000002) >> 1);
  chain_err->header_len            = (uint8_t)((value & 0x00000004) >> 2);
  chain_err->header_chksum         = (uint8_t)((value & 0x00000008) >> 3);
  chain_err->header_stat           = (uint8_t)((value & 0x00000FF0) >> 4);
  chain_err->pointer_table_miss    = (uint8_t)((value & 0x00001000) >> 12);
  chain_err->payload_table_miss    = (uint8_t)((value & 0x00002000) >> 13);
  chain_err->pointer_table_invalid = (uint8_t)((value & 0x00010000) >> 16);
  chain_err->payload_table_invalid = (uint8_t)((value & 0x00020000) >> 17);
}


void
xpcie_fpga_force_chain_err(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_t *chain_err)
{
  uint32_t value;

  value =  ((uint32_t)chain_err->header_marker         & 0x01);
  value |= ((uint32_t)chain_err->payload_len           & 0x01) << 1;
  value |= ((uint32_t)chain_err->header_len            & 0x01) << 2;
  value |= ((uint32_t)chain_err->header_chksum         & 0x01) << 3;
  value |= ((uint32_t)chain_err->header_stat           & 0xFF) << 4;
  value |= ((uint32_t)chain_err->pointer_table_miss    & 0x01) << 12;
  value |= ((uint32_t)chain_err->payload_table_miss    & 0x01) << 13;
  value |= ((uint32_t)chain_err->pointer_table_invalid & 0x01) << 16;
  value |= ((uint32_t)chain_err->payload_table_invalid & 0x01) << 17;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_0_FORCE, chain_err->lane, value);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_1_FORCE, chain_err->lane, value);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_0_FORCE, chain_err->lane, value);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_1_FORCE, chain_err->lane, value);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }
}


void
xpcie_fpga_get_force_chain_err(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_t *chain_err)
{
  uint32_t value;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_0_FORCE, chain_err->lane);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_1_FORCE, chain_err->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_0_FORCE, chain_err->lane);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_1_FORCE, chain_err->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }
  chain_err->header_marker         = (uint8_t)(value  & 0x00000001);
  chain_err->payload_len           = (uint8_t)((value & 0x00000002) >> 1);
  chain_err->header_len            = (uint8_t)((value & 0x00000004) >> 2);
  chain_err->header_chksum         = (uint8_t)((value & 0x00000008) >> 3);
  chain_err->header_stat           = (uint8_t)((value & 0x00000FF0) >> 4);
  chain_err->pointer_table_miss    = (uint8_t)((value & 0x00001000) >> 12);
  chain_err->payload_table_miss    = (uint8_t)((value & 0x00002000) >> 13);
  chain_err->pointer_table_invalid = (uint8_t)((value & 0x00010000) >> 16);
  chain_err->payload_table_invalid = (uint8_t)((value & 0x00020000) >> 17);
}


void
xpcie_fpga_detect_chain_err_table(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_table_t *chain_err)
{
  uint32_t value;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_SESSION, chain_err->lane, chain_err->cid_fchid);

    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_SND_DETECT_FAULT_0_VALUE, chain_err->lane);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_SND_DETECT_FAULT_1_VALUE, chain_err->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_CHANNEL, chain_err->lane, chain_err->cid_fchid);

    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_RCV_DETECT_FAULT_VALUE, chain_err->lane);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }

  chain_err->con_table_miss    = (uint8_t)(value & 0x00000001);
  chain_err->con_table_invalid = (uint8_t)((value & 0x00010000) >> 16);
}


void
xpcie_fpga_mask_chain_err_table(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_table_t *chain_err)
{
  uint32_t value;

  value =  ((uint32_t)chain_err->con_table_miss    & 0x01);
  value |= ((uint32_t)chain_err->con_table_invalid & 0x01) << 16;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_SND_DETECT_FAULT_0_MASK, chain_err->lane, value);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_SND_DETECT_FAULT_1_MASK, chain_err->lane, value);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_RCV_DETECT_FAULT_MASK, chain_err->lane, value);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }
}


void
xpcie_fpga_get_mask_chain_err_table(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_table_t *chain_err)
{
  uint32_t value;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_SND_DETECT_FAULT_0_MASK, chain_err->lane);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_SND_DETECT_FAULT_1_MASK, chain_err->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_RCV_DETECT_FAULT_MASK, chain_err->lane);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }

  chain_err->con_table_miss    = (uint8_t)(value & 0x00000001);
  chain_err->con_table_invalid = (uint8_t)((value & 0x00010000) >> 16);
}


void
xpcie_fpga_force_chain_err_table(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_table_t *chain_err)
{
  uint32_t value;

  value =  ((uint32_t)chain_err->con_table_miss    & 0x01);
  value |= ((uint32_t)chain_err->con_table_invalid & 0x01) << 16;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_SND_DETECT_FAULT_0_FORCE, chain_err->lane, value);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_SND_DETECT_FAULT_1_FORCE, chain_err->lane, value);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_RCV_DETECT_FAULT_FORCE, chain_err->lane, value);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }
}


void
xpcie_fpga_get_force_chain_err_table(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_table_t *chain_err)
{
  uint32_t value;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_SND_DETECT_FAULT_0_FORCE, chain_err->lane);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_SND_DETECT_FAULT_1_FORCE, chain_err->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_RCV_DETECT_FAULT_FORCE, chain_err->lane);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }
  chain_err->con_table_miss    = (uint8_t)(value & 0x00000001);
  chain_err->con_table_invalid = (uint8_t)((value & 0x00010000) >> 16);
}


void
xpcie_fpga_ins_chain_err(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_t *chain_err)
{
  uint32_t value;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    value =  ((uint32_t)chain_err->pointer_table_invalid & 0x01);
    value |= ((uint32_t)chain_err->payload_table_invalid & 0x01) << 1;
    value |= ((uint32_t)chain_err->con_table_invalid     & 0x01) << 2;
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_0_INS, chain_err->lane, value);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_1_INS, chain_err->lane, value);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    value =  ((uint32_t)chain_err->header_marker         & 0x01);
    value |= ((uint32_t)chain_err->payload_len           & 0x01) << 1;
    value |= ((uint32_t)chain_err->header_len            & 0x01) << 2;
    value |= ((uint32_t)chain_err->header_chksum         & 0x01) << 3;
    value |= ((uint32_t)chain_err->header_stat           & 0xFF) << 4;
    value |= ((uint32_t)chain_err->pointer_table_invalid & 0x01) << 16;
    value |= ((uint32_t)chain_err->payload_table_invalid & 0x01) << 17;
    value |= ((uint32_t)chain_err->con_table_invalid     & 0x01) << 18;
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_0_INS, chain_err->lane, value);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_1_INS, chain_err->lane, value);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }
}


void
xpcie_fpga_get_ins_chain_err(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_t *chain_err)
{
  uint32_t value;

  if (chain_err->dir == FPGA_CID_KIND_INGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_0_INS, chain_err->lane);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_RCV_DETECT_FAULT_1_INS, chain_err->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
    chain_err->pointer_table_invalid = (uint8_t)(value & 0x00000001);
    chain_err->payload_table_invalid = (uint8_t)((value & 0x00000002) >> 1);
    chain_err->con_table_invalid     = (uint8_t)((value & 0x00000004) >> 2);
  }
  else if (chain_err->dir == FPGA_CID_KIND_EGRESS) {
    if (chain_err->extif_id == FPGA_EXTIF_NUMBER_0) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_0_INS, chain_err->lane);
    } else if (chain_err->extif_id == FPGA_EXTIF_NUMBER_1) {
      value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_SND_DETECT_FAULT_1_INS, chain_err->lane);
    } else {
      /* Do Nothing */
      xpcie_err("extif_id(%u) is not the expected value.\n", chain_err->extif_id);
    }
    chain_err->header_marker         = (uint8_t)(value  & 0x00000001);
    chain_err->payload_len           = (uint8_t)((value & 0x00000002) >> 1);
    chain_err->header_len            = (uint8_t)((value & 0x00000004) >> 2);
    chain_err->header_chksum         = (uint8_t)((value & 0x00000008) >> 3);
    chain_err->header_stat           = (uint8_t)((value & 0x00000FF0) >> 4);
    chain_err->pointer_table_invalid = (uint8_t)((value & 0x00010000) >> 16);
    chain_err->payload_table_invalid = (uint8_t)((value & 0x00020000) >> 17);
    chain_err->con_table_invalid     = (uint8_t)((value & 0x00040000) >> 18);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err->dir);
  }
}


void
xpcie_fpga_detect_chain_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_prot_t *chain_err_prot)
{
  uint32_t value;

  if (chain_err_prot->dir == FPGA_CID_KIND_INGRESS) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_SND_PROTOCOL_FAULT, chain_err_prot->lane);
  }
  else if (chain_err_prot->dir == FPGA_CID_KIND_EGRESS) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_RCV_PROTOCOL_FAULT, chain_err_prot->lane);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err_prot->dir);
  }

  chain_err_prot->prot_ch               = (uint8_t)(value  & 0x00000001);
  chain_err_prot->prot_len              = (uint8_t)((value & 0x00000002) >> 1);
  chain_err_prot->prot_sof              = (uint8_t)((value & 0x00000004) >> 2);
  chain_err_prot->prot_eof              = (uint8_t)((value & 0x00000008) >> 3);
  chain_err_prot->prot_reqresp          = (uint8_t)((value & 0x00000010) >> 4);
  chain_err_prot->prot_datanum          = (uint8_t)((value & 0x00000020) >> 5);
  chain_err_prot->prot_req_outstanding  = (uint8_t)((value & 0x00000040) >> 6);
  chain_err_prot->prot_resp_outstanding = (uint8_t)((value & 0x00000080) >> 7);
  chain_err_prot->prot_max_datanum      = (uint8_t)((value & 0x00000100) >> 8);
  chain_err_prot->prot_reqlen           = (uint8_t)((value & 0x00001000) >> 12);
  chain_err_prot->prot_reqresplen       = (uint8_t)((value & 0x00002000) >> 13);
}


void
xpcie_fpga_clear_chain_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_prot_t *chain_err_prot)
{
  uint32_t value;

  value =  ((uint32_t)chain_err_prot->prot_ch               & 0x01);
  value |= ((uint32_t)chain_err_prot->prot_len              & 0x01) << 1;
  value |= ((uint32_t)chain_err_prot->prot_sof              & 0x01) << 2;
  value |= ((uint32_t)chain_err_prot->prot_eof              & 0x01) << 3;
  value |= ((uint32_t)chain_err_prot->prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)chain_err_prot->prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)chain_err_prot->prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)chain_err_prot->prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)chain_err_prot->prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)chain_err_prot->prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)chain_err_prot->prot_reqresplen       & 0x01) << 13;

  if (chain_err_prot->dir == FPGA_CID_KIND_INGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_SND_PROTOCOL_FAULT, chain_err_prot->lane, value);
  }
  else if (chain_err_prot->dir == FPGA_CID_KIND_EGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_RCV_PROTOCOL_FAULT, chain_err_prot->lane, value);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err_prot->dir);
  }
}


void
xpcie_fpga_mask_chain_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_prot_t *chain_err_prot)
{
  uint32_t value;

  value =  ((uint32_t)chain_err_prot->prot_ch               & 0x01);
  value |= ((uint32_t)chain_err_prot->prot_len              & 0x01) << 1;
  value |= ((uint32_t)chain_err_prot->prot_sof              & 0x01) << 2;
  value |= ((uint32_t)chain_err_prot->prot_eof              & 0x01) << 3;
  value |= ((uint32_t)chain_err_prot->prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)chain_err_prot->prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)chain_err_prot->prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)chain_err_prot->prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)chain_err_prot->prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)chain_err_prot->prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)chain_err_prot->prot_reqresplen       & 0x01) << 13;

  if (chain_err_prot->dir == FPGA_CID_KIND_INGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_SND_PROTOCOL_FAULT_MASK, chain_err_prot->lane, value);
  }
  else if (chain_err_prot->dir == FPGA_CID_KIND_EGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_RCV_PROTOCOL_FAULT_MASK, chain_err_prot->lane, value);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err_prot->dir);
  }
}


void
xpcie_fpga_get_mask_chain_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_prot_t *chain_err_prot)
{
  uint32_t value;

  if (chain_err_prot->dir == FPGA_CID_KIND_INGRESS) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_SND_PROTOCOL_FAULT_MASK, chain_err_prot->lane);
  }
  else if (chain_err_prot->dir == FPGA_CID_KIND_EGRESS) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_RCV_PROTOCOL_FAULT_MASK, chain_err_prot->lane);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err_prot->dir);
  }

  chain_err_prot->prot_ch               = (uint8_t)(value  & 0x00000001);
  chain_err_prot->prot_len              = (uint8_t)((value & 0x00000002) >> 1);
  chain_err_prot->prot_sof              = (uint8_t)((value & 0x00000004) >> 2);
  chain_err_prot->prot_eof              = (uint8_t)((value & 0x00000008) >> 3);
  chain_err_prot->prot_reqresp          = (uint8_t)((value & 0x00000010) >> 4);
  chain_err_prot->prot_datanum          = (uint8_t)((value & 0x00000020) >> 5);
  chain_err_prot->prot_req_outstanding  = (uint8_t)((value & 0x00000040) >> 6);
  chain_err_prot->prot_resp_outstanding = (uint8_t)((value & 0x00000080) >> 7);
  chain_err_prot->prot_max_datanum      = (uint8_t)((value & 0x00000100) >> 8);
  chain_err_prot->prot_reqlen           = (uint8_t)((value & 0x00001000) >> 12);
  chain_err_prot->prot_reqresplen       = (uint8_t)((value & 0x00002000) >> 13);
}


void
xpcie_fpga_force_chain_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_prot_t *chain_err_prot)
{
  uint32_t value;

  value =  ((uint32_t)chain_err_prot->prot_ch               & 0x01);
  value |= ((uint32_t)chain_err_prot->prot_len              & 0x01) << 1;
  value |= ((uint32_t)chain_err_prot->prot_sof              & 0x01) << 2;
  value |= ((uint32_t)chain_err_prot->prot_eof              & 0x01) << 3;
  value |= ((uint32_t)chain_err_prot->prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)chain_err_prot->prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)chain_err_prot->prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)chain_err_prot->prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)chain_err_prot->prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)chain_err_prot->prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)chain_err_prot->prot_reqresplen       & 0x01) << 13;

  if (chain_err_prot->dir == FPGA_CID_KIND_INGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_SND_PROTOCOL_FAULT_FORCE, chain_err_prot->lane, value);
  }
  else if (chain_err_prot->dir == FPGA_CID_KIND_EGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_RCV_PROTOCOL_FAULT_FORCE, chain_err_prot->lane, value);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err_prot->dir);
  }
}


void
xpcie_fpga_get_force_chain_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_prot_t *chain_err_prot)
{
  uint32_t value;

  if (chain_err_prot->dir == FPGA_CID_KIND_INGRESS) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_SND_PROTOCOL_FAULT_FORCE, chain_err_prot->lane);
  }
  else if (chain_err_prot->dir == FPGA_CID_KIND_EGRESS) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_RCV_PROTOCOL_FAULT_FORCE, chain_err_prot->lane);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err_prot->dir);
  }

  chain_err_prot->prot_ch               = (uint8_t)(value  & 0x00000001);
  chain_err_prot->prot_len              = (uint8_t)((value & 0x00000002) >> 1);
  chain_err_prot->prot_sof              = (uint8_t)((value & 0x00000004) >> 2);
  chain_err_prot->prot_eof              = (uint8_t)((value & 0x00000008) >> 3);
  chain_err_prot->prot_reqresp          = (uint8_t)((value & 0x00000010) >> 4);
  chain_err_prot->prot_datanum          = (uint8_t)((value & 0x00000020) >> 5);
  chain_err_prot->prot_req_outstanding  = (uint8_t)((value & 0x00000040) >> 6);
  chain_err_prot->prot_resp_outstanding = (uint8_t)((value & 0x00000080) >> 7);
  chain_err_prot->prot_max_datanum      = (uint8_t)((value & 0x00000100) >> 8);
  chain_err_prot->prot_reqlen           = (uint8_t)((value & 0x00001000) >> 12);
  chain_err_prot->prot_reqresplen       = (uint8_t)((value & 0x00002000) >> 13);
}


void
xpcie_fpga_ins_chain_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_prot_t *chain_err_prot)
{
  uint32_t value;

  value =  ((uint32_t)chain_err_prot->prot_ch               & 0x01);
  value |= ((uint32_t)chain_err_prot->prot_len              & 0x01) << 1;
  value |= ((uint32_t)chain_err_prot->prot_sof              & 0x01) << 2;
  value |= ((uint32_t)chain_err_prot->prot_eof              & 0x01) << 3;
  value |= ((uint32_t)chain_err_prot->prot_reqresp          & 0x01) << 4;
  value |= ((uint32_t)chain_err_prot->prot_datanum          & 0x01) << 5;
  value |= ((uint32_t)chain_err_prot->prot_req_outstanding  & 0x01) << 6;
  value |= ((uint32_t)chain_err_prot->prot_resp_outstanding & 0x01) << 7;
  value |= ((uint32_t)chain_err_prot->prot_max_datanum      & 0x01) << 8;
  value |= ((uint32_t)chain_err_prot->prot_reqlen           & 0x01) << 12;
  value |= ((uint32_t)chain_err_prot->prot_reqresplen       & 0x01) << 13;

  if (chain_err_prot->dir == FPGA_CID_KIND_INGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_INGR_SND_PROTOCOL_FAULT_INS, chain_err_prot->lane, value);
  }
  else if (chain_err_prot->dir == FPGA_CID_KIND_EGRESS) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EGR_RCV_PROTOCOL_FAULT_INS, chain_err_prot->lane, value);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err_prot->dir);
  }
}


void
xpcie_fpga_get_ins_chain_err_prot(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_prot_t *chain_err_prot)
{
  uint32_t value;

  if (chain_err_prot->dir == FPGA_CID_KIND_INGRESS) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_INGR_SND_PROTOCOL_FAULT_INS, chain_err_prot->lane);
  }
  else if (chain_err_prot->dir == FPGA_CID_KIND_EGRESS) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EGR_RCV_PROTOCOL_FAULT_INS, chain_err_prot->lane);
  } else {
    /* Do Nothing */
    xpcie_err("dir(%u) is not the expected value.\n", chain_err_prot->dir);
  }

  chain_err_prot->prot_ch               = (uint8_t)(value  & 0x00000001);
  chain_err_prot->prot_len              = (uint8_t)((value & 0x00000002) >> 1);
  chain_err_prot->prot_sof              = (uint8_t)((value & 0x00000004) >> 2);
  chain_err_prot->prot_eof              = (uint8_t)((value & 0x00000008) >> 3);
  chain_err_prot->prot_reqresp          = (uint8_t)((value & 0x00000010) >> 4);
  chain_err_prot->prot_datanum          = (uint8_t)((value & 0x00000020) >> 5);
  chain_err_prot->prot_req_outstanding  = (uint8_t)((value & 0x00000040) >> 6);
  chain_err_prot->prot_resp_outstanding = (uint8_t)((value & 0x00000080) >> 7);
  chain_err_prot->prot_max_datanum      = (uint8_t)((value & 0x00000100) >> 8);
  chain_err_prot->prot_reqlen           = (uint8_t)((value & 0x00001000) >> 12);
  chain_err_prot->prot_reqresplen       = (uint8_t)((value & 0x00002000) >> 13);
}


void
xpcie_fpga_detect_chain_err_evt(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_evt_t *chain_err_evt)
{
  uint32_t value;

  if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_0) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EXTIF0_EVENT_FAULT, chain_err_evt->lane);
  }
  else if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_1) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EXTIF1_EVENT_FAULT, chain_err_evt->lane);
  } else {
    /* Do Nothing */
    xpcie_err("extif_id(%u) is not the expected value.\n", chain_err_evt->extif_id);
  }

  chain_err_evt->established      = (uint8_t)(value  & 0x00000001);
  chain_err_evt->close_wait       = (uint8_t)((value & 0x00000002) >> 1);
  chain_err_evt->erased           = (uint8_t)((value & 0x00000004) >> 2);
  chain_err_evt->syn_timeout      = (uint8_t)((value & 0x00000008) >> 3);
  chain_err_evt->syn_ack_timeout  = (uint8_t)((value & 0x00000010) >> 4);
  chain_err_evt->timeout          = (uint8_t)((value & 0x00000020) >> 5);
  chain_err_evt->recv_data        = (uint8_t)((value & 0x00000040) >> 6);
  chain_err_evt->send_data        = (uint8_t)((value & 0x00000080) >> 7);
  chain_err_evt->recv_urgent_data = (uint8_t)((value & 0x00000100) >> 8);
  chain_err_evt->recv_rst         = (uint8_t)((value & 0x00000200) >> 9);
}


void
xpcie_fpga_clear_chain_err_evt(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_evt_t *chain_err_evt)
{
  uint32_t value;

  value =  ((uint32_t)chain_err_evt->established      & 0x01);
  value |= ((uint32_t)chain_err_evt->close_wait       & 0x01) << 1;
  value |= ((uint32_t)chain_err_evt->erased           & 0x01) << 2;
  value |= ((uint32_t)chain_err_evt->syn_timeout      & 0x01) << 3;
  value |= ((uint32_t)chain_err_evt->syn_ack_timeout  & 0x01) << 4;
  value |= ((uint32_t)chain_err_evt->timeout          & 0x01) << 5;
  value |= ((uint32_t)chain_err_evt->recv_data        & 0x01) << 6;
  value |= ((uint32_t)chain_err_evt->send_data        & 0x01) << 7;
  value |= ((uint32_t)chain_err_evt->recv_urgent_data & 0x01) << 8;
  value |= ((uint32_t)chain_err_evt->recv_rst         & 0x01) << 9;

  if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_0) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EXTIF0_EVENT_FAULT, chain_err_evt->lane, value);
  }
  else if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_1) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EXTIF1_EVENT_FAULT, chain_err_evt->lane, value);
  } else {
    /* Do Nothing */
    xpcie_err("extif_id(%u) is not the expected value.\n", chain_err_evt->extif_id);
  }
}


void
xpcie_fpga_mask_chain_err_evt(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_evt_t *chain_err_evt)
{
  uint32_t value;

  value =  ((uint32_t)chain_err_evt->established      & 0x01);
  value |= ((uint32_t)chain_err_evt->close_wait       & 0x01) << 1;
  value |= ((uint32_t)chain_err_evt->erased           & 0x01) << 2;
  value |= ((uint32_t)chain_err_evt->syn_timeout      & 0x01) << 3;
  value |= ((uint32_t)chain_err_evt->syn_ack_timeout  & 0x01) << 4;
  value |= ((uint32_t)chain_err_evt->timeout          & 0x01) << 5;
  value |= ((uint32_t)chain_err_evt->recv_data        & 0x01) << 6;
  value |= ((uint32_t)chain_err_evt->send_data        & 0x01) << 7;
  value |= ((uint32_t)chain_err_evt->recv_urgent_data & 0x01) << 8;
  value |= ((uint32_t)chain_err_evt->recv_rst         & 0x01) << 9;

  if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_0) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EXTIF0_EVENT_FAULT_MASK, chain_err_evt->lane, value);
  }
  else if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_1) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EXTIF1_EVENT_FAULT_MASK, chain_err_evt->lane, value);
  } else {
    /* Do Nothing */
    xpcie_err("extif_id(%u) is not the expected value.\n", chain_err_evt->extif_id);
  }
}


void
xpcie_fpga_get_mask_chain_err_evt(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_evt_t *chain_err_evt)
{
  uint32_t value;

  if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_0) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EXTIF0_EVENT_FAULT_MASK, chain_err_evt->lane);
  }
  else if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_1) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EXTIF1_EVENT_FAULT_MASK, chain_err_evt->lane);
  } else {
    /* Do Nothing */
    xpcie_err("extif_id(%u) is not the expected value.\n", chain_err_evt->extif_id);
  }

  chain_err_evt->established      = (uint8_t)(value  & 0x00000001);
  chain_err_evt->close_wait       = (uint8_t)((value & 0x00000002) >> 1);
  chain_err_evt->erased           = (uint8_t)((value & 0x00000004) >> 2);
  chain_err_evt->syn_timeout      = (uint8_t)((value & 0x00000008) >> 3);
  chain_err_evt->syn_ack_timeout  = (uint8_t)((value & 0x00000010) >> 4);
  chain_err_evt->timeout          = (uint8_t)((value & 0x00000020) >> 5);
  chain_err_evt->recv_data        = (uint8_t)((value & 0x00000040) >> 6);
  chain_err_evt->send_data        = (uint8_t)((value & 0x00000080) >> 7);
  chain_err_evt->recv_urgent_data = (uint8_t)((value & 0x00000100) >> 8);
  chain_err_evt->recv_rst         = (uint8_t)((value & 0x00000200) >> 9);
}


void
xpcie_fpga_force_chain_err_evt(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_evt_t *chain_err_evt)
{
  uint32_t value;

  value =  ((uint32_t)chain_err_evt->established      & 0x01);
  value |= ((uint32_t)chain_err_evt->close_wait       & 0x01) << 1;
  value |= ((uint32_t)chain_err_evt->erased           & 0x01) << 2;
  value |= ((uint32_t)chain_err_evt->syn_timeout      & 0x01) << 3;
  value |= ((uint32_t)chain_err_evt->syn_ack_timeout  & 0x01) << 4;
  value |= ((uint32_t)chain_err_evt->timeout          & 0x01) << 5;
  value |= ((uint32_t)chain_err_evt->recv_data        & 0x01) << 6;
  value |= ((uint32_t)chain_err_evt->send_data        & 0x01) << 7;
  value |= ((uint32_t)chain_err_evt->recv_urgent_data & 0x01) << 8;
  value |= ((uint32_t)chain_err_evt->recv_rst         & 0x01) << 9;

  if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_0) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EXTIF0_EVENT_FAULT_FORCE, chain_err_evt->lane, value);
  }
  else if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_1) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EXTIF1_EVENT_FAULT_FORCE, chain_err_evt->lane, value);
  } else {
    /* Do Nothing */
    xpcie_err("extif_id(%u) is not the expected value.\n", chain_err_evt->extif_id);
  }
}


void
xpcie_fpga_get_force_chain_err_evt(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_evt_t *chain_err_evt)
{
  uint32_t value;

  if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_0) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EXTIF0_EVENT_FAULT_FORCE, chain_err_evt->lane);
  }
  else if (chain_err_evt->extif_id == FPGA_EXTIF_NUMBER_1) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EXTIF1_EVENT_FAULT_FORCE, chain_err_evt->lane);
  } else {
    /* Do Nothing */
    xpcie_err("extif_id(%u) is not the expected value.\n", chain_err_evt->extif_id);
  }

  chain_err_evt->established      = (uint8_t)(value  & 0x00000001);
  chain_err_evt->close_wait       = (uint8_t)((value & 0x00000002) >> 1);
  chain_err_evt->erased           = (uint8_t)((value & 0x00000004) >> 2);
  chain_err_evt->syn_timeout      = (uint8_t)((value & 0x00000008) >> 3);
  chain_err_evt->syn_ack_timeout  = (uint8_t)((value & 0x00000010) >> 4);
  chain_err_evt->timeout          = (uint8_t)((value & 0x00000020) >> 5);
  chain_err_evt->recv_data        = (uint8_t)((value & 0x00000040) >> 6);
  chain_err_evt->send_data        = (uint8_t)((value & 0x00000080) >> 7);
  chain_err_evt->recv_urgent_data = (uint8_t)((value & 0x00000100) >> 8);
  chain_err_evt->recv_rst         = (uint8_t)((value & 0x00000200) >> 9);
}


void
xpcie_fpga_detect_chain_err_stif(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_stif_t *chain_err_stif)
{
  uint32_t value;

  value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_STREAMIF_STALL, chain_err_stif->lane);

  chain_err_stif->ingress_req   = (uint8_t)(value  & 0x00000001);
  chain_err_stif->ingress_resp  = (uint8_t)((value & 0x00000002) >> 1);
  chain_err_stif->ingress_data  = (uint8_t)((value & 0x00000004) >> 2);
  chain_err_stif->egress_req    = (uint8_t)((value & 0x00000008) >> 3);
  chain_err_stif->egress_resp   = (uint8_t)((value & 0x00000010) >> 4);
  chain_err_stif->egress_data   = (uint8_t)((value & 0x00000020) >> 5);
  chain_err_stif->extif_event   = (uint8_t)((value & 0x00000040) >> 6); // extif0 event
  chain_err_stif->extif_command = (uint8_t)((value & 0x00000080) >> 7); // extif0 command
  chain_err_stif->extif_event   |= (uint8_t)((value & 0x00000100) >> 7); // extif1 event
  chain_err_stif->extif_command |= (uint8_t)((value & 0x00000200) >> 8); // extif1 command"
}


void
xpcie_fpga_mask_chain_err_stif(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_stif_t *chain_err_stif)
{
  uint32_t value;

  value =  ((uint32_t)chain_err_stif->ingress_req   & 0x01);
  value |= ((uint32_t)chain_err_stif->ingress_resp  & 0x01) << 1;
  value |= ((uint32_t)chain_err_stif->ingress_data  & 0x01) << 2;
  value |= ((uint32_t)chain_err_stif->egress_req    & 0x01) << 3;
  value |= ((uint32_t)chain_err_stif->egress_resp   & 0x01) << 4;
  value |= ((uint32_t)chain_err_stif->egress_data   & 0x01) << 5;
  value |= ((uint32_t)chain_err_stif->extif_event   & 0x01) << 6;
  value |= ((uint32_t)chain_err_stif->extif_command & 0x01) << 7;
  value |= ((uint32_t)chain_err_stif->extif_event   & 0x02) << 7;
  value |= ((uint32_t)chain_err_stif->extif_command & 0x02) << 8;

  chain_reg_write(dev, XPCIE_FPGA_CHAIN_STREAMIF_STALL_MASK, chain_err_stif->lane, value);
}


void
xpcie_fpga_get_mask_chain_err_stif(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_stif_t *chain_err_stif)
{
  uint32_t value;

  value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_STREAMIF_STALL_MASK, chain_err_stif->lane);

  chain_err_stif->ingress_req   = (uint8_t)(value  & 0x00000001);
  chain_err_stif->ingress_resp  = (uint8_t)((value & 0x00000002) >> 1);
  chain_err_stif->ingress_data  = (uint8_t)((value & 0x00000004) >> 2);
  chain_err_stif->egress_req    = (uint8_t)((value & 0x00000008) >> 3);
  chain_err_stif->egress_resp   = (uint8_t)((value & 0x00000010) >> 4);
  chain_err_stif->egress_data   = (uint8_t)((value & 0x00000020) >> 5);
  chain_err_stif->extif_event   = (uint8_t)((value & 0x00000040) >> 6);
  chain_err_stif->extif_command = (uint8_t)((value & 0x00000080) >> 7);
  chain_err_stif->extif_event   |= (uint8_t)((value & 0x00000100) >> 7);
  chain_err_stif->extif_command |= (uint8_t)((value & 0x00000200) >> 8);
}


void
xpcie_fpga_force_chain_err_stif(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_stif_t *chain_err_stif)
{
  uint32_t value;

  value =  ((uint32_t)chain_err_stif->ingress_req   & 0x01);
  value |= ((uint32_t)chain_err_stif->ingress_resp  & 0x01) << 1;
  value |= ((uint32_t)chain_err_stif->ingress_data  & 0x01) << 2;
  value |= ((uint32_t)chain_err_stif->egress_req    & 0x01) << 3;
  value |= ((uint32_t)chain_err_stif->egress_resp   & 0x01) << 4;
  value |= ((uint32_t)chain_err_stif->egress_data   & 0x01) << 5;
  value |= ((uint32_t)chain_err_stif->extif_event   & 0x01) << 6;
  value |= ((uint32_t)chain_err_stif->extif_command & 0x01) << 7;
  value |= ((uint32_t)chain_err_stif->extif_event   & 0x02) << 7;
  value |= ((uint32_t)chain_err_stif->extif_command & 0x02) << 8;

  chain_reg_write(dev, XPCIE_FPGA_CHAIN_STREAMIF_STALL_FORCE, chain_err_stif->lane, value);
}


void
xpcie_fpga_get_force_chain_err_stif(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_stif_t *chain_err_stif)
{
  uint32_t value;

  value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_STREAMIF_STALL_FORCE, chain_err_stif->lane);

  chain_err_stif->ingress_req   = (uint8_t)(value  & 0x00000001);
  chain_err_stif->ingress_resp  = (uint8_t)((value & 0x00000002) >> 1);
  chain_err_stif->ingress_data  = (uint8_t)((value & 0x00000004) >> 2);
  chain_err_stif->egress_req    = (uint8_t)((value & 0x00000008) >> 3);
  chain_err_stif->egress_resp   = (uint8_t)((value & 0x00000010) >> 4);
  chain_err_stif->egress_data   = (uint8_t)((value & 0x00000020) >> 5);
  chain_err_stif->extif_event   = (uint8_t)((value & 0x00000040) >> 6);
  chain_err_stif->extif_command = (uint8_t)((value & 0x00000080) >> 7);
  chain_err_stif->extif_event   |= (uint8_t)((value & 0x00000100) >> 7);
  chain_err_stif->extif_command |= (uint8_t)((value & 0x00000200) >> 8);
}


void
xpcie_fpga_ins_chain_err_cmdfault(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_cmdfault_t *chain_err_cmdfault)
{
  uint32_t value;

  /* [0] cid overwrite en, [31:16] cid */
  value =  ((uint32_t)chain_err_cmdfault->enable & 0x0001);
  value |= ((uint32_t)chain_err_cmdfault->cid    & 0xFFFF) << 16;

  if (chain_err_cmdfault->extif_id == FPGA_EXTIF_NUMBER_0) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EXTIF0_CMDFAULT_INS, chain_err_cmdfault->lane, value);
  }
  else if (chain_err_cmdfault->extif_id == FPGA_EXTIF_NUMBER_1) {
    chain_reg_write(dev, XPCIE_FPGA_CHAIN_EXTIF1_CMDFAULT_INS, chain_err_cmdfault->lane, value);
  } else {
    /* Do Nothing */
    xpcie_err("extif_id(%u) is not the expected value.\n", chain_err_cmdfault->extif_id);
  }
}


void
xpcie_fpga_get_ins_chain_err_cmdfault(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_err_cmdfault_t *chain_err_cmdfault)
{
  uint32_t value;

  if (chain_err_cmdfault->extif_id == FPGA_EXTIF_NUMBER_0) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EXTIF0_CMDFAULT_INS, chain_err_cmdfault->lane);
  }
  else if (chain_err_cmdfault->extif_id == FPGA_EXTIF_NUMBER_1) {
    value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EXTIF1_CMDFAULT_INS, chain_err_cmdfault->lane);
  } else {
    /* Do Nothing */
    xpcie_err("extif_id(%u) is not the expected value.\n", chain_err_cmdfault->extif_id);
  }


  chain_err_cmdfault->enable = (uint16_t)(value  & 0x00000001);
  chain_err_cmdfault->cid    = (uint16_t)((value  & 0xFFFF0000) >> 16);
}


void
xpcie_fpga_get_chain_con_status(
  fpga_dev_info_t *dev,
  fpga_ioctl_chain_con_status_t *status)
{
  uint32_t wr_value;
  uint32_t rd_value;

  wr_value = (status->cid  & 0x000001FF);
  chain_reg_write(dev, XPCIE_FPGA_CHAIN_STAT_SEL_SESSION, status->lane, wr_value);
  if (status->extif_id == FPGA_EXTIF_NUMBER_0) {
    rd_value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EXTIF0_SESSION_STATUS, status->lane);
  } else if (status->extif_id == FPGA_EXTIF_NUMBER_1) {
    rd_value = chain_reg_read(dev, XPCIE_FPGA_CHAIN_EXTIF1_SESSION_STATUS, status->lane);
  } else {
    xpcie_err("extif_id(%u) is not the expected value.\n", status->extif_id);
  }
  status->value = rd_value & 0x00000001;
}
