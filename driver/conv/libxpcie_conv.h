/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file libxpcie_conv.h
 * @brief Header file for functions for conversion module
 */

#ifndef __CONV_LIBXPCIE_CONV_H__
#define __CONV_LIBXPCIE_CONV_H__

#include <libxpcie.h>


/**
 * @brief CONV: Function which get information about CONV module
 */
int xpcie_fpga_common_get_conv_module_info(
        fpga_dev_info_t *dev);

#endif  /* __CONV_LIBXPCIE_CONV_H__ */
