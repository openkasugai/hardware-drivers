/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxpcie_global.h"


long
xpcie_fpga_ioctl_global(struct file *filp, unsigned int cmd, unsigned long arg)
{
  struct xpcie_file_private *private = filp->private_data;
  fpga_dev_info_t *dev = private->dev;
  long ret;

  ret = 0;
  switch (cmd) {
  /* fpga soft reset */
  case XPCIE_DEV_GLOBAL_CTRL_SOFT_RST:
    {
      xpcie_fpga_soft_rst(dev);
    }
    break;

  /* fpga check err */
  case XPCIE_DEV_GLOBAL_GET_CHK_ERR:
    {
      uint32_t check_err;
      xpcie_fpga_chk_err(dev, &check_err);
      if (copy_to_user((void __user *)arg, &check_err, sizeof(check_err))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  /* *** clkdown *** */
  case XPCIE_DEV_GLOBAL_GET_CLKDOWN:
    {
      fpga_ioctl_clkdown_t clkdown;

      xpcie_fpga_clk_dwn_det(dev, &clkdown);
      if (copy_to_user((void __user *)arg, &clkdown, sizeof(clkdown))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_GLOBAL_GET_CLKDOWN_RAW:
    {
      fpga_ioctl_clkdown_t clkdown;

      xpcie_fpga_clk_dwn_raw_det(dev, &clkdown);
      if (copy_to_user((void __user *)arg, &clkdown, sizeof(clkdown))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_GLOBAL_SET_CLKDOWN_CLR:
    {
      fpga_ioctl_clkdown_t clkdown;

      if (copy_from_user(&clkdown, (void __user *)arg, sizeof(clkdown))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_clk_dwn_clear(dev, &clkdown);
    }
    break;
  case XPCIE_DEV_GLOBAL_SET_CLKDOWN_MASK:
    {
      fpga_ioctl_clkdown_t clkdown;

      if (copy_from_user(&clkdown, (void __user *)arg, sizeof(clkdown))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_clk_dwn_mask(dev, &clkdown);
    }
    break;
  case XPCIE_DEV_GLOBAL_GET_CLKDOWN_MASK:
    {
      fpga_ioctl_clkdown_t clkdown;

      xpcie_fpga_clk_dwn_get_mask(dev, &clkdown);
      if (copy_to_user((void __user *)arg, &clkdown, sizeof(clkdown))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_GLOBAL_SET_CLKDOWN_FORCE:
    {
      fpga_ioctl_clkdown_t clkdown;

      if (copy_from_user(&clkdown, (void __user *)arg, sizeof(clkdown))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_clk_dwn_force(dev, &clkdown);
    }
    break;
  case XPCIE_DEV_GLOBAL_GET_CLKDOWN_FORCE:
    {
      fpga_ioctl_clkdown_t clkdown;

      xpcie_fpga_clk_dwn_get_force(dev, &clkdown);
      if (copy_to_user((void __user *)arg, &clkdown, sizeof(clkdown))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  /* *** ECCerr *** */
  case XPCIE_DEV_GLOBAL_GET_ECCERR:
    {
      fpga_ioctl_eccerr_t ioctl_eccerr;

      if (copy_from_user(&ioctl_eccerr, (void __user *)arg, sizeof(fpga_ioctl_eccerr_t))) {
        ret = -EFAULT;
        break;
      }

      xpcie_fpga_ecc_err_det(dev, &ioctl_eccerr);

      if (copy_to_user((void __user *)arg, &ioctl_eccerr, sizeof(fpga_ioctl_eccerr_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_GLOBAL_GET_ECCERR_RAW:
    {
      fpga_ioctl_eccerr_t ioctl_eccerr;

      if (copy_from_user(&ioctl_eccerr, (void __user *)arg, sizeof(fpga_ioctl_eccerr_t))) {
        ret = -EFAULT;
        break;
      }

      xpcie_fpga_ecc_err_raw_det(dev, &ioctl_eccerr);

      if (copy_to_user((void __user *)arg, &ioctl_eccerr, sizeof(fpga_ioctl_eccerr_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_GLOBAL_SET_ECCERR_CLR:
    {
      fpga_ioctl_eccerr_t ioctl_eccerr;

      if (copy_from_user(&ioctl_eccerr, (void __user *)arg, sizeof(fpga_ioctl_eccerr_t))) {
        ret = -EFAULT;
        break;
      }

      xpcie_fpga_ecc_err_clear(dev, &ioctl_eccerr);
    }
    break;
  case XPCIE_DEV_GLOBAL_SET_ECCERR_MASK:
    {
      fpga_ioctl_eccerr_t ioctl_eccerr;

      if (copy_from_user(&ioctl_eccerr, (void __user *)arg, sizeof(fpga_ioctl_eccerr_t))) {
        ret = -EFAULT;
        break;
      }

      xpcie_fpga_ecc_err_mask(dev, &ioctl_eccerr);
    }
    break;
  case XPCIE_DEV_GLOBAL_GET_ECCERR_MASK:
    {
      fpga_ioctl_eccerr_t ioctl_eccerr;

      if (copy_from_user(&ioctl_eccerr, (void __user *)arg, sizeof(fpga_ioctl_eccerr_t))) {
        ret = -EFAULT;
        break;
      }

      xpcie_fpga_ecc_err_get_mask(dev, &ioctl_eccerr);

      if (copy_to_user((void __user *)arg, &ioctl_eccerr, sizeof(fpga_ioctl_eccerr_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;
  case XPCIE_DEV_GLOBAL_SET_ECCERR_FORCE:
    {
      fpga_ioctl_eccerr_t ioctl_eccerr;

      if (copy_from_user(&ioctl_eccerr, (void __user *)arg, sizeof(fpga_ioctl_eccerr_t))) {
        ret = -EFAULT;
        break;
      }

      xpcie_fpga_ecc_err_force(dev, &ioctl_eccerr);
    }
    break;
  case XPCIE_DEV_GLOBAL_GET_ECCERR_FORCE:
    {
      fpga_ioctl_eccerr_t ioctl_eccerr;

      if (copy_from_user(&ioctl_eccerr, (void __user *)arg, sizeof(fpga_ioctl_eccerr_t))) {
        ret = -EFAULT;
        break;
      }

      xpcie_fpga_ecc_err_get_force(dev, &ioctl_eccerr);

      if (copy_to_user((void __user *)arg, &ioctl_eccerr, sizeof(fpga_ioctl_eccerr_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  case XPCIE_DEV_GLOBAL_UPDATE_MAJOR_VERSION:
    dev->bitstream_id.child = xpcie_fpga_global_get_major_version(dev);
    break;

  case XPCIE_DEV_GLOBAL_GET_MINOR_VERSION:
    {
      uint32_t minor_version = xpcie_fpga_global_get_minor_version(dev);

      if (copy_to_user((void __user *)arg, &minor_version, sizeof(minor_version))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  default:
    private->is_valid_command = false;
    ret = -EINVAL;
    break;
  }

  return ret;
}
