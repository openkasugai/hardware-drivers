/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libdirecttrans.h
 * @brief Header file for Direct Transfer Adapter configuring
 */

#ifndef LIBFPGA_INCLUDE_LIBDIRECTTRANS_H_
#define LIBFPGA_INCLUDE_LIBDIRECTTRANS_H_

#include <libfpgactl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief API which start Direct Transfer Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Start the Direct Transfer Adapter
 */
int fpga_direct_start(
        uint32_t dev_id,
        uint32_t lane);

/**
 * @brief API which stop Direct Transfer Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Stop Direct Transfer Adapter
 */
int fpga_direct_stop(
        uint32_t dev_id,
        uint32_t lane);

/**
 * @brief API which get Direct Transfer Adapter control value
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] control
 *   control register value
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get the control value for a Direct Transfer Adapter
 */
int fpga_direct_get_control(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *control);

/**
 * @brief API which get module_id value for Direct Transfer Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] module_id
 *   module_id register value
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get the module_id value of a Direct Transfer Adapter
 */
int fpga_direct_get_module_id(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *module_id);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBDIRECTTRANS_H_
