/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libdirecttrans_err.h
 * @brief Header file for getting and setting Direct Transfer Adapter error
 */

#ifndef LIBFPGA_INCLUDE_LIBDIRECTTRANS_ERR_H_
#define LIBFPGA_INCLUDE_LIBDIRECTTRANS_ERR_H_

#include <libfpgactl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct fpga_direct_err_prot_t
 * @brief Struct for Direct Transfer Adapter Protocol error Information Structure @n
 *    all valid range 1bit
 * @var fpga_direct_err_prot_t::prot_ch
 *      channel protocol error
 * @var fpga_direct_err_prot_t::prot_len
 *      length protocol error
 * @var fpga_direct_err_prot_t::prot_sof
 *      SOF protocol error
 * @var fpga_direct_err_prot_t::prot_eof
 *      EOF protocol error
 * @var fpga_direct_err_prot_t::prot_reqresp
 *      number of req/resp protocol error
 * @var fpga_direct_err_prot_t::prot_datanum
 *      number of data protocol error
 * @var fpga_direct_err_prot_t::prot_req_outstanding
 *      number of request outstanding protocol error
 * @var fpga_direct_err_prot_t::prot_resp_outstanding
 *      number of response outstanding protocol error
 * @var fpga_direct_err_prot_t::prot_max_datanum
 *      data maximum number error
 * @var fpga_direct_err_prot_t::prot_reqlen
 *      req.length > 0 error
 * @var fpga_direct_err_prot_t::prot_reqresplen
 *      req.length == resp.length error
 */
typedef struct fpga_direct_err_prot {
  uint8_t prot_ch;
  uint8_t prot_len;
  uint8_t prot_sof;
  uint8_t prot_eof;
  uint8_t prot_reqresp;
  uint8_t prot_datanum;
  uint8_t prot_req_outstanding;
  uint8_t prot_resp_outstanding;
  uint8_t prot_max_datanum;
  uint8_t prot_reqlen;
  uint8_t prot_reqresplen;
} fpga_direct_err_prot_t;

/**
 * @struct fpga_direct_err_stif_t
 * @brief Struct for Direct Transfer Adapter Stream Interface Stall Information Structure
 *    all valid range 1bit
 * @var fpga_direct_err_stif_t::ingress_rcv_req
 *      ingress receive request
 * @var fpga_direct_err_stif_t::ingress_rcv_resp
 *      ingress receive response
 * @var fpga_direct_err_stif_t::ingress_rcv_data
 *      ingress receive data
 * @var fpga_direct_err_stif_t::ingress_snd_req
 *      ingress send request
 * @var fpga_direct_err_stif_t::ingress_snd_resp
 *      ingress send response
 * @var fpga_direct_err_stif_t::ingress_snd_data
 *      ingress send data
 * @var fpga_direct_err_stif_t::egress_rcv_req
 *      egress receive request
 * @var fpga_direct_err_stif_t::egress_rcv_resp
 *      egress receive response
 * @var fpga_direct_err_stif_t::egress_rcv_data
 *      egress receive data
 * @var fpga_direct_err_stif_t::egress_snd_req
 *      egress send request
 * @var fpga_direct_err_stif_t::egress_snd_resp
 *      egress send response
 * @var fpga_direct_err_stif_t::egress_snd_data
 *      egress send data
 */
typedef struct fpga_direct_err_stif {
  uint8_t ingress_rcv_req;
  uint8_t ingress_rcv_resp;
  uint8_t ingress_rcv_data;
  uint8_t ingress_snd_req;
  uint8_t ingress_snd_resp;
  uint8_t ingress_snd_data;
  uint8_t egress_rcv_req;
  uint8_t egress_rcv_resp;
  uint8_t egress_rcv_data;
  uint8_t egress_snd_req;
  uint8_t egress_snd_resp;
  uint8_t egress_snd_data;
} fpga_direct_err_stif_t;


/**
 * @brief API which get aggregate results of Direct Transfer Adapter failures (excluding interface stalls)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] err_det
 *   failure detected
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get whether or not a Direct Transfer Adapter failure is detected (except for interface stalls).
 */
int fpga_direct_get_check_err(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *err_det);

/**
 * @brief API which get Direct Transfer Adapter Ingress Output/Egress Input Protocol error Notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   Direction
 * @param[out] direct_err_prot
 *   detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get Direct Transfer Adapter protocol error notification (ingress output/egress input)
 */
int fpga_direct_get_err_prot(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_direct_err_prot_t *direct_err_prot);

/**
 * @brief API which clear Direct Transfer Adapter Ingress output/Egress input protocol error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *  dir: Direction
 * @param[in] direct_err_prot
 *  direct_err_prot: detailed error information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Clear Direct Transfer Adapter protocol error notifications (ingress out/egress in)
 */
int fpga_direct_set_err_prot_clear(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_direct_err_prot_t direct_err_prot);

/**
 * @brief API which mask Direct Transfer Adapter Ingress output/Egress input protocol error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *  dir: Direction
 * @param[in] direct_err_prot
 *   error details
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 */
int fpga_direct_set_err_prot_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_direct_err_prot_t direct_err_prot);

/**
 * @brief API which get Direct Transfer Adapter ingress output/egress input protocol error notification mask setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *  dir: Direction
 * @param[out] direct_err_prot
 *   error details
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get the mask setting for the protocol error notification (ingress output/egress input) of the Direct Transfer Adapter.
 */
int fpga_direct_get_err_prot_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_direct_err_prot_t *direct_err_prot);

/**
 * @brief API which force Direct Transfer Adapter Ingress output/Egress input protocol error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *  dir: Direction
 * @param[in] direct_err_prot
 *   error details
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Force generation of Direct Transfer Adapter protocol error notification (ingress output/egress input)
 */
int fpga_direct_set_err_prot_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_direct_err_prot_t direct_err_prot);

/**
 * @brief API which get Direct Transfer Adapter Ingress output/Egress input protocol error notification force setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *  dir: Direction
 * @param[out] direct_err_prot
 *   error details
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get the setting for forced generation of the protocol error notification (ingress output/egress input) of Direct Transfer Adapter
 */
int fpga_direct_get_err_prot_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_direct_err_prot_t *direct_err_prot);

/**
 * @brief API which insert Direct Transfer Adapter Ingress I/O/Egress I/O Protocol error
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir_type
 *  dir_type: 0: Ingress input/1: Ingress output/2: Egress input/3: Egress output/Others: Disabled
 * @param[in] direct_err_prot
 *   error details
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Insert Direct Transfer Adapter protocol error
 */
int fpga_direct_err_prot_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir_type,
        fpga_direct_err_prot_t direct_err_prot);

/**
 * @brief API which get Direct Transfer Adapter Ingress I/O/Egress I/O Protocol error insertion setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir_type
 *  dir_type: 0: Ingress input/1: Ingress output/2: Egress input/3: Egress output/Others: Disabled
 * @param[out] direct_err_prot
 *   error details
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  get insertion setting of the protocol error notification of Direct Transfer Adapter
 */
int fpga_direct_err_prot_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir_type,
        fpga_direct_err_prot_t *direct_err_prot);

/**
 * @brief API which get Direct Transfer Adapter stream interface stall notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] direct_err_stif
 *   error details
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 * get stream interface stall notification for Direct Transfer Adapter
 */
int fpga_direct_get_err_stif(
        uint32_t dev_id,
        uint32_t lane,
        fpga_direct_err_stif_t *direct_err_stif);

/**
 * @brief API which mask Direct Transfer Adapter stream interface stall notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] direct_err_stif
 *   error details
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  mask stream interface stall notification for Direct Transfer Adapter
 */
int fpga_direct_set_err_stif_mask(
        uint32_t dev_id,
        uint32_t lane,
        fpga_direct_err_stif_t direct_err_stif);

/**
 * @brief API which get Direct Transfer Adapter stream interface stall notification mask setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] direct_err_stif
 *   error details
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get mask setting for stream interface stall notification of Direct Transfer Adapter
 */
int fpga_direct_get_err_stif_mask(
        uint32_t dev_id,
        uint32_t lane,
        fpga_direct_err_stif_t *direct_err_stif);

/**
 * @brief API which force Direct Transfer Adapter stream interface stall
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] direct_err_stif
 *   error details
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  force stream interface stall notification for Direct Transfer Adapter
 */
int fpga_direct_set_err_stif_force(
        uint32_t dev_id,
        uint32_t lane,
        fpga_direct_err_stif_t direct_err_stif);

/**
 * @brief API which get Direct Transfer Adapter stream interface stall force setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] direct_err_stif
 *   error details
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *  Get the setting for forced generation of stream interface stall notification of Direct Transfer Adapter
 */
int fpga_direct_get_err_stif_force(
        uint32_t dev_id,
        uint32_t lane,
        fpga_direct_err_stif_t *direct_err_stif);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBDIRECTTRANS_ERR_H_
