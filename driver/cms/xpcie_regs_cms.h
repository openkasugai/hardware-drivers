/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file xpcie_regs_cms.h
 * @brief Header file for register map for CMS module
 */

#ifndef __CMS_XPCIE_REGS_CMS_H__
#define __CMS_XPCIE_REGS_CMS_H__


// CMS(MicroBlaze) Register Map
#define XPCIE_FPGA_CMS_OFFSET             0x00040000  /**< Base address for CMS */
#define XPCIE_FPGA_CMS_SIZE               0x40000     /**< Size of CMS */

#define XPCIE_FPGA_CMS_UNREST             0x20000

// POWER efficiency
#define XPCIE_FPGA_POWER_PCIE_12V_VOLTAGE 0x28028
#define XPCIE_FPGA_POWER_PCIE_12V_CURRENT 0x280D0
#define XPCIE_FPGA_POWER_AUX_12V_VOLTAGE  0x2804C
#define XPCIE_FPGA_POWER_AUX_12V_CURRENT  0x280DC
#define XPCIE_FPGA_POWER_PEX_3V3_VOLTAGE  0x28034
#define XPCIE_FPGA_POWER_PEX_3V3_CURRENT  0x28280
#define XPCIE_FPGA_POWER_PEX_3V3_POWER    0x282EC
#define XPCIE_FPGA_POWER_AUX_3V3_VOLTAGE  0x28040
#define XPCIE_FPGA_POWER_AUX_3V3_CURRENT  0x282F8
#define XPCIE_FPGA_POWER_VCCINT_VOLTAGE   0x280E8
#define XPCIE_FPGA_POWER_VCCINT_CURRENT   0x280F4

// Temperature
#define XPCIE_FPGA_TEMP_CAGE_TEMP0        0x28174
#define XPCIE_FPGA_TEMP_CAGE_TEMP1        0x28180
#define XPCIE_FPGA_TEMP_DIMM_TEMP0        0x28114
#define XPCIE_FPGA_TEMP_DIMM_TEMP1        0x28120
#define XPCIE_FPGA_TEMP_DIMM_TEMP2        0x2812C
#define XPCIE_FPGA_TEMP_DIMM_TEMP3        0x28138
#define XPCIE_FPGA_TEMP_FAN_TEMP          0x28108
#define XPCIE_FPGA_TEMP_FPGA_TEMP         0x280FC
#define XPCIE_FPGA_TEMP_SE98_TEMP0        0x28144
#define XPCIE_FPGA_TEMP_SE98_TEMP1        0x28150
#define XPCIE_FPGA_TEMP_SE98_TEMP2        0x2815C
#define XPCIE_FPGA_TEMP_VCCINT_TEMP       0x282D0

// mailbox
#define XPCIE_FPGA_CMS_MAILBOX_STATUS     0x28018
#define XPCIE_FPGA_CMS_MAILBOX_POS        0x28300
#define XPCIE_FPGA_CMS_MESSAGE_ERROR      0x28304
#define XPCIE_FPGA_CMS_MAILBOX            0x29000
#define XPCIE_FPGA_CMS_MAILBOX_SERIAL_ID  (0x1C + XPCIE_FPGA_CMS_MAILBOX)
#define XPCIE_FPGA_CMS_MAILBOX_CARD_NAME  (0x06 + XPCIE_FPGA_CMS_MAILBOX)
#define XPCIE_4BYTE_ALIGNED(ADDR)         (((ADDR) - 0b1) & (~(0b11)))

// Commands for CMS
#define XPCIE_FPGA_CMS_SERIAL_GET_CMD     0x04000000
#define XPCIE_FPGA_CMS_MAILBOX_NOTICE_CMD 0x00000020

#endif  /* __CMS_XPCIE_REGS_CMS_H__ */
