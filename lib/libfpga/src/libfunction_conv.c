/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfunction_conv.h>
#include <liblogging.h>

#include <libfpga_internal/libfpga_json.h>
#include <libfpga_internal/libfpgautil.h>

/**
 * Register map for modulized fpga
 */
#include <libfpga_internal/libfunction_regmap.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBFUNCTION


int fpga_conv_init(
  uint32_t dev_id,
  uint32_t lane,
  const char *json_txt
) {
  // Do Nothing
  return 0;
}


/**
 * @brief Function which set value into register as conv adapter
 */
static int __fpga_conv_set(
  fpga_device_t *dev,
  uint32_t lane,
  uint32_t i_width,
  uint32_t i_height,
  uint32_t frame_buffer_l,
  uint32_t frame_buffer_h,
  uint32_t module
) {
  int len = sizeof(uint32_t);
  uint32_t value;

  // [MODULE]Stop function kernel module before set frame size of function kernel
  // When each of i_width or i_height are ALL F, skip this parameter
  if (i_width != -1 && i_height != -1) {
    value = XPCIE_FPGA_STOP_MODULE;
    if (pwrite(dev->fd, &value, len, XPCIE_FPGA_CONV_CONTROL(lane)) != len) goto failed;
  }

  // [FRAME]Set input height frame size
  // When i_width is ALL F, skip this parameter
  if (~i_width) {
    llf_dbg("  parameter(%s) : %u\n", "i_width", i_width);
    if (pwrite(dev->fd, &i_width, len, XPCIE_FPGA_CONV_COLS_INPUT(lane)) != len) goto failed;
  }

  // [FRAME]Set input height frame size
  // When i_height is ALL F, skip this parameter
  if (~i_height) {
    llf_dbg("  parameter(%s): %u\n", "i_height", i_height);
    if (pwrite(dev->fd, &i_height, len, XPCIE_FPGA_CONV_ROWS_INPUT(lane)) != len) goto failed;
  }

  // [ADDRESS]Set axi ingr frame buffer offset address(low)
  // When frame_buffer_l is ALL F, skip this parameter
  if (~frame_buffer_l) {
    llf_dbg("  parameter(%s) : %u\n", "frame_buffer_l", frame_buffer_l);
    if (pwrite(dev->fd, &frame_buffer_l, len, XPCIE_FPGA_CONV_AXI_INGR_FRAME_BUFFER_L(lane)) != len) goto failed;
  }

  // [ADDRESS]Set axi ingr frame buffer offset address(high)
  // When frame_buffer_h is ALL F, skip this parameter
  if (~frame_buffer_h) {
    llf_dbg("  parameter(%s): %u\n", "frame_buffer_h", frame_buffer_h);
    if (pwrite(dev->fd, &frame_buffer_h, len, XPCIE_FPGA_CONV_AXI_INGR_FRAME_BUFFER_H(lane)) != len) goto failed;
  }

  // [MODULE]Start kernel module
  // When module is ALL F, skip this parameter
  if (~module) {
    llf_dbg("  parameter(%s)  : %u\n", "module", module);
    value = 0;
    if (module == 1)
      value = XPCIE_FPGA_START_MODULE;
    else
      value = XPCIE_FPGA_STOP_MODULE;
    if (pwrite(dev->fd, &value, len, XPCIE_FPGA_CONV_CONTROL(lane)) != len) goto failed;
  }

  return 0;

failed:
  llf_err(FAILURE_WRITE, "%s(Failed to set parameter.)\n", __func__);
  if (errno == EBUSY)
    llf_err(FAILURE_WRITE, "%s(Invalid operation: Maybe FPGA registers are locked yet.)\n", __func__);
  return -FAILURE_WRITE;
}


int fpga_conv_set(
  uint32_t dev_id,
  uint32_t lane,
  const char *json_txt
) {
  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CONV(dev)) || !json_txt) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -INVALID_ARGUMENT;
  }

  uint32_t i_width, i_height, frame_buffer_l, frame_buffer_h, module;
  i_width        = __fpga_get_parameter(json_txt, "i_width");
  i_height       = __fpga_get_parameter(json_txt, "i_height");
  frame_buffer_l = XPCIE_FPGA_DDR_VALUE_AXI_INGR_FRAME_BUFFER_L;
  frame_buffer_h = XPCIE_FPGA_DDR_VALUE_AXI_INGR_FRAME_BUFFER_H(lane);

  llf_dbg("%s(dev_id(%u), lane(%u), i_width(%u), i_height(%u), frame_buffer_l(%u), frame_buffer_h(%u))\n",
    __func__, dev_id, lane, i_width, i_height, frame_buffer_l, frame_buffer_h);

  // When any of the frame sizes is 0xFFFFFFFF(= failed to get), input value is invalid
  if (i_width == -1 || i_height == -1) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -INVALID_ARGUMENT;
  }

  if (!(i_width | i_height)) {
    // When all the frame sizes are 0, stop kernel
    module = 0;
    llf_err(FAILURE_WRITE, "%s(Module Stop.)\n", __func__);
  } else {
    module = 1;
  }

  return __fpga_conv_set(
    dev,
    lane,
    i_width,
    i_height,
    frame_buffer_l,
    frame_buffer_h,
    module);
}


int fpga_conv_finish(
  uint32_t dev_id,
  uint32_t lane,
  const char *json_txt
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CONV(dev))) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%s))\n",
      __func__, dev_id, lane, json_txt ? json_txt : "<null>");
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), json(%s))\n",
    __func__, dev_id, lane, json_txt ? json_txt : "<null>");

  return __fpga_conv_set(
    dev,
    lane,
    0,          // i_width
    0,          // i_height
    0,          // frame_buffer_l
    0,          // frame_buffer_h
    0);         // module(stop)
}


int fpga_conv_get_setting(
  uint32_t dev_id,
  uint32_t lane,
  char **json_txt
) {
  // Read register and Get parameter
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || (lane >= KERNEL_NUM_CONV(dev)) || !json_txt) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), lane(%u), json(%#x))\n",
      __func__, dev_id, lane, json_txt);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), lane(%u), json(%#x))\n",
    __func__, dev_id, lane, json_txt);

  uint32_t i_width, i_height;
  uint32_t frame_buffer_l, frame_buffer_h;

  size_t len = sizeof(uint32_t);
  if (pread(dev->fd, &i_width, len, XPCIE_FPGA_CONV_COLS_INPUT(lane)) != len) goto failed;
  if (pread(dev->fd, &i_height, len, XPCIE_FPGA_CONV_ROWS_INPUT(lane)) != len) goto failed;
  if (pread(dev->fd, &frame_buffer_l, len, XPCIE_FPGA_CONV_AXI_INGR_FRAME_BUFFER_L(lane)) != len) goto failed;
  if (pread(dev->fd, &frame_buffer_h, len, XPCIE_FPGA_CONV_AXI_INGR_FRAME_BUFFER_H(lane)) != len) goto failed;

  // Create json text
  char tmp_json[256];
  memset(tmp_json, 0, sizeof(tmp_json));
  snprintf(tmp_json, sizeof(tmp_json), LIBFUNCTION_CONV_PARAMS_JSON_FMT, i_width, i_height, frame_buffer_l, frame_buffer_h);
  *json_txt = strdup(tmp_json);
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


int fpga_conv_get_control(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *control
) {
  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !control || (lane >= KERNEL_NUM_CONV(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), control(%#lx))\n", __func__, dev_id, lane, (uintptr_t)control);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), control(%#lx))\n", __func__, dev_id, lane, (uintptr_t)control);

  size_t len = sizeof(uint32_t);
  if (pread(dev->fd, control, len, XPCIE_FPGA_CONV_CONTROL(lane)) != len) goto failed;

  return 0;

failed:
  llf_err(FAILURE_READ, "%s(Failed to set parameter.)\n", __func__);
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "%s(Invalid operation: Maybe FPGA registers are locked yet.)\n", __func__);
  return -FAILURE_READ;
}


int fpga_conv_get_module_id(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t *module_id
) {
  llf_dbg("%s()\n", __func__);

  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !module_id || (lane >= KERNEL_NUM_CONV(dev))) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), lane(%u), module_id(%#lx))\n", __func__, dev_id, lane, (uintptr_t)module_id);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), lane(%u), module_id(%#lx))\n", __func__, dev_id, lane, (uintptr_t)module_id);

  size_t len = sizeof(uint32_t);
  if (pread(dev->fd, module_id, len, XPCIE_FPGA_CONV_MODULE_ID(lane)) != len) goto failed;

  return 0;

failed:
  llf_err(FAILURE_READ, "%s(Failed to set parameter.)\n", __func__);
  if (errno == EBUSY)
    llf_err(FAILURE_READ, "%s(Invalid operation: Maybe FPGA registers are locked yet.)\n", __func__);
  return -FAILURE_READ;
}
