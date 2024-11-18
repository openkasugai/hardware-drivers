/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxpcie_chain.h"


inline long
xpcie_fpga_ioctl_chain(struct file *filp, unsigned int cmd, unsigned long arg)
{
  struct xpcie_file_private *private = filp->private_data;
  fpga_dev_info_t *dev = private->dev;
  long ret = 0;

  switch (cmd) {
  /* *** update function chain table *** */
  case XPCIE_DEV_CHAIN_UPDATE_TABLE_INGR:
    {
      fpga_id_t id;
      if (copy_from_user(&id, (void __user *)arg, sizeof(fpga_id_t))) {
        ret = -EFAULT;
        break;
      }
      ret = xpcie_fpga_update_func_chain_table(dev, &id, FPGA_CID_KIND_INGRESS);
    }
    break;
  case XPCIE_DEV_CHAIN_UPDATE_TABLE_EGR:
    {
      fpga_id_t id;
      if (copy_from_user(&id, (void __user *)arg, sizeof(fpga_id_t))) {
        ret = -EFAULT;
        break;
      }
      ret = xpcie_fpga_update_func_chain_table(dev, &id, FPGA_CID_KIND_EGRESS);
    }
    break;
  case XPCIE_DEV_CHAIN_DELETE_TABLE_INGR:
    {
      fpga_id_t id;
      if (copy_from_user(&id, (void __user *)arg, sizeof(fpga_id_t))) {
        ret = -EFAULT;
        break;
      }
      if((ret = xpcie_fpga_delete_func_chain_table(dev, &id, FPGA_CID_KIND_INGRESS))){
        // 1: timeout / 2: no chain
        break;
      }
      if (copy_to_user((void __user *)arg, &id, sizeof(fpga_id_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_DELETE_TABLE_EGR:
    {
      fpga_id_t id;
      if (copy_from_user(&id, (void __user *)arg, sizeof(fpga_id_t))) {
        ret = -EFAULT;
        break;
      }
      if((ret = xpcie_fpga_delete_func_chain_table(dev, &id, FPGA_CID_KIND_EGRESS))){
        // 1: timeout / 2: no chain
        break;
      }
      if (copy_to_user((void __user *)arg, &id, sizeof(fpga_id_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_READ_TABLE_INGR:
    {
      fpga_id_t id;
      if (copy_from_user(&id, (void __user *)arg, sizeof(fpga_id_t))) {
        ret = -EFAULT;
        break;
      }
      if((ret = xpcie_fpga_read_func_chain_table(dev, &id, FPGA_CID_KIND_INGRESS))){
        // 1: timeout / 2: no chain
        break;
      }
      if (copy_to_user((void __user *)arg, &id, sizeof(fpga_id_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_READ_TABLE_EGR:
    {
      fpga_id_t id;
      if (copy_from_user(&id, (void __user *)arg, sizeof(fpga_id_t))) {
        ret = -EFAULT;
        break;
      }
      if((ret = xpcie_fpga_read_func_chain_table(dev, &id, FPGA_CID_KIND_EGRESS))){
        // 1: timeout / 2: no chain
        break;
      }
      if (copy_to_user((void __user *)arg, &id, sizeof(fpga_id_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  /* *** Chain Start/Sop module *** */
  case XPCIE_DEV_CHAIN_START_MODULE:
    {
      uint32_t lane;
      if (copy_from_user(&lane, (void __user *)arg, sizeof(int))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_start_chain_module(dev, lane);
    }
    break;
  case XPCIE_DEV_CHAIN_STOP_MODULE:
    {
      uint32_t lane;
      if (copy_from_user(&lane, (void __user *)arg, sizeof(int))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_stop_chain_module(dev, lane);
    }
    break;
  /* *** DDR offset *** */
  case XPCIE_DEV_CHAIN_SET_DDR_OFFSET_FRAME:
    {
      fpga_ioctl_extif_t extif;
      if (copy_from_user(&extif, (void __user *)arg, sizeof(fpga_ioctl_extif_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_set_ddr_offset_frame(dev, &extif);
      if (copy_to_user((void __user *)arg, &extif, sizeof(fpga_ioctl_extif_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_GET_DDR_OFFSET_FRAME:
    {
      fpga_ioctl_chain_ddr_t  chain_ddr;
      if (copy_from_user(&chain_ddr, (void __user *)arg, sizeof(fpga_ioctl_chain_ddr_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_ddr_offset_frame(dev, &chain_ddr);
      if (copy_to_user((void __user *)arg, &chain_ddr, sizeof(fpga_ioctl_chain_ddr_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  /* *** latency *** */
  case XPCIE_DEV_CHAIN_GET_LATENCY_CHAIN:
    {
      fpga_ioctl_chain_latency_t latency;
      if (copy_from_user(&latency, (void __user *)arg, sizeof(fpga_ioctl_chain_latency_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_latency_chain(dev, &latency);
      if (copy_to_user((void __user *)arg, &latency, sizeof(fpga_ioctl_chain_latency_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_GET_LATENCY_FUNC:
    {
      fpga_ioctl_chain_func_latency_t latency;
      if (copy_from_user(&latency, (void __user *)arg, sizeof(fpga_ioctl_chain_func_latency_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_latency_func(dev, &latency);
      if (copy_to_user((void __user *)arg, &latency, sizeof(fpga_ioctl_chain_func_latency_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  /* *** Chain stat *** */
  case XPCIE_DEV_CHAIN_GET_CHAIN_BYTES:
    {
      fpga_ioctl_chain_bytenum_t bytenum;
      if (copy_from_user(&bytenum, (void __user *)arg, sizeof(fpga_ioctl_chain_bytenum_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_chain_bytes(dev, &bytenum);
      if (copy_to_user((void __user *)arg, &bytenum, sizeof(fpga_ioctl_chain_bytenum_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_GET_CHAIN_FRAMES:
    {
      fpga_ioctl_chain_framenum_t framenum;
      if (copy_from_user(&framenum, (void __user *)arg, sizeof(fpga_ioctl_chain_framenum_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_chain_frames(dev, &framenum);
      if (copy_to_user((void __user *)arg, &framenum, sizeof(fpga_ioctl_chain_framenum_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_GET_CHAIN_BUFF:
    {
      fpga_ioctl_chain_framenum_t framenum;
      if (copy_from_user(&framenum, (void __user *)arg, sizeof(fpga_ioctl_chain_framenum_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_chain_buff(dev, &framenum);
      if (copy_to_user((void __user *)arg, &framenum, sizeof(fpga_ioctl_chain_framenum_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_GET_CHAIN_BP:
    {
      fpga_ioctl_chain_framenum_t framenum;
      if (copy_from_user(&framenum, (void __user *)arg, sizeof(fpga_ioctl_chain_framenum_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_chain_bp(dev, &framenum);
      if (copy_to_user((void __user *)arg, &framenum, sizeof(fpga_ioctl_chain_framenum_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_SET_CHAIN_BP_CLR:
    {
      fpga_ioctl_chain_framenum_t framenum;
      if (copy_from_user(&framenum, (void __user *)arg, sizeof(fpga_ioctl_chain_framenum_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_clear_chain_bp(dev, &framenum);
      if (copy_to_user((void __user *)arg, &framenum, sizeof(fpga_ioctl_chain_framenum_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_GET_EGR_BUSY:
    {
      fpga_ioctl_chain_framenum_t busy;
      if (copy_from_user(&busy, (void __user *)arg, sizeof(fpga_ioctl_chain_framenum_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_chain_busy(dev, &busy);
      if (copy_to_user((void __user *)arg, &busy, sizeof(fpga_ioctl_chain_framenum_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  /* *** Chain err *** */
  case XPCIE_DEV_CHAIN_GET_CHK_ERR:
    {
      fpga_ioctl_err_all_t err;
      if (copy_from_user(&err, (void __user *)arg, sizeof(fpga_ioctl_err_all_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_check_chain_err(dev, &err);
      if (copy_to_user((void __user *)arg, &err, sizeof(fpga_ioctl_err_all_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR:
    {
      fpga_ioctl_chain_err_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_detect_chain_err(dev, &chain_err);
      if (copy_to_user((void __user *)arg, &chain_err, sizeof(fpga_ioctl_chain_err_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_MASK:
    {
      fpga_ioctl_chain_err_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_mask_chain_err(dev, &chain_err);
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR_MASK:
    {
      fpga_ioctl_chain_err_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_mask_chain_err(dev, &chain_err);
      if (copy_to_user((void __user *)arg, &chain_err, sizeof(fpga_ioctl_chain_err_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_FORCE:
    {
      fpga_ioctl_chain_err_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_force_chain_err(dev, &chain_err);
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR_FORCE:
    {
      fpga_ioctl_chain_err_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_force_chain_err(dev, &chain_err);
      if (copy_to_user((void __user *)arg, &chain_err, sizeof(fpga_ioctl_chain_err_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_ERR_INS:
    {
      fpga_ioctl_chain_err_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_ins_chain_err(dev, &chain_err);
    }
    break;
  case XPCIE_DEV_CHAIN_ERR_GET_INS:
    {
      fpga_ioctl_chain_err_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_ins_chain_err(dev, &chain_err);
      if (copy_to_user((void __user *)arg, &chain_err, sizeof(fpga_ioctl_chain_err_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR_TBL:
    {
      fpga_ioctl_chain_err_table_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_table_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_detect_chain_err_table(dev, &chain_err);
      if (copy_to_user((void __user *)arg, &chain_err, sizeof(fpga_ioctl_chain_err_table_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_TBL_MASK:
    {
      fpga_ioctl_chain_err_table_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_table_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_mask_chain_err_table(dev, &chain_err);
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR_TBL_MASK:
    {
      fpga_ioctl_chain_err_table_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_table_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_mask_chain_err_table(dev, &chain_err);
      if (copy_to_user((void __user *)arg, &chain_err, sizeof(fpga_ioctl_chain_err_table_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_TBL_FORCE:
    {
      fpga_ioctl_chain_err_table_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_table_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_force_chain_err_table(dev, &chain_err);
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR_TBL_FORCE:
    {
      fpga_ioctl_chain_err_table_t chain_err;
      if (copy_from_user(&chain_err, (void __user *)arg, sizeof(fpga_ioctl_chain_err_table_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_force_chain_err_table(dev, &chain_err);
      if (copy_to_user((void __user *)arg, &chain_err, sizeof(fpga_ioctl_chain_err_table_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  /* *** Chain protocol err *** */
  case XPCIE_DEV_CHAIN_GET_ERR_PROT:
    {
      fpga_ioctl_chain_err_prot_t chain_err_prot;
      if (copy_from_user(&chain_err_prot, (void __user *)arg, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_detect_chain_err_prot(dev, &chain_err_prot);
      if (copy_to_user((void __user *)arg, &chain_err_prot, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_PROT_CLR:
    {
      fpga_ioctl_chain_err_prot_t chain_err_prot;
      if (copy_from_user(&chain_err_prot, (void __user *)arg, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_clear_chain_err_prot(dev, &chain_err_prot);
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_PROT_MASK:
    {
      fpga_ioctl_chain_err_prot_t chain_err_prot;
      if (copy_from_user(&chain_err_prot, (void __user *)arg, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_mask_chain_err_prot(dev, &chain_err_prot);
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR_PROT_MASK:
    {
      fpga_ioctl_chain_err_prot_t chain_err_prot;
      if (copy_from_user(&chain_err_prot, (void __user *)arg, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_mask_chain_err_prot(dev, &chain_err_prot);
      if (copy_to_user((void __user *)arg, &chain_err_prot, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_PROT_FORCE:
    {
      fpga_ioctl_chain_err_prot_t chain_err_prot;
      if (copy_from_user(&chain_err_prot, (void __user *)arg, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_force_chain_err_prot(dev, &chain_err_prot);
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR_PROT_FORCE:
    {
      fpga_ioctl_chain_err_prot_t chain_err_prot;
      if (copy_from_user(&chain_err_prot, (void __user *)arg, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_force_chain_err_prot(dev, &chain_err_prot);
      if (copy_to_user((void __user *)arg, &chain_err_prot, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_ERR_PROT_INS:
    {
      fpga_ioctl_chain_err_prot_t chain_err_prot;
      if (copy_from_user(&chain_err_prot, (void __user *)arg, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_ins_chain_err_prot(dev, &chain_err_prot);
    }
    break;
  case XPCIE_DEV_CHAIN_ERR_PROT_GET_INS:
    {
      fpga_ioctl_chain_err_prot_t chain_err_prot;
      if (copy_from_user(&chain_err_prot, (void __user *)arg, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_ins_chain_err_prot(dev, &chain_err_prot);
      if (copy_to_user((void __user *)arg, &chain_err_prot, sizeof(fpga_ioctl_chain_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  /* *** Chain extIF event err *** */
  case XPCIE_DEV_CHAIN_GET_ERR_EVT:
    {
      fpga_ioctl_chain_err_evt_t chain_err_evt;
      if (copy_from_user(&chain_err_evt, (void __user *)arg, sizeof(fpga_ioctl_chain_err_evt_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_detect_chain_err_evt(dev, &chain_err_evt);
      if (copy_to_user((void __user *)arg, &chain_err_evt, sizeof(fpga_ioctl_chain_err_evt_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_EVT_CLR:
    {
      fpga_ioctl_chain_err_evt_t chain_err_evt;
      if (copy_from_user(&chain_err_evt, (void __user *)arg, sizeof(fpga_ioctl_chain_err_evt_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_clear_chain_err_evt(dev, &chain_err_evt);
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_EVT_MASK:
    {
      fpga_ioctl_chain_err_evt_t chain_err_evt;
      if (copy_from_user(&chain_err_evt, (void __user *)arg, sizeof(fpga_ioctl_chain_err_evt_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_mask_chain_err_evt(dev, &chain_err_evt);
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR_EVT_MASK:
    {
      fpga_ioctl_chain_err_evt_t chain_err_evt;
      if (copy_from_user(&chain_err_evt, (void __user *)arg, sizeof(fpga_ioctl_chain_err_evt_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_mask_chain_err_evt(dev, &chain_err_evt);
      if (copy_to_user((void __user *)arg, &chain_err_evt, sizeof(fpga_ioctl_chain_err_evt_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_EVT_FORCE:
    {
      fpga_ioctl_chain_err_evt_t chain_err_evt;
      if (copy_from_user(&chain_err_evt, (void __user *)arg, sizeof(fpga_ioctl_chain_err_evt_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_force_chain_err_evt(dev, &chain_err_evt);
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR_EVT_FORCE:
    {
      fpga_ioctl_chain_err_evt_t chain_err_evt;
      if (copy_from_user(&chain_err_evt, (void __user *)arg, sizeof(fpga_ioctl_chain_err_evt_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_force_chain_err_evt(dev, &chain_err_evt);
      if (copy_to_user((void __user *)arg, &chain_err_evt, sizeof(fpga_ioctl_chain_err_evt_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  /* *** Chain streamIF stall err *** */
  case XPCIE_DEV_CHAIN_GET_ERR_STIF:
    {
      fpga_ioctl_chain_err_stif_t chain_err_stif;
      if (copy_from_user(&chain_err_stif, (void __user *)arg, sizeof(fpga_ioctl_chain_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_detect_chain_err_stif(dev, &chain_err_stif);
      if (copy_to_user((void __user *)arg, &chain_err_stif, sizeof(fpga_ioctl_chain_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_STIF_MASK:
    {
      fpga_ioctl_chain_err_stif_t chain_err_stif;
      if (copy_from_user(&chain_err_stif, (void __user *)arg, sizeof(fpga_ioctl_chain_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_mask_chain_err_stif(dev, &chain_err_stif);
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR_STIF_MASK:
    {
      fpga_ioctl_chain_err_stif_t chain_err_stif;
      if (copy_from_user(&chain_err_stif, (void __user *)arg, sizeof(fpga_ioctl_chain_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_mask_chain_err_stif(dev, &chain_err_stif);
      if (copy_to_user((void __user *)arg, &chain_err_stif, sizeof(fpga_ioctl_chain_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_SET_ERR_STIF_FORCE:
    {
      fpga_ioctl_chain_err_stif_t chain_err_stif;
      if (copy_from_user(&chain_err_stif, (void __user *)arg, sizeof(fpga_ioctl_chain_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_force_chain_err_stif(dev, &chain_err_stif);
    }
    break;
  case XPCIE_DEV_CHAIN_GET_ERR_STIF_FORCE:
    {
      fpga_ioctl_chain_err_stif_t chain_err_stif;
      if (copy_from_user(&chain_err_stif, (void __user *)arg, sizeof(fpga_ioctl_chain_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_force_chain_err_stif(dev, &chain_err_stif);
      if (copy_to_user((void __user *)arg, &chain_err_stif, sizeof(fpga_ioctl_chain_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_ERR_CMDFAULT_INS:
    {
      fpga_ioctl_chain_err_cmdfault_t chain_err_cmdfault;
      if (copy_from_user(&chain_err_cmdfault, (void __user *)arg, sizeof(fpga_ioctl_chain_err_cmdfault_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_ins_chain_err_cmdfault(dev, &chain_err_cmdfault);
    }
    break;
  case XPCIE_DEV_CHAIN_ERR_CMDFAULT_GET_INS:
    {
      fpga_ioctl_chain_err_cmdfault_t chain_err_cmdfault;
      if (copy_from_user(&chain_err_cmdfault, (void __user *)arg, sizeof(fpga_ioctl_chain_err_cmdfault_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_ins_chain_err_cmdfault(dev, &chain_err_cmdfault);
      if (copy_to_user((void __user *)arg, &chain_err_cmdfault, sizeof(fpga_ioctl_chain_err_cmdfault_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_GET_MODULE:
    {
      fpga_ioctl_chain_ctrl_t chain_ctrl;
      if (copy_from_user(&chain_ctrl, (void __user *)arg, sizeof(fpga_ioctl_chain_ctrl_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_chain_ctrl(dev, &chain_ctrl);
      if (copy_to_user((void __user *)arg, &chain_ctrl, sizeof(fpga_ioctl_chain_ctrl_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_GET_MODULE_ID:
    {
      fpga_ioctl_chain_ctrl_t chain_ctrl;
      if (copy_from_user(&chain_ctrl, (void __user *)arg, sizeof(fpga_ioctl_chain_ctrl_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_chain_module_id(dev, &chain_ctrl);
      if (copy_to_user((void __user *)arg, &chain_ctrl, sizeof(fpga_ioctl_chain_ctrl_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_CHAIN_GET_CONNECTION:
    {
      fpga_ioctl_chain_con_status_t status;
      if (copy_from_user(&status, (void __user *)arg, sizeof(fpga_ioctl_chain_con_status_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_chain_con_status(dev, &status);
      if (copy_to_user((void __user *)arg, &status, sizeof(fpga_ioctl_chain_con_status_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  case XPCIE_DEV_CHAIN_READ_SOFT_TABLE:
    {
      fpga_ioctl_chain_ids_t ioctl_arg;
      if (copy_from_user(&ioctl_arg, (void __user *)arg, sizeof(ioctl_arg))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_read_chain_soft_table(
        dev,
        ioctl_arg.lane,
        ioctl_arg.fchid,
        &ioctl_arg.ingress_extif_id,
        &ioctl_arg.ingress_cid,
        &ioctl_arg.egress_extif_id,
        &ioctl_arg.egress_cid);
      if (copy_to_user((void __user *)arg, &ioctl_arg, sizeof(ioctl_arg))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  case XPCIE_DEV_CHAIN_RESET_SOFT_TABLE:
    xpcie_fpga_reset_chain_soft_table(dev);
    break;

  default:
    private->is_valid_command = false;
    ret = -EINVAL;
  }

  return ret;
}
