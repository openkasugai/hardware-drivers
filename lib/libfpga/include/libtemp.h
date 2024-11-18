/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libtemp.h
 * @brief Header file for getting FPGA's temperature
 */

#ifndef LIBFPGA_INCLUDE_LIBTEMP_H_
#define LIBFPGA_INCLUDE_LIBTEMP_H_

#include <libfpgactl.h>

#ifdef __cplusplus
extern "C" {
#endif


#define ALVEO_U250_CAGE_TEMP0_NAME    "U250_CAGE_TEMP0"   /**< used for fpga_get_temp()'s JSON parameter name */
#define ALVEO_U250_CAGE_TEMP1_NAME    "U250_CAGE_TEMP1"   /**< used for fpga_get_temp()'s JSON parameter name */
#define ALVEO_U250_DIMM_TEMP0_NAME    "U250_DIMM_TEMP0"   /**< used for fpga_get_temp()'s JSON parameter name */
#define ALVEO_U250_DIMM_TEMP1_NAME    "U250_DIMM_TEMP1"   /**< used for fpga_get_temp()'s JSON parameter name */
#define ALVEO_U250_DIMM_TEMP2_NAME    "U250_DIMM_TEMP2"   /**< used for fpga_get_temp()'s JSON parameter name */
#define ALVEO_U250_DIMM_TEMP3_NAME    "U250_DIMM_TEMP3"   /**< used for fpga_get_temp()'s JSON parameter name */
#define ALVEO_U250_FAN_TEMP_NAME      "U250_FAN_TEMP"     /**< used for fpga_get_temp()'s JSON parameter name */
#define ALVEO_U250_FPGA_TEMP_NAME     "U250_FPGA_TEMP"    /**< used for fpga_get_temp()'s JSON parameter name */
#define ALVEO_U250_SE98_TEMP0_NAME    "U250_SE98_TEMP0"   /**< used for fpga_get_temp()'s JSON parameter name */
#define ALVEO_U250_SE98_TEMP1_NAME    "U250_SE98_TEMP1"   /**< used for fpga_get_temp()'s JSON parameter name */
#define ALVEO_U250_SE98_TEMP2_NAME    "U250_SE98_TEMP2"   /**< used for fpga_get_temp()'s JSON parameter name */
#define ALVEO_U250_VCCINT_TEMP_NAME   "U250_VCCINT_TEMP"  /**< used for fpga_get_temp()'s JSON parameter name */


/**
 * @brief API which get FPGA's temerature information by json format string
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] temp_info
 *   pointer variable to get temperature information register values
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `temp_info` is null
 * @retval -FAILURE_IOCTL
 *   e.g.) Maybe xpcie_device.h used in driver and lib differ
 * @retval -INVALID_PARAMETER
 *   e.g.) FPGA's card name is not supported in libfpgactl
 * @retval -NO_DEVICES
 *   e.g.) FPGA's card name is not supported in libtemp
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 *
 * @details
 *   Get `temp_info` from FPGA's register value and return it with shaping as json format.@n
 *   The json text will return with memory allocated by this library through `temp_info`,
 *    so please explicitly free by free().@n
 *   The value of `*temp_info` is undefined when this API fails.
 */
int fpga_get_temp(
        uint32_t dev_id,
        char **temp_info);

#ifdef __cplusplus
}
#endif

#endif  /* LIBFPGA_INCLUDE_LIBTEMP_H_ */
