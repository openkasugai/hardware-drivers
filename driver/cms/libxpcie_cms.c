/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include <linux/delay.h>

#include "libxpcie_cms.h"
#include "xpcie_regs_cms.h"


int
xpcie_fpga_common_get_cms_module_info(
  fpga_dev_info_t *dev)
{
  fpga_module_info_t *info = &dev->mods.cms;

  // Set Module base address
  info->base = XPCIE_FPGA_CMS_OFFSET;

  // Set Module length per 1 lane
  info->len = XPCIE_FPGA_CMS_SIZE;

  // Set Module num
  info->num = 1;

  return 0;
}


void
xpcie_fpga_get_power_info_u250(
  fpga_dev_info_t *dev,
  fpga_power_t *power_info)
{
  power_info->pcie_12V_voltage  = cms_reg_read(dev, XPCIE_FPGA_POWER_PCIE_12V_VOLTAGE);
  power_info->pcie_12V_current  = cms_reg_read(dev, XPCIE_FPGA_POWER_PCIE_12V_CURRENT);
  power_info->aux_12V_voltage   = cms_reg_read(dev, XPCIE_FPGA_POWER_AUX_12V_VOLTAGE);
  power_info->aux_12V_current   = cms_reg_read(dev, XPCIE_FPGA_POWER_AUX_12V_CURRENT);
  power_info->pex_3V3_voltage   = cms_reg_read(dev, XPCIE_FPGA_POWER_PEX_3V3_VOLTAGE);
  power_info->pex_3V3_current   = cms_reg_read(dev, XPCIE_FPGA_POWER_PEX_3V3_CURRENT);
  power_info->pex_3V3_power     = cms_reg_read(dev, XPCIE_FPGA_POWER_PEX_3V3_POWER);
  power_info->aux_3V3_voltage   = cms_reg_read(dev, XPCIE_FPGA_POWER_AUX_3V3_VOLTAGE);
  power_info->aux_3V3_current   = cms_reg_read(dev, XPCIE_FPGA_POWER_AUX_3V3_CURRENT);
  power_info->vccint_voltage    = cms_reg_read(dev, XPCIE_FPGA_POWER_VCCINT_VOLTAGE);
  power_info->vccint_current    = cms_reg_read(dev, XPCIE_FPGA_POWER_VCCINT_CURRENT);
}


void
xpcie_fpga_get_power_info(
  fpga_dev_info_t *dev,
  fpga_ioctl_power_t *power_info)
{
  switch (power_info->flag) {
    case U250_PCIE_12V_VOLTAGE :
      power_info->power = cms_reg_read(dev, XPCIE_FPGA_POWER_PCIE_12V_VOLTAGE);
      break;
    case U250_PCIE_12V_CURRENT:
      power_info->power = cms_reg_read(dev, XPCIE_FPGA_POWER_PCIE_12V_CURRENT);
      break;
    case U250_AUX_12V_VOLTAGE:
      power_info->power = cms_reg_read(dev, XPCIE_FPGA_POWER_AUX_12V_VOLTAGE);
      break;
    case U250_AUX_12V_CURRENT:
      power_info->power = cms_reg_read(dev, XPCIE_FPGA_POWER_AUX_12V_CURRENT);
      break;
    case U250_PEX_3V3_VOLTAGE:
      power_info->power = cms_reg_read(dev, XPCIE_FPGA_POWER_PEX_3V3_VOLTAGE);
      break;
    case U250_PEX_3V3_CURRENT:
      power_info->power = cms_reg_read(dev, XPCIE_FPGA_POWER_PEX_3V3_CURRENT);
      break;
    case U250_PEX_3V3_POWER:
      power_info->power = cms_reg_read(dev, XPCIE_FPGA_POWER_PEX_3V3_POWER);
      break;
    case U250_AUX_3V3_VOLTAGE:
      power_info->power = cms_reg_read(dev, XPCIE_FPGA_POWER_AUX_3V3_VOLTAGE);
      break;
    case U250_AUX_3V3_CURRENT:
      power_info->power = cms_reg_read(dev, XPCIE_FPGA_POWER_AUX_3V3_CURRENT);
      break;
    case U250_VCCINT_VOLTAGE:
      power_info->power = cms_reg_read(dev, XPCIE_FPGA_POWER_VCCINT_VOLTAGE);
      break;
    case U250_VCCINT_CURRENT:
      power_info->power = cms_reg_read(dev, XPCIE_FPGA_POWER_VCCINT_CURRENT);
      break;
    default:
      // Do Nothing
      xpcie_err("%s : flag(%u) is not the expected value.", __func__, power_info->flag);
      break;
  }
}


void
xpcie_fpga_get_temp_info(
  fpga_dev_info_t *dev,
  fpga_ioctl_temp_t *temp_info)
{
  switch (temp_info->flag) {
    case U250_CAGE_TEMP0:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_CAGE_TEMP0);
      break;
    case U250_CAGE_TEMP1:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_CAGE_TEMP1);
      break;
    case U250_DIMM_TEMP0:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_DIMM_TEMP0);
      break;
    case U250_DIMM_TEMP1:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_DIMM_TEMP1);
      break;
    case U250_DIMM_TEMP2:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_DIMM_TEMP2);
      break;
    case U250_DIMM_TEMP3:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_DIMM_TEMP3);
      break;
    case U250_FAN_TEMP:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_FAN_TEMP);
      break;
    case U250_FPGA_TEMP:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_FPGA_TEMP);
      break;
    case U250_SE98_TEMP0:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_SE98_TEMP0);
      break;
    case U250_SE98_TEMP1:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_SE98_TEMP1);
      break;
    case U250_SE98_TEMP2:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_SE98_TEMP2);
      break;
    case U250_VCCINT_TEMP:
      temp_info->temp = cms_reg_read(dev, XPCIE_FPGA_TEMP_VCCINT_TEMP);
      break;
    default:
      // Do Nothing
      xpcie_err("flag(%u) is not the expected value.\n", temp_info->flag);
      break;
  }
}


void
xpcie_fpga_set_cms_unrest(
  fpga_dev_info_t *dev,
  uint32_t data)
{
  xpcie_trace("%s:data(%d)", __func__, data);

  // Reset CMS
  cms_reg_write(dev, XPCIE_FPGA_CMS_UNREST, data);

  // 1s wait
  ssleep(1);
}


int
xpcie_fpga_get_mailbox(
  fpga_dev_info_t *dev,
  char serial_id[],
  char card_name[])
{
  int ret, try_num, try_num_max;
  uint32_t data;
  uint32_t addr, index;

  xpcie_trace("%s:(-)", __func__);

  // Reset CMS
  xpcie_fpga_set_cms_unrest(dev, 1);

  // Check whether no error
  ret = cms_reg_read(dev, XPCIE_FPGA_CMS_MAILBOX_STATUS);
  if(ret & 0b10){
    xpcie_err("CMS error exists...");
    return -EBUSY;
  }

  // Write command
  cms_reg_write(dev, XPCIE_FPGA_CMS_MAILBOX, XPCIE_FPGA_CMS_SERIAL_GET_CMD);

  // Notice command written in mailbox
  cms_reg_write(dev, XPCIE_FPGA_CMS_MAILBOX_STATUS, XPCIE_FPGA_CMS_MAILBOX_NOTICE_CMD);

  // Wait 1sec
  ssleep(1);

  try_num_max = 60 * 5; // wait for getting serial_id max 5min
  for(try_num = 0; try_num < try_num_max; try_num++){
    // Check whether no error
    ret = cms_reg_read(dev, XPCIE_FPGA_CMS_MAILBOX_STATUS);
    if(!(ret & 0b100000)){
      break;
    }
    if (!(try_num % 10))
      xpcie_info("CMS status check validation[%d]", try_num);
    ssleep(1);
  }
  if(try_num == try_num_max){
    xpcie_err("CMS status did NOT become valid...");
    xpcie_err(" offset(%#x): %#x", XPCIE_FPGA_CMS_MAILBOX_STATUS, ret);
    return -EBUSY;
  }

  // Check no message error
  ret = cms_reg_read(dev, XPCIE_FPGA_CMS_MESSAGE_ERROR);
  if(ret & 0b11111111111){
    xpcie_err("CMS error exists...");
    return -EBUSY;
  }

  // Get serial_id
  for(try_num = 0; try_num < SERIAL_ID_LEN/sizeof(uint32_t); try_num++){
    data = cms_reg_read(dev, XPCIE_FPGA_CMS_MAILBOX_SERIAL_ID + try_num * sizeof(uint32_t));
    memcpy(&serial_id[try_num * sizeof(uint32_t)], &data, sizeof(uint32_t));
  }

  // Get card_name
  addr = XPCIE_4BYTE_ALIGNED(XPCIE_FPGA_CMS_MAILBOX_CARD_NAME);
  data = cms_reg_read(dev, addr);
  index = 0;
  card_name[index++] = (data >> 16) & 0xFF;
  card_name[index++] = (data >> 24) & 0xFF;
  addr += sizeof(uint32_t);
  data = cms_reg_read(dev, addr);
  memcpy(&card_name[index], &data, sizeof(uint32_t));
  addr += sizeof(uint32_t);
  data = cms_reg_read(dev, addr);
  index += 4;
  memcpy(&card_name[index], &data, sizeof(uint32_t));
  addr += sizeof(uint32_t);
  data = cms_reg_read(dev, addr);
  index += 4;
  memcpy(&card_name[index], &data, sizeof(uint32_t)/sizeof(uint32_t) * 3);

  return 0;
}
