/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file xpcie_regs_conv.h
 * @brief Header file for register map for conversion module
 */

#ifndef __CONV_XPCIE_REGS_CONV_H__
#define __CONV_XPCIE_REGS_CONV_H__


// Conv_Adapter Register Map : No Functional Access from driver
#define XPCIE_FPGA_CONV_OFFSET                    0x00024000  /**< Base address for Conv_Adapter */
#define XPCIE_FPGA_CONV_SIZE                      0x0400      /**< Size of Conv_Adapter per a lane */

#define XPCIE_FPGA_CONV_MODULE_ID                 0x0010

#define XPCIE_FPGA_CONV_MODULE_ID_DECODE_VALUE    0x0000F1C1  /**< module_id : decode/change_color */
#define XPCIE_FPGA_CONV_MODULE_ID_FR_RSZ_VALUE    0x0000F1C2  /**< module_id : filter_resize */
#define XPCIE_FPGA_CONV_MODULE_ID_INFERENCE_VALUE 0x0000F1C3  /**< module_id : Inference */


#endif  /* __CONV_XPCIE_REGS_CONV_H__ */
