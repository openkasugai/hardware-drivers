/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file libxpcie_ptu.h
 * @brief Header file for functions for PTU module
 */

#ifndef __PTU_LIBXPCIE_PTU_H__
#define __PTU_LIBXPCIE_PTU_H__

#include <libxpcie.h>


/**
 * @brief PTU: Function which get information about PTU module
 */
int xpcie_fpga_common_get_ptu_module_info(
        fpga_dev_info_t *dev);

#endif  /* __PTU_LIBXPCIE_PTU_H__ */
