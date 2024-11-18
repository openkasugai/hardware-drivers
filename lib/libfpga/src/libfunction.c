/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfunction.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <dlfcn.h>

#ifndef enable_external_libfunction_filter_resize  // defined by Makefile if need
#include <libfunction_filter_resize.h>
#endif


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBFUNCTION

/**
 * Function list's Max value
 */
#define LIBFUNCTION_FUNCTION_MAX (FPGA_MAX_DEVICES * LIBFPGA_KERNEL_MAX_ALL)


/**
 * @struct fpga_function_lib_t
 * @brief Struct for management of opening shared library
 * @var fpga_function_lib_t::library_name
 *      function name, used for matching function
 * @var fpga_function_lib_t::handle
 *      return value of dlopen()
 */
typedef struct fpga_function_lib {
  char *library_name;     // library name
  void *handle;         // shared library handle
} fpga_function_lib_t;


/**
 * static global variable : Function operations list of each function
 */
static const fpga_function_ops_t*
  function_operations_list[LIBFUNCTION_FUNCTION_MAX];

/**
 * static global variable : Function operations table with dev_id and lane as indexes
 */
static const fpga_function_ops_t*
  function_operations_table[FPGA_MAX_DEVICES][LIBFPGA_KERNEL_MAX_ALL];

/**
 * static global variable : Json string table with dev_id and lane as indexes got by fpga_function_get()
 */
static char*
  function_json_params_table[FPGA_MAX_DEVICES][LIBFPGA_KERNEL_MAX_ALL];

/**
 * static global variable : Opening shared libraries management table
 */
static fpga_function_lib_t
  function_shared_libs_table[LIBFUNCTION_FUNCTION_MAX];


/**
 * @brief Function which register default function operations
 */
static int __libfunction_register_default_function(void) {
  int ret = 0;

  // When the header files are included, call each register APIs
#ifdef LIBFPGA_INCLUDE_LIBFUNCTION_FILTER_RESIZE_H_
  ret = fpga_function_register_filter_resize();
  if (ret)
    return ret;
#endif

  // cppcheck-suppress identicalConditionAfterEarlyExit
  return ret;
}


/**
 * @brief Function which initialize list/tables at first once.
 */
static int __libfunction_init(void) {
  static bool init_once = true;
  if (init_once) {
    init_once = !init_once;
    memset(function_operations_table, 0, sizeof(function_operations_table));
    memset(function_operations_list, 0, sizeof(function_operations_list));
    memset(function_json_params_table, 0, sizeof(function_json_params_table));
    memset(function_shared_libs_table, 0, sizeof(function_shared_libs_table));
    if (__libfunction_register_default_function()) {
      llf_err(LIBFPGA_FATAL_ERROR, "Failed to register DEFAULT Function!!!\n");
      return -LIBFPGA_FATAL_ERROR;
    }
  }
  return 0;
}


/**
 * @brief Function which convert hyphen to underscore
 */
static char* __libfunction_convert_hyphen2underscore(
  const char* func_name
) {
  if (!func_name)
    return NULL;
  char *str = strdup(func_name);
  if (!str)
    return NULL;
  for (int index = 0; *(str+index); index++)
    if (*(str+index) == '-')
      *(str+index) = '_';
  if (strcmp(func_name, str))
    llf_dbg(" Input parameter converted:\'%s\' ==> \'%s\'\n", func_name, str);
  return str;
}


int fpga_function_config(
  uint32_t dev_id,
  uint32_t lane,
  const char *func_type
) {
  if (__libfunction_init()) {
    llf_err(LIBFPGA_FATAL_ERROR, "%s(dev_id(%u), lane(%u), func_type(%s))\n",
      __func__, dev_id, lane, func_type ? func_type : "<null>");
    return -LIBFPGA_FATAL_ERROR;
  }

  fpga_device_t *dev = fpga_get_device(dev_id);

  if (!dev || lane >= KERNEL_NUM_FUNC(dev) || !func_type) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), func_type(%s))\n",
      __func__, dev_id, lane, func_type ? func_type : "<null>");
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), lane(%u), func_type(%s))\n",
    __func__, dev_id, lane, func_type);

  // Delete FPGA's configuration when func_name = ""
  if (strlen(func_type) == 0) {
    function_operations_table[dev_id][lane] = NULL;
    return 0;
  }

  char *func_name = __libfunction_convert_hyphen2underscore(func_type);
  if (!func_name) {
    int err = errno;
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for temporary function name(errno:%d)\n", err);
    return -FAILURE_MEMORY_ALLOC;
  }

  // Matching input function name and function names in function operations list
  for (int index = 0; index < LIBFUNCTION_FUNCTION_MAX; index++) {
    const fpga_function_ops_t *ops = function_operations_list[index];
    if (ops && ops->name && !strcmp(ops->name, func_name)) {
      function_operations_table[dev_id][lane] = ops;
      free(func_name);
      return 0;
    }
  }

  llf_err(INVALID_DATA, "Invalid operation: %s not found.\n", func_name);
  free(func_name);
  return -INVALID_DATA;
}


int fpga_function_get_config_name(
  uint32_t dev_id,
  uint32_t lane,
  char **func_name
) {
  if (__libfunction_init()) {
    llf_err(LIBFPGA_FATAL_ERROR, "%s(dev_id(%u), lane(%u), func_name(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)func_name);
    return -LIBFPGA_FATAL_ERROR;
  }

  fpga_device_t *dev = fpga_get_device(dev_id);

  if (!dev || lane >= KERNEL_NUM_FUNC(dev) || !func_name) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), func_name(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)func_name);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), lane(%u), func_name(%#lx))\n",
    __func__, dev_id, lane, (uintptr_t)func_name);

  if (!function_operations_table[dev_id][lane]) {
    llf_err(INVALID_DATA, "Invalid operation: Function table is NOT set.\n");
    return -INVALID_DATA;
  }

  // Get FPGA's configuration func name
  *func_name = strdup(function_operations_table[dev_id][lane]->name);
  if (!*func_name) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for function name");
    return -FAILURE_MEMORY_ALLOC;
  }

  return 0;
}


int fpga_function_init(
  uint32_t dev_id,
  uint32_t lane,
  const char *json_txt
) {
  if (__libfunction_init()) {
    llf_err(LIBFPGA_FATAL_ERROR, "%s(dev_id(%u), lane(%u), json_txt(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -LIBFPGA_FATAL_ERROR;
  }

  fpga_device_t *dev = fpga_get_device(dev_id);

  // Check input
  if (!dev || lane >= KERNEL_NUM_FUNC(dev)) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), lane(%u),json(%s))\n",
    __func__, dev_id, lane, json_txt ? json_txt : "<null>");

  // Check valid data
  if (!function_operations_table[dev_id][lane]) {
    llf_err(INVALID_DATA, "Invalid operation: Function table is NOT set.\n");
    return -INVALID_DATA;
  }
  if (!function_operations_table[dev_id][lane]->init) {
    llf_err(INVALID_DATA, "Invalid operation: Function is NOT implement.\n");
    return -INVALID_DATA;
  }

  int ret = function_operations_table[dev_id][lane]->init(dev_id, lane, json_txt);
  if (ret)
    llf_err(-ret, "%s(dev_id(%u), lane(%u),json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");

  return ret;
}


int fpga_function_set(
  uint32_t dev_id,
  uint32_t lane,
  const char* json_txt
) {
  if (__libfunction_init()) {
    llf_err(LIBFPGA_FATAL_ERROR, "%s(dev_id(%u), lane(%u), json_txt(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -LIBFPGA_FATAL_ERROR;
  }

  fpga_device_t *dev = fpga_get_device(dev_id);

  // Check input
  if (!dev || lane >= KERNEL_NUM_FUNC(dev)) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u),json(%s))\n",
    __func__, dev_id, lane, json_txt ? json_txt : "<null>");

  // Check valid data
  if (!function_operations_table[dev_id][lane]) {
    llf_err(INVALID_DATA, "Invalid operation: Function table is NOT set.\n");
    return -INVALID_DATA;
  }
  if (!function_operations_table[dev_id][lane]->set) {
    llf_err(INVALID_DATA, "Invalid operation: Function is NOT implement.\n");
    return -INVALID_DATA;
  }

  int ret = function_operations_table[dev_id][lane]->set(dev_id, lane, json_txt);
  if (ret)
    llf_err(-ret, "%s(dev_id(%u), lane(%u),json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");

  return ret;
}


int fpga_function_get(
  uint32_t dev_id,
  uint32_t lane,
  char** json_txt
) {
  if (__libfunction_init()) {
    llf_err(LIBFPGA_FATAL_ERROR, "%s(dev_id(%u), lane(%u), json(%#x))\n",
      __func__, dev_id, lane, (uintptr_t)json_txt);
    return -LIBFPGA_FATAL_ERROR;
  }

  fpga_device_t *dev = fpga_get_device(dev_id);

  // Check input
  if (!dev || lane >= KERNEL_NUM_FUNC(dev)) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%#x))\n",
      __func__, dev_id, lane, (uintptr_t)json_txt);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u),json(%#x))\n", __func__, dev_id, lane, (uintptr_t)json_txt);

  // Check valid data
  if (!function_operations_table[dev_id][lane]) {
    llf_err(INVALID_DATA, "Invalid operation: Function table is NOT set.\n");
    return -INVALID_DATA;
  }
  if (!function_operations_table[dev_id][lane]->get) {
    llf_err(INVALID_DATA, "Invalid operation: Function is NOT implement.\n");
    return -INVALID_DATA;
  }

  // Free previous json text if exist
  if (function_json_params_table[dev_id][lane]) {
    free(function_json_params_table[dev_id][lane]);
    function_json_params_table[dev_id][lane] = NULL;
  }

  int ret = function_operations_table[dev_id][lane]->get(dev_id, lane, json_txt);
  if (ret)
    llf_err(-ret, "%s(dev_id(%u), lane(%u),json(%#x))\n",
      __func__, dev_id, lane, (uintptr_t)json_txt);

  if (ret == 0 && json_txt && (*json_txt))
    function_json_params_table[dev_id][lane] = *json_txt;

  return ret;
}


int fpga_function_finish(
  uint32_t dev_id,
  uint32_t lane,
  const char* json_txt
) {
  if (__libfunction_init()) {
    llf_err(LIBFPGA_FATAL_ERROR, "%s(dev_id(%u), lane(%u), json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -LIBFPGA_FATAL_ERROR;
  }

  fpga_device_t *dev = fpga_get_device(dev_id);

  // Check input
  if (!dev || lane >= KERNEL_NUM_FUNC(dev)) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u),json(%s))\n",
    __func__, dev_id, lane, json_txt ? json_txt : "<null>");

  // Check valid data
  if (!function_operations_table[dev_id][lane]) {
    llf_err(INVALID_DATA, "Invalid operation: Function table is NOT set.\n");
    return -INVALID_DATA;
  }
  if (!function_operations_table[dev_id][lane]->finish) {
    llf_err(INVALID_DATA, "Invalid operation: Function is NOT implement.\n");
    return -INVALID_DATA;
  }

  int ret = function_operations_table[dev_id][lane]->finish(dev_id, lane, json_txt);
  if (ret)
    llf_err(-ret, "%s(dev_id(%u), lane(%u),json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");

  function_operations_table[dev_id][lane] = NULL;
  // Free previous json text if exist
  if (function_json_params_table[dev_id][lane]) {
    free(function_json_params_table[dev_id][lane]);
    function_json_params_table[dev_id][lane] = NULL;
  }

  return ret;
}


int fpga_function_register(
  const fpga_function_ops_t *ops
) {
  // clean function_operations_table[][] and init function_operations_list[] array at first
  if (__libfunction_init()) {
    llf_err(LIBFPGA_FATAL_ERROR, "%s(ops(%#x)[%s])\n",
      __func__, ops, ops->name ? ops->name : "<null>");
    return -LIBFPGA_FATAL_ERROR;
  }

  if (!ops || !ops->name) {
    llf_err(INVALID_ARGUMENT, "%s(ops(%#x))\n", __func__, ops);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(ops(%#x)[%s])\n", __func__, ops, ops->name);

  int index, get_index = -1;
  int list_len = sizeof(function_operations_list)/sizeof(function_operations_list[0]);
  for (index = 0; index < list_len; index++) {
    const fpga_function_ops_t *ops_element = function_operations_list[index];
    if (!ops_element && get_index == -1) {
      get_index = index;
    }
    if (ops_element && !strcmp(ops_element->name, ops->name)) {
      get_index = -2;
      break;
    }
  }

  if (get_index == -1) {
    llf_err(FULL_ELEMENT,
      "Invalid operation: Function list is full.\n");
    return -FULL_ELEMENT;
  }
  if (get_index == -2) {
    llf_err(ALREADY_ASSIGNED,
      "Invalid operation: %s is already registerd.\n", ops->name);
    return -ALREADY_ASSIGNED;
  }

  function_operations_list[get_index] = ops;

  return 0;
}


int fpga_function_unregister(
  const char *name
) {
  if (__libfunction_init()) {
    llf_err(LIBFPGA_FATAL_ERROR, "%s(name(%s))\n", __func__, name ? name : "<null>");
    return -LIBFPGA_FATAL_ERROR;
  }

  // input check
  if (!name) {
    llf_err(INVALID_ARGUMENT, "%s(name(<null>))\n", __func__);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(name(%s))\n", __func__, name);

  // unregister function from list
  int list_len = sizeof(function_operations_list)/sizeof(function_operations_list[0]);
  for (int index = 0; index < list_len; index++) {
    const fpga_function_ops_t *ops_element = function_operations_list[index];
    if (ops_element && !strcmp(ops_element->name, name)) {
      function_operations_list[index] = NULL;
      return 0;
    }
  }

  return -INVALID_DATA;
}


// cppcheck-suppress unusedFunction
int fpga_function_load(
  const char* library_name
) {
  if (__libfunction_init()) {
    llf_err(LIBFPGA_FATAL_ERROR, "%s(library_name(%s))\n",
      __func__, library_name ? library_name : "<null>");
    return -LIBFPGA_FATAL_ERROR;
  }

  int ret = 0;
  void *handle;
  char *library_set_name = NULL;
  char *error_msg = NULL;
  fpga_function_lib_t *lib_element = NULL;

  // Check input
  if (!library_name || strlen(library_name) >= FILENAME_MAX) {
    llf_err(INVALID_ARGUMENT, "%s(library_name(%s))\n",
      __func__, library_name ? library_name : "<null>");
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(library_name(%s))\n", __func__, library_name);

  // Get the smallest index at first
  for (int index = 0; index < LIBFUNCTION_FUNCTION_MAX; index++) {
    lib_element = &function_shared_libs_table[index];
    if (!lib_element->handle)
      break;
    lib_element = NULL;
  }
  if (!lib_element) {
    llf_err(FULL_ELEMENT, "Invalid operation: Max num for load shared library: %d\n",
      LIBFUNCTION_FUNCTION_MAX);
    return -FULL_ELEMENT;
  }

  // convert argument from hyphen into under_score
  char *library_name_under_score = __libfunction_convert_hyphen2underscore(library_name);
  if (!library_name_under_score) {
    int err = errno;
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for temporary function name(errno:%d)\n", err);
    return -FAILURE_MEMORY_ALLOC;
  }

  // Open shared library
  char library_tmp_name[FILENAME_MAX];
  // Create shared-library-name at first
  memset(library_tmp_name, 0, FILENAME_MAX);
  snprintf(library_tmp_name, FILENAME_MAX, "libfunction_%s.so", library_name_under_score);
  handle = dlopen(library_tmp_name, RTLD_LAZY);
  if (handle) {
    // Set name which succeed to open shared library
    library_set_name = strdup(library_name_under_score);
  } else {
    // Use raw argument for shared-library-name
    handle = dlopen(library_name_under_score, RTLD_LAZY);
    if (handle) {
      // Set name which succeed to open shared library
      memset(library_tmp_name, 0, FILENAME_MAX);
      // cppcheck-suppress invalidscanf
      if (sscanf(library_name_under_score, "libfunction_%[^.].so", library_tmp_name) != 1) {
        ret = -INVALID_ARGUMENT;
        llf_err(-ret, "Invalid operation: Name should be libfunction_<func-type>.so\n");
        goto failed;
      }
      library_set_name = strdup(library_tmp_name);
    } else {
      // Failed to open shared library
      error_msg = dlerror();
      llf_err(FAILURE_OPEN, "Failed to open shared library(%s:%s)(error message:%s)\n",
        library_tmp_name, library_name_under_score, error_msg ? error_msg : "Nothing?");
      if (library_name_under_score)
        free(library_name_under_score);
      return -FAILURE_OPEN;
    }
  }
  if (!library_set_name) {
    ret = -FAILURE_MEMORY_ALLOC;
    llf_err(-ret, "Failed to allocate memory for shared library name\n");
    goto failed;
  }
  if (library_name_under_score)
    free(library_name_under_score);

  // get register function(fpga_function_register_<library_name>)
  // Create register-function-symbol-name at first
  char symbol_tmp_name[FILENAME_MAX];
  memset(symbol_tmp_name, 0, FILENAME_MAX);
  snprintf(symbol_tmp_name, FILENAME_MAX, "fpga_function_register_%s", library_set_name);
  // Get the simbol for register function
  int (*register_function)(void) = NULL;
  register_function = (int(*)(void))dlsym(handle, symbol_tmp_name);
  error_msg = dlerror();
  if (error_msg) {
    // Failed to get the simbol for register function
    ret = -INVALID_DATA;
    llf_err(-ret, "Failed to get function(%s)(error message:%s)\n",
      symbol_tmp_name, error_msg);
    goto failed;
  }
  // Call register function in shared library
  ret = (*register_function)();
  if (ret) {
    llf_err(-ret, "Failed function(%s)\n", symbol_tmp_name);
    goto failed;
  }

  // Set handle and function name into table
  //  - handle should be being opened because it is expected to used
  //    init()/set()/get()/finish() function from loaded library
  lib_element->handle = handle;
  lib_element->library_name = library_set_name;

  return ret;

failed:
  if (library_set_name)
    free(library_set_name);
  if (handle)
    dlclose(handle);
  if (library_name_under_score)
    free(library_name_under_score);
  return ret;
}


// cppcheck-suppress unusedFunction
int fpga_function_unload(
  const char* library_name
) {
  if (__libfunction_init()) {
    llf_err(LIBFPGA_FATAL_ERROR, "%s(library_name(%s))\n",
      __func__, library_name ? library_name : "<null>");
    return -LIBFPGA_FATAL_ERROR;
  }

  fpga_function_lib_t *target_library = NULL;
  char *error_msg = NULL;

  // Check input
  if (!library_name || strlen(library_name) >= FILENAME_MAX) {
    llf_err(INVALID_ARGUMENT, "%s(library_name(%s))\n", __func__, "<null>");
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(library_name(%s))\n", __func__, library_name);

  // convert argument from hyphen into under_score
  char *library_name_under_score = __libfunction_convert_hyphen2underscore(library_name);
  if (!library_name_under_score) {
    int err = errno;
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for temporary function name(errno:%d)\n", err);
    return -FAILURE_MEMORY_ALLOC;
  }

  // Get function name from input
  char tmp_library_name[FILENAME_MAX];
  memset(tmp_library_name, 0, sizeof(tmp_library_name));
  // cppcheck-suppress invalidscanf
  if (sscanf(library_name_under_score, "libfunction_%[^.].so", tmp_library_name) != 1)
    strncpy(tmp_library_name, library_name_under_score, FILENAME_MAX - 1);

  free(library_name_under_score);

  // Matching Function
  for (int index = 0; index < LIBFUNCTION_FUNCTION_MAX; index++) {
    target_library = &function_shared_libs_table[index];
    if (target_library->library_name && !strcmp(target_library->library_name, tmp_library_name))
      // Succeed to match function
      break;
    target_library = NULL;
  }
  if (!target_library) {
    // Failed to match function
    llf_err(INVALID_DATA, "Invalid operation: Function not found: %s\n",
      tmp_library_name);
    return -INVALID_DATA;
  }

  // Delete only reference for init/set/get/finish() when function-device-table has unloading function,
  // but not call fpga_function_finish() because finish() may need json argument
  for (int device_id = 0; device_id < FPGA_MAX_DEVICES; device_id++) {
    for (int lane = 0; lane < LIBFPGA_KERNEL_MAX_ALL; lane++) {
      if (function_operations_table[device_id][lane]
          && function_operations_table[device_id][lane]->name
          && !strcmp(function_operations_table[device_id][lane]->name, tmp_library_name)) {
        // Delete reference
        function_operations_table[device_id][lane] = NULL;
        // Free previous json text if exist
        if (function_json_params_table[device_id][lane]) {
          free(function_json_params_table[device_id][lane]);
          function_json_params_table[device_id][lane] = NULL;
        }
        llf_dbg("Delete config of registering function(%s)\n", tmp_library_name);
      }
    }
  }

  // Unregister function when available-function-list has unloading function,
  llf_dbg(" Try to unregister function(%s)\n", tmp_library_name);
  int ret = fpga_function_unregister(tmp_library_name);
  if (!ret)
    llf_dbg(" Succeed to unregister function(%s)\n", tmp_library_name);

  // Free allocated memory and close shared library
  free(target_library->library_name);
  target_library->library_name = NULL;
  dlclose(target_library->handle);
  target_library->handle = NULL;
  error_msg = dlerror();
  if (error_msg)
    llf_warn(LIBFPGA_FATAL_ERROR, "Failed to close shared library(%s)(error message:%s)\n",
      tmp_library_name, error_msg);

  return 0;
}
