/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libdirecttrans_stat.h
 * @brief Header file for get Direct Transfer Adapter statistics
 */

#ifndef LIBFPGA_INCLUDE_LIBDIRECTTRANS_STAT_H_
#define LIBFPGA_INCLUDE_LIBDIRECTTRANS_STAT_H_

#include <libfpgactl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The minimum value of the ID
 */
#ifndef FUNCTION_CHAIN_ID_MIN
#define FUNCTION_CHAIN_ID_MIN   XPCIE_FUNCTION_CHAIN_ID_MIN
#endif

/**
 * Maximum value of ID
 */
#ifndef FUNCTION_CHAIN_ID_MAX
#define FUNCTION_CHAIN_ID_MAX   XPCIE_FUNCTION_CHAIN_ID_MAX
#endif


/**
 * @brief API which get Direct Transfer Adapter statistics (bytes)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[in] reg_id
 *   Statistics Register ID
 * @param[out] byte_num
 *   number of bytes
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get Direct Transfer Adapter statistics (bytes)
 */
int fpga_direct_get_stat_bytes(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t reg_id,
        uint64_t *byte_num);

/**
 * @brief API which get Direct Transfer Adapter statistics (frames)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channnel id
 * @param[in] reg_id
 *   Statistics Register ID
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
 *   get Direct Transfer Adapter statistics (frame count)
 */
int fpga_direct_get_stat_frames(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t reg_id,
        uint32_t *frame_num);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBDIRECTTRANS_STAT_H_
