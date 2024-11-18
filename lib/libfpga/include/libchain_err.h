/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libchain_err.h
 * @brief Header file for getting and setting Chain Control error
 */

#ifndef LIBFPGA_INCLUDE_LIBCHAIN_ERR_H_
#define LIBFPGA_INCLUDE_LIBCHAIN_ERR_H_

#include <libfpgactl.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @struct fpga_chain_err_t
 * @brief Struct for Chain Control Ingress Input/Egress Output Failure Detection Information Structure
 * @var fpga_chain_err_t::header_marker
 *      header marker error
 * @var fpga_chain_err_t::payload_len
 *      payload length error
 * @var fpga_chain_err_t::header_len
 *      header length error
 * @var fpga_chain_err_t::header_chksum
 *      header checksum error
 * @var fpga_chain_err_t::header_stat
 *      header_status error
 * @var fpga_chain_err_t::pointer_table_miss
 *      pointer table misshit
 * @var fpga_chain_err_t::payload_table_miss
 *      payload table misshit
 * @var fpga_chain_err_t::con_table_miss
 *      connection-channel table misshit
 * @var fpga_chain_err_t::pointer_table_invalid
 *      pointer table invalid
 * @var fpga_chain_err_t::payload_table_invalid
 *      payload table invalid
 * @var fpga_chain_err_t::con_table_invalid
 *      connection-channel table invalid
 */
typedef struct fpga_chain_err {
  uint8_t header_marker;
  uint8_t payload_len;
  uint8_t header_len;
  uint8_t header_chksum;
  uint8_t header_stat;
  uint8_t pointer_table_miss;
  uint8_t payload_table_miss;
  uint8_t con_table_miss;
  uint8_t pointer_table_invalid;
  uint8_t payload_table_invalid;
  uint8_t con_table_invalid;
} fpga_chain_err_t;

/**
 * @struct fpga_chain_err_table_t
 * @brief Struct for Chain Control Ingress Output Failure/Egress Input Failure Detection Information Structure
 *   all valid range 1bit
 * @var fpga_chain_err_table_t::con_table_miss
 *      connection-channel table misshit
 * @var fpga_chain_err_table_t::con_table_invalid
 *      connection-channel table invalid
 */
typedef struct fpga_chain_err_table {
  uint8_t con_table_miss;
  uint8_t con_table_invalid;
} fpga_chain_err_table_t;

/**
 * @struct fpga_chain_err_prot_t
 * @brief Struct for Chain Control Protocol error Information Structure
 * @var fpga_chain_err_prot_t::prot_ch
 *      channel protocol error
 * @var fpga_chain_err_prot_t::prot_len
 *      length protocol error
 * @var fpga_chain_err_prot_t::prot_sof
 *      SOF protocol error
 * @var fpga_chain_err_prot_t::prot_eof
 *      EOF protocol error
 * @var fpga_chain_err_prot_t::prot_reqresp
 *      number of req/resp protocol error
 * @var fpga_chain_err_prot_t::prot_datanum
 *      number of data protocol error
 * @var fpga_chain_err_prot_t::prot_req_outstanding
 *      number of request outstanding protocol error
 * @var fpga_chain_err_prot_t::prot_resp_outstanding
 *      number of response outstanding protocol error
 * @var fpga_chain_err_prot_t::prot_max_datanum
 *      data maximum number error
 * @var fpga_chain_err_prot_t::prot_reqlen
 *      req.length > 0 error
 * @var fpga_chain_err_prot_t::prot_reqresplen
 *      req.length == resp.length error
 */
typedef struct fpga_chain_err_prot {
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
} fpga_chain_err_prot_t;

/**
 * @struct fpga_chain_err_evt_t
 * @brief Struct for External IF Event Error Info Structure
 *   all valid range 1bit
 * @var fpga_chain_err_evt_t::established
 *      established
 * @var fpga_chain_err_evt_t::close_wait
 *      close wait
 * @var fpga_chain_err_evt_t::erased
 *      erased
 * @var fpga_chain_err_evt_t::syn_timeout
 *      syn timeout
 * @var fpga_chain_err_evt_t::syn_ack_timeout
 *      syn ack timeout
 * @var fpga_chain_err_evt_t::timeout
 *      timeout
 * @var fpga_chain_err_evt_t::recv_data
 *      recv data
 * @var fpga_chain_err_evt_t::send_data
 *      send data
 * @var fpga_chain_err_evt_t::recv_urgent_data
 *      recv urgent data
 * @var fpga_chain_err_evt_t::recv_rst
 *      recv_rst
 */
typedef struct fpga_chain_err_evt {
  uint8_t established;
  uint8_t close_wait;
  uint8_t erased;
  uint8_t syn_timeout;
  uint8_t syn_ack_timeout;
  uint8_t timeout;
  uint8_t recv_data;
  uint8_t send_data;
  uint8_t recv_urgent_data;
  uint8_t recv_rst;
} fpga_chain_err_evt_t;

/**
 * @struct fpga_chain_err_stif_t
 * @brief Struct for Chain Control Stream Interface Install Information Structure
 * @var fpga_chain_err_stif_t::ingress_req
 *      valid range 1bit
 * @var fpga_chain_err_stif_t::ingress_resp
 *      valid range 1bit
 * @var fpga_chain_err_stif_t::ingress_data
 *      valid range 1bit
 * @var fpga_chain_err_stif_t::egress_req
 *      valid range 1bit
 * @var fpga_chain_err_stif_t::egress_resp
 *      valid range 1bit
 * @var fpga_chain_err_stif_t::egress_data
 *      valid range 1bit
 * @var fpga_chain_err_stif_t::extif_event
 *      external interface event
 * @var fpga_chain_err_stif_t::extif_command
 *      external interface command
 */
typedef struct fpga_chain_err_stif {
  uint8_t ingress_req;
  uint8_t ingress_resp;
  uint8_t ingress_data;
  uint8_t egress_req;
  uint8_t egress_resp;
  uint8_t egress_data;
  uint8_t extif_event;
  uint8_t extif_command;
} fpga_chain_err_stif_t;


/**
 * @brief API which get Chain Control failures (excluding interface stalls)
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
 *   Get Chain Control failure detection (excluding interface stalls)
 */
int fpga_chain_get_check_err(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *err_det);

/**
 * @brief API which gt Chain Control ingress/egress output failure detection notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] cid_fchid
 *   connection ID/function channel ID
 * @param[in] dir
 *   Direction
 * @param[out] chain_err
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
 *   Get Chain Control failure detection notification (ingress input/egress output)
 */
int fpga_chain_get_err(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t cid_fchid,
        uint32_t dir,
        fpga_chain_err_t *chain_err);

/**
 * @brief API which mask Chain Control ingress input/egress output failure detection notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] dir
 *   Direction
 * @param[in] chain_err
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
 *   Set the mask for failure detection notification (ingress input/egress output) of Chain Control.
 */
int fpga_chain_set_err_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_t chain_err);

/**
 * @brief API which get Chain Control ingress input/egress output failure detection notification mask
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] dir
 *   Direction
 * @param[out] chain_err
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
 *   Get mask setting for error detection notification (ingress input/egress output) of Chain Control.
 */
int fpga_chain_get_err_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_t *chain_err);

/**
 * @brief API which force Chain Control Ingress input/Egress output failure detection notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] dir
 *   Direction
 * @param[in] chain_err
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
 *   Set the forced generation of Chain Control error detection notification (ingress input/egress output)
 */
int fpga_chain_set_err_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_t chain_err);

/**
 * @brief API which get Chain Control ingress/egress output failure detection notification force setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] dir
 *   Direction
 * @param[out] chain_err
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
 *   Get the setting of forced generation of failure detection notification (ingress input/egress output) of Chain Control
 */
int fpga_chain_get_err_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_t *chain_err);

/**
 * @brief API which insert Chain Control ingress/egress output error
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID [0,1]
 * @param[in] dir
 *   Direction (0: ingress, 1: egress)
 * @param[in] chain_err
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
 *   Set false failure for (ingress input/egress output) of Chain Control.
 */
int fpga_chain_err_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_t chain_err);

/**
 * @brief API which get Chain Control ingress input/egress output pseudo failure settings
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID [0,1]
 * @param[in] dir
 *   Direction (0: ingress, 1: egress)
 * @param[out] chain_err
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
 *   Get false failure setting of (ingress input/egress output) of Chain Control
 */
int fpga_chain_err_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_t *chain_err);

/**
 * @brief API which get Chain Control ingress output/egress input failure detection notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] cid_fchid
 *   connection ID/function channel ID
 * @param[in] dir
 *   Direction
 * @param[out] chain_err
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
 *   Get Chain Control failure detection notification (ingress output/egress input)
 */
int fpga_chain_get_err_table(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t cid_fchid,
        uint32_t dir,
        fpga_chain_err_table_t *chain_err);

/**
 * @brief API which mask Chain Control ingress output/egress input failure detection notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] dir
 *   Direction
 * @param[in] chain_err
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
 *   Set the mask for failure detection notification (ingress output/egress input) of Chain Control.
 */
int fpga_chain_set_err_table_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_table_t chain_err);

/**
 * @brief API which get Chain Control ingress output/egress input failure detection notification mask
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] dir
 *   Direction
 * @param[out] chain_err
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
 *   Get the setting of the mask for Chain Control failure detection notification (ingress output/egress input)
 */
int fpga_chain_get_err_table_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_table_t *chain_err);

/**
 * @brief API which force Chain Control Ingress output/Egress input failure detection notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] dir
 *   Direction
 * @param[in] chain_err
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
 *   Set forced generation of Chain Control failure detection notification (ingress output/egress input)
 */
int fpga_chain_set_err_table_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_table_t chain_err);

/**
 * @brief API which get Chain Control ingress output/egress input failure detection notification force setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] dir
 *   Direction
 * @param[out] chain_err
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
 *   Get the setting of forced generation of Chain Control failure detection notification (ingress output/egress input)
 */
int fpga_chain_get_err_table_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint32_t dir,
        fpga_chain_err_table_t *chain_err);

/**
 * @brief API which get Chain Control ingress output/egress input protocol error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   Direction
 * @param[out] chain_err_prot
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
 *   Get Chain Control protocol error notification (ingress output/egress input)
 */
int fpga_chain_get_err_prot(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t *chain_err_prot);

/**
 * @brief API which clear Chain Control ingress output/egress input protocol error notifications
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   Direction
 * @param[in] chain_err_prot
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
 *   Clear Chain Control protocol error notification (ingress output/egress input)
 */
int fpga_chain_set_err_prot_clear(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t chain_err_prot);

/**
 * @brief API which mask Chain Control ingress output/egress input protocol error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *  Direction
 * @param[in] chain_err_prot
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
 *   Set mask for Chain Control protocol error notification (ingress output/egress input)
 */
int fpga_chain_set_err_prot_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t chain_err_prot);

/**
 * @brief API which get Chain Control ingress output/egress input protocol error notification mask
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   Direction
 * @param[out] chain_err_prot
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
 *   Get mask setting for Chain Control protocol error notification (ingress output/egress input)
 */
int fpga_chain_get_err_prot_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t *chain_err_prot);

/**
 * @brief API which force Chain Control Ingress output/Egress input protocol error notification forced
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   Direction
 * @param[in] chain_err_prot
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
 *   Force generation of Chain Control protocol error notification (ingress output/egress input)
 */
int fpga_chain_set_err_prot_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t chain_err_prot);

/**
 * @brief API which get Chain Control ingress/egress input protocol error notification force setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   Direction
 * @param[out] chain_err_prot
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
 *   Get the setting of forced generation of Chain Control protocol error notification (ingress output/egress input)
 */
int fpga_chain_get_err_prot_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t *chain_err_prot);

/**
 * @brief API which insert Chain Control ingress output/egress input protocol error
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   Direction (0: ingress, 1: egress)
 * @param[in] chain_err_prot
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
 *   Set the false failure of the protocol error (ingress output/egress input) of Chain Control.
 */
int fpga_chain_err_prot_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t chain_err_prot);

/**
 * @brief API which get Chain Control ingress output/egress input protocol error insertion setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] dir
 *   Direction (0: ingress, 1: egress)
 * @param[out] chain_err_prot
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
 *   Get of false failure setting of Chain Control protocol error (ingress output/egress input).
 */
int fpga_chain_err_prot_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t dir,
        fpga_chain_err_prot_t *chain_err_prot);

/**
 * @brief API which get external IF event error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[out] chain_err_evt
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
 *   Get Chain Control external IF event error detection notification
 */
int fpga_chain_get_err_evt(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        fpga_chain_err_evt_t *chain_err_evt);

/**
 * @brief API which clear External IF event error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] chain_err_evt
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
 *   Clear Chain Control external IF event error detection notification
 */
int fpga_chain_set_err_evt_clear(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        fpga_chain_err_evt_t chain_err_evt);

/**
 * @brief API which mask External IF event error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] chain_err_evt
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
 *   Set the mask for Chain Control external IF event error detection notification
 */
int fpga_chain_set_err_evt_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        fpga_chain_err_evt_t chain_err_evt);

/**
 * @brief API which get external IF event error notification mask setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[out] chain_err_evt
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
 *   Get mask setting for Chain Control external IF event error detection notification
 */
int fpga_chain_get_err_evt_mask(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        fpga_chain_err_evt_t *chain_err_evt);

/**
 * @brief API which force external IF event error notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[in] chain_err_evt
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
 *   Force Chain Control external IF event error detection notification
 */
int fpga_chain_set_err_evt_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        fpga_chain_err_evt_t chain_err_evt);

/**
 * @brief API which get external IF event error notification force setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[out] chain_err_evt
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
 *   Force Chain Control external IF event error detection notification
 */
int fpga_chain_get_err_evt_force(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        fpga_chain_err_evt_t *chain_err_evt);

/**
 * @brief API which get Chain Control stream interface stall notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] chain_err_stif
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
 *   Get stream interface stall notification for Chain Control
 */
int fpga_chain_get_err_stif(
        uint32_t dev_id,
        uint32_t lane,
        fpga_chain_err_stif_t *chain_err_stif);

/**
 * @brief API which mask Chain Control stream interface stall notification
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] chain_err_stif
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
 *   Set mask for stream interface stall notification for Chain Control
 */
int fpga_chain_set_err_stif_mask(
        uint32_t dev_id,
        uint32_t lane,
        fpga_chain_err_stif_t chain_err_stif);

/**
 * @brief API which get Chain Control stream interface stall notification mask setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] chain_err_stif
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
 *   Get mask setting for stream interface stall notification of Chain Control
 */
int fpga_chain_get_err_stif_mask(
        uint32_t dev_id,
        uint32_t lane,
        fpga_chain_err_stif_t *chain_err_stif);

/**
 * @brief API which force Chain Control stream interface stall
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] chain_err_stif
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
 *   Force Chain Control stream interface stall notification
 */
int fpga_chain_set_err_stif_force(
        uint32_t dev_id,
        uint32_t lane,
        fpga_chain_err_stif_t chain_err_stif);

/**
 * @brief API which get Chain Control stream interface stall force setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] chain_err_stif
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
 *   Force stream interface stall notification of Chain Control
 */
int fpga_chain_get_err_stif_force(
        uint32_t dev_id,
        uint32_t lane,
        fpga_chain_err_stif_t *chain_err_stif);

/**
 * @brief API which Insert Chain Control command error
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID [0,1]
 * @param[in] ins_enable
 *   Enables pseudo failure (CID override) (0: Clear, 1: Pseudo-failure setting).
 * @param[in] cid
 *   Target connection id
 *  cid: Connection ID (Don'tCare when ins_enable= 0
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Command pseudo failure setting of Chain Control is performed.
 */
int fpga_chain_err_cmdfault_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint16_t ins_enable,
        uint16_t cid);

/**
 * @brief API which get Chain Control command get pseudo fault settings
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID [0,1]
 * @param[out] ins_enable
 *   Enables pseudo failure (CID override) (0: Clear, 1: Pseudo-failure setting).
 * @param[out] cid
 *   Target connection id
 *  cid: Connection ID (Don'tCare when ins_enable= 0
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Command pseudo failure setting of Chain Control is performed.
 */
int fpga_chain_err_cmdfault_get_ins(
        uint32_t dev_id,
        uint32_t lane,
        uint8_t extif_id,
        uint16_t *ins_enable,
        uint16_t *cid);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBCHAIN_ERR_H_
