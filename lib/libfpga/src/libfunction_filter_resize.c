/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfunction_filter_resize.h>
#include <libfunction_conv.h>
#include <liblogging.h>

#include <libfpga_internal/libfpga_json.h>
#include <libfpga_internal/libfpgautil.h>
#include <libfpga_internal/libfunction_regmap.h>
#include <libfpga_internal/libfpgactl_internal.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBFUNCTION


static int fpga_filter_resize_init(
        uint32_t dev_id,
        uint32_t lane,
        const char *json_txt);

static int fpga_filter_resize_set(
        uint32_t dev_id,
        uint32_t lane,
        const char *json_txt);

static int fpga_filter_resize_get_setting(
        uint32_t dev_id,
        uint32_t lane,
        char **json_txt);

static int fpga_filter_resize_finish(
        uint32_t dev_id,
        uint32_t lane,
        const char *json_txt);


/**
 * static global variable: libfunction_filter_resize's operations
 */
static const fpga_function_ops_t libfunc_filter_resize_ops = {
  .name   = "filter_resize",
  .init   = fpga_filter_resize_init,
  .set    = fpga_filter_resize_set,
  .get    = fpga_filter_resize_get_setting,
  .finish = fpga_filter_resize_finish
};


/**
 * @brief Function which initialize information for filter_resize
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param json_txt
 *   Don't Care
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   `dev_id` is invalid value, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   e.g.) Maybe xpcie_device.h used in driver and lib differ
 * @retval -NOT_INITIALIZED
 *   e.g.) FPGA's bitstream may be broken
 *
 * @details
 *   Update FPGA's inforamtion and Check whether FPGA's Function module_id matches.@n
 *   When FPGA's type is modulized FPGA, Initialize for conv adapter too.
 */
static int fpga_filter_resize_init(
  uint32_t dev_id,
  uint32_t lane,
  const char *json_txt
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || lane >= KERNEL_NUM_FUNC(dev)) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), lane(%u), json(%s))\n",
    __func__, dev_id, lane, json_txt ? json_txt : "<null>");

  // Check FPGA's function
  uint32_t module_id = 0;
  int ret = fpga_filter_resize_get_module_id(dev_id, lane, &module_id);
  if (ret) {
    llf_err(-ret, " Failed to get Function module_id...");
    // return ret;
  } else {
    if (module_id != XPCIE_FPGA_FRFUNC_MODULE_ID_VALUE) {
      // TBD: Check whether FPGA's function is valid and return error
      // llf_err(INVALID_DATA, " Function Matching NG(%08x)[dev_id(%u), name(%s)] should be %08x\n",
      llf_warn(INVALID_DATA, " Failed to Match Function(%08x)[dev_id(%u), name(%s)] should be %08x\n",
        module_id, dev_id, dev->name, XPCIE_FPGA_FRFUNC_MODULE_ID_VALUE);
      // return -INVALID_DATA;
    } else {
      llf_dbg(" Succeed to Match Function(%08x)[dev_id(%u), name(%s)]\n", module_id, dev_id, dev->name);
    }
  }

  // Initialize Conv Adapter too
  fpga_conv_init(dev_id, lane, json_txt);

  return ret;
}


/**
 * @brief Function which set value into register as filter_resize of modulized fpga
 */
static int __fpga_filter_resize_set(
  fpga_device_t *dev,
  uint32_t lane,
  uint32_t i_width,
  uint32_t i_height,
  uint32_t o_width,
  uint32_t o_height,
  uint32_t module
) {
  int len = sizeof(uint32_t);
  uint32_t value;

  // [MODULE]Stop function kernel module before set frame size of function kernel
  // When all parameters for Function kernel are not ALL F, skip this parameter
  if (i_width != -1 && i_height != -1 && o_width != -1 && o_height != -1) {
    value = XPCIE_FPGA_STOP_MODULE;
    if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_KRNL_OFFSET(lane)) != len) goto failed;
  }

  // [FRAME]Set output height frame size
  // When i_width is ALL F, skip this parameter
  if (~i_width) {
    llf_dbg("  parameter(%s) : %u\n", "i_width", i_width);
    if (pwrite(dev->fd, &i_width, len, XPCIE_FPGA_FRFUNC_COLS_INPUT(lane)) != len) goto failed;
  }

  // [FRAME]Set output height frame size
  // When i_height is ALL F, skip this parameter
  if (~i_height) {
    llf_dbg("  parameter(%s): %u\n", "i_height", i_height);
    if (pwrite(dev->fd, &i_height, len, XPCIE_FPGA_FRFUNC_ROWS_INPUT(lane)) != len) goto failed;
  }

  // [FRAME]Set output height frame size
  // When o_width is ALL F, skip this parameter
  if (~o_width) {
    llf_dbg("  parameter(%s) : %u\n", "o_width", o_width);
    if (pwrite(dev->fd, &o_width, len, XPCIE_FPGA_FRFUNC_COLS_OUTPUT(lane)) != len) goto failed;
  }

  // [FRAME]Set output height frame size
  // When o_height is ALL F, skip this parameter
  if (~o_height) {
    llf_dbg("  parameter(%s): %u\n", "o_height", o_height);
    if (pwrite(dev->fd, &o_height, len, XPCIE_FPGA_FRFUNC_ROWS_OUTPUT(lane)) != len) goto failed;
  }

  // Check Later
  /*
  // [PAYLOAD]Set payload type into framework sub kernel
  // When payload is ALL F, skip this parameter
  if (~payload) {
    llf_dbg("  parameter(%s) : %u\n", "payload", payload);
    value = XPCIE_FPGA_PAYLOAD_TYPE_IMAGE;
    if (pwrite(dev->fd, &value, len, XPCIE_FPGA_SUB_PAYLOAD_TYPE(lane)) != len) goto failed;
  }
  */

  // [MODULE]Start kernel module
  // When module is ALL F, skip this parameter
  if (~module) {
    llf_dbg("  parameter(%s)  : %u\n", "module", module);
    if (module == 1)
      value = XPCIE_FPGA_START_MODULE;
    else
      value = XPCIE_FPGA_STOP_MODULE;
    if (pwrite(dev->fd, &value, len, XPCIE_FPGA_FRFUNC_KRNL_OFFSET(lane)) != len) goto failed;
  }

  return 0;

failed:
  llf_err(FAILURE_WRITE, "%s(Failed to set parameter.)\n", __func__);
  if (errno == EBUSY)
    llf_err(FAILURE_WRITE, "%s(Invalid operation: Maybe FPGA registers are locked yet.)\n", __func__);
  return -FAILURE_WRITE;
}


/**
 * @brief Function which setup as filter_resize of modulized fpga
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] json_txt
 *   Parameters for filter_resize module
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large, `json_txt` is null
 * @retval -FAILURE_WRITE
 *   e.g.) write is locked, See also fpga_enable_regrw()
 * @retval fpga_conv_set() returns
 *
 * @details
 *  Parse `json_txt` and set parameters, the parameters are as follows:
 *   @li i_width frame size of input width as dec
 *   @li i_height frame size of input height as dec
 *   @li o_width frame size of output width as dec
 *   @li o_height frame size of output height as dec
 *  The process of setting is as follows:
 *   @li When any frame size is not set in json_txt, return error.
 *   @li When all frame sizes are 0, stop All kernels and return.
 *   @li Stop Function module.
 *   @li Set frame size.
 *   @li Start Function module.
 *   @li Call conv adapter's set()
 *
 * @sa LIBFUNCTION_FILTER_RESIZE_PARAMS_JSON_FMT
 *
 */
static int fpga_filter_resize_set(
  uint32_t dev_id,
  uint32_t lane,
  const char *json_txt
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || lane >= KERNEL_NUM_FUNC(dev) || !json_txt) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -INVALID_ARGUMENT;
  }

  // Check input
  uint32_t i_width, i_height, o_width, o_height, module;
  i_width   = __fpga_get_parameter(json_txt, "i_width");
  i_height  = __fpga_get_parameter(json_txt, "i_height");
  o_width   = __fpga_get_parameter(json_txt, "o_width");
  o_height  = __fpga_get_parameter(json_txt, "o_height");

  llf_dbg("%s(dev_id(%u), lane(%u), i_width(%u), i_height(%u), o_width(%u), o_height(%u))\n",
    __func__, dev_id, lane, i_width, i_height, o_width, o_height);

  if (i_width == -1 || i_height == -1 || o_width == -1 || o_height == -1) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -INVALID_ARGUMENT;
  }
  if (!(i_width | i_height | o_width | o_height)) {
    // When all the frame sizes are 0, stop kernel
    module = 0;
    llf_err(FAILURE_WRITE, "%s(Module Stop.)\n", __func__);
  } else {
    module = 1;
  }

  int ret_fr = __fpga_filter_resize_set(
    dev,
    lane,
    i_width,
    i_height,
    o_width,
    o_height,
    module);

  // Conv Adapter set
  int ret_cv = fpga_conv_set(dev_id, lane, json_txt);

  if (ret_fr != 0) {
    return ret_fr;
  } else if (ret_cv != 0) {
    return ret_cv;
  } else {
    return 0;
  }
}


/**
 * @brief Function which get setup setup data for filter_resize
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] json_txt
 *   pointer variable to get filter_resize parameter's value as json format
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large, `json_txt` is null
 * @retval -FAILURE_READ
 *   e.g.) read is locked, See also fpga_enable_regrw()
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory.
 * @retval fpga_conv_finish() returns
 *
 * @details
 *   Read the register which is set by fpga_function_set()
 *    and create json format text and return it.
 *   When the data is numerical value, Hex's expression is ambiguous so use Dec.
 *    @li i_width frame size of input width as dec
 *    @li i_height frame size of input height as dec
 *    @li o_width frame size of output width as dec
 *    @li o_height frame size of output height as dec
 *   `json_txt` will return with memory allocated by this library,
 *     and the memory will be freed by fpga_function_finish().
 *  if there are any special notes around conv
 *
 */
static int fpga_filter_resize_get_setting(
  uint32_t dev_id,
  uint32_t lane,
  char **json_txt
) {
  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || lane >= KERNEL_NUM_FUNC(dev) || !json_txt) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%#x))\n",
      __func__, dev_id, lane, json_txt);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), lane(%u), json(%#x))\n",
    __func__, dev_id, lane, json_txt);

  // Read register and Get parameter
  uint32_t i_width, i_height, o_width, o_height;
  size_t len = sizeof(uint32_t);
  if (pread(dev->fd, &i_width, len, XPCIE_FPGA_FRFUNC_COLS_INPUT(lane)) != len) goto failed;
  if (pread(dev->fd, &i_height, len, XPCIE_FPGA_FRFUNC_ROWS_INPUT(lane)) != len) goto failed;
  if (pread(dev->fd, &o_width, len, XPCIE_FPGA_FRFUNC_COLS_OUTPUT(lane)) != len) goto failed;
  if (pread(dev->fd, &o_height, len, XPCIE_FPGA_FRFUNC_ROWS_OUTPUT(lane)) != len) goto failed;

  // Create json text
  char tmp_json[256];
  memset(tmp_json, 0, sizeof(tmp_json));
  snprintf(tmp_json, sizeof(tmp_json), LIBFUNCTION_FILTER_RESIZE_PARAMS_JSON_FMT, i_width, i_height, o_width, o_height);

  char *conv_json;
  char func_json[512];
  // Conv Adapter get setting
  fpga_conv_get_setting(dev_id, lane, &conv_json);
  snprintf(func_json, sizeof(func_json), "{\"fr\":%s, \"conv\":%s}", tmp_json, conv_json);
  // Conv tmp_json[256] free
  free(conv_json);

  *json_txt = strdup(func_json);
  if (!(*json_txt)) {
    llf_err(FAILURE_MEMORY_ALLOC, "%s(Failed to allocate memory for json string.)\n", __func__);
    return -FAILURE_MEMORY_ALLOC;
  }

  llf_dbg("  json_txt : %s\n", *json_txt);

  return 0;

failed:
  llf_err(FAILURE_READ, "%s(Failed to set parameter.)\n", __func__);
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "%s(Invalid operation: Maybe FPGA registers are locked yet.)\n", __func__);
  return -FAILURE_READ;
}


/**
 * @brief Function which finalize function for filter_resize
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param json_txt
 *   Don't Care
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   e.g.) write is locked, See also fpga_enable_regrw()
 * @retval fpga_conv_finish() returns
 *
 * @details
 *  Set parameters, the parameters are as follows:
 *   @li Stop Function kernel.
 *   @li Set frame size as 0(initial value).
 *   @li Stop Function kernel.
 *   @li Call conv adapter's finish()
 *  if there are any special notes around conv
 *
 */
static int fpga_filter_resize_finish(
  uint32_t dev_id,
  uint32_t lane,
  const char *json_txt
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || lane >= KERNEL_NUM_FUNC(dev)) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), json(%s))\n",
    __func__, dev_id, lane, json_txt ? json_txt : "<null>");

  int ret_fr = __fpga_filter_resize_set(
    dev,
    lane,
    0,          // i_width
    0,          // i_height
    0,          // o_width
    0,          // o_height
    0);         // module(stop)

  // Conv Adapter finish
  int ret_cv = fpga_conv_finish(dev_id, lane, json_txt);

  if (ret_fr != 0) {
    return ret_fr;
  } else if (ret_cv != 0) {
    return ret_cv;
  } else {
    return 0;
  }
}


int fpga_function_register_filter_resize(void) {
  return fpga_function_register(&libfunc_filter_resize_ops);
}


/**
 * @brief Function which get control for filter_resize module
 */
int fpga_filter_resize_get_control(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *control
) {
  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !control || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), control(%#lx))\n",
      __func__, dev_id, lane, (uintptr_t)control);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), control(%#lx))\n",
    __func__, dev_id, lane, (uintptr_t)control);

  size_t len = sizeof(uint32_t);
  if (pread(dev->fd, control, len, XPCIE_FPGA_FRFUNC_CONTROL(lane)) != len) goto failed;

  return 0;

failed:
  llf_err(FAILURE_READ, "%s(Failed to set parameter.)\n", __func__);
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "%s(Invalid operation: Maybe FPGA registers are locked yet.)\n", __func__);
  return -FAILURE_READ;
}


/**
 * @brief Function which get module_id for filter_resize module
 */
int fpga_filter_resize_get_module_id(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *module_id
) {
  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !module_id || (lane >= KERNEL_NUM_FUNC(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), module_id(%#lx))\n", __func__, dev_id, lane, (uintptr_t)module_id);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), module_id(%#lx))\n", __func__, dev_id, lane, (uintptr_t)module_id);

  size_t len = sizeof(uint32_t);
  if (pread(dev->fd, module_id, len, XPCIE_FPGA_FRFUNC_MODULE_ID(lane)) != len) goto failed;

  return 0;

failed:
  llf_err(FAILURE_READ, "%s(Failed to set parameter.)\n", __func__);
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "%s(Invalid operation: Maybe FPGA registers are locked yet.)\n", __func__);
  return -FAILURE_READ;
}
