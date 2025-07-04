/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _LIBSTREAM_ENGINE_H__
#define _LIBSTREAM_ENGINE_H__

#include <stdint.h>
#include <libutil.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* fpga_stream_engine_t;

// User API

/**
 * @brief Initialize the object for streaming DMA.
 * @param[in] dev_id device number
 * @param[in] is_d2h streaming direction
 * @param[in] ch streaming channel
 * @param[out] info the object initialized
 * @param[in] check_interval interval cycles to check queue head and tail.
 * @return status code
 **/
xse_status_t fpga_stream_engine_init(int dev_id, bool is_d2h, int ch, fpga_stream_engine_t* info, int check_interval = -1);

/**
 * @brief Set queue address and depth to the hardware module.
 * @param[in] info the object of the specified channel
 * @param[in] req_q_token memory token for request queue
 * @param[in] req_q_offset offset from the base address indicated by the req_q_token
 * @param[in] cpl_q_token memory token for completion queue
 * @param[in] cpl_q_offset offset from the base address indicated by the cpl_q_token
 * @param[in] req_q_head_tail_token memory token for request queue head and tail
 * @param[in] req_q_offset offset from the base address indicated by the req_q_head_tail_token
 * @param[in] cpl_q_head_tail_token memory token for completion queue head and tail
 * @param[in] cpl_q_offset offset from the base address indicated by the cpl_q_head_tail_token
 * @param[in] req_q_depth specify request queue depth
 * @param[in] cpl_q_depth specify completion queue depth
 * @return status code
 **/
xse_status_t fpga_stream_engine_set_queue_with_token(fpga_stream_engine_t info,
                                                     uint64_t req_q_token, uint32_t req_q_offset, uint64_t cpl_q_token, uint32_t cpl_q_offset,
                                                     uint64_t req_q_head_tail_token, uint32_t req_q_head_tail_offset,
                                                     uint64_t cpl_q_head_tail_token, uint32_t cpl_q_head_tail_offset, int req_q_depth, int cpl_q_depth);

/**
 * @brief Set buffer information to the hardware module to convert from virtual addresses in any queue to physical addresses for DMA.
 * @param[in] info the object of the specified channel
 * @param[in] vaddr virtual address of the counterpart virutal space
 * @param[in] size contiguous size in the counterpart virutal space
 * @param[in] token memory token for the vaddr
 * @return status code
 **/
xse_status_t fpga_stream_engine_set_buffer(fpga_stream_engine_t info,
                                           uint64_t vaddr, uint32_t size, uint64_t token = 0);

/**
 * @brief Start streaming DMA in the hardware module.
 * @param[in] info the object of the specified channel
 * @param[in] d2d_mode specify transfer mode
 * @return status code
 **/
xse_status_t fpga_stream_engine_enable(fpga_stream_engine_t info, bool d2d_mode = false);

/**
 * @brief Stop streaming DMA in the hardware module.
 * @param[in] info the object of the specified channel
 * @return status code
 **/
xse_status_t fpga_stream_engine_disable(fpga_stream_engine_t info);

/**
 * @brief Finalize the object for the streaming DMA.
 * @param[in] info the object of the specified channel
 * @return status code
 **/
xse_status_t fpga_stream_engine_finish(fpga_stream_engine_t info);

/**
 * @brief Set to use the doorbell to the specified channel for d2d RX side.
 * @param[in] info the object of the specified channel
 * @param[in] counterpart_dev_id device number of the counterpart
 * @param[in] counterpart_ch_id channel id of the counterpart
 * @return status code
 **/
xse_status_t fpga_stream_engine_set_doorbell_addr(fpga_stream_engine_t info, uint32_t counterpart_dev_id, uint32_t counterpart_ch_id);

/**
 * @brief Set the counterpart queue informations for d2d TX side.
 * @param[in] info the object of the specified channel
 * @param[in] counterpart_dev_id device number of the counterpart
 * @param[in] counterpart_ch_id channel id of the counterpart
 * @return status code
 **/
xse_status_t fpga_stream_engine_set_counterpart_queue(fpga_stream_engine_t info, uint32_t counterpart_dev_id, uint32_t counterpart_ch_id);

/**
 * @brief Set the counterpart address range for d2d TX side.
 * @param[in] info the object of the specified channel
 * @param[in] counterpart_dev_id device number of the counterpart
 * @param[in] counterpart_ch_id channel id of the counterpart
 * @return status code
 **/
xse_status_t fpga_stream_engine_set_counterpart_vpmap(fpga_stream_engine_t info, uint32_t counterpart_dev_id, uint32_t counterpart_ch_id);

#ifdef __cplusplus
}
#endif

#endif // _LIBSTREAM_ENGINE_H__
