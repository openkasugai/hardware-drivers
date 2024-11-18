/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file libxpcie_global.h
 * @brief Header file for functions for global module
 */

#ifndef __GLOBAL_LIBXPCIE_GLOBAL_H__
#define __GLOBAL_LIBXPCIE_GLOBAL_H__

#include <libxpcie.h>


/**
 * @brief Global: Function which execute ioctl commands for Global module
 */
inline long xpcie_fpga_ioctl_global(
        struct file *filp,
        unsigned int cmd,
        unsigned long arg);

/**
 * @brief Global: Function which get information about Global module
 */
int xpcie_fpga_common_get_global_module_info(
        fpga_dev_info_t *dev);

/**
 * @brief Global: Function which get global major version
 */
uint32_t xpcie_fpga_global_get_major_version(
        fpga_dev_info_t *dev);

/**
 * @brief Global: Function which get global minor version
 */
uint32_t xpcie_fpga_global_get_minor_version(
        fpga_dev_info_t *dev);

/**
 * @brief Global: SoftReset
 */
void xpcie_fpga_soft_rst(
        fpga_dev_info_t *dev);

/**
 * @brief Global
 */
void xpcie_fpga_chk_err(
        fpga_dev_info_t *dev,
        uint32_t *check_err);

/**
 * @brief Global
 */
void xpcie_fpga_clk_dwn_det(
        fpga_dev_info_t *dev,
        fpga_ioctl_clkdown_t *clkdown);

/**
 * @brief Global
 */
void xpcie_fpga_clk_dwn_clear(
        fpga_dev_info_t *dev,
        fpga_ioctl_clkdown_t *clkdown);

/**
 * @brief Global
 */
void xpcie_fpga_clk_dwn_raw_det(
        fpga_dev_info_t *dev,
        fpga_ioctl_clkdown_t *clkdown);

/**
 * @brief Global
 */
void xpcie_fpga_clk_dwn_mask(
        fpga_dev_info_t *dev,
        fpga_ioctl_clkdown_t *clkdown);

/**
 * @brief Global
 */
void xpcie_fpga_clk_dwn_get_mask(
        fpga_dev_info_t *dev,
        fpga_ioctl_clkdown_t *clkdown);

/**
 * @brief Global
 */
void xpcie_fpga_clk_dwn_force(
        fpga_dev_info_t *dev,
        fpga_ioctl_clkdown_t *clkdown);

/**
 * @brief Global
 */
void xpcie_fpga_clk_dwn_get_force(
        fpga_dev_info_t *dev,
        fpga_ioctl_clkdown_t *clkdown);

/**
 * @brief Global
 */
void xpcie_fpga_ecc_err_det(
        fpga_dev_info_t *dev,
        fpga_ioctl_eccerr_t *eccerr);

/**
 * @brief Global
 */
void xpcie_fpga_ecc_err_clear(
        fpga_dev_info_t *dev,
        fpga_ioctl_eccerr_t *eccerr);

/**
 * @brief Global
 */
void xpcie_fpga_ecc_err_raw_det(
        fpga_dev_info_t *dev,
        fpga_ioctl_eccerr_t *eccerr);

/**
 * @brief Global
 */
void xpcie_fpga_ecc_err_mask(
        fpga_dev_info_t *dev,
        fpga_ioctl_eccerr_t *eccerr);

/**
 * @brief Global
 */
void xpcie_fpga_ecc_err_get_mask(
        fpga_dev_info_t *dev,
        fpga_ioctl_eccerr_t *eccerr);

/**
 * @brief Global
 */
void xpcie_fpga_ecc_err_force(
        fpga_dev_info_t *dev,
        fpga_ioctl_eccerr_t *eccerr);

/**
 * @brief Global
 */
void xpcie_fpga_ecc_err_get_force(
        fpga_dev_info_t *dev,
        fpga_ioctl_eccerr_t *eccerr);

#endif  /* __GLOBAL_LIBXPCIE_GLOBAL_H__ */
