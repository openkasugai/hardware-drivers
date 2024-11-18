/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxpcie_lldma.h"


inline long
xpcie_fpga_ioctl_lldma(struct file *filp, unsigned int cmd, unsigned long arg)
{
  struct xpcie_file_private *private = filp->private_data;
  fpga_dev_info_t *dev = private->dev;
  long ret = 0;

  switch (cmd) {
  case XPCIE_DEV_LLDMA_GET_VERSION:
    // Get FPGA's bitstream_id
    {
      uint32_t data;
      data = xpcie_fpga_get_version(dev);
      xpcie_info("XPCIE_DEV_LLDMA_GET_VERSION: data(%#010x)", data);
      if (copy_to_user((void __user *)arg, &data, sizeof(data))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  case XPCIE_DEV_LLDMA_ALLOC_QUEUE:
    // Get command queue, set connector_id and start polling
    {
      fpga_ioctl_queue_t ioctl_queue;
      if (copy_from_user(&ioctl_queue, (void __user *)arg, sizeof(fpga_ioctl_queue_t))) {
        ret = -EFAULT;
        break;
      }
      ret = xpcie_fpga_get_queue(dev, &ioctl_queue);
      if (ret < 0) {
        break;
      }
      if (copy_to_user((void __user *)arg, &ioctl_queue, sizeof(fpga_ioctl_queue_t))) {
        xpcie_fpga_put_queue(dev, &ioctl_queue);
        ret = -EFAULT;
        break;
      }
      // Bind dma channel's information with this file descripter
      private->chid = ioctl_queue.chid;
      private->que_kind = ioctl_queue.dir;
      private->is_get_queue = true;
    }
    break;
  case XPCIE_DEV_LLDMA_BIND_QUEUE:
    // Get command queue matching connector_id
    {
      fpga_ioctl_queue_t ioctl_queue;
      if (copy_from_user(&ioctl_queue, (void __user *)arg, sizeof(fpga_ioctl_queue_t))) {
        ret = -EFAULT;
        break;
      }
      if (xpcie_fpga_ref_queue(dev, &ioctl_queue) < 0) {
        ret = -EBUSY;
        break;
      }
      if (copy_to_user((void __user *)arg, &ioctl_queue, sizeof(fpga_ioctl_queue_t))) {
        ret = -EFAULT;
        break;
      }
      // Bind dma channel's information with this file descripter
      private->chid = ioctl_queue.chid;
      private->que_kind = ioctl_queue.dir;
    }
    break;
  case XPCIE_DEV_LLDMA_FREE_QUEUE:
    // Put command queue and stop polling
    {
      fpga_ioctl_queue_t ioctl_queue;
      if (copy_from_user(&ioctl_queue, (void __user *)arg, sizeof(fpga_ioctl_queue_t))) {
        ret = -EFAULT;
        break;
      }
      if(private->que_kind != ioctl_queue.dir)
      {
        ret = -EBUSY;
        break;
      }
      if (xpcie_fpga_put_queue(dev, &ioctl_queue) < 0) {
        ret = -EBUSY;
        break;
      }
      // Initialize dma channel's information of this file descripter
      private->chid = -1;
      private->que_kind = -1;
      private->is_get_queue = false;
    }
    break;
  case XPCIE_DEV_LLDMA_GET_CH_STAT:
    // Get availability and active status of dma channel
    {
      fpga_ioctl_chsts_t ioctl_chsts;
      if (copy_from_user(&ioctl_chsts, (void __user *)arg, sizeof(fpga_ioctl_chsts_t))) {
        ret = -EFAULT;
        break;
      }
      ioctl_chsts.avail_status = xpcie_fpga_get_avail_status(dev, ioctl_chsts.dir);
      ioctl_chsts.active_status = xpcie_fpga_get_active_status(dev, ioctl_chsts.dir);
      if (copy_to_user((void __user *)arg, &ioctl_chsts, sizeof(fpga_ioctl_chsts_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_LLDMA_GET_CID_CHAIN:
    // unused
    {
      fpga_ioctl_cidchain_t ioctl_cidchain;
      if (copy_from_user(&ioctl_cidchain, (void __user *)arg, sizeof(fpga_ioctl_cidchain_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_cid_chain_queue(dev, &ioctl_cidchain);
      if (copy_to_user((void __user *)arg, &ioctl_cidchain, sizeof(fpga_ioctl_cidchain_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  case XPCIE_DEV_LLDMA_ALLOC_CONNECTION:
    // Set information of D2D peer device for 1 side of FPGAs
    {
      fpga_ioctl_connect_t ioctl_connect;
      uint64_t peer_addr;
      int dir;
      if (copy_from_user(&ioctl_connect, (void __user *)arg, sizeof(ioctl_connect))) {
        ret = -EFAULT;
        break;
      }

      // Check if device is different or not
      if(dev->dev_id == ioctl_connect.peer_minor){
        xpcie_err("FPGA[%02d] tried connect self, but invalid action...", dev->dev_id);
        ret = -EINVAL;
        break;
      }

      // Get hw base address of peer FPGA from minor
      peer_addr = xpcie_fpga_get_baseaddr(ioctl_connect.peer_minor);
      if (peer_addr == 0){
        ret = -EINVAL;
        break;
      }

      // Convert direction from RX/TX -> D2D_RX/D2D_TX(used only in driver)
      if (ioctl_connect.self_dir == DMA_HOST_TO_DEV) {
        if (ioctl_connect.buf_addr == 0)
          dir = DMA_D2D_D_RX;
        else
          dir = DMA_D2D_RX;
      }
      if(ioctl_connect.self_dir == DMA_DEV_TO_HOST) {
        if (ioctl_connect.buf_addr == 0)
          dir = DMA_D2D_D_TX;
        else
          dir = DMA_D2D_TX;
      }

      ret = xpcie_fpga_dev_connect(
        dev,
        ioctl_connect.self_chid,
        ioctl_connect.peer_chid,
        dir,
        peer_addr,
        ioctl_connect.buf_size,
        ioctl_connect.buf_addr,
        ioctl_connect.connector_id);
      if(ret){
        break;
      }

      // Bind dma channel's information with this file descripter
      private->chid = ioctl_connect.self_chid;
      private->que_kind = dir;
      private->is_get_queue = true;
    }
    break;
  case XPCIE_DEV_LLDMA_FREE_CONNECTION:
    // Delete information of D2D peer device for 1 side of FPGAs
    {
      if(!private->is_get_queue){
        xpcie_err("This Command should be done AFTER Connecting");
        ret = -EINVAL;
        break;
      }
      ret = xpcie_fpga_dev_disconnect(
        dev,
        private->chid,
        private->que_kind);
      if(ret){
        break;
      }

      // Initialize dma channel's information of this file descripter
      private->chid = -1;
      private->que_kind = -1;
      private->is_get_queue = false;
    }
    break;

  case XPCIE_DEV_LLDMA_CTRL_DDR_BUFFER:
    {
      fpga_ioctl_lldma_buffer_t ioctl_info;
      if (copy_from_user(&ioctl_info, (void __user *)arg, sizeof(ioctl_info))) {
        ret = -EFAULT;
        break;
      }
      switch (ioctl_info.cmd) {
      case CMD_FPGA_IOCTL_LLDMA_BUF_SET:
        xpcie_fpga_set_lldma_buffer(dev, true);
        break;
      case CMD_FPGA_IOCTL_LLDMA_BUF_CLR:
        xpcie_fpga_set_lldma_buffer(dev, false);
        break;
      case CMD_FPGA_IOCTL_LLDMA_BUF_GET:
        xpcie_fpga_read_cif_ddr4_regs(dev, &ioctl_info.regs);
        break;
      default:
        xpcie_err("Invalid command received(%u)", ioctl_info.cmd);
        ret = -EINVAL;
        break;
      }
      if (ret)
        break;
      if (copy_to_user((void __user *)arg, &ioctl_info, sizeof(ioctl_info))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  case XPCIE_DEV_LLDMA_GET_UP_SIZE:
    {
      fpga_ioctl_up_info_t info;
      if (copy_from_user(&info, (void __user *)arg, sizeof(fpga_ioctl_up_info_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_request_size(dev, info.chid, &info.size);
      if (copy_to_user((void __user *)arg, &info, sizeof(fpga_ioctl_up_info_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  // Get RX channel's ctrl0 register value
  case XPCIE_DEV_LLDMA_GET_RXCH_CTRL0:
    {
      uint32_t value;
      xpcie_fpga_get_rxch_ctrl0(dev, &value);
      if (copy_to_user((void __user *)arg, &value, sizeof(uint32_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  default:
    private->is_valid_command = false;
    ret = -EINVAL;
  }

  return ret;
}
