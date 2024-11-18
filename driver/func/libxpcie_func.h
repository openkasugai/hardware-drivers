/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file libxpcie_func.h
 * @brief Header file for functions for function module
 */

#ifndef __FUNC_LIBXPCIE_FUNC_H__
#define __FUNC_LIBXPCIE_FUNC_H__

#include <libxpcie.h>


/**
 * @brief FUNC: Function which get information about FUNC module
 */
int xpcie_fpga_common_get_func_module_info(
        fpga_dev_info_t *dev);

#endif  /* __FUNC_LIBXPCIE_FUNC_H__ */
