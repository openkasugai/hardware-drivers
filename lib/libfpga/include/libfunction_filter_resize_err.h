/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfunction_filter_resize_err.h
 * @brief Header file for getting and setting Conversion Adapter error
 */

#ifndef LIBFPGA_INCLUDE_LIBFUNCTION_FILTER_RESIZE_ERR_H_
#define LIBFPGA_INCLUDE_LIBFUNCTION_FILTER_RESIZE_ERR_H_

#include <libfunction.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FRFUNC_DIR_INGRESS     0  /**< Filter Resize ingress*/
#define FRFUNC_DIR_EGRESS      1  /**< Filter Resize egress*/
#define FRFUNC_FUNC_NUMBER_0   0  /**< Filter Resize#0*/
#define FRFUNC_FUNC_NUMBER_1   1  /**< Filter Resize#1*/


/**
 * @struct fpga_fr_err_stif_t
 * @brief Struct for Filter Resize Stream Interface stall information structure
 * @var fpga_fr_err_stif_t::ingress0_rcv_req
 *      Filter Resize#0 ingress receive request
 * @var fpga_fr_err_stif_t::ingress0_rcv_resp
 *      Filter Resize#0 ingress receive response
 * @var fpga_fr_err_stif_t::ingress0_rcv_data
 *      Filter Resize#0 ingress receive data
 * @var fpga_fr_err_stif_t::ingress1_rcv_req
 *      Filter Resize#1 ingress receive request
 * @var fpga_fr_err_stif_t::ingress1_rcv_resp
 *      Filter Resize#1 ingress receive response
 * @var fpga_fr_err_stif_t::ingress1_rcv_data
 *      Filter Resize#1 ingress receive data
 * @var fpga_fr_err_stif_t::egress0_snd_req
 *      Filter Resize#0 egress send request
 * @var fpga_fr_err_stif_t::egress0_snd_resp
 *      Filter Resize#0 egress send response
 * @var fpga_fr_err_stif_t::egress0_snd_data
 *      Filter Resize#0 egress send data
 * @var fpga_fr_err_stif_t::egress1_snd_req
 *      Filter Resize#1 egress send request
 * @var fpga_fr_err_stif_t::egress1_snd_resp
 *      Filter Resize#1 egress send response
 * @var fpga_fr_err_stif_t::egress1_snd_data
 *      Filter Resize#1 egress send data
 */
typedef struct fpga_fr_err_stif {
  uint8_t ingress0_rcv_req;
  uint8_t ingress0_rcv_resp;
  uint8_t ingress0_rcv_data;
  uint8_t ingress1_rcv_req;
  uint8_t ingress1_rcv_resp;
  uint8_t ingress1_rcv_data;
  uint8_t egress0_snd_req;
  uint8_t egress0_snd_resp;
  uint8_t egress0_snd_data;
  uint8_t egress1_snd_req;
  uint8_t egress1_snd_resp;
  uint8_t egress1_snd_data;
} fpga_fr_err_stif_t;


/**
 * @brief API which get results of failure detection (excluding interface stalls) for a Filter Resize function
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] err_det
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   Get whether the Filter Resize function detects failures (excluding interface stalls)
 */
int fpga_filter_resize_get_check_err(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *err_det);

/**
 * @brief API which get Filter Resize Function Ingress Input/Egress Output Protocol error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   control function ID
 * @param[in] dir
 *   Ingress/Egress
 * @param[out] func_err_prot
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   Get Filter Resize function ingress input/egress output protocol error notification
 */
int fpga_filter_resize_get_err_prot(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

/**
 * @brief API which clear Filter Resize Function Ingress Input/Egress Output protocol error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   control function ID
 * @param[in] dir
 *   Ingress/Egress
 * @param[in] func_err_prot
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   pwrite() failure
 *
 * @details
 *   Clear the Filter Resize function's ingress input/egress output protocol error notification
 */
int fpga_filter_resize_set_err_prot_clear(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which mask Filter Resize Function Ingress Input/Egress Output Protocol error
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   control function ID
 * @param[in] dir
 *   Ingress/Egress
 * @param[in] func_err_prot
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   pwrite() failure
 *
 * @details
 *   Set mask for Filter Resize function ingress input/egress output protocol error notification
 */
int fpga_filter_resize_set_err_prot_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which get Filter Resize Function Ingress Input/Egress Output Protocol error mask setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   control function ID
 * @param[in] dir
 *   Ingress/Egress
 * @param[out] func_err_prot
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   Get mask setting for Filter Resize function ingress input/egress output protocol error notification
 */
int fpga_filter_resize_get_err_prot_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

/**
 * @brief API which force Filter Resize Function Ingress Input/Egress Output Protocol error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   control function ID
 * @param[in] dir
 *   Ingress/Egress
 * @param[in] func_err_prot
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   pwrite() failure
 *
 * @details
 *   Force Ingress input/egress output protocol error notification for Filter Resize function
 */
int fpga_filter_resize_set_err_prot_force(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which get Filter Resize Function Ingress Input/Egress Output Protocol error force setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   control function ID
 * @param[in] dir
 *   Ingress/Egress
 * @param[out] func_err_prot
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   Get force setting for ingress input/egress output protocol error notification of Filter Resize function
 */
int fpga_filter_resize_get_err_prot_force(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

/**
 * @brief API which get Filter Resize Function Stream Interface Stall notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] fr_err_stif
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   Get Filter Resize Function Stream Interface Get stall notification
 */
int fpga_filter_resize_get_err_stif(
        uint32_t dev_id,
        uint32_t lane,
        fpga_fr_err_stif_t *fr_err_stif);

/**
 * @brief API which mask Filter Resize Function Stream Interface Stall notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_err_stif
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   pwrite() failure
 *
 * @details
 *   Mask Filter Resize Function Stream Interface Stall notification
 */
int fpga_filter_resize_set_err_stif_mask(
        uint32_t dev_id,
        uint32_t lane,
        fpga_fr_err_stif_t fr_err_stif);

/**
 * @brief API which get Filter Resize Function Stream Interface Stall notification mask setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] fr_err_stif
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   get Filter Resize Function Stream Interface Stall notification mask Setting
 */
int fpga_filter_resize_get_err_stif_mask(
        uint32_t dev_id,
        uint32_t lane,
        fpga_fr_err_stif_t *fr_err_stif);

/**
 * @brief API which force Filter Resize Stream Interface Stall notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_err_stif
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   pwrite() failure
 *
 * @details
 *   Force Filter Resize stream interface stall notification
 */
int fpga_filter_resize_set_err_stif_force(
        uint32_t dev_id,
        uint32_t lane,
        fpga_fr_err_stif_t fr_err_stif);

/**
 * @brief API which get Filter Resize stream interface stall notification force generation setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] fr_err_stif
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   Get setting for forcing Filter Resize stream interface stall notification
 */
int fpga_filter_resize_get_err_stif_force(
        uint32_t dev_id,
        uint32_t lane,
        fpga_fr_err_stif_t *fr_err_stif);

/**
 * @brief API which insert Filter Resize Function Ingress Input/Egress Output Protocol Failure
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   control function ID
 * @param[in] dir
 *   Ingress/Egress
 * @param[in] func_err_prot
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_WRITE
 *   pwrite() failure
 *
 * @details
 *   Insert Filter Resize Function Ingress input/egress output protocol error
 */
int fpga_filter_resize_err_prot_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which get Filter Resize Function Ingress Input/Egress Output Protocol error insertion settings
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   control function ID
 * @param[in] dir
 *   Ingress/Egress
 * @param[out] func_err_prot
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_READ
 *   pread() failure
 *
 * @details
 *   Get setting for false failure occurrence of ingress input/egress output protocol
 */
int fpga_filter_resize_err_prot_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFUNCTION_FILTER_RESIZE_ERR_H_
