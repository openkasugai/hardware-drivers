/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file xpcie_regs_func.h
 * @brief Header file for register map for function module
 */

#ifndef __FUNC_XPCIE_REGS_FUNC_H__
#define __FUNC_XPCIE_REGS_FUNC_H__


// Function Register Map : No Functional Access from driver
#define XPCIE_FPGA_FUNC_OFFSET                  0x00025000  /**< Base address for Function */
#define XPCIE_FPGA_FUNC_SIZE                    0x1000      /**< Size of Function per a lane */

#define XPCIE_FPGA_FUNC_MODULE_ID               0x0010

#define XPCIE_FPGA_FUNC_MODULE_ID_FR_RSZ_VALUE  0x0000F2C2  /**< module_id : filter_resize */

#endif  /* __FUNC_XPCIE_REGS_FUNC_H__ */
