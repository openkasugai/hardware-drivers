/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _LIBFUNCTION_BASE_H__
#define _LIBFUNCTION_BASE_H__

#include <stdint.h>
#include <libutil.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* fpga_function_base_t;

// User API

/**
 * @brief Initialize the object for setting any function.
 * @param[in] dev_id device number
 * @param[in] func_name function name
 * @param[out] info the object initialized
 * @return status code
 **/
xse_status_t fpga_function_base_init(int dev_id, const char* func_name, fpga_function_base_t* info);

/**
 * @brief Finalize the object for setting any function.
 * @param[in] info the object to finalize
 * @return status code
 **/
xse_status_t fpga_function_base_finish(fpga_function_base_t info);

/**
 * @brief Write a specified value to the function register.
 * @param[in] info the object for the function
 * @param[in] offset offset for register address
 * @param[in] value write value
 * @return status code
 **/
xse_status_t fpga_function_base_reg_write_uint64(fpga_function_base_t info, uint32_t offset, uint64_t value);
xse_status_t fpga_function_base_reg_write_uint32(fpga_function_base_t info, uint32_t offset, uint32_t value);
xse_status_t fpga_function_base_reg_write_fp32(fpga_function_base_t info, uint32_t offset, float value);

/**
 * @brief Read a specified register of the function.
 * @param[in] info the object for the function
 * @param[in] offset offset for register address
 * @param[out] value read value
 * @return status code
 **/
xse_status_t fpga_function_base_reg_read_uint64(fpga_function_base_t info, uint32_t offset, uint64_t* value);
xse_status_t fpga_function_base_reg_read_uint32(fpga_function_base_t info, uint32_t offset, uint32_t* value);
xse_status_t fpga_function_base_reg_read_fp32(fpga_function_base_t info, uint32_t offset, float* value);

#ifdef __cplusplus
}
#endif

#endif // _LIBFUNCTION_BASE_H__
