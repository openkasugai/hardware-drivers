/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libtemp.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>
#include <libfpga_internal/libfpgactl_internal.h>
#include <libfpga_internal/libfpga_json.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBTEMP

/**
 * Get the num of array elements
 */
#define GET_ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))


/**
 * @struct fpga_temp_table_t
 * @brief Struct for ioctl's register flag and parameter name
 * @var fpga_temp_table_t::flag
 *      flag for ioctl
 * @var fpga_temp_table_t::name
 *      JSON's parameter name
 */
typedef struct fpga_temp_table {
  uint32_t flag;
  char name[32];
} fpga_temp_table_t;


/**
 * static global variable: Alveo U250 table
 */
static const fpga_temp_table_t fpga_temp_table_u250[] = {
  /* { flag,name } */
  { U250_CAGE_TEMP0,   ALVEO_U250_CAGE_TEMP0_NAME},
  { U250_CAGE_TEMP1,   ALVEO_U250_CAGE_TEMP1_NAME},
  { U250_DIMM_TEMP0,   ALVEO_U250_DIMM_TEMP0_NAME},
  { U250_DIMM_TEMP1,   ALVEO_U250_DIMM_TEMP1_NAME},
  { U250_DIMM_TEMP2,   ALVEO_U250_DIMM_TEMP2_NAME},
  { U250_DIMM_TEMP3,   ALVEO_U250_DIMM_TEMP3_NAME},
  { U250_FAN_TEMP,     ALVEO_U250_FAN_TEMP_NAME},
  { U250_FPGA_TEMP,    ALVEO_U250_FPGA_TEMP_NAME},
  { U250_SE98_TEMP0,   ALVEO_U250_SE98_TEMP0_NAME},
  { U250_SE98_TEMP1,   ALVEO_U250_SE98_TEMP1_NAME},
  { U250_SE98_TEMP2,   ALVEO_U250_SE98_TEMP2_NAME},
  { U250_VCCINT_TEMP,  ALVEO_U250_VCCINT_TEMP_NAME}
};


int fpga_get_temp(
  uint32_t dev_id,
  char **temp_info
) {
  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !temp_info) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), temp_info(%#lx))\n", __func__, dev_id, (uintptr_t)temp_info);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), temp_info(%#lx))\n", __func__, dev_id, (uintptr_t)temp_info);

  // Get card_id with card_name
  int card_id;
  card_id = __fpga_get_device_card_id(dev_id);
  if (card_id < 0) {
    llf_err(-card_id, "%s(device_kind(%s) is not supported.)\n", __func__, dev->info.card_name);
    return card_id;
  }

  // Get temperature information with card_id
  fpga_ioctl_temp_t ioctl_temp;
  switch (card_id) {
  case FPGA_CARD_U250:
    {
      json_param_u32_t json_params[GET_ARRAY_SIZE(fpga_temp_table_u250)+1];     // data + sentinel
      memset(json_params, 0, sizeof(json_params));  // set sentinel as 0

      for (int i = 0; i < GET_ARRAY_SIZE(fpga_temp_table_u250); i++) {
        ioctl_temp.flag = fpga_temp_table_u250[i].flag;
        // Get temperature information from driver
        if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CMS_GET_TEMP, &ioctl_temp)) {
          int err = errno;
          llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CMS_GET_TEMP(errno:%d)\n", err);
          return -FAILURE_IOCTL;
        }
        json_params[i].val = ioctl_temp.temp;
        json_params[i].str = (char*)fpga_temp_table_u250[i].name;  // NOLINT
      }

      // create json_txt
      *temp_info = __fpga_json_malloc_string_u32(json_params);
    }
    break;

  case FPGA_CARD_U280:
  default:
    llf_err(NO_DEVICES, "%s(device_type(%s) is not supported.)\n", __func__, dev->info.card_name);
    return -NO_DEVICES;
    break;
  }

  if (!(*temp_info)) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to get json string for FPGA temperature information.\n");
    return -FAILURE_MEMORY_ALLOC;
  }

  return 0;
}
