/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfunction_conv.h
 * @brief Header file for Conversion Adapter configuring
 */

#ifndef LIBFPGA_INCLUDE_LIBFUNCTION_CONV_H_
#define LIBFPGA_INCLUDE_LIBFUNCTION_CONV_H_

#include <libfunction.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Example of libfunction_conv's json format.@n
 */
#define LIBFUNCTION_CONV_PARAMS_JSON_FMT \
"{"                                         \
LIBFUNCTION_PARAM_FMT1("i_width", u)        \
LIBFUNCTION_PARAM_FMT1("i_height", u)       \
LIBFUNCTION_PARAM_FMT1("frame_buffer_l", u) \
LIBFUNCTION_PARAM_FMT0("frame_buffer_h", u) \
"}"


/**
 * @brief API which initialize Conversion Adapter process
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param json_txt
 *   Don't care
 * @retval 0
 *   Success
 *
 * @details
 *   Conversion Adapter Initialization Process
 *   TBD: Check if the function type of the FPGA is the same as that of the filter size.
 *        return an error if they are not identical.
 *   The current state always returns 0.
 *   Since json_txt is not used, it can be any value, including NULL.
 */
int fpga_conv_init(
        uint32_t dev_id,
        uint32_t lane,
        const char *json_txt);

/**
 * @brief API which set Conversion Adapter process
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] json_txt
 *   e.g.) parameters
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   pwrite() failure
 * @details
 *   Parse json_txt and set the following parameters needed for filter size.@n
 *   - i_width: horizontal frame size of input image@n
 *   - i_height: input image vertical frame size@n
 */
int fpga_conv_set(
        uint32_t dev_id,
        uint32_t lane,
        const char *json_txt);

/**
 * @brief API which get Conversion Adapter parameters
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] json_txt
 *   pointer variable to get Conversion Adapter parameters
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval FAILURE_READ
 *   read() failure
 *
 * @details
 *   Read the register of the following parameters set by fpga_function_set().
 *   The value is returned to the user as a JSON-formatted string.
 *   The JSON-formatted string shall be returned in decimal format,
 *   since hexadecimal format would cause blurring of prefix formatting.
 *   - i_width: input image horizontal frame size (dec)@n
 *   - i_height: input image vertical frame size (dec)@n
 *   - frame_buffer_l: Frame buffer offset lower address@n
 *   - frame_buffer_h: Frame buffer offset upper address@n
 *   The string returned to the user is reserved by the library.
 *   Be careful about freeing, see fpga_function_get().
 */
int fpga_conv_get_setting(
        uint32_t dev_id,
        uint32_t lane,
        char **json_txt);

/**
 * @brief API which finalize Conversion Adapter process
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] json_txt
 *   e.g.) parameters
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   pwrite() failure
 *
 * @details
 *   Reset the set frame size and stop the module.
 */
int fpga_conv_finish(
        uint32_t dev_id,
        uint32_t lane,
        const char *json_txt);

/**
 * @brief API which get control value of Conversion Adapter
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
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   Get the control value of Conversion Adapter
 */
int fpga_conv_get_control(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *control);

/**
 * @brief API which get module_id value of Conversion Adapter
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
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   Get module_id value of Conversion Adapter
 */
int fpga_conv_get_module_id(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *module_id);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFUNCTION_CONV_H_
