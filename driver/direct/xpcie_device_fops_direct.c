/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxpcie_direct.h"
#include "xpcie_regs_direct.h"


inline long
xpcie_fpga_ioctl_direct(struct file *filp, unsigned int cmd, unsigned long arg)
{
  struct xpcie_file_private *private = filp->private_data;
  fpga_dev_info_t *dev = private->dev;
  long ret = 0;

  switch (cmd) {
  /* *** Direct Start/Sop module *** */
  case XPCIE_DEV_DIRECT_START_MODULE:
    {
      uint32_t lane;
      if (copy_from_user(&lane, (void __user *)arg, sizeof(int))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_start_direct_module(dev, lane);
    }
    break;
  case XPCIE_DEV_DIRECT_STOP_MODULE:
    {
      uint32_t lane;
      if (copy_from_user(&lane, (void __user *)arg, sizeof(int))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_stop_direct_module(dev, lane);
    }
    break;
  /* *** Direct stat *** */
  case XPCIE_DEV_DIRECT_GET_BYTES:
    {
      fpga_ioctl_direct_bytenum_t bytenum;
      if (copy_from_user(&bytenum, (void __user *)arg, sizeof(fpga_ioctl_direct_bytenum_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_direct_bytes(dev, &bytenum);
      if (copy_to_user((void __user *)arg, &bytenum, sizeof(fpga_ioctl_direct_bytenum_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_DIRECT_GET_FRAMES:
    {
      fpga_ioctl_direct_framenum_t framenum;
      if (copy_from_user(&framenum, (void __user *)arg, sizeof(fpga_ioctl_direct_framenum_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_direct_frames(dev, &framenum);
      if (copy_to_user((void __user *)arg, &framenum, sizeof(fpga_ioctl_direct_framenum_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  /* *** Direct err *** */
  case XPCIE_DEV_DIRECT_GET_ERR_ALL:
    {
      fpga_ioctl_err_all_t err;
      if (copy_from_user(&err, (void __user *)arg, sizeof(fpga_ioctl_err_all_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_check_direct_err(dev, &err);
      if (copy_to_user((void __user *)arg, &err, sizeof(fpga_ioctl_err_all_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  /* *** Direct protocol err *** */
  case XPCIE_DEV_DIRECT_GET_ERR_PROT:
    {
      fpga_ioctl_direct_err_prot_t direct_err_prot;
      if (copy_from_user(&direct_err_prot, (void __user *)arg, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_detect_direct_err_prot(dev, &direct_err_prot);
      if (copy_to_user((void __user *)arg, &direct_err_prot, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_DIRECT_SET_ERR_PROT_CLR:
    {
      fpga_ioctl_direct_err_prot_t direct_err_prot;
      if (copy_from_user(&direct_err_prot, (void __user *)arg, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_clear_direct_err_prot(dev, &direct_err_prot);
    }
    break;
  case XPCIE_DEV_DIRECT_SET_ERR_PROT_MASK:
    {
      fpga_ioctl_direct_err_prot_t direct_err_prot;
      if (copy_from_user(&direct_err_prot, (void __user *)arg, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_mask_direct_err_prot(dev, &direct_err_prot);
    }
    break;
  case XPCIE_DEV_DIRECT_GET_ERR_PROT_MASK:
    {
      fpga_ioctl_direct_err_prot_t direct_err_prot;
      if (copy_from_user(&direct_err_prot, (void __user *)arg, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_mask_direct_err_prot(dev, &direct_err_prot);
      if (copy_to_user((void __user *)arg, &direct_err_prot, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_DIRECT_SET_ERR_PROT_FORCE:
    {
      fpga_ioctl_direct_err_prot_t direct_err_prot;
      if (copy_from_user(&direct_err_prot, (void __user *)arg, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_force_direct_err_prot(dev, &direct_err_prot);
    }
    break;
  case XPCIE_DEV_DIRECT_GET_ERR_PROT_FORCE:
    {
      fpga_ioctl_direct_err_prot_t direct_err_prot;
      if (copy_from_user(&direct_err_prot, (void __user *)arg, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_force_direct_err_prot(dev, &direct_err_prot);
      if (copy_to_user((void __user *)arg, &direct_err_prot, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_DIRECT_ERR_PROT_INS:
    {
      fpga_ioctl_direct_err_prot_t direct_err_prot;
      if (copy_from_user(&direct_err_prot, (void __user *)arg, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_ins_direct_err_prot(dev, &direct_err_prot);
    }
    break;
  case XPCIE_DEV_DIRECT_ERR_PROT_GET_INS:
    {
      fpga_ioctl_direct_err_prot_t direct_err_prot;
      if (copy_from_user(&direct_err_prot, (void __user *)arg, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_ins_direct_err_prot(dev, &direct_err_prot);
      if (copy_to_user((void __user *)arg, &direct_err_prot, sizeof(fpga_ioctl_direct_err_prot_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  /* *** Direct streamIF stall err *** */
  case XPCIE_DEV_DIRECT_GET_ERR_STIF:
    {
      fpga_ioctl_direct_err_stif_t direct_err_stif;
      if (copy_from_user(&direct_err_stif, (void __user *)arg, sizeof(fpga_ioctl_direct_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_detect_direct_err_stif(dev, &direct_err_stif);
      if (copy_to_user((void __user *)arg, &direct_err_stif, sizeof(fpga_ioctl_direct_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_DIRECT_SET_ERR_STIF_MASK:
    {
      fpga_ioctl_direct_err_stif_t direct_err_stif;
      if (copy_from_user(&direct_err_stif, (void __user *)arg, sizeof(fpga_ioctl_direct_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_mask_direct_err_stif(dev, &direct_err_stif);
    }
    break;
  case XPCIE_DEV_DIRECT_GET_ERR_STIF_MASK:
    {
      fpga_ioctl_direct_err_stif_t direct_err_stif;
      if (copy_from_user(&direct_err_stif, (void __user *)arg, sizeof(fpga_ioctl_direct_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_mask_direct_err_stif(dev, &direct_err_stif);
      if (copy_to_user((void __user *)arg, &direct_err_stif, sizeof(fpga_ioctl_direct_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_DIRECT_SET_ERR_STIF_FORCE:
    {
      fpga_ioctl_direct_err_stif_t direct_err_stif;
      if (copy_from_user(&direct_err_stif, (void __user *)arg, sizeof(fpga_ioctl_direct_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_force_direct_err_stif(dev, &direct_err_stif);
    }
    break;
  case XPCIE_DEV_DIRECT_GET_ERR_STIF_FORCE:
    {
      fpga_ioctl_direct_err_stif_t direct_err_stif;
      if (copy_from_user(&direct_err_stif, (void __user *)arg, sizeof(fpga_ioctl_direct_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_force_direct_err_stif(dev, &direct_err_stif);
      if (copy_to_user((void __user *)arg, &direct_err_stif, sizeof(fpga_ioctl_direct_err_stif_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_DIRECT_GET_MODULE:
    {
      fpga_ioctl_direct_ctrl_t direct_ctrl;
      if (copy_from_user(&direct_ctrl, (void __user *)arg, sizeof(fpga_ioctl_direct_ctrl_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_direct_ctrl(dev, &direct_ctrl);
      if (copy_to_user((void __user *)arg, &direct_ctrl, sizeof(fpga_ioctl_direct_ctrl_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_DIRECT_GET_MODULE_ID:
    {
      fpga_ioctl_direct_ctrl_t direct_ctrl;
      if (copy_from_user(&direct_ctrl, (void __user *)arg, sizeof(fpga_ioctl_direct_ctrl_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_direct_module_id(dev, &direct_ctrl);
      if (copy_to_user((void __user *)arg, &direct_ctrl, sizeof(fpga_ioctl_direct_ctrl_t))) {
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
