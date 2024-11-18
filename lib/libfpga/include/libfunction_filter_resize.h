/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfunction_filter_resize.h
 * @brief Header file for setting for FPGA's filter resize module
 */

#ifndef LIBFPGA_INCLUDE_LIBFUNCTION_FILTER_RESIZE_H_
#define LIBFPGA_INCLUDE_LIBFUNCTION_FILTER_RESIZE_H_

#include <libfunction.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Example of libfunction_filter_resize's json format.@n
 * It is possible to use this format as follows:
~~~{.c}
  char buf[1024];
  uint32_t i_width = 3840, i_height = 2160;
  uint32_t o_width = 1280, o_height = 1280;
  sprintf(buffer, LIBFUNCTION_FILTER_RESIZE_PARAMS_JSON_FMT,
    i_width, i_height, o_width, o_height);
~~~
 * `buffer` will be as follows:
~~~{.json}
{
  "i_width" : 3840,
  "i_height" : 2160,
  "o_width" : 1280,
  "o_height" : 1280
}
~~~
 */
#define LIBFUNCTION_FILTER_RESIZE_PARAMS_JSON_FMT \
"{"                                        \
LIBFUNCTION_PARAM_FMT1("i_width", u)       \
LIBFUNCTION_PARAM_FMT1("i_height", u)      \
LIBFUNCTION_PARAM_FMT1("o_width", u)       \
LIBFUNCTION_PARAM_FMT0("o_height", u)      \
"}"

/**
 * Example of libfunction_filter_resize's get()'s json format.@n
 * output will be as follows:
~~~{.json}
{
  "i_width" : 3840,
  "i_height" : 2160,
  "o_width" : 1280,
  "o_height" : 1280,
  "func_i_width" : 3840,
  "func_i_height" : 2160,
  "func_o_width" : 1280,
  "func_o_height" : 1280,
  "sub_i_width" : 3840,
  "sub_i_height" : 2160,
  "sub_o_width" : 1280,
  "sub_o_height" : 1280,
}
~~~
 */
#define LIBFUNCTION_FILTER_RESIZE_PARAMS_JSON_ERROR_FMT \
"{"                                        \
LIBFUNCTION_PARAM_FMT1("i_width", u)       \
LIBFUNCTION_PARAM_FMT1("i_height", u)      \
LIBFUNCTION_PARAM_FMT1("o_width", u)       \
LIBFUNCTION_PARAM_FMT1("o_height", u)      \
LIBFUNCTION_PARAM_FMT1("func_i_width", u)  \
LIBFUNCTION_PARAM_FMT1("func_i_height", u) \
LIBFUNCTION_PARAM_FMT1("func_o_width", u)  \
LIBFUNCTION_PARAM_FMT1("func_o_height", u) \
LIBFUNCTION_PARAM_FMT1("sub_i_width", u)   \
LIBFUNCTION_PARAM_FMT1("sub_i_height", u)  \
LIBFUNCTION_PARAM_FMT1("sub_o_width", u)   \
LIBFUNCTION_PARAM_FMT0("sub_o_height", u)  \
"}"


/**
 * @brief API which register libfunction_filter_resize into libfunction's function list
 * @param void
 * @return
 *   Same as fpga_function_register()
 *
 * @details
 *   Call fpga_function_register() to register libfunction_filter_resize.
 * @sa fpga_function_register()@n
 */
int fpga_function_register_filter_resize(void);

/**
 * @brief API which get Filter Resize control value
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
 */
int fpga_filter_resize_get_control(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *control);

/**
 * @brief API which get module_id value of Filter Resize
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] control
 *   module_id register value
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_READ
 *   pread() failure
 */
int fpga_filter_resize_get_module_id(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *module_id);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFUNCTION_FILTER_RESIZE_H_
