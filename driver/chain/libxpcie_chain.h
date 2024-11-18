/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file libxpcie_chain.h
 * @brief Header file for functions for chain module
 */

#ifndef __CHAIN_LIBXPCIE_CHAIN_H__
#define __CHAIN_LIBXPCIE_CHAIN_H__

#include <libxpcie.h>


/**
 * @brief Chain: Function which execute ioctl command for Chain module
 */
inline long xpcie_fpga_ioctl_chain(
        struct file *filp,
        unsigned int cmd,
        unsigned long arg);

/**
 * @brief Chain: Function which get information about Chain module
 */
int xpcie_fpga_common_get_chain_module_info(
        fpga_dev_info_t *dev);

/**
 * @brief Chain: Function which update Function chain table
 */
int xpcie_fpga_update_func_chain_table(
        fpga_dev_info_t *dev,
        fpga_id_t *id,
        uint32_t kind);

/**
 * @brief Chain: Function which delete Function chain table
 */
int xpcie_fpga_delete_func_chain_table(
        fpga_dev_info_t *dev,
        fpga_id_t *id,
        uint32_t kind);

/**
 * @brief Chain: Function which read Function chain table
 */
int xpcie_fpga_read_func_chain_table(
        fpga_dev_info_t *dev,
        fpga_id_t *id,
        uint32_t kind);

/**
 * @brief Chain: Function which read Function chain table in driver
 */
void xpcie_fpga_read_chain_soft_table(
        fpga_dev_info_t *dev,
        uint32_t lane,
        uint32_t fchid,
        uint32_t *ingress_extif_id,
        uint32_t *ingress_cid,
        uint32_t *egress_extif_id,
        uint32_t *egress_cid);

/**
 * @brief Chain: Function which reset Function chain table in driver
 */
void xpcie_fpga_reset_chain_soft_table(
        fpga_dev_info_t *dev);

/**
 * @brief Chain: Function which start Chain module
 */
void xpcie_fpga_start_chain_module(
        fpga_dev_info_t *dev,
        uint32_t kernel_lane);

/**
 * @brief Chain: Function which stop Chain module
 */
void xpcie_fpga_stop_chain_module(
        fpga_dev_info_t *dev,
        uint32_t kernel_lane);

/**
 * @brief Chain
 */
void xpcie_fpga_set_ddr_offset_frame(
        fpga_dev_info_t *dev,
        fpga_ioctl_extif_t *extif);

/**
 * @brief Chain
 */
void xpcie_fpga_get_ddr_offset_frame(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_ddr_t  *chain_ddr);

/**
 * @brief Chain
 */
void xpcie_fpga_get_latency_chain(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_latency_t *latency);

/**
 * @brief Chain
 */
void xpcie_fpga_get_latency_func(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_func_latency_t *latency);

/**
 * @brief Chain
 */
void xpcie_fpga_get_chain_bytes(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_bytenum_t *bytenum);

/**
 * @brief Chain
 */
void xpcie_fpga_get_chain_frames(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_framenum_t *framenum);

/**
 * @brief Chain
 */
void xpcie_fpga_get_chain_buff(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_framenum_t *framenum);

/**
 * @brief Chain
 */
void xpcie_fpga_get_chain_bp(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_framenum_t *framenum);

/**
 * @brief Chain
 */
void xpcie_fpga_clear_chain_bp(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_framenum_t *framenum);

/**
 * @brief Chain
 */
void xpcie_fpga_get_chain_busy(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_framenum_t *busy);

/**
 * @brief Chain
 */
void xpcie_fpga_check_chain_err(
        fpga_dev_info_t *dev,
        fpga_ioctl_err_all_t *err);

/**
 * @brief Chain
 */
void xpcie_fpga_detect_chain_err(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_mask_chain_err(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_get_mask_chain_err(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_force_chain_err(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_get_force_chain_err(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_ins_chain_err(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_get_ins_chain_err(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_detect_chain_err_table(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_table_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_mask_chain_err_table(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_table_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_get_mask_chain_err_table(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_table_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_force_chain_err_table(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_table_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_get_force_chain_err_table(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_table_t *chain_err);

/**
 * @brief Chain
 */
void xpcie_fpga_detect_chain_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_prot_t *chain_err_prot);

/**
 * @brief Chain
 */
void xpcie_fpga_clear_chain_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_prot_t *chain_err_prot);

/**
 * @brief Chain
 */
void xpcie_fpga_mask_chain_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_prot_t *chain_err_prot);

/**
 * @brief Chain
 */
void xpcie_fpga_get_mask_chain_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_prot_t *chain_err_prot);

/**
 * @brief Chain
 */
void xpcie_fpga_force_chain_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_prot_t *chain_err_prot);

/**
 * @brief Chain
 */
void xpcie_fpga_get_force_chain_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_prot_t *chain_err_prot);

/**
 * @brief Chain
 */
void xpcie_fpga_ins_chain_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_prot_t *chain_err_prot);

/**
 * @brief Chain
 */
void xpcie_fpga_get_ins_chain_err_prot(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_prot_t *chain_err_prot);

/**
 * @brief Chain
 */
void xpcie_fpga_detect_chain_err_evt(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_evt_t *chain_err_evt);

/**
 * @brief Chain
 */
void xpcie_fpga_clear_chain_err_evt(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_evt_t *chain_err_evt);

/**
 * @brief Chain
 */
void xpcie_fpga_mask_chain_err_evt(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_evt_t *chain_err_evt);

/**
 * @brief Chain
 */
void xpcie_fpga_get_mask_chain_err_evt(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_evt_t *chain_err_evt);

/**
 * @brief Chain
 */
void xpcie_fpga_force_chain_err_evt(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_evt_t *chain_err_evt);

/**
 * @brief Chain
 */
void xpcie_fpga_get_force_chain_err_evt(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_evt_t *chain_err_evt);

/**
 * @brief Chain
 */
void xpcie_fpga_detect_chain_err_stif(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_stif_t *chain_err_stif);

/**
 * @brief Chain
 */
void xpcie_fpga_mask_chain_err_stif(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_stif_t *chain_err_stif);

/**
 * @brief Chain
 */
void xpcie_fpga_get_mask_chain_err_stif(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_stif_t *chain_err_stif);

/**
 * @brief Chain
 */
void xpcie_fpga_force_chain_err_stif(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_stif_t *chain_err_stif);

/**
 * @brief Chain
 */
void xpcie_fpga_get_force_chain_err_stif(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_stif_t *chain_err_stif);

/**
 * @brief Chain
 */
void xpcie_fpga_ins_chain_err_cmdfault(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_cmdfault_t *chain_err_cmdfault);

/**
 * @brief Chain
 */
void xpcie_fpga_get_ins_chain_err_cmdfault(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_err_cmdfault_t *chain_err_cmdfault);

/**
 * @brief Chain
 */
void xpcie_fpga_get_chain_ctrl(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_ctrl_t *chain_ctrl);

/**
 * @brief Chain
 */
void xpcie_fpga_get_chain_module_id(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_ctrl_t *chain_ctrl);

/**
 * @brief Chain
 */
void xpcie_fpga_get_chain_con_status(
        fpga_dev_info_t *dev,
        fpga_ioctl_chain_con_status_t *status);

#endif  /* __CHAIN_LIBXPCIE_CHAIN_H__ */
