/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxpcie_cms.h"


long
xpcie_fpga_ioctl_cms(struct file *filp, unsigned int cmd, unsigned long arg)
{
  struct xpcie_file_private *private = filp->private_data;
  fpga_dev_info_t *dev = private->dev;
  long ret;

  ret = 0;
  switch (cmd) {
  // Get power info(only for u250)
  case XPCIE_DEV_CMS_GET_POWER_U250:
    {
      fpga_power_t power_info;
      xpcie_fpga_get_power_info_u250(dev, &power_info);
      if (copy_to_user((void __user *)arg, &power_info, sizeof(power_info))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  // Get power info
  case XPCIE_DEV_CMS_GET_POWER:
    {
      fpga_ioctl_power_t power_info;
      if (copy_from_user(&power_info, (void __user *)arg, sizeof(power_info))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_power_info(dev, &power_info);
      if (copy_to_user((void __user *)arg, &power_info, sizeof(power_info))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  // Get temperature info
  case XPCIE_DEV_CMS_GET_TEMP:
    {
      fpga_ioctl_temp_t temp_info;
      if (copy_from_user(&temp_info, (void __user *)arg, sizeof(fpga_ioctl_temp_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_get_temp_info(dev, &temp_info);
      if (copy_to_user((void __user *)arg, &temp_info, sizeof(fpga_ioctl_temp_t))) {
        ret = -EFAULT;
        break;
      }
    }
    break;

  // Reset CMS
  case XPCIE_DEV_CMS_SET_RESET:
    {
      uint32_t data;
      if (copy_from_user(&data, (void __user *)arg, sizeof(uint32_t))) {
        ret = -EFAULT;
        break;
      }
      xpcie_fpga_set_cms_unrest(dev, data);
    }
    break;

  default:
    private->is_valid_command = false;
    ret = -EINVAL;
    break;
  }

  return ret;
}
