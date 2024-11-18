/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file liblldma.h
 * @brief Header file for LLDMA maintener
 */

#ifndef LIBFPGA_INCLUDE_LIBLLDMA_H_
#define LIBFPGA_INCLUDE_LIBLLDMA_H_

#include <libfpgactl.h>
#include <libdmacommon.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct fpga_lldma_connect_t
 * @brief D2D setting information struct
 * @var fpga_lldma_connect_t::tx_dev_id
 *      prev FPGA's device id
 * @var fpga_lldma_connect_t::tx_chid
 *      prev FPGA's LLDMA's chid
 * @var fpga_lldma_connect_t::rx_dev_id
 *      next FPGA's device id
 * @var fpga_lldma_connect_t::rx_chid
 *      next FPGA's LLDMA's chid
 * @var fpga_lldma_connect_t::buf_size
 *      buffer size for D2D-H
 * @var fpga_lldma_connect_t::buf_addr
 *      buffer address for D2D-H
 * @var fpga_lldma_connect_t::connector_id
 *      for debug
 */
typedef struct {
  uint32_t tx_dev_id;
  uint32_t tx_chid;
  uint32_t rx_dev_id;
  uint32_t rx_chid;
  uint32_t buf_size;
  void *buf_addr;
  char *connector_id;
} fpga_lldma_connect_t;

/**
 * @struct fpga_lldma_buffer_t
 * @brief LLDMA's DDR4 buffer with chain module struct
 * @var fpga_lldma_buffer_t::dn_rx_val_l
 *      XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_L(lane)
 * @var fpga_lldma_buffer_t::dn_rx_val_h
 *      XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_H(lane)
 * @var fpga_lldma_buffer_t::up_tx_val_l
 *      XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_L(lane)
 * @var fpga_lldma_buffer_t::up_tx_val_h
 *      XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_H(lane)
 * @var fpga_lldma_buffer_t::dn_rx_ddr_size
 *      XPCIE_FPGA_LLDMA_CIF_DN_RX_DDR_SIZE_VAL
 * @var fpga_lldma_buffer_t::up_tx_ddr_size
 *      XPCIE_FPGA_LLDMA_CIF_UP_TX_DDR_SIZE_VAL
 */
typedef struct fpga_lldma_buffer_t {
  uint32_t dn_rx_val_l[XPCIE_KERNEL_LANE_MAX];
  uint32_t dn_rx_val_h[XPCIE_KERNEL_LANE_MAX];
  uint32_t up_tx_val_l[XPCIE_KERNEL_LANE_MAX];
  uint32_t up_tx_val_h[XPCIE_KERNEL_LANE_MAX];
  uint32_t dn_rx_ddr_size;
  uint32_t up_tx_ddr_size;
} fpga_lldma_buffer_t;

/**
 * @brief API which Activate LLDMA channel
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] dir
 *   Target LLDMA direction
 *   (DMA_DEV_TO_HOST/DMA_HOST_TO_DEV/DMA_DEV_TO_NW/DMA_NW_TO_DEV)
 * @param[in] chid
 *   Target LLDMA channel id
 * @param[in] connector_id
 *   Identifer for user
 * @param[out] dma_info
 *   pointer variable to get DMA channel's info
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `dma_info` is null
 * @retval -INVALID_DATA
 *   e.g) Another thread close `dev_id`'s FPGA
 * @retval -FAILURE_DEVICE_OPEN
 *   Failed to open Device file
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 * @retval -UNAVAILABLE_CHID
 *   `chid` is not implemented
 * @retval -ALREADY_ACTIVE_CHID
 *   `chid` is already activated.
 * @retval -ALREADY_ASSIGNED
 *   `chid` is already assinged.
 *
 * @details
 *   Get free DMA channel and activate it after checking if it is used or not.@n
 *   The channel this API got is binded with `connector_id`,
 *    and users can use activating channel's command queue without knowing where the chennle is
 *    by user using this `connector_id`.@n
 *   The activated channel will be stopped when fpga_lldma_finish() is called with `dma_info`,
 *    or the process becomes dead.@n
 *   `*dma_info` will be not allocated by this API, so allocate by user.@n
 *   The value of `*dma_info` is undefined when this API fails.
 *
 * @note
 *   The valid directions for fpga_lldma_init() are as follows.@n
 *   DMA_DEV_TO_HOST(DMA_TX) : DEV =>(PCI)=> HOST@n
 *                           : PCI-( )-DEV-(*)-PCI,  NW-( )-DEV-(*)-PCI@n
 *   DMA_HOST_TO_DEV(DMA_RX) : HOST =>(PCI)=> DEV@n
 *                           : PCI-(*)-DEV-( )-PCI, PCI-(*)-DEV-( )-NW@n
 *   DMA_DEV_TO_NW(TX) : DEV => NW@n
 *                     : PCI-( )-DEV-(*)-NW@n
 *   DMA_NW_TO_DEV(RX) : NW => DEV@n
 *                     : NW- (*)-DEV-( )-PCI
 */
int fpga_lldma_init(
        uint32_t dev_id,
        dma_dir_t dir,
        uint32_t chid,
        const char *connector_id,
        dma_info_t *dma_info);

/**
 * @brief API which Disactivate DMA channel
 * @param[in] dma_info
 *   DMA channel's information got by fpga_lldma_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dma_info` is null
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Stop DMA channel binded with `dma_info` and free.
 */
int fpga_lldma_finish(
        dma_info_t *dma_info);

/**
 * @brief API which Activate LLDMA channels and connect them through buffer
 * @param[in] connect_info
 *   Target data which user set to create connection
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `connect_info` is null
 * @retval -INVALID_DATA
 *   e.g) Another thread close `dev_id`'s FPGA
 * @retval -FAILURE_DEVICE_OPEN
 *   Failed to open Device file
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 * @retval -UNAVAILABLE_CHID
 *   `chid` is not implemented
 * @retval -ALREADY_ACTIVE_CHID
 *   `chid` is already activated.
 * @retval -ALREADY_ASSIGNED
 *   `chid` is already assinged.
 *
 * @details
 *   Establish Device to Device with Host memory buffer(D2D-H).@n
 *   Check if target DMA channels are free or not,
 *    and when both of them are free, activate them and set buffer.@n
 *   The order of setting is, TX -> RX, and if RX setting fails,
 *    TX setting will be delted.@n
 *   The activated channel will be stopped when fpga_lldma_buf_disconnect()
 *    is called with `connect_info`, or the process becomes dead.
 */
int fpga_lldma_buf_connect(
        fpga_lldma_connect_t *connect_info);

/**
 * @brief API which Disactivate LLDMA channels and disconnect them
 * @param[in] connect_info
 *   Target data which user set to delete connection
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `connect_info` is null
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Delete setting for Device to Device with Host memory buffer(D2D-H).@n
 *   The order to delete setting is, RX -> TX, and if RX setting fails,
 *    try to continue deleteing TX setting too.
 */
int fpga_lldma_buf_disconnect(
        fpga_lldma_connect_t *connect_info);

/**
 * @brief API which Activate LLDMA channels and connect them directly
 * @param[in] connect_info
 *   Target data which user set to create connection
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `connect_info` is null
 * @retval -INVALID_DATA
 *   e.g) Another thread close `dev_id`'s FPGA
 * @retval -FAILURE_DEVICE_OPEN
 *   Failed to open Device file
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 * @retval -UNAVAILABLE_CHID
 *   `chid` is not implemented
 * @retval -ALREADY_ACTIVE_CHID
 *   `chid` is already activated.
 * @retval -ALREADY_ASSIGNED
 *   `chid` is already assinged.
 *
 * @details
 *   Establish Device to Device directly(D2D-D).@n
 *   Check if target DMA channels are free or not,
 *    and when both of them are free, activate them.@n
 *   The order of setting is, TX -> RX, and if RX setting fails,
 *    TX setting will be delted.@n
 *   The activated channel will be stopped when fpga_lldma_direct_disconnect()
 *    is called with `connect_info`, or the process becomes dead.@n
 *   fpga_lldma_connect_t::buf_addr and fpga_lldma_connect_t::buf_size
 *    is not used by this API.
 */
int fpga_lldma_direct_connect(
        fpga_lldma_connect_t *connect_info);

/**
 * @brief API which Disactivate LLDMA channels and disconnect them
 * @param[in] connect_info
 *   Target data which user set to delete connection
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `connect_info` is null
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Delete setting for Device to Device directly(D2D-D).@n
 *   The order to delete setting is, RX -> TX, and if RX setting fails,
 *    try to continue deleteing TX setting too.
 */
int fpga_lldma_direct_disconnect(
        fpga_lldma_connect_t *connect_info);

/**
 * @brief API which get lldma's status
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] rxch_ctrl0
 *   read data(DMA status)
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `rxch_ctrl0` is null
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Get the value of LLDMA register named as rxch_ctrl0.
 */
int fpga_lldma_get_rxch_ctrl0(
        uint32_t dev_id,
        uint32_t *rxch_ctrl0);

/**
 * @brief API which get transfer data's request size
 * @param[in] dma_info
 *   Target channel's info
 * @param[out] rsize
 *   read data(request size)
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `rsize` is null
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Get request transfer data size.@n
 *   Request data size is 64byte alignment and the lower 6bit will be changed
 *    when request size is updated.
 */
int fpga_lldma_get_rsize(
        dma_info_t *dma_info,
        uint32_t* rsize);

/**
 * @brief API which set lldma's buffer with chain module
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Set lldma's buffer with chain module.
 *    @li DDR4 buffer address
 *    @li DDR4 buffer length
 *   There is no need to call this API again unless FPGA is reconfigured
 *    or fpga_lldma_cleanup_buffer() is called.
 */
int fpga_lldma_setup_buffer(
        uint32_t dev_id);

/**
 * @brief API which delete lldma's buffer with chain module
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Delete lldma's buffer with chain module.
 *    @li DDR4 buffer addressx
 *    @li DDR4 buffer length
 */
int fpga_lldma_cleanup_buffer(
        uint32_t dev_id);

/**
 * @brief API which get lldma's buffer with chain module
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] buf
 *   Output pointer to get buffer information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `bus` is null
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Get lldma's buffer with chain module.@n
 *    @li DDR4 buffer address
 *    @li DDR4 buffer length
 *   `*buf` will be not allocated by this API, so allocate by user.@n
 *   The value of `*buf` is undefined when this API fails.
 */
int fpga_lldma_get_buffer(
        uint32_t dev_id,
        fpga_lldma_buffer_t *buf);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBLLDMA_H_
