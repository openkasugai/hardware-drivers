/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxpcie_ptu.h"
#include "xpcie_regs_ptu.h"


int
xpcie_fpga_common_get_ptu_module_info(
  fpga_dev_info_t *dev)
{
  fpga_module_info_t *info = &dev->mods.ptu;

  // Set Module base address
  info->base = XPCIE_FPGA_PTU_OFFSET;

  // Set Module length per 1 lane
  info->len = XPCIE_FPGA_PTU_SIZE;

  // Set Module num
  // Because Ptu doesn't have lane module_id, get from chain module lane num
  info->num = dev->mods.chain.num;

  return 0;
}
