/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file libxpcie_lldma.h
 * @brief Header file for functions for LLDMA module
 */

#ifndef __LLDMA_LIBXPCIE_LLDMA_H__
#define __LLDMA_LIBXPCIE_LLDMA_H__

#include <libxpcie.h>


/**
 * @brief LLDMA: Function which execute ioctl commands for lldma module
 */
inline long xpcie_fpga_ioctl_lldma(
        struct file *filp,
        unsigned int cmd,
        unsigned long arg);

/**
 * @brief LLDMA: Function which get information about LLDMA module
 */
int xpcie_fpga_common_get_lldma_module_info(
        fpga_dev_info_t *dev);

/**
 * @brief Function which allocate memory for command queue
 * @param[out] qp pointer variable to set command queue's information
 * @param[in] size The num of command queue's descripters
 * @retval 0 Success
 * @retval (<0) Failed
 */
int queue_que_init(
        fpga_queue_enqdeq_t *qp,
        uint16_t size);

/**
 * @brief Function which free memory for command queue
 * @param[in,out] qp pointer variable to get/set command queue's information
 */
void queue_que_free(
        fpga_queue_enqdeq_t *qp);

/**
 * @brief Function which get DMA channel and command queue
 */
int xpcie_fpga_get_queue(
        fpga_dev_info_t *,
        fpga_ioctl_queue_t *ioctl_queue);

/**
 * @brief Function which put DMA channel and command queue
 */
int xpcie_fpga_put_queue(
        fpga_dev_info_t *,
        fpga_ioctl_queue_t *ioctl_queue);

/**
 * @brief Function which put command queue's status
 */
int xpcie_fpga_put_queue_info(
        fpga_dev_info_t *dev,
        uint16_t chid,
        uint16_t dir);

/**
 * @brief Function which get command queue matching connector_id
 */
int xpcie_fpga_ref_queue(
        fpga_dev_info_t *dev,
        fpga_ioctl_queue_t *ioctl_queue);

/**
 * @brief LLDMA: Function which set buffer for D2D-H
 */
void xpcie_fpga_set_buf(
        fpga_dev_info_t *dev,
        uint32_t dir,
        uint32_t buf_size,
        uint64_t buf_addr);

/**
 * @brief LLDMA: Function which start DMA channel polling
 */
void xpcie_fpga_start_queue(
        fpga_dev_info_t *,
        uint16_t chid,
        uint16_t dir);

/**
 * @brief LLDMA: Function which stop DMA channel polling
 */
int xpcie_fpga_stop_queue(
        fpga_dev_info_t *,
        uint16_t chid,
        uint16_t dir);

/**
 * @brief LLDMA: Function which get DMA channel's status(is implemeted)
 */
int xpcie_fpga_get_avail_status(
        fpga_dev_info_t *,
        uint16_t dir);

/**
 * @brief LLDMA: Function which get DMA channel's status(is used)
 */
int xpcie_fpga_get_active_status(
        fpga_dev_info_t *,
        uint16_t dir);

/**
 * @brief LLDMA: Function which get connection id and function chain controller id
 * @notice Not used.
 */
void xpcie_fpga_get_cid_chain_queue(
        fpga_dev_info_t *dev,
        fpga_ioctl_cidchain_t *cidchain);

/**
 * @brief LLDMA: Function which do setting for D2D
 */
int xpcie_fpga_dev_connect(
        fpga_dev_info_t *dev,
        uint32_t own_chid,
        uint32_t peer_chid,
        uint16_t dir,
        uint64_t peer_addr,
        uint32_t buf_size,
        uint64_t buf_addr,
        char *connector_id);

/**
 * @brief LLDMA: Function which delete setting for D2D
 */
int xpcie_fpga_dev_disconnect(
        fpga_dev_info_t *dev,
        uint32_t self_chid,
        uint32_t dir);

/**
 * @brief LLDMA: Function which get setting for LLDMA's buffer with chain module
 */
void xpcie_fpga_read_cif_ddr4_regs(
        fpga_dev_info_t *dev,
        fpga_ioctl_lldma_buffer_regs_t *regs);

/**
 * @brief LLDMA: Function which get bitstream-id
 */
uint32_t xpcie_fpga_get_version(
        fpga_dev_info_t *dev);

/**
 * @brief LLDMA: Function which get how many channels are implemented
 */
void xpcie_fpga_count_available_dma_channel(
        fpga_dev_info_t *dev);

/**
 * @brief LLDMA: Function which get target channel's status(non-d2d/d2d-h/d2d-d)
 */
uint32_t xpcie_fpga_get_channel_status(
        fpga_dev_info_t *dev,
        uint32_t chid,
        dma_dir_t dir);

/**
 * @brief LLDMA: Function which get transfer request size
 */
void xpcie_fpga_get_request_size(
        fpga_dev_info_t *dev,
        uint16_t chid,
        uint32_t *req_size);

/**
 * @brief LLDMA: Function which get value at `XPCIE_FPGA_LLDMA_RXCH_CTRL0`
 */
void xpcie_fpga_get_rxch_ctrl0(
        fpga_dev_info_t *dev,
        uint32_t *value);

/**
 * @brief LLDMA: Function which set buffer for LLDMA with chain interface buffer
 */
void xpcie_fpga_set_lldma_buffer(
        fpga_dev_info_t *dev,
        bool is_init);


#endif  /* __LLDMA_LIBXPCIE_LLDMA_H__ */
