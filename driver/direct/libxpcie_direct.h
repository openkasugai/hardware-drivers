/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file libxpcie_direct.h
 * @brief Header file for functions for direct module
 */

#ifndef __DIRECT_LIBXPCIE_DIRECT_H__
#define __DIRECT_LIBXPCIE_DIRECT_H__

#include <libxpcie.h>


/**
 * @brief Direct: Function which execute ioctl commands for Direct module
 */
inline long xpcie_fpga_ioctl_direct(
        struct file *filp,
        unsigned int cmd,
        unsigned long arg);

/**
 * @brief Direct: Function which get information about Direct module
 */
int xpcie_fpga_common_get_direct_module_info(
        fpga_dev_info_t *dev);

/**
 * @brief Direct
 */
void xpcie_fpga_start_direct_module(
        fpga_dev_info_t *dev,
        uint32_t kernel_lane);

/**
 * @brief Direct
 */
void xpcie_fpga_stop_direct_module(
        fpga_dev_info_t *dev,
        uint32_t kernel_lane);

/**
 * @brief Direct
 */
void xpcie_fpga_get_direct_bytes(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_bytenum_t *bytenum);

/**
 * @brief Direct
 */
void xpcie_fpga_get_direct_frames(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_framenum_t *framenum);

/**
 * @brief Direct
 */
void xpcie_fpga_check_direct_err(
        fpga_dev_info_t *dev,
        fpga_ioctl_err_all_t *err);

/**
 * @brief Direct
 */
void xpcie_fpga_detect_direct_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_prot_t *direct_err_prot);

/**
 * @brief Direct
 */
void xpcie_fpga_clear_direct_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_prot_t *direct_err_prot);

/**
 * @brief Direct
 */
void xpcie_fpga_mask_direct_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_prot_t *direct_err_prot);

/**
 * @brief Direct
 */
void xpcie_fpga_get_mask_direct_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_prot_t *direct_err_prot);

/**
 * @brief Direct
 */
void xpcie_fpga_force_direct_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_prot_t *direct_err_prot);

/**
 * @brief Direct
 */
void xpcie_fpga_get_force_direct_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_prot_t *direct_err_prot);

/**
 * @brief Direct
 */
void xpcie_fpga_ins_direct_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_prot_t *direct_err_prot);

/**
 * @brief Direct
 */
void xpcie_fpga_get_ins_direct_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_prot_t *direct_err_prot);

/**
 * @brief Direct
 */
void xpcie_fpga_detect_direct_err_stif(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_stif_t *direct_err_stif);

/**
 * @brief Direct
 */
void xpcie_fpga_mask_direct_err_stif(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_stif_t *direct_err_stif);

/**
 * @brief Direct
 */
void xpcie_fpga_get_mask_direct_err_stif(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_stif_t *direct_err_stif);

/**
 * @brief Direct
 */
void xpcie_fpga_force_direct_err_stif(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_stif_t *direct_err_stif);

/**
 * @brief Direct
 */
void xpcie_fpga_get_force_direct_err_stif(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_err_stif_t *direct_err_stif);

/**
 * @brief Direct
 */
void xpcie_fpga_get_direct_ctrl(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_ctrl_t *direct_ctrl);

/**
 * @brief Direct
 */
void xpcie_fpga_get_direct_module_id(
        fpga_dev_info_t *dev,
        fpga_ioctl_direct_ctrl_t *direct_ctrl);

#endif  /* __DIRECT_LIBXPCIE_DIRECT_H__ */
