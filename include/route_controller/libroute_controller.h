/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _LIBROUTE_CONTROLLER_H__
#define _LIBROUTE_CONTROLLER_H__

#include <stdint.h>
#include <libutil.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* fpga_route_controller_t;

// User API

/**
 * @brief Initialize the object for routings in the FPGA.
 * @param[in] dev_id device number
 * @param[out] info the object initialized
 * @return status code
 **/
xse_status_t fpga_route_controller_init(int dev_id, fpga_route_controller_t* info);

/**
 * @brief Bind a RX DMA channel and a function input port.
 * @param[in] info the object initialized
 * @param[in] ch DMA channel number
 * @param[in] dest function port number
 * @param[in] need_relation whether or not to associate the input and output ports of the function
 * @param[out] connected associated the input and output ports of the function
 * @param[in] instance_id instance index in the device.
 * @return status code
 **/
    xse_status_t fpga_route_controller_set_tx_stream(fpga_route_controller_t info,
                                                     int ch, int dest, bool need_relation, bool* connected, int instance_id = 0);

/**
 * @brief Bind a TX DMA channel and a function output port.
 * @param[in] info the object initialized
 * @param[in] ch DMA channel number
 * @param[in] dest function port number
 * @param[in] need_relation whether or not to associate the input and output ports of the function
 * @param[out] connected associated the input and output ports of the function
 * @param[in] instance_id instance index in the device.
 * @return status code
 **/
xse_status_t fpga_route_controller_set_rx_stream(fpga_route_controller_t info,
                                                 int ch, int dest, bool need_relation, bool* connected, int instance_id = 0);

/**
 * @brief Finalize the object for routing.
 * @param[in] info the object initialized
 * @return status code
 */
xse_status_t fpga_route_controller_finish(fpga_route_controller_t info);

/**
 * @brief Allocate a device memory with the DMA channel binding.
 * @param[in] info the object initialized
 * @param[in] is_tx DMA direction
 * @param[in] ch DMA channel number
 * @param[in] mem_id memory I/F number. -1 means any memory I/F.
 * @param[in] size allocate size
 * @param[out] token allocated token
 * @param[in] instance_id instance index in the device
 * @return status code
 */
xse_status_t fpga_route_controller_allocate_mem(fpga_route_controller_t info,
                                                bool is_tx, int ch, uint32_t size, uint64_t* token, int instance_id = 0);
xse_status_t fpga_route_controller_allocate_mem_with_region(fpga_route_controller_t info,
                                                            bool is_tx, int ch, int mem_id, uint32_t size, uint64_t* token,
                                                            int instance_id = 0);

/**
 * @brief Free the device memory.
 * @param[in] info the object initialized
 * @param[in] token allocated token
 * @return status code
 */
xse_status_t fpga_route_controller_free_mem(fpga_route_controller_t info, uint64_t token);

/**
 * @brief Set buffer size to the specified channel
 * @param[in] info the object initialized
 * @param[in] is_tx DMA direction
 * @param[in] ch DMA channel number
 * @param[in] size allocate size
 * @param[in] instance_id instance index in the device
 * @return status code
 */
xse_status_t fpga_route_controller_set_buffer_size(fpga_route_controller_t info,
                                                   bool is_tx, int ch, uint32_t size, int instance_id = 0);

/**
 * @brief Set buffer address to the specified channel
 * @param[in] info the object initialized
 * @param[in] is_tx DMA direction
 * @param[in] ch DMA channel number
 * @param[in] idx buffer index
 * @param[in] token allocated token
 * @param[in] offset offset from the start address indicated by the token
 * @param[in] instance_id instance index in the device
 * @return status code
 */
xse_status_t fpga_route_controller_set_mem(fpga_route_controller_t info,
                                           bool is_tx, int ch, int idx, uint64_t token, uint32_t offset, int instance_id = 0);

#ifdef __cplusplus
}
#endif

#endif // _LIBROUTE_CONTROLLER_H__
