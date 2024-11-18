/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfpga_json.h
 * @brief Header file for parsing json by `parson` library
 */

#ifndef LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGA_JSON_H_
#define LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGA_JSON_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct json_param_u32_t
 * @brief struct for uint32_t data and parameter name
 * @var json_param_u32_t::val
 *      4byte data
 * @var json_param_u32_t::str
 *      json parameter name
 */
typedef struct libfpga_json_param_u32 {
  uint32_t val;
  char *str;
} json_param_u32_t;


/**
 * @brief Function which get parameter's value
 * @param[in] json_txt : json format text
 * @param[in] parameter : parameter name to get data
 * @param[out] value : pointer variable to get value of `parameter` from `json_txt`
 * @retval 0 : success
 * @retval -INVALID_ARGUMENT : e.g.) arguments are null, failed to parse
 * @retval -INVALID_DATA : e.g.) `parameter`'s type is NOT integer
 * @details `*value` will be not allocated by this function, so allocate by API.
 * @details The value of `*value` is undefined when this function fails.
 */
int __fpga_json_get_param_u32(
        const char *json_txt,
        const char *parameter,
        uint32_t *value);

/**
 * @brief Function which check if parameter is exist
 * @param[in] json_txt : json format text
 * @param[in] parameter : parameter name to get data
 * @retval 0 : success
 * @retval -INVALID_ARGUMENT : e.g.) arguments are null, failed to parse
 * @retval -INVALID_DATA : e.g.) `parameter` is NOT exist
 * @details The type of `parameter` is not considered.
 */
int __fpga_json_check_param(
        const char *json_txt,
        const char *parameter);

/**
 * @brief Function which get device config info
 * @param[in] json_txt : json format text
 * @param[in] bitstream_id : matching key `bitstream-id`'s string
 * @param[out] config_json : pointer variable to get mathing data
 * @retval 0 : success
 * @retval -INVALID_ARGUMENT : e.g.) arguments are null, failed to parse
 * @retval -INVALID_DATA : e.g.) matching `bitstream_id` is NOT exist
 * @details The json text will return with memory allocated by this function through `config_json`,
 *          so please explicitly free through free()/__fpga_json_free_string().
 * @details The value of `*config_json` is undefined when this function fails.
 */
int __fpga_json_get_device_config(
        const char *json_txt,
        const char *bitstream_id,
        char **config_json);

/**
 * @brief Function which get json string by required parameter name and value
 * @param[in] params : table corresponding parameter name with value
 * @retval NULL(log:INVALID_ARGUMENT) : e.g.) `params` is null
 * @retval NULL(log:INVALID_DATA) : e.g.) failed parson's API
 * @retval NULL(log:FAILURE_MEMORY_ALLOC) : e.g.) failed to allocate memory
 * @return valid memory address allocated for required json text
 * @details The json text will return with memory allocated by this function through `retval`,
 *          so please explicitly free through __fpga_json_free_string().
 * @details When `retval` is NULL, libfpga's errno will be printed as log.
 */
char *__fpga_json_malloc_string_u32(
        const json_param_u32_t params[]);

/**
 * @brief Function which free memory allocated by libfpga_json
 * @param[in] json_txt : pointer variable for json to free
 * @return void
 * @details This function is a wrap function for free() to map names to allocate functions.
 */
void __fpga_json_free_string(
        char *json_txt);

/**
 * @brief Function which get parameter's value
 * @param[in] json_txt : json format text
 * @param[in] parameter : parameter name to get data
 * @retval (uint32_t)-1 : Failed to get value of `parameter` from `json_txt`
 * @return The value of `parameter` from `json_txt`
 */
static inline uint32_t __fpga_get_parameter(
  const char *json_txt,
  const char *parameter
) {
  uint32_t value = (uint32_t)-1;
  if (!__fpga_json_check_param(json_txt, parameter)) {
    int ret;
    if ((ret = __fpga_json_get_param_u32(json_txt, parameter, &value)))
      do {}while(0);
  }
  return value;
}

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGA_JSON_H_
