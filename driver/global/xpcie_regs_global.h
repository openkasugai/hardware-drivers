/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file xpcie_regs_global.h
 * @brief Header file for register map for global module
 */

#ifndef __GLOBAL_XPCIE_REGS_GLOBAL_H__
#define __GLOBAL_XPCIE_REGS_GLOBAL_H__


// Global Register Map : 0x0000_0000-0x0000_0FFF
#define XPCIE_FPGA_GLOBAL_OFFSET              0x0000000000 /**< Base address for Global */
#define XPCIE_FPGA_GLOBAL_SIZE                0x1000       /**< Size of Global */

#define XPCIE_FPGA_GLOBAL_MAJOR_VERSION       0x0000
#define XPCIE_FPGA_GLOBAL_MINOR_VERSION       0x0004

#define XPCIE_FPGA_SOFT_RST                   0x000C

#define XPCIE_FPGA_CHECK_ERR                  0x0100

#define XPCIE_FPGA_CLKDOWN                    0x0110
#define XPCIE_FPGA_CLKDOWN_RAW                0x0114
#define XPCIE_FPGA_CLKDOWN_MASK               0x0118
#define XPCIE_FPGA_CLKDOWN_FORCE              0x011C

#define XPCIE_FPGA_DDR4_ECC_SINGLE            0x0120
#define XPCIE_FPGA_DDR4_ECC_MULTI             0x0124
#define XPCIE_FPGA_DDR4_ECC_SINGLE_RAW        0x0128
#define XPCIE_FPGA_DDR4_ECC_MULTI_RAW         0x012C
#define XPCIE_FPGA_DDR4_ECC_SINGLE_MASK       0x0130
#define XPCIE_FPGA_DDR4_ECC_MULTI_MASK        0x0134
#define XPCIE_FPGA_DDR4_ECC_SINGLE_FORCE      0x0138
#define XPCIE_FPGA_DDR4_ECC_MULTI_FORCE       0x013C

#endif  /* __GLOBAL_XPCIE_REGS_GLOBAL_H__ */
