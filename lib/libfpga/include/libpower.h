/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libpower.h
 * @brief Header file for getting FPGA's power consumption
 */

#ifndef LIBFPGA_INCLUDE_LIBPOWER_H_
#define LIBFPGA_INCLUDE_LIBPOWER_H_

#include <libfpgactl.h>

#ifdef __cplusplus
extern "C" {
#endif


#define PCIE_12V_VOLTAGE_NAME   "PCIE_12V_VOLTAGE"  /**< used for fpga_get_power_by_json()'s JSON parameter name */
#define PCIE_12V_CURRENT_NAME   "PCIE_12V_CURRENT"  /**< used for fpga_get_power_by_json()'s JSON parameter name */
#define AUX_12V_VOLTAGE_NAME    "AUX_12V_VOLTAGE"   /**< used for fpga_get_power_by_json()'s JSON parameter name */
#define AUX_12V_CURRENT_NAME    "AUX_12V_CURRENT"   /**< used for fpga_get_power_by_json()'s JSON parameter name */
#define PEX_3V3_VOLTAGE_NAME    "PEX_3V3_VOLTAGE"   /**< used for fpga_get_power_by_json()'s JSON parameter name */
#define PEX_3V3_CURRENT_NAME    "PEX_3V3_CURRENT"   /**< used for fpga_get_power_by_json()'s JSON parameter name */
#define PEX_3V3_POWER_NAME      "PEX_3V3_POWER"     /**< used for fpga_get_power_by_json()'s JSON parameter name */
#define AUX_3V3_VOLTAGE_NAME    "AUX_3V3_VOLTAGE"   /**< used for fpga_get_power_by_json()'s JSON parameter name */
#define AUX_3V3_CURRENT_NAME    "AUX_3V3_CURRENT"   /**< used for fpga_get_power_by_json()'s JSON parameter name */
#define VCCINT_VOLTAGE_NAME     "VCCINT_VOLTAGE"    /**< used for fpga_get_power_by_json()'s JSON parameter name */
#define VCCINT_CURRENT_NAME     "VCCINT_CURRENT"    /**< used for fpga_get_power_by_json()'s JSON parameter name */


/**
 * @struct fpga_power_info_t
 * @brief power information struct for alveo u250
 * @var fpga_power_info_t::pcie_12V_voltage
 *      PCIe 12V voltage
 * @var fpga_power_info_t::pcie_12V_current
 *      PCIe 12V current
 * @var fpga_power_info_t::aux_12V_voltage
 *      AUX 12V voltage
 * @var fpga_power_info_t::aux_12V_current
 *      AUX 12V current
 * @var fpga_power_info_t::pex_3V3_voltage
 *      PEX 3.3V voltage
 * @var fpga_power_info_t::pex_3V3_current
 *      PEX 3.3V current
 * @var fpga_power_info_t::pex_3V3_power
 *      PEX 3.3V power
 * @var fpga_power_info_t::aux_3V3_voltage
 *      AUX 3.3V voltage
 * @var fpga_power_info_t::aux_3V3_current
 *      AUX 3.3V current
 * @var fpga_power_info_t::vccint_voltage
 *      VccInt voltage
 * @var fpga_power_info_t::vccint_current
 *      VccInt current
 */
typedef struct fpga_power_info {
  uint32_t  pcie_12V_voltage;
  uint32_t  pcie_12V_current;
  uint32_t  aux_12V_voltage;
  uint32_t  aux_12V_current;
  uint32_t  pex_3V3_voltage;
  uint32_t  pex_3V3_current;
  uint32_t  pex_3V3_power;
  uint32_t  aux_3V3_voltage;
  uint32_t  aux_3V3_current;
  uint32_t  vccint_voltage;
  uint32_t  vccint_current;
} fpga_power_info_t;


/**
 * @brief API which get FPGA's power information by struct(uint32_t)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] power_info
 *   pointer variable to get power_info register values
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `power_info` is null
 * @retval -FAILURE_IOCTL
 *   e.g.) Maybe xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Get `power_info` from FPGA's register value and return it.@n
 *   `*power_info` will be not allocated by this API, so allocate by user.@n
 *   The value of `*power_info` is undefined when this API fails.@n
 *   In future, this API will be depricated,
 *    so please use fpga_get_power_by_json()
 * @remarks
 *   This API is only for Alveo U250.
 */
int fpga_get_power(
        uint32_t dev_id,
        fpga_power_info_t * power_info);

/**
 * @brief API which get FPGA's power information by json format string
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] power_json
 *   pointer variable to get power_info register values
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `power_info` is null
 * @retval -FAILURE_IOCTL
 *   e.g.) Maybe xpcie_device.h used in driver and lib differ
 * @retval -INVALID_PARAMETER
 *   FPGA's card name is not supported in libfpgactl
 * @retval -NO_DEVICES
 *   FPGA's card name is not supported in libpower
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 *
 * @details
 *   Get `power_json` from FPGA's register value and return it with shaping as json format.@n
 *   The json text will return with memory allocated by this library through `power_json`,
 *    so please explicitly free through free().@n
 *   The value of `*power_json` is undefined when this API fails.
 */
int fpga_get_power_by_json(
        uint32_t dev_id,
        char **power_json);

/**
 * @brief API which reset FPGA's CMS space
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @retval 0
 *   success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   e.g.) Maybe xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Reset CMS space through ioctl.
 */
int fpga_set_cms_unrest(
        uint32_t dev_id);

#ifdef __cplusplus
}
#endif

#endif  /* LIBFPGA_INCLUDE_LIBPOWER_H_ */
