/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxpcie_global.h"
#include "xpcie_regs_global.h"


int
xpcie_fpga_common_get_global_module_info(
  fpga_dev_info_t *dev)
{
  fpga_module_info_t *info = &dev->mods.global;

  // Set Module base address
  info->base = XPCIE_FPGA_GLOBAL_OFFSET;

  // Set Module length per 1 lane
  info->len = XPCIE_FPGA_GLOBAL_SIZE;

  // Set Module num
  info->num = 1;

  return 0;
}


uint32_t
xpcie_fpga_global_get_major_version(
  fpga_dev_info_t *dev)
{
  return reg_read32(dev, XPCIE_FPGA_GLOBAL_MAJOR_VERSION);
}


uint32_t
xpcie_fpga_global_get_minor_version(
  fpga_dev_info_t *dev)
{
  return reg_read32(dev, XPCIE_FPGA_GLOBAL_MINOR_VERSION);
}


void
xpcie_fpga_soft_rst(
  fpga_dev_info_t *dev)
{
  reg_write32(dev, XPCIE_FPGA_SOFT_RST, 1);
  reg_write32(dev, XPCIE_FPGA_SOFT_RST, 0);
}


void
xpcie_fpga_chk_err(
  fpga_dev_info_t *dev,
  uint32_t *check_err)
{
  *check_err = reg_read32(dev, XPCIE_FPGA_CHECK_ERR);
}


void
xpcie_fpga_clk_dwn_det(
  fpga_dev_info_t *dev,
  fpga_ioctl_clkdown_t *clkdown)
{
  uint32_t value = reg_read32(dev, XPCIE_FPGA_CLKDOWN);
  clkdown->user_clk  = (uint8_t)(value  & 0x00000001);
  clkdown->ddr4_clk0 = (uint8_t)((value & 0x00000002) >> 1);
  clkdown->ddr4_clk1 = (uint8_t)((value & 0x00000004) >> 2);
  clkdown->ddr4_clk2 = (uint8_t)((value & 0x00000008) >> 3);
  clkdown->ddr4_clk3 = (uint8_t)((value & 0x00000010) >> 4);
  clkdown->qsfp_clk0 = (uint8_t)((value & 0x00000020) >> 5);
  clkdown->qsfp_clk1 = (uint8_t)((value & 0x00000040) >> 6);
}


void
xpcie_fpga_clk_dwn_clear(
  fpga_dev_info_t *dev,
  fpga_ioctl_clkdown_t *clkdown)
{
  uint32_t value;
  value =  ((uint32_t)clkdown->user_clk  & 0x01);
  value |= ((uint32_t)clkdown->ddr4_clk0 & 0x01) << 1;
  value |= ((uint32_t)clkdown->ddr4_clk1 & 0x01) << 2;
  value |= ((uint32_t)clkdown->ddr4_clk2 & 0x01) << 3;
  value |= ((uint32_t)clkdown->ddr4_clk3 & 0x01) << 4;
  value |= ((uint32_t)clkdown->qsfp_clk0 & 0x01) << 5;
  value |= ((uint32_t)clkdown->qsfp_clk1 & 0x01) << 6;
  reg_write32(dev, XPCIE_FPGA_CLKDOWN, value);
}


void
xpcie_fpga_clk_dwn_raw_det(
  fpga_dev_info_t *dev,
  fpga_ioctl_clkdown_t *clkdown)
{
  uint32_t value = reg_read32(dev, XPCIE_FPGA_CLKDOWN_RAW);
  clkdown->user_clk  = (uint8_t)(value  & 0x00000001);
  clkdown->ddr4_clk0 = (uint8_t)((value & 0x00000002) >> 1);
  clkdown->ddr4_clk1 = (uint8_t)((value & 0x00000004) >> 2);
  clkdown->ddr4_clk2 = (uint8_t)((value & 0x00000008) >> 3);
  clkdown->ddr4_clk3 = (uint8_t)((value & 0x00000010) >> 4);
  clkdown->qsfp_clk0 = (uint8_t)((value & 0x00000020) >> 5);
  clkdown->qsfp_clk1 = (uint8_t)((value & 0x00000040) >> 6);
}


void
xpcie_fpga_clk_dwn_mask(
  fpga_dev_info_t *dev,
  fpga_ioctl_clkdown_t *clkdown)
{
  uint32_t value;
  value =  ((uint32_t)clkdown->user_clk  & 0x01);
  value |= ((uint32_t)clkdown->ddr4_clk0 & 0x01) << 1;
  value |= ((uint32_t)clkdown->ddr4_clk1 & 0x01) << 2;
  value |= ((uint32_t)clkdown->ddr4_clk2 & 0x01) << 3;
  value |= ((uint32_t)clkdown->ddr4_clk3 & 0x01) << 4;
  value |= ((uint32_t)clkdown->qsfp_clk0 & 0x01) << 5;
  value |= ((uint32_t)clkdown->qsfp_clk1 & 0x01) << 6;
  reg_write32(dev, XPCIE_FPGA_CLKDOWN_MASK, value);
}


void
xpcie_fpga_clk_dwn_get_mask(
  fpga_dev_info_t *dev,
  fpga_ioctl_clkdown_t *clkdown)
{
  uint32_t value = reg_read32(dev, XPCIE_FPGA_CLKDOWN_MASK);
  clkdown->user_clk  = (uint8_t)(value  & 0x00000001);
  clkdown->ddr4_clk0 = (uint8_t)((value & 0x00000002) >> 1);
  clkdown->ddr4_clk1 = (uint8_t)((value & 0x00000004) >> 2);
  clkdown->ddr4_clk2 = (uint8_t)((value & 0x00000008) >> 3);
  clkdown->ddr4_clk3 = (uint8_t)((value & 0x00000010) >> 4);
  clkdown->qsfp_clk0 = (uint8_t)((value & 0x00000020) >> 5);
  clkdown->qsfp_clk1 = (uint8_t)((value & 0x00000040) >> 6);
}


void
xpcie_fpga_clk_dwn_force(
  fpga_dev_info_t *dev,
  fpga_ioctl_clkdown_t *clkdown)
{
  uint32_t value;
  value =  ((uint32_t)clkdown->user_clk  & 0x01);
  value |= ((uint32_t)clkdown->ddr4_clk0 & 0x01) << 1;
  value |= ((uint32_t)clkdown->ddr4_clk1 & 0x01) << 2;
  value |= ((uint32_t)clkdown->ddr4_clk2 & 0x01) << 3;
  value |= ((uint32_t)clkdown->ddr4_clk3 & 0x01) << 4;
  value |= ((uint32_t)clkdown->qsfp_clk0 & 0x01) << 5;
  value |= ((uint32_t)clkdown->qsfp_clk1 & 0x01) << 6;
  reg_write32(dev, XPCIE_FPGA_CLKDOWN_FORCE, value);
}


void
xpcie_fpga_clk_dwn_get_force(
  fpga_dev_info_t *dev,
  fpga_ioctl_clkdown_t *clkdown)
{
  uint32_t value = reg_read32(dev, XPCIE_FPGA_CLKDOWN_FORCE);
  clkdown->user_clk  = (uint8_t)(value  & 0x00000001);
  clkdown->ddr4_clk0 = (uint8_t)((value & 0x00000002) >> 1);
  clkdown->ddr4_clk1 = (uint8_t)((value & 0x00000004) >> 2);
  clkdown->ddr4_clk2 = (uint8_t)((value & 0x00000008) >> 3);
  clkdown->ddr4_clk3 = (uint8_t)((value & 0x00000010) >> 4);
  clkdown->qsfp_clk0 = (uint8_t)((value & 0x00000020) >> 5);
  clkdown->qsfp_clk1 = (uint8_t)((value & 0x00000040) >> 6);
}


void
xpcie_fpga_ecc_err_det(
  fpga_dev_info_t *dev,
  fpga_ioctl_eccerr_t *eccerr)
{
  if (eccerr->type == ECCERR_TYPE_SINGLE) {
    /* single */
    eccerr->eccerr = reg_read32(dev, XPCIE_FPGA_DDR4_ECC_SINGLE);
  }
  else if (eccerr->type == ECCERR_TYPE_MULTI) {
    /* multi */
    eccerr->eccerr = reg_read32(dev, XPCIE_FPGA_DDR4_ECC_MULTI);
  } else {
    /* Do Nothing */
    xpcie_err("type(%u) is not the expected value.\n", eccerr->type);
  }
}


void
xpcie_fpga_ecc_err_clear(
  fpga_dev_info_t *dev,
  fpga_ioctl_eccerr_t *eccerr)
{
  if (eccerr->type == ECCERR_TYPE_SINGLE) {
    /* single */
    reg_write32(dev, XPCIE_FPGA_DDR4_ECC_SINGLE, eccerr->eccerr);
  }
  else if (eccerr->type == ECCERR_TYPE_MULTI) {
    /* multi */
    reg_write32(dev, XPCIE_FPGA_DDR4_ECC_MULTI, eccerr->eccerr);
  } else {
    /* Do Nothing */
    xpcie_err("type(%u) is not the expected value.\n", eccerr->type);
  }
}


void
xpcie_fpga_ecc_err_raw_det(
  fpga_dev_info_t *dev,
  fpga_ioctl_eccerr_t *eccerr)
{
  if (eccerr->type == ECCERR_TYPE_SINGLE) {
    /* single */
    eccerr->eccerr = reg_read32(dev, XPCIE_FPGA_DDR4_ECC_SINGLE_RAW);
  }
  else if (eccerr->type == ECCERR_TYPE_MULTI) {
    /* multi */
    eccerr->eccerr = reg_read32(dev, XPCIE_FPGA_DDR4_ECC_MULTI_RAW);
  } else {
    /* Do Nothing */
    xpcie_err("type(%u) is not the expected value.\n", eccerr->type);
  }
}


void
xpcie_fpga_ecc_err_mask(
  fpga_dev_info_t *dev,
  fpga_ioctl_eccerr_t *eccerr)
{
  if (eccerr->type == ECCERR_TYPE_SINGLE) {
    /* single */
    reg_write32(dev, XPCIE_FPGA_DDR4_ECC_SINGLE_MASK, eccerr->eccerr);
  }
  else if (eccerr->type == ECCERR_TYPE_MULTI) {
    /* multi */
    reg_write32(dev, XPCIE_FPGA_DDR4_ECC_MULTI_MASK, eccerr->eccerr);
  } else {
    /* Do Nothing */
    xpcie_err("type(%u) is not the expected value.\n", eccerr->type);
  }
}


void
xpcie_fpga_ecc_err_get_mask(
  fpga_dev_info_t *dev,
  fpga_ioctl_eccerr_t *eccerr)
{
  if (eccerr->type == ECCERR_TYPE_SINGLE) {
    /* single */
    eccerr->eccerr = reg_read32(dev, XPCIE_FPGA_DDR4_ECC_SINGLE_MASK);
  }
  else if (eccerr->type == ECCERR_TYPE_MULTI) {
    /* multi */
    eccerr->eccerr = reg_read32(dev, XPCIE_FPGA_DDR4_ECC_MULTI_MASK);
  } else {
    /* Do Nothing */
    xpcie_err("type(%u) is not the expected value.\n", eccerr->type);
  }
}


void
xpcie_fpga_ecc_err_force(
  fpga_dev_info_t *dev,
  fpga_ioctl_eccerr_t *eccerr)
{
  if (eccerr->type == ECCERR_TYPE_SINGLE) {
    /* single */
    reg_write32(dev, XPCIE_FPGA_DDR4_ECC_SINGLE_FORCE, eccerr->eccerr);
  }
  else if (eccerr->type == ECCERR_TYPE_MULTI) {
    /* multi */
    reg_write32(dev, XPCIE_FPGA_DDR4_ECC_MULTI_FORCE, eccerr->eccerr);
  } else {
    /* Do Nothing */
    xpcie_err("type(%u) is not the expected value.\n", eccerr->type);
  }
}


void
xpcie_fpga_ecc_err_get_force(
  fpga_dev_info_t *dev,
  fpga_ioctl_eccerr_t *eccerr)
{
  if (eccerr->type == ECCERR_TYPE_SINGLE) {
    /* single */
    eccerr->eccerr = reg_read32(dev, XPCIE_FPGA_DDR4_ECC_SINGLE_FORCE);
  }
  else if (eccerr->type == ECCERR_TYPE_MULTI) {
    /* multi */
    eccerr->eccerr = reg_read32(dev, XPCIE_FPGA_DDR4_ECC_MULTI_FORCE);
  } else {
    /* Do Nothing */
    xpcie_err("type(%u) is not the expected value.\n", eccerr->type);
  }
}
