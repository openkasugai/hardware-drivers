/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfunction_conv_stat.h
 * @brief Header file for get Conversion Adapter statistics
 */

#ifndef LIBFPGA_INCLUDE_LIBFUNCTION_CONV_STAT_H_
#define LIBFPGA_INCLUDE_LIBFUNCTION_CONV_STAT_H_

#include <libfunction.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum FUNC_REG_COUNTER_FR
 *   Enumeration for Conversion Adapter statistics register identification
 */
enum FUNC_REG_COUNTER_CONV {
  CONV_STAT_INGR_RCV = 0, /**< Conversion Adapter ingress receive*/
  CONV_STAT_INGR_SND0,    /**< Conversion Adapter#0 ingress send*/
  CONV_STAT_INGR_SND1,    /**< Conversion Adapter#1 ingress send*/
  CONV_STAT_EGR_RCV0,     /**< Conversion Adapter#0 egress receive*/
  CONV_STAT_EGR_RCV1,     /**< Conversion Adapter#1 egress receive*/
  CONV_STAT_EGR_SND       /**< Conversion Adapter egress send*/
  /* If statistics register is added, it is added after.*/
};


/**
 * @brief API which get Conversion Adapter statistics (bytes)
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
 *   Get Conversion Adapter statistics in bytes
 */
int fpga_conv_get_stat_bytes(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t reg_id,
        uint64_t *byte_num);

/**
 * @brief API which get Conversion Adapter statistics (frame count)
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
 *   Get Conversion Adapter statistics (number of frames)
 */
int fpga_conv_get_stat_frames(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t reg_id,
        uint32_t *frame_num);

/**
 * @brief API which get buffer overflow information for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] ovf_result
 *   Buffer overflow information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   Get buffer overflow information for Conversion Adapter
 */
int fpga_conv_get_stat_ovf(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *ovf_result);

/**
 * @brief API which clear buffer overflow information for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] ovf_result
 *   Buffer overflow information clear setting
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   pwrite() failure
 *
 * @details
 *   Clear buffer overflow information for Conversion Adapter
 */
int fpga_conv_set_stat_ovf_clear(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t ovf_result);

/**
 * @brief API which get the number of frame buffer used for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[out] usage_num
 *   number of frame buffer used
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
 *   Get the number of frame buffer used by Conversion Adapter
 */
int fpga_conv_get_stat_buff_usage(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t *usage_num);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFUNCTION_CONV_STAT_H_
