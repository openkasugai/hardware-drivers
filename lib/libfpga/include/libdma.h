/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libdma.h
 * @brief Header file for executer of dma request
 */

#ifndef LIBFPGA_INCLUDE_LIBDMA_H_
#define LIBFPGA_INCLUDE_LIBDMA_H_

#include <libfpgactl.h>
#include <libdmacommon.h>

#ifdef __cplusplus
extern "C" {
#endif

// Definition for fpga_dequeue()
#define DEQ_TIMEOUT_MIN           10000     /**< Min Timeout for fpga_dequeue() : 10000[us] = 10[ms] */
#define DEQ_TIMEOUT_DEFAULT       100000    /**< Default Timeout for fpga_dequeue() : 100000[us] = 100[ms] */
#define DEQ_INTERVAL_DEFAULT      100       /**< Default Interval for fpga_dequeue() : 100[us] = 0.1[ms] */
#define DEQ_INTERVAL_MAX          999999    /**< Max Interval for fpga_dequeue() : 999999[us] = 0.999999[s] */

// Definition for fpga_lldma_queue_setup()
#define REFQ_TIMEOUT_MAX          60        /**< Max Timeout for fpga_lldma_queue_setup() : 60[s] */
#define REFQ_TIMEOUT_DEFAULT      20        /**< Default Timeout for fpga_lldma_queue_setup() : 20[s] */
#define REFQ_INTERVAL_MAX         60        /**< Max Interval for fpga_lldma_queue_setup() : 60[s] */
#define REFQ_INTERVAL_DEFAULT     1         /**< Default Interval for fpga_lldma_queue_setup() : 1[s] */


/**
 * @brief API which get activating LLDMA channel's command queue
 * @param[in] connector_id
 *   Identifer to get command queue
 * @param[out] dma_info
 *   pointer variable to get dma channel's info
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `connector_id`, `dma_info` is null
 * @retval -FAILURE_DEVICE_OPEN
 *   e.g.) driver is not loaded
 * @retval -FAILURE_MMAP
 *   Failed to memory map
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -CONNECTOR_ID_MISMATCH
 *   There are no connector_id in opening devices
 *
 * @details
 *   Search and get DMA channel by matching connector_id.@n
 *   When the valid DMA channel found, execute mmap to get command queue
 *    and store channel's information into `*dma_info`.@n
 *   `*dma_info` will be not allocated by this API, so allocate by user.@n
 *   The value of `*dma_info` is undefined when this API fails.@n
 *   fpga_enqueue() and fpga_dequeue() should use `dma_info` got by this API.@n
 *   If no valid DMA channel found, the search is repeated
 *    at the `interval` cycle until the `timeout` time.
 * @sa fpga_enqueue()
 * @sa fpga_dequeue()
 * @sa fpga_set_refqueue_polling_timeout()
 * @sa fpga_set_refqueue_polling_interval()
 *
 * @note
 *   The configuration parameters that are queuing executable
 *    for fpga_lldma_init() are as follows.@n
 *   DMA_DEV_TO_HOST(DMA_TX) : FPGA =>(PCI)=> HOST@n
 *                           : PCI-( )-FPGA-(*)-PCI,  NW-( )-FPGA-(*)-PCI@n
 *   DMA_HOST_TO_DEV(DMA_RX) : HOST =>(PCI)=> FPGA@n
 *                           : PCI-(*)-FPGA-( )-PCI, PCI-(*)-FPGA-( )-NW@n
 *   The other parameters are rounded to one of the above
 *    so that this function will return 0, but is not queuing executable.
 */
int fpga_lldma_queue_setup(
        const char *connector_id,
        dma_info_t *dma_info);

/**
 * @brief API which put LLDMA channel's command queue
 * @param[in] dma_info
 *   dma channel's info got by fpga_lldma_queue_setup()
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dma_info` is null
 *
 * @details
 *   Unmap the memory and close device opened in libdma, and allocated memory.
 */
int fpga_lldma_queue_finish(
        dma_info_t *dma_info);

/**
 * @brief API which set queuing command
 * @param[out] cmd_info
 *   pointer variable to get command variable
 * @param[in] task_id
 *   task id for DMA request(user can set freely other than 0)
 * @param[in] data_addr
 *   data address for DMA request
 * @param[in] data_len
 *   data length for DMA request
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `cmd_info` is null
 *
 * @details
 *   Set data into `*cmd_info` and return it.@n
 *   This API check queuing interval and timeout by actually executing.@n
 *   The value of interval will be larger by about 50[ms]
 */
int set_dma_cmd(
        dmacmd_info_t *cmd_info,
        uint16_t task_id,
        void *data_addr,
        uint32_t data_len);

/**
 * @brief API which get LLDMA channel's command data or dequeue result
 * @param[in] cmd_info
 *   data for get command info
 * @param[out] task_id
 *   pointer variable to get dmacmd_info_t::task_id/dmacmd_info_t::result_task_id
 * @param[out] data_addr
 *   pointer variable to get dmacmd_info_t::data_addr/dmacmd_info_t::result_data_addr
 * @param[out] data_len
 *   pointer variable to get dmacmd_info_t::data_len/dmacmd_info_t::result_data_len
 * @param[out] result_status
 *   pointer variable to get dmacmd_info_t::result_status
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   All of `task_id`, `data_addr`, `data_len`, `result_status` are NULL.
 *
 * @details
 *   Get data from `*cmd_info` and return data by argument pointer variable.@n
 *   pointer vairables's entity will be not allocated by this API, so allocate by user.@n
 *   When `result_status` is NULL, return data set by set_dma_cmd().@n
 *   When `result_status` is NOT NULL, return data set by fpga_dequeue().@n
 *   When all pointer variables are NULL, This API will return an error;
 *    otherwise, This API will return 0 and store the appropriate data
 *    into the non-null pointer variable entity.
 */
int get_dma_cmd(
        dmacmd_info_t cmd_info,
        uint16_t *task_id,
        void **data_addr,
        uint32_t *data_len,
        uint32_t *result_status);

/**
 * @brief API which request LLDMA
 * @param[in] dma_info
 *   channel's info(fpga_lldma_queue_setup()'s output)
 * @param[in] cmd_info
 *   command info(set_dma_cmd()'s output)
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dma_info` is null, `cmd_info` is null
 * @retval -INVALID_ADDRESS
 *   e.g.) data's address is something wrong
 * @retval -ENQUEUE_QUEFULL
 *   e.g.) commnad queue is full
 *
 * @details
 *   Request LLDMA to transfer data after checking data address's validation.
 */
int fpga_enqueue(
        dma_info_t *dma_info,
        dmacmd_info_t *cmd_info);

/**
 * @brief [debug] API which request LLDMA(for debug) with memory not registered in libshmem
 * @param[in] dma_info
 *   channel's info(fpga_lldma_queue_setup()'s output)
 * @param[in] cmd_info
 *   command info(set_dma_cmd()'s output)
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dma_info` is null, `cmd_info` is null
 * @retval -INVALID_ADDRESS
 *   e.g.) data's address is something wrong
 * @retval -ENQUEUE_QUEFULL
 *   e.g.) commnad queue is full
 *
 * @details
 *   Request LLDMA to transfer data without checking data address's physical validation.
 * @sa fpga_enqueue()
 */
int fpga_enqueue_without_addrcheck(
        dma_info_t *dma_info,
        dmacmd_info_t *cmd_info);

/**
 * @brief [debug] API which request LLDMA(for debug) with phys address directly
 * @param[in] dma_info
 *   channel's info(fpga_lldma_queue_setup()'s output)
 * @param[in,out] cmd_info
 *   command info(set_dma_cmd()'s output)
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dma_info` is null, `cmd_info` is null
 * @retval -INVALID_ADDRESS
 *   e.g.) data's address is something wrong
 * @retval -ENQUEUE_QUEFULL
 *   e.g.) commnad queue is full
 *
 * @details
 *   Request LLDMA to transfer data with physical address directly
 * @sa fpga_enqueue()
 */
int fpga_enqueue_with_physaddr(
        dma_info_t *dma_info,
        dmacmd_info_t *cmd_info);

/**
 * @brief API which get a result of request LLDMA
 * @param[in] dma_info
 *   channel's info(fpga_lldma_queue_setup()'s output)
 * @param[in,out] cmd_info
 *   command info
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dma_info` is null, `cmd_info` is null
 * @retval -DEQUEUE_TIMEOUT
 *   e.g.) The load is too high and the operation has not yet finished,
 *                                  so by waiting a few seconds, API may finish successfully.
 *
 * @details
 *   Get the result of command enqueued by fpga_enqueue().@n
 *   The result of command can be got by get_dma_cmd().
 * @remarks
 *   Currently, this API does not have a matching function for task_id,
 *    and the results of commnads are obtained in order from the top.
 */
int fpga_dequeue(
        dma_info_t *dma_info,
        dmacmd_info_t *cmd_info);

/**
 * @brief API which parse dma options
 * @param[in] argc
 *   cmdline's argc
 * @param[in] argv
 *   cmdline's argv
 * @return num of options which handled by this API
 * @retval -INVALID_ARGUMENT
 *   Invalid option
 *
 * @details
 *   usage: `<APP> [-p <timeout>] [-i <interval>] [-r <timeout>] [-q <interval>]@n
 *   options :
 * @li -p, --polling-timeout   : Set timeout[us] for fpga_dequeue()
 * @li -i, --polling-ineterval : Set interval[us] for fpga_dequeue()
 * @li -r, --refqueue-timeout  : Set timeout[s] for fpga_lldma_queue_setup()
 * @li -q, --refqueue-interval : Set interval[s] for fpga_lldma_queue_setup()
 */
int fpga_dma_options_init(
        int argc,
        char **argv);

/**
 * @brief API which set timeout for fpga_dequeue()
 * @param[in] timeout
 *   set data
 * @return void
 *
 * @details
 *   Set timeout[us] for polling of fpga_dequeue()@n
 *   Because it is managed by a single integer global variable,
 *    it cannot be changed on a per-channel basis, but only on a per-process basis.
 */
void fpga_set_dequeue_polling_timeout(
        int64_t timeout);

/**
 * @brief API which set interval for fpga_dequeue()
 * @param[in] interval
 *   set data
 * @return void
 *
 * @details
 *   Set interval[us] for polling of fpga_dequeue()@n
 *   Because it is managed by a single integer global variable,
 *    it cannot be changed on a per-channel basis, but only on a per-process basis.
 */
void fpga_set_dequeue_polling_interval(
        int64_t interval);

/**
 * @brief API which set timeout for fpga_lldma_queue_setup()
 * @param[in] timeout
 *   set data
 * @return void
 *
 * @details
 *   Set timeout[s] for polling of fpga_lldma_queue_setup()@n
 *   regions:[0,REFQ_TIMEOUT_MAX]@n
 *   Because it is managed by a single integer global variable,
 *    it cannot be changed on a per-channel basis, but only on a per-process basis.
 */
void fpga_set_refqueue_polling_timeout(
        int64_t timeout);

/**
 * @brief API which set interval for fpga_lldma_queue_setup()
 * @param[in] interval
 *   set data
 * @return void
 *
 * @details
 *   Set interval[s] for polling of fpga_lldma_queue_setup().@n
 *   When the value is too large, or too small, nothing done.@n
 *   regions:[0,REFQ_INTERVAL_MAX]@n
 *   Because it is managed by a single integer global variable,
 *    it cannot be changed on a per-channel basis, but only on a per-process basis.
 */
void fpga_set_refqueue_polling_interval(
        int64_t interval);

/**
 * @brief API which get timeout for fpga_dequeue()
 * @return fpga_dequeue()'s timeout
 *
 * @details
 *   Get timeout[us] for polling of fpga_dequeue()
 */
int64_t fpga_get_dequeue_polling_timeout(void);

/**
 * @brief API which get timeout for fpga_dequeue()
 * @return fpga_dequeue()'s interval
 *
 * @details
 *   Get interval[us] for polling of fpga_dequeue()
 */
int64_t fpga_get_dequeue_polling_interval(void);

/**
 * @brief API which get timeout for fpga_lldma_queue_setup()
 * @return fpga_lldma_queue_setup()'s timeout
 *
 * @details
 *   Get timeout[s] for polling of fpga_lldma_queue_setup()
 */
int64_t fpga_get_refqueue_polling_timeout(void);

/**
 * @brief API which get timeout for fpga_lldma_queue_setup()
 * @return fpga_lldma_queue_setup()'s interval
 *
 * @details
 *   Get interval[s] for polling of fpga_lldma_queue_setup()
 */
int64_t fpga_get_refqueue_polling_interval(void);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBDMA_H_
