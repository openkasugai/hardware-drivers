/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file xpcie_regs_ptu.h
 * @brief Header file for register map for PTU module
 */

#ifndef __PTU_XPCIE_REGS_PTU_H__
#define __PTU_XPCIE_REGS_PTU_H__


// PTU Register Map : No Functional Access from driver
#define XPCIE_FPGA_PTU_OFFSET 0x00020000  /**< Base address for ptu module */
#define XPCIE_FPGA_PTU_SIZE   0x1000      /**< Size of ptu module per a lane */

#endif  /* __PTU_XPCIE_REGS_PTU_H__ */
