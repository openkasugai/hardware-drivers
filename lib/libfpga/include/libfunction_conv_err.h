/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfunction_conv_err.h
 * @brief Header file for getting and setting Conversion Adapter error
 */

#ifndef LIBFPGA_INCLUDE_LIBFUNCTION_CONV_ERR_H_
#define LIBFPGA_INCLUDE_LIBFUNCTION_CONV_ERR_H_

#include <libfunction.h>

#ifdef __cplusplus
extern "C" {
#endif


#define CONV_DIR_INGRESS     0  /**< Conversion Adapter ingress */
#define CONV_DIR_EGRESS      1  /**< Conversion Adapter egress */
#define CONV_FUNC_NUMBER_0   0  /**< Conversion Adapter#0 */
#define CONV_FUNC_NUMBER_1   1  /**< Conversion Adapter#1 */


/**
 * @struct fpga_conv_err_stif_t
 * @brief Struct for Conversion Adapter Stream Interface Stall information
 * @var fpga_conv_err_stif_t::ingress_rcv_req
 *      ingress receive request
 * @var fpga_conv_err_stif_t::ingress_rcv_resp
 *      ingress receive response
 * @var fpga_conv_err_stif_t::ingress_rcv_data
 *      ingress receive data
 * @var fpga_conv_err_stif_t::ingress0_snd_req
 *      ingress#0 send request
 * @var fpga_conv_err_stif_t::ingress0_snd_resp
 *      ingress#0 send response
 * @var fpga_conv_err_stif_t::ingress0_snd_data
 *      ingress#0 send data
 * @var fpga_conv_err_stif_t::ingress1_snd_req
 *      ingress#1 send request
 * @var fpga_conv_err_stif_t::ingress1_snd_resp
 *      ingress#1 send response
 * @var fpga_conv_err_stif_t::ingress1_snd_data
 *      ingress#1 send data
 * @var fpga_conv_err_stif_t::egress0_rcv_req
 *      egress#0 receive request
 * @var fpga_conv_err_stif_t::egress0_rcv_resp
 *      egress#0 receive response
 * @var fpga_conv_err_stif_t::egress0_rcv_data
 *      egress#0 receive data
 * @var fpga_conv_err_stif_t::egress1_rcv_req
 *      egress#1 receive request
 * @var fpga_conv_err_stif_t::egress1_rcv_resp
 *      egress#1 receive response
 * @var fpga_conv_err_stif_t::egress1_rcv_data
 *      egress#1 receive data
 * @var fpga_conv_err_stif_t::egress_snd_req
 *      egress send request
 * @var fpga_conv_err_stif_t::egress_snd_resp
 *      egress send response
 * @var fpga_conv_err_stif_t::egress_snd_data
 *      egress send data
 */
typedef struct fpga_conv_err_stif {
  uint8_t ingress_rcv_req;
  uint8_t ingress_rcv_resp;
  uint8_t ingress_rcv_data;
  uint8_t ingress0_snd_req;
  uint8_t ingress0_snd_resp;
  uint8_t ingress0_snd_data;
  uint8_t ingress1_snd_req;
  uint8_t ingress1_snd_resp;
  uint8_t ingress1_snd_data;
  uint8_t egress0_rcv_req;
  uint8_t egress0_rcv_resp;
  uint8_t egress0_rcv_data;
  uint8_t egress1_rcv_req;
  uint8_t egress1_rcv_resp;
  uint8_t egress1_rcv_data;
  uint8_t egress_snd_req;
  uint8_t egress_snd_resp;
  uint8_t egress_snd_data;
} fpga_conv_err_stif_t;


/**
 * @brief API which get results of failure detection (excluding interface stalls) for Conversion Adapter
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
 *   Get whether a Conversion Adapter has failure detection (except for interface stalls)
 */
int fpga_conv_get_check_err(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *err_det);

/**
 * @brief API which get Conversion Adapter ingress input/egress output protocol error
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *   Get ingress input/egress output Protocol error notification for Conversion Adapter
 */
int fpga_conv_get_err_prot(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

/**
 * @brief API which clear Conversion Adapter ingress input/egress output Protocol error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *   Clear ingress input/egress output protocol error notification for Conversion Adapter
 */
int fpga_conv_set_err_prot_clear(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which mask Conversion Adapter ingress input/egress output protocol error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *   Mask ingress input/egress output protocol error notification for Conversion Adapter
 */
int fpga_conv_set_err_prot_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which get ingress input/egress output protocol error mask setting for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *   Get mask setting for ingress input/egress output protocol error notification of Conversion Adapter
 */
int fpga_conv_get_err_prot_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

/**
 * @brief API which force ingress input/egress output protocol error for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *   Force ingress input/egress output protocol error notification for Conversion Adapter
 */
int fpga_conv_set_err_prot_force(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which get ingress input/egress output protocol error forced setting for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *   Get settings for forcing ingress input/egress output protocol error notification for Conversion Adapter
 */
int fpga_conv_get_err_prot_force(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

/**
 * @brief API which get ingress output/egress input protocol error notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   control function ID
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *   Get Conversion Adapter ingress output/egress input protocol error notification
 */
int fpga_conv_get_err_prot_func(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

/**
 * @brief API which clear ingress output/egress input protocol error notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   control function ID
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *   Clear ingress output/egress input protocol error notification for Conversion Adapter
 */
int fpga_conv_set_err_prot_func_clear(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which mask ingress output/egress input protocol error notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   function ID
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *  Mask ingress output/egress input protocol error notification for Conversion Adapter
 */
int fpga_conv_set_err_prot_func_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which get ingress output/egress input protocol error notification mask setting for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   function ID
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *  get ingress output/egress input protocol error notification mask setting for Conversion Adapter
 */
int fpga_conv_get_err_prot_func_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

/**
 * @brief API which force ingress output/egress input protocol error notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   function ID
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *  Force ingress output/egress input protocol error notification for Conversion Adapter
 */
int fpga_conv_set_err_prot_func_force(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which get ingress output/egress input protocol error notification force setting for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   function ID
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *  Get force setting for ingress output/egress input protocol error notification of Conversion Adapter
 */
int fpga_conv_get_err_prot_func_force(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

/**
 * @brief API which get stream interface stall notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] conv_err_stif
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
 *  Get Conversion Adapter stream interface stall notification
 */
int fpga_conv_get_err_stif(
        uint32_t dev_id,
        uint32_t lane,
        fpga_conv_err_stif_t *conv_err_stif);

/**
 * @brief API which mask stream interface stall notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] conv_err_stif
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
 *  Set mask for Conversion Adapter stream interface stall notification
 */
int fpga_conv_set_err_stif_mask(
        uint32_t dev_id,
        uint32_t lane,
        fpga_conv_err_stif_t conv_err_stif);

/**
 * @brief API which get stream interface stall notification mask setting for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] conv_err_stif
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
 *  Get mask setting for Conversion Adapter stream interface stall notification
 */
int fpga_conv_get_err_stif_mask(
        uint32_t dev_id,
        uint32_t lane,
        fpga_conv_err_stif_t *conv_err_stif);

/**
 * @brief API which force stream interface stall notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] conv_err_stif
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
 *  Force Conversion Adapter stream interface stall notification
 */
int fpga_conv_set_err_stif_force(
        uint32_t dev_id,
        uint32_t lane,
        fpga_conv_err_stif_t conv_err_stif);

/**
 * @brief API which get stream interface stall notification force setting for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] conv_err_stif
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
 *  Get force setting of stream interface stall notification for Conversion Adapter
 */
int fpga_conv_get_err_stif_force(
        uint32_t dev_id,
        uint32_t lane,
        fpga_conv_err_stif_t *conv_err_stif);

/**
 * @brief API which get memory parity error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] err_parity
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
 *  Get Conversion Adapter memory parity error notification
 */
int fpga_conv_get_err_mem_parity(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *err_parity);

/**
 * @brief API which clear memory parity error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] err_parity
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
 *  Clear Conversion Adapter memory parity error
 */
int fpga_conv_set_err_mem_parity_clear(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t err_parity);

/**
 * @brief API which mask Memory parity error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] err_parity
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
 *  Mask memory parity error notification for Conversion Adapter
 */
int fpga_conv_set_err_mem_parity_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t err_parity);

/**
 * @brief API which get memory parity error mask setting for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] err_parity
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
 *  Get mask setting for Conversion Adapter memory parity error
 */
int fpga_conv_get_err_mem_parity_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *err_parity);

/**
 * @brief API which force memory parity error notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] err_parity
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
 *  Force memory parity error notification for Conversion Adapter
 */
int fpga_conv_set_err_mem_parity_force(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t err_parity);

/**
 * @brief API which get memory parity error force setting for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] err_parity
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
 *  Get force setting of Conversion Adapter memory parity error
 */
int fpga_conv_get_err_mem_parity_force(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *err_parity);

/**
 * @brief API which get ingress input frame length error notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] length_fault
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
 *  Get Conversion Adapter ingress input frame length error notification
 */
int fpga_conv_get_err_length_fault(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *length_fault);

/**
 * @brief API which clear ingress input frame length error notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] length_fault
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
 *  Clear Conversion Adapter ingress input frame length error notification
 */
int fpga_conv_set_err_length_fault_clear(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t length_fault);

/**
 * @brief API which mask ingress input frame length error notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] length_fault
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
 *  Mask Conversion Adapter ingress input frame length error notification
 */
int fpga_conv_set_err_length_fault_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t length_fault);

/**
 * @brief API which get ingress input frame length error mask setting for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] length_fault
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
 *  Get mask setting for the Conversion Adapter ingress input frame length error notification
 */
int fpga_conv_get_err_length_fault_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *length_fault);

/**
 * @brief API which force ingress input frame length fault notification for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] length_fault
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
 *  Force Conversion Adapter ingress input frame length error notification
 */
int fpga_conv_set_err_length_fault_force(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t length_fault);

/**
 * @brief API which get ingress input frame length error force setting for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] length_fault
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
 *  Get force setting of input frame length error notification for Conversion Adapter
 */
int fpga_conv_get_err_length_fault_force(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *length_fault);

/**
 * @brief API which insert ingress input/egress output protocol error
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *  Fake failure of ingress input/egress output protocol of Conversion Adapter
 */
int fpga_conv_err_prot_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which get ingress input/egress output protocol error insertion settings for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *  Get setting of false failure occurrence of ingress input/egress output protocol of Conversion Adapter
 */
int fpga_conv_err_prot_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

/**
 * @brief API which insert ingress output/egress input protocol failure
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   function ID
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *  Fake failure of ingress output/egress input protocol of Conversion Adapter
 */
int fpga_conv_err_prot_func_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t func_err_prot);

/**
 * @brief API which get ingress output/egress input protocol failure settings for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fr_id
 *   function ID
 * @param[in] dir
 *   direction (0:Ingress/1:Egress)
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
 *  Get setting of false failure of ingress output/egress input protocol of Conversion Adapter
 */
int fpga_conv_err_prot_func_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fr_id,
        uint32_t dir,
        fpga_func_err_prot_t *func_err_prot);

/**
 * @brief API which insert memory parity error failure for Conversion Adapter
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] err_parity
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
 *  Insert failure of Conversion Adapter memory parity error
 */
int fpga_conv_err_mem_parity_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t err_parity);

/**
 * @brief API which get memory parity error failure settings
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] err_parity
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
 *  Get setting of false failure occurrence of Conversion Adapter memory parity error
 */
int fpga_conv_err_mem_parity_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *err_parity);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFUNCTION_CONV_ERR_H_
