/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libchain_stat.h
 * @brief Header file for getting Chain Control statistics
 */

#ifndef LIBFPGA_INCLUDE_LIBCHAIN_STAT_H_
#define LIBFPGA_INCLUDE_LIBCHAIN_STAT_H_

#include <libfpgactl.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief API which get chaining delay (number of clock cycles)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   External IF ID
 * @param[in] dir
 *   Direction
 * @param[in] cid
 *   Connection ID
 * @param[out] latency
 *   delay in clock cycles
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get delay information (number of clock cycles) for Chain Control
 */
int fpga_chain_get_stat_latency_self(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint8_t dir,
        uint32_t cid,
        uint32_t *latency);

/**
 * @brief API which get chain control latency (number of clock cycles)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[out] latency
 *   number of clock cycles
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get delay information (number of clock cycles) of an FPGA function
 */
int fpga_chain_get_stat_latency_func(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t *latency);

/**
 * @brief API which get chain control statistics (number of bytes)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] cid_fchid
 *   connection ID/function channel ID
 * @param[in] reg_id
 *   reg_id: statistics register identifier
 * @param[out] byte_num
 *   number-of-bytes
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get Chain Control statistics (number of bytes)
 */
int fpga_chain_get_stat_bytes(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t cid_fchid,
        uint32_t reg_id,
        uint64_t *byte_num);

/**
 * @brief API which get chain control statistics (number of frames)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[in] reg_id
 *   statistics register identifier
 * @param[out] frame_num
 *   number of frames
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get Chain Control statistics (number of frames)
 */
int fpga_chain_get_stat_frames(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t reg_id,
        uint32_t *frame_num);

/**
 * @brief API which get chain control statistics (discard bytes)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[in] reg_id
 *   statistics register identifier
 * @param[out] byte_num
 *   number-of-bytes-to-be-discarded
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get Chain Control statistics (number of discarded bytes)
 */
int fpga_chain_get_stat_discard_bytes(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t reg_id,
        uint64_t *byte_num);

/**
 * @brief API which get header buffer storage count
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[out] buff_num
 *   number of header buffers
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get number of header buffers stored
 */
int fpga_chain_get_stat_buff(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t *buff_num);

/**
 * @brief API which get notification of BP occurrence due to header buffer full
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[out] bp
 *   Notification of BP occurrence associated with header buffer full
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get notification of BP occurrence associated with header buffer full
 */
int fpga_chain_get_stat_bp(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t *bp);

/**
 * @brief API which clear BP occurrence due to header buffer full
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[in] bp
 *   Notification of BP occurrence associated with header buffer full
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Clear notification of BP occurrence associated with header buffer full
 */
int fpga_chain_set_stat_bp_clear(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t bp);

/**
 * @brief API which get Egress side data transfer busy
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[out] busy
 *   Egress side data transfer busy
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   get egress side data transfer busy state
 */
int fpga_chain_get_stat_egr_busy(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t *busy);

/**
 * @brief API which polling egress Chain Control status until becoming free
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[in] timeout
 *   Timeout for polling
 * @param[in] interval
 *   Interval for polling
 * @param[out] is_success
 *   The result which egress chain status become free
 *   true(non-zero):became free/false(zero):busy yet
 * @retval 0
 *   Success(Don't care if egress chain status become free or not)
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @return
 *   @sa fpga_chain_get_stat_egr_busy
 *
 * @details
 *   This API calls fpga_chain_get_stat_egr_busy() periodically to check
 *    if egress chain is busy or not.@n
 *   The interval of API call is defined by `interval`,
 *    and the timeout of API polling is defined by `timeout`.
 *    If you set `NULL` for them, they will be considered as 0 sec.@n
 *   The retval means only if API calling succeed or not,
 *    so it doesn't care if egress chain became free or not.
 *    When you want to check egress chain is not free, please check `is_success` too.@n
 *   Please allocate memory for `is_success` by user.@n
 *   When this API fails, the value of `is_success` is not defined,
 *    so if you want to use `is_success`, please check retval first.
 */
int fpga_chain_wait_stat_egr_free(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        const struct timeval *timeout,
        const struct timeval *interval,
        uint32_t *is_success);


#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBCHAIN_STAT_H_
