/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfunction_filter_resize_stat.h
 * @brief Header file for get Filter Resize statistics
 */

#ifndef LIBFPGA_INCLUDE_LIBFUNCTION_FILTER_RESIZE_STAT_H_
#define LIBFPGA_INCLUDE_LIBFUNCTION_FILTER_RESIZE_STAT_H_

#include <libfunction.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum FUNC_REG_COUNTER_FR
 * @brief Enumeration for Filter Resize Function Statistics Register Identification
 */
enum FUNC_REG_COUNTER_FR {
  FR_STAT_INGR_RCV0 = 0,  /**< Filter Resize#0 ingress receive*/
  FR_STAT_INGR_RCV1,      /**< Filter Resize#1 ingress receive*/
  FR_STAT_EGR_SND0,       /**< Filter Resize#0 egress send*/
  FR_STAT_EGR_SND1        /**< Filter Resize#1 egress send*/
  /* If statistics register is added, it is added after.*/
};


/**
 * @brief API which get Filter Resize function statistics (bytes)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[in] reg_id
 *   statistical information type
 * @param[out] byte_num
 *   statistics (bytes)
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   pwrite() failure
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   get conversion adapter module statistics (bytes)
 */
int fpga_filter_resize_get_stat_bytes(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t reg_id,
        uint64_t *byte_num);

/**
 * @brief API which get Filter Resize function statistics (number of frames)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[in] reg_id
 *   statistical information type
 * @param[out] frame_num
 *   statistics (frames)
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   pwrite() failure
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   Get conversion adapter module statistics (number of frames)
 */
int fpga_filter_resize_get_stat_frames(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t reg_id,
        uint32_t *frame_num);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFUNCTION_FILTER_RESIZE_STAT_H_
