/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file libxpcie_cms.h
 * @brief Header file for functions for CMS module
 */

#ifndef __CMS_LIBXPCIE_CMS_H__
#define __CMS_LIBXPCIE_CMS_H__

#include <libxpcie.h>


/**
 * @brief CMS: Function which execute ioctl commands for CMS module
 */
inline long xpcie_fpga_ioctl_cms(
        struct file *filp,
        unsigned int cmd,
        unsigned long arg);

/**
 * @brief CMS: Function which get information about CMS
 */
int xpcie_fpga_common_get_cms_module_info(
        fpga_dev_info_t *dev);

/**
 * @brief CMS: Function which get power information for Alveo U250
 */
void xpcie_fpga_get_power_info_u250(
        fpga_dev_info_t *dev,
        fpga_power_t *power_info);

/**
 * @brief CMS: Function which get power information for at selected register
 */
void xpcie_fpga_get_power_info(
        fpga_dev_info_t *dev,
        fpga_ioctl_power_t *power_info);

/**
 * @brief CMS: Function which reset CMS
 */
void xpcie_fpga_set_cms_unrest(
        fpga_dev_info_t *dev,
        uint32_t data);

/**
 * @brief CMS: Function which get temperature information for at selected register
 */
void xpcie_fpga_get_temp_info(
        fpga_dev_info_t *dev,
        fpga_ioctl_temp_t *temp_info);

/**
 * @brief CMS: Function which get serial id and card name
 */
int xpcie_fpga_get_mailbox(
        fpga_dev_info_t *dev,
        char *seiral_id,
        char *card_name);

#endif  /* __CMS_LIBXPCIE_CMS_H__ */
