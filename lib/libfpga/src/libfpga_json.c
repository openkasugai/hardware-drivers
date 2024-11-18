/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfpga_internal/libfpga_json.h>
#include <liblogging.h>

#include <parson.h>  // parson

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBFPGACTL


#define PARAM_IS_EXIST 0  /**< flag for __fpga_json_check_param */
#define PARAM_IS_GET   1  /**< flag for __fpga_json_get_param_u32 */


static int __json_parse_string(
  const char *json_txt,
  JSON_Value **root,
  JSON_Object **obj
) {
  int ret = 0;

  *root = json_parse_string_with_comments(json_txt);
  if (!(*root)) {
    llf_err(INVALID_ARGUMENT, "Failed to parse string: %s\n", json_txt);
    return -INVALID_ARGUMENT;
  }

  *obj = json_value_get_object(*root);
  if (!(*obj)) {
    ret = -INVALID_ARGUMENT;
    llf_err(INVALID_ARGUMENT, "Failed to get object: %s\n", json_txt);
    json_value_free(*root);
  }

  return ret;
}


static int __json_parse_file(
  const char *json_file,
  JSON_Value **root,
  JSON_Object **obj
) {
  int ret = 0;

  *root = json_parse_file_with_comments(json_file);
  if (!(*root)) {
    llf_err(INVALID_ARGUMENT, "Failed to parse file: %s\n", json_file);
    return -INVALID_ARGUMENT;
  }

  *obj = json_value_get_object(*root);
  if (!(*obj)) {
    ret = -INVALID_ARGUMENT;
    llf_err(INVALID_ARGUMENT, "Failed to get object: %s\n", json_file);
    json_value_free(*root);
  }

  return ret;
}


static int __json_parse_param_u32(
  const char *json_txt,
  const char *parameter,
  int flag,
  uint32_t *value
) {
  int ret = 0;

  JSON_Value *root;
  JSON_Object *obj;

  if ((ret = __json_parse_string(json_txt, &root, &obj)))
    return ret;

  JSON_Value *param = json_object_get_value(obj, parameter);
  JSON_Value_Type type = json_value_get_type(param);

  switch (flag) {
  case PARAM_IS_GET:
    if (type == JSONNumber) {
      *value = (int)json_object_get_number(obj, parameter);  // NOLINT
    } else {
      ret = -INVALID_DATA;
      llf_err(INVALID_DATA, "Invalid data: Parameter(%s) is type(%d)\n", parameter, type);
    }
    break;
  case PARAM_IS_EXIST:
    if (type == JSONError) {
      ret = -INVALID_DATA;
    }
    break;
  default:
    break;
  }

  json_value_free(root);

  return ret;
}


// cppcheck-suppress unusedFunction
int __fpga_json_get_param_u32(
  const char *json_txt,
  const char *parameter,
  uint32_t *value
) {
  if (!json_txt || !parameter || !value) {
    llf_err(INVALID_ARGUMENT, "%s(json_txt(%s), parameter(%s), value(%#x))\n",
      __func__, json_txt ? json_txt : "<null>",
      parameter ? parameter : "<null>", (uintptr_t)value);
    return -INVALID_ARGUMENT;
  }

  return __json_parse_param_u32(json_txt, parameter, PARAM_IS_GET, value);
}


// cppcheck-suppress unusedFunction
int __fpga_json_check_param(
  const char *json_txt,
  const char *parameter
) {
  if (!json_txt || !parameter) {
    llf_err(INVALID_ARGUMENT, "%s(json_txt(%s), parameter(%s))\n",
      __func__, json_txt ? json_txt : "<null>", parameter ? parameter : "<null>");
    return -INVALID_ARGUMENT;
  }

  return __json_parse_param_u32(json_txt, parameter, PARAM_IS_EXIST, NULL);
}


int __fpga_json_get_device_config(
  const char *json_file,
  const char *bitstream_id,
  char **config_json
) {
  if (!json_file || !bitstream_id || !config_json) {
    llf_err(INVALID_ARGUMENT, "%s(json_file(%s), bitstream_id(%s), config_json(%#x))\n",
      __func__, json_file, bitstream_id, (uintptr_t)config_json);
    return -INVALID_ARGUMENT;
  }

  JSON_Value *root;
  JSON_Object *obj;
  int ret;

  if ((ret = __json_parse_file(json_file, &root, &obj)))
    return ret;

  JSON_Array *array = json_object_get_array(obj, "configs");
  for (int index = 0;; index++) {
    JSON_Object *elem = json_array_get_object(array, index);
    if (!elem) {
      ret = -INVALID_DATA;
      llf_err(INVALID_DATA, "Failed to access array[index:%d]\n", index);
      break;
    }
    const char *bs = json_object_get_string(elem, "bitstream-id");
    if (!bs) {
      ret = -INVALID_DATA;
      llf_err(INVALID_DATA, "Failed to find bitstream-id: %s[last_index:%d]\n",
        bitstream_id, index);
      break;
    }
    if (!strcmp(bs, bitstream_id)) {
      JSON_Value *match = json_object_get_wrapping_value(elem);
      char *config_string = json_serialize_to_string_pretty(match);
      *config_json = strdup(config_string);
      json_free_serialized_string(config_string);
      break;
    }
  }

  json_value_free(root);

  return ret;
}


char *__fpga_json_malloc_string_u32(
  const json_param_u32_t params[]
) {
  // Check input
  if (!params) {
    llf_err(INVALID_ARGUMENT, "%s(params(%#x))\n", __func__, (uintptr_t)params);
    return NULL;
  }

  char *json_txt;

  // Initialize JSON_Value and Get JSON_Object from JSON_Value
  JSON_Value *val = json_value_init_object();
  JSON_Object *obj = json_object(val);

  // Add pairs of parameter name and value into JSON_Object
  for (int index = 0; params[index].str; index++) {
    JSON_Status retval = json_object_set_number(obj, params[index].str, params[index].val);
    if (retval) {
      llf_err(INVALID_DATA, "Fatal error: json_object_set_number failed in %d\n", retval);
      json_value_free(val);
      return NULL;
    }
  }

  // Create json_txt in parson
  char *str = json_serialize_to_string_pretty(val);
  if (!str) {
      llf_err(INVALID_DATA, "Fatal error: json_serialize_to_string_pretty failed\n");
      json_value_free(val);
      return NULL;
    }

  // Copy normal string in this library
  json_txt = strdup(str);
  if (!json_txt) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for json_txt\n");
  }

  // Free json_txt in parson and JSON_Value
  json_free_serialized_string(str);
  json_value_free(val);

  return json_txt;
}


// cppcheck-suppress unusedFunction
void __fpga_json_free_string(
  char *json_txt
) {
  // Check input
  if (!json_txt) {
    llf_err(INVALID_ARGUMENT, "%s(json_txt(<null>))\n", __func__);
    return;
  }

  free(json_txt);
}
