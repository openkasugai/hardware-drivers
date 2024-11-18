/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfpgactl_err.h
 * @brief Header file for getting and setting fpga error
 */

#ifndef LIBFPGA_INCLUDE_LIBFPGACTL_ERR_H_
#define LIBFPGA_INCLUDE_LIBFPGACTL_ERR_H_

#include <stdlib.h>

#include <xpcie_device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct fpga_clkdwn_t
 * @brief Struct for FPGA CLOCK down infomation structure
 * @var fpga_clkdwn_t::user_clk
 *      user clock down
 * @var fpga_clkdwn_t::ddr4_clk0
 *      ddr4#0 clock down
 * @var fpga_clkdwn_t::ddr4_clk1
 *      ddr4#1 clock down
 * @var fpga_clkdwn_t::ddr4_clk2
 *      ddr4#2 clock down
 * @var fpga_clkdwn_t::ddr4_clk3
 *      ddr4#3 clock down
 * @var fpga_clkdwn_t::qsfp_clk0
 *      qsfp clock0 down
 * @var fpga_clkdwn_t::qsfp_clk1
 *      qsfp clock1 down
 */
typedef struct fpga_clkdwn {
  uint8_t user_clk;
  uint8_t ddr4_clk0;
  uint8_t ddr4_clk1;
  uint8_t ddr4_clk2;
  uint8_t ddr4_clk3;
  uint8_t qsfp_clk0;
  uint8_t qsfp_clk1;
} fpga_clkdwn_t;

/**
 * @struct fpga_eccerr_t
 * @brief Struct for DDR ECC error info structure
 * @var fpga_eccerr_t::ddr4_single0
 *      ddr4#0 single ecc error
 * @var fpga_eccerr_t::ddr4_single1
 *      ddr4#1 single ecc error
 * @var fpga_eccerr_t::ddr4_single2
 *      ddr4#2 single ecc error
 * @var fpga_eccerr_t::ddr4_single3
 *      ddr4#3 single ecc error
 * @var fpga_eccerr_t::ddr4_multi0
 *      ddr4#0 multi ecc error
 * @var fpga_eccerr_t::ddr4_multi1
 *      ddr4#1 multi ecc error
 * @var fpga_eccerr_t::ddr4_multi2
 *      ddr4#2 multi ecc error
 * @var fpga_eccerr_t::ddr4_multi3
 *      ddr4#3 multi ecc error
 */
typedef struct fpga_eccerr {
  uint8_t ddr4_single0;
  uint8_t ddr4_single1;
  uint8_t ddr4_single2;
  uint8_t ddr4_single3;
  uint8_t ddr4_multi0;
  uint8_t ddr4_multi1;
  uint8_t ddr4_multi2;
  uint8_t ddr4_multi3;
} fpga_eccerr_t;


/**
 * @brief API which get FPGA failure detection aggregate result
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] check_err
 *   fpga error detection
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   get aggregated failure detection results for each block and FPGA
 */
int fpga_dev_get_check_err(
        uint32_t dev_id,
        uint32_t *check_err);

/**
 * @brief API which get notification of clock down detection
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] clk_dwn
 *   Acquisition value of notification of clock down detection
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get notification of clock down detection
 */
int fpga_dev_get_clk_dwn(
        uint32_t dev_id,
        fpga_clkdwn_t *clk_dwn);

/**
 * @brief API which clear clock down notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] clk_dwn
 *   Settings to clear notifications
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Clear the clock down detection notification
 */
int fpga_dev_set_clk_dwn_clear(
        uint32_t dev_id,
        fpga_clkdwn_t clk_dwn);

/**
 * @brief API which get notification of clock break (raw value)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] clk_dwn
 *   Acquisition value of notification of clock down detection (raw value)
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get notification of clock down detection (raw value)
 */
int fpga_dev_get_clk_dwn_raw(
        uint32_t dev_id,
        fpga_clkdwn_t *clk_dwn);

/**
 * @brief API which mask Clock down notification mask
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] clk_dwn
 *   Clock down detection notification mask setting value
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Mask for the clock down detection notification.
 */
int fpga_dev_set_clk_dwn_mask(
        uint32_t dev_id,
        fpga_clkdwn_t clk_dwn);

/**
 * @brief API which get clock break notification mask setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] clk_dwn
 *   Clock down detection notification mask setting value
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get mask setting for clock down detection notification
 */
int fpga_dev_get_clk_dwn_mask(
        uint32_t dev_id,
        fpga_clkdwn_t *clk_dwn);

/**
 * @brief API which force- Clockbreak notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] clk_dwn
 *   Forcibly generating notification of clock down detection
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Force notification of clock down detection
 */
int fpga_dev_set_clk_dwn_force(
        uint32_t dev_id,
        fpga_clkdwn_t clk_dwn);

/**
 * @brief API which get setting to force notification of clock down
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] clk_dwn
 *   Forcibly generating notification of clock down detection
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get the setting of forced generation of clock down detection notification
 */
int fpga_dev_get_clk_dwn_force(
        uint32_t dev_id,
        fpga_clkdwn_t *clk_dwn);

/**
 * @brief API which get DDR ECC(single, multi) error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] eccerr
 *   value to get ECC error notification of DDR
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get ECC(single, multi) error notification for DDR
 */
int fpga_dev_get_ecc_err(
        uint32_t dev_id,
        fpga_eccerr_t *eccerr);

/**
 * @brief API which clear DDR ECC(single, multi) error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] eccerr
 *   setting to clear ECC error notification for DDR
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Clear ECC(single, multi) Error Notifications for DDR
 */
int fpga_dev_set_ecc_err_clear(
        uint32_t dev_id,
        fpga_eccerr_t eccerr);

/**
 * @brief API which get DDR ECC(single, multi) error notification (raw)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] eccerr
 *   eccerr: Value obtained from ECC error notification (raw value) of DDR
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get DDR ECC(single, multi) error notification (raw value)
 */
int fpga_dev_get_ecc_err_raw(
        uint32_t dev_id,
        fpga_eccerr_t *eccerr);

/**
 * @brief API which mask ECC(single, multi) error notification for DDR
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] eccerr
 *   DDR ECC error notification mask setting
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Set the mask for DDR ECC(single, multi) error notifications
 */
int fpga_dev_set_ecc_err_mask(
        uint32_t dev_id,
        fpga_eccerr_t eccerr);

/**
 * @brief API which get DDR ECC(single, multi) error notification mask setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] eccerr
 *   DDR ECC error notification mask setting
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get mask setting for ECC(single, multi) error notification of DDR
 */
int fpga_dev_get_ecc_err_mask(
        uint32_t dev_id,
        fpga_eccerr_t *eccerr);

/**
 * @brief API which force DDR ECC(single, multi) error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] eccerr
 *   Force generation of ECC error notification for DDR
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Force ECC(single, multi) error notification for DDR
 */
int fpga_dev_set_ecc_err_force(
        uint32_t dev_id,
        fpga_eccerr_t eccerr);

/**
 * @brief API which get DDR ECC(single, multi) error notification force setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[out] eccerr
 *   Force generation of ECC error notification for DDR
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get force setting of ECC(single, multi) error notification of DDR.
 */
int fpga_dev_get_ecc_err_force(
        uint32_t dev_id,
        fpga_eccerr_t *eccerr);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFPGACTL_ERR_H_
