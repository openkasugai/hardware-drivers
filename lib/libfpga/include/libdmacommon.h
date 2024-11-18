/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libdmacommon.h
 * @brief Header file for internal LLDMA definition
 */

#ifndef LIBFPGA_INCLUDE_LIBDMACOMMON_H_
#define LIBFPGA_INCLUDE_LIBDMACOMMON_H_

#include <libfpgactl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Status for command queue
#define CMD_INVALID 0   /**< Not exist valid data */
#define CMD_READY   1   /**< Exist valid data, Not executed */
#define CMD_DONE    2   /**< Existed valid data, Executed */

/**
 * This library's avaliable max device num
 */
#define LLDMA_DEV_MAX FPGA_MAX_DEVICES

/**
 * This library's avaliable max channel num
 */
#define LLDMA_CH_MAX  32

/**
 * This library's avaliable max direction type
 */
#ifndef DMA_DIR_MAX
#define LLDMA_DIR_MAX 4
#else
#define LLDMA_DIR_MAX DMA_DIR_MAX
#endif

/**
 * This library's avaliable max direction type
 */
#define IS_DMA_RX(dir)  ((dir) == DMA_HOST_TO_DEV || (dir) == DMA_NW_TO_DEV)


/**
 * @struct dma_info_t
 * @brief DMA channel's information
 * @var dma_info_t::dev_id
 *      Device_id got by fpga_dev_init()
 * @var dma_info_t::dir
 *      DMA's transfer direction
 * @var dma_info_t::chid
 *      Target channel id
 * @var dma_info_t::queue_addr
 *      Command queue's head address
 * @var dma_info_t::queue_size
 *      The num of descriptors in a command queue
 * @var dma_info_t::connector_id
 *      Matching key string
 */
typedef struct dma_info {
  uint32_t dev_id;
  dma_dir_t dir;
  uint16_t chid;
  void *queue_addr;
  uint32_t queue_size;
  char *connector_id;
} dma_info_t;

/**
 * @struct dmacmd_info_t
 * @brief DMA request's information
 * @var dmacmd_info_t::task_id
 *      task_id set by user
 * @var dmacmd_info_t::data_len
 *      The size of transfer data
 * @var dmacmd_info_t::data_addr
 *      Target (src/dst) memory address
 * @var dmacmd_info_t::desc_addr
 *      The address of descriptor(in future, may be used for matching)
 * @var dmacmd_info_t::result_status
 *      The result status of fpga_dequeue()
 * @var dmacmd_info_t::result_task_id
 *      The result task_id of fpga_dequeue()
 * @var dmacmd_info_t::result_data_len
 *      The result data_len of fpga_dequeue()
 * @var dmacmd_info_t::result_data_addr
 *      The result data_addr of fpga_dequeue()
 */
typedef struct dmacmd_info {
  uint32_t task_id;
  uint32_t data_len;
  void *data_addr;
  void *desc_addr;
  uint32_t result_status;
  uint16_t result_task_id;
  uint32_t result_data_len;
  void *result_data_addr;
} dmacmd_info_t;

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBDMACOMMON_H_
