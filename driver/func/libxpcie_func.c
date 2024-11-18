/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxpcie_func.h"
#include "xpcie_regs_func.h"


int
xpcie_fpga_common_get_func_module_info(
  fpga_dev_info_t *dev)
{
  fpga_module_info_t *info = &dev->mods.func;

  // Set Module base address
  info->base = XPCIE_FPGA_FUNC_OFFSET;

  // Set Module length per 1 lane
  info->len = XPCIE_FPGA_FUNC_SIZE;

  // Get Module num
  for (info->num = 0; info->num < XPCIE_KERNEL_LANE_MAX; info->num++) {
    // Read Module ID
    uint32_t value = reg_read32(dev, info->base + info->num * info->len
     + XPCIE_FPGA_FUNC_MODULE_ID);
    switch (value) {
    case XPCIE_FPGA_FUNC_MODULE_ID_FR_RSZ_VALUE:
      // do nothing
      break;
    default:
      // To break for loop
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
