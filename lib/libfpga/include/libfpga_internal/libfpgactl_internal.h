/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfpgactl_internal.h
 * @brief Header file for internal definition or function of libfpgactl
 */

#ifndef LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGACTL_INTERNAL_H_
#define LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGACTL_INTERNAL_H_

#include <xpcie_device.h>

#ifdef __cplusplus
extern "C" {
#endif


// FPGA Card Name got by CMS space
#define CARD_NAME_ALVEO_U250          "ALVEO U250 PQ" /**< Alveo U250 */
#define CARD_NAME_ALVEO_U250_ACT      "AU250A64G"     /**< Alveo U250 */
#define CARD_NAME_ALVEO_U280          "ALVEO U280 PQ" /**< Alveo U280 */


/**
 * @enum FPGA_AVAILABLE_CARD_ID
 * @brief FPGA's card id definitioin for converting from card name
 */
enum FPGA_AVAILABLE_CARD_ID {
  FPGA_CARD_U250 = 0, /**< Alveo U250 */
  FPGA_CARD_U280,     /**< Alveo U280 */
};


/**
 * @struct fpga_card_table_elem
 * @brief Table defintion for corresponding FPGA's card id with card name
 * @var fpga_card_table_elem::card_id
 *      FPGA's card id defined by this header file.
 * @var fpga_card_table_elem::card_name
 *      FPGA's card name got by FPGA's CMS space
 */
struct fpga_card_table_elem{
  enum FPGA_AVAILABLE_CARD_ID card_id;
  char *card_name;
};


/**
 * @brief Function which get card_id
 * @param[in] dev_id : FPGA's device id got by fpga_dev_init()
 * @retval (>=0) : card_id defined by FPGA_AVAILABLE_CARD_ID.
 * @retval -INVALID_ARGUMENT : dev_id is invalid value
 * @retval -INVALID_PARAMETER : No mathing data is in fpga_available_card_table
 */
int __fpga_get_device_card_id(
        uint32_t dev_id);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBFPGACTL_INTERNAL_H_
