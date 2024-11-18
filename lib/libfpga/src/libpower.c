/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libpower.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>
#include <libfpga_internal/libfpgactl_internal.h>
#include <libfpga_internal/libfpga_json.h>

#include <string.h>
#include <errno.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBPOWER

/**
 * Get the num of array elements
 */
#define GET_ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))


/**
 * @struct fpga_power_table_t
 * @brief Struct for ioctl's register flag and parameter name
 * @var fpga_power_table_t::flag
 *      flag for ioctl
 * @var fpga_power_table_t::name
 *      JSON's parameter name
 */
typedef struct fpga_power_table {
  uint32_t flag;
  char name[32];
} fpga_power_table_t;


/**
 * static global variable: Alveo U250 table
 */
static const fpga_power_table_t fpga_power_table_u250[] = {
  /* { flag,name } */
  { U250_PCIE_12V_VOLTAGE, PCIE_12V_VOLTAGE_NAME},
  { U250_PCIE_12V_CURRENT, PCIE_12V_CURRENT_NAME},
  { U250_AUX_12V_VOLTAGE,  AUX_12V_VOLTAGE_NAME},
  { U250_AUX_12V_CURRENT,  AUX_12V_CURRENT_NAME},
  { U250_PEX_3V3_VOLTAGE,  PEX_3V3_VOLTAGE_NAME},
  { U250_PEX_3V3_CURRENT,  PEX_3V3_CURRENT_NAME},
  { U250_PEX_3V3_POWER,    PEX_3V3_POWER_NAME},
  { U250_AUX_3V3_VOLTAGE,  AUX_3V3_VOLTAGE_NAME},
  { U250_AUX_3V3_CURRENT,  AUX_3V3_CURRENT_NAME},
  { U250_VCCINT_VOLTAGE,   VCCINT_VOLTAGE_NAME},
  { U250_VCCINT_CURRENT,   VCCINT_CURRENT_NAME}
};


int fpga_get_power(
  uint32_t dev_id,
  fpga_power_info_t * power_info
) {
  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !power_info) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), power_info(%#lx))\n", __func__, dev_id, (uintptr_t)power_info);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), power_info(%#lx))\n", __func__, dev_id, (uintptr_t)power_info);

  // Get power information
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CMS_GET_POWER_U250, power_info)) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CMS_GET_POWER_U250(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


int fpga_get_power_by_json(
  uint32_t dev_id,
  char **power_info
) {
  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !power_info) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), power_info(%#lx))\n", __func__, dev_id, (uintptr_t)power_info);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), power_info(%#lx))\n", __func__, dev_id, (uintptr_t)power_info);

  // Get card_id with card_name
  int card_id;
  card_id = __fpga_get_device_card_id(dev_id);
  if (card_id < 0) {
    llf_err(-card_id, "%s(device_type(%s) is not supported.)\n", __func__, dev->info.card_name);
    return card_id;
  }

  // Get power_information with card_id
  fpga_ioctl_power_t ioctl_power;
  switch (card_id) {
  case FPGA_CARD_U250:
    {
      json_param_u32_t json_params[GET_ARRAY_SIZE(fpga_power_table_u250)+1];  // data + sentinel
      memset(json_params, 0, sizeof(json_params));  // set sentinel as 0

      for (int i = 0; i < GET_ARRAY_SIZE(fpga_power_table_u250); i++) {
        ioctl_power.flag = fpga_power_table_u250[i].flag;
        // Get power information from driver
        if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CMS_GET_POWER, &ioctl_power)) {
          int err = errno;
          llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CMS_GET_POWER(errno:%d)\n", err);
          return -FAILURE_IOCTL;
        }
        json_params[i].val = ioctl_power.power;
        json_params[i].str = (char*)fpga_power_table_u250[i].name;  // NOLINT
      }

      // create json_txt
      *power_info = __fpga_json_malloc_string_u32(json_params);
    }
    break;

  case FPGA_CARD_U280:
  default:
    llf_err(NO_DEVICES, "%s(device_type(%s) is not supported.)\n", __func__, dev->info.card_name);
    return -NO_DEVICES;
    break;
  }

  if (!(*power_info)) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to get json string for FPGA power information.\n");
    return -FAILURE_MEMORY_ALLOC;
  }

  return 0;
}


int fpga_set_cms_unrest(
  uint32_t dev_id
) {
  // Check input
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  // reset command(1)
  uint32_t data = 0x1;

  // reset CMS
  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_CMS_SET_RESET, &data)) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_CMS_SET_RESET(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}
