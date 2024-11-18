/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libchain.h
 * @brief Header file for setting for chain connection
 */

#ifndef LIBFPGA_INCLUDE_LIBCHAIN_H_
#define LIBFPGA_INCLUDE_LIBCHAIN_H_

#include <libfpgactl.h>

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Min connection id value from xpcie_device.h
 */
#define CID_MIN                 XPCIE_CID_MIN

/**
 * Max connection id value from xpcie_device.h
 */
#define CID_MAX                 XPCIE_CID_MAX

/**
 * Min function chain id value from xpcie_device.h
 */
#define FUNCTION_CHAIN_ID_MIN   XPCIE_FUNCTION_CHAIN_ID_MIN

/**
 * Max function chain id value from xpcie_device.h
 */
#define FUNCTION_CHAIN_ID_MAX   XPCIE_FUNCTION_CHAIN_ID_MAX

/**
 * Max function chain id value from xpcie_device.h
 */
#define IS_VALID_FUNCTION_CHAIN_ID(fchid) \
  ((fchid) <= FUNCTION_CHAIN_ID_MAX && (fchid) >= FUNCTION_CHAIN_ID_MIN)


/**
 * @enum FUNCTION_CHAIN_DIR
 * @brief
 *   Not for user@n
 *   Enumeration for identifer wheter setting direction is egress or ingress.@n
 *   These num should be sequential from 0.
 */
enum FUNCTION_CHAIN_DIR{
  FUNCTION_CHAIN_DIR_EGRESS = 0,  /**< Egress Chain */
  FUNCTION_CHAIN_DIR_INGRESS,     /**< Ingress Chain */
  FUNCTION_CHAIN_DIR_MAX,         /**< Sentinel */
};

/**
 * Not for user: IOCTL chain update command's common interface for library
 */
#define FUNCTION_CHAIN_TABLE_UPDATE_CMD(dir) \
     ((dir == FUNCTION_CHAIN_DIR_EGRESS)  ? XPCIE_DEV_CHAIN_UPDATE_TABLE_EGR \
                                          : XPCIE_DEV_CHAIN_UPDATE_TABLE_INGR)

/**
 * Not for user: IOCTL chain delete command's common interface for library
 */
#define FUNCTION_CHAIN_TABLE_DELETE_CMD(dir) \
     ((dir == FUNCTION_CHAIN_DIR_EGRESS)  ? XPCIE_DEV_CHAIN_DELETE_TABLE_EGR \
                                          : XPCIE_DEV_CHAIN_DELETE_TABLE_INGR)

/**
 * @struct fpga_chain_ddr_t
 * @brief Struct for DDR setting
 * @var fpga_chain_ddr_t::base
 *      DDR buffer base address
 * @var fpga_chain_ddr_t::rx_offset
 *      RX Buffer Offset
 * @var fpga_chain_ddr_t::rx_stride
 *      RX Buffer Stride
 * @var fpga_chain_ddr_t::tx_offset
 *      TX Buffer Offset
 * @var fpga_chain_ddr_t::tx_stride
 *      TX Buffer Stride
 * @var fpga_chain_ddr_t::rx_size
 *      External IF RX buffer channel size selection
 * @var fpga_chain_ddr_t::tx_size
 *      External IF TX buffer channel size selection
 */
typedef struct fpga_chain_ddr {
  uint64_t base;
  uint64_t rx_offset;
  uint32_t rx_stride;
  uint64_t tx_offset;
  uint32_t tx_stride;
  uint8_t rx_size;
  uint8_t tx_size;
} fpga_chain_ddr_t;


/**
 * @brief API which start Chain Control
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Activate the Chain Control
 */
int fpga_chain_start(
        uint32_t dev_id,
        uint32_t lane);

/**
 * @brief API which stop Chain Control
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Stop Chain Control
 */
int fpga_chain_stop(
        uint32_t dev_id,
        uint32_t lane);

/**
 * @brief API which set DDR offset
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   External IF ID
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Set Chain Control DDR offset
 */
int fpga_chain_set_ddr(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t extif_id);

/**
 * @brief API which get DDR offset setting
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IF ID
 * @param[out] chain_ddr
 *   DDR offset setting information
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get Chain Control DDR offset setting and store it in chain_ddr
 */
int fpga_chain_get_ddr(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t extif_id,
        fpga_chain_ddr_t *chain_ddr);


/**
 * @brief API which configure for ingress and egress
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @param[in] ingress_extif_id
 *   external IFID for ingress side
 * @param[in] ingress_cid
 *   Target connection id for ingress chain
 * @param[in] egress_extif_id
 *   external IFID of egress side
 * @param[in] egress_cid
 *   Target connection id for egress chain
 * @param[in] ingress_active_flag
 *   Forwarding permission setting
 * @param[in] egress_active_flag
 *   Forwarding permission setting
 * @param[in] direct_flag
 *   direct forwarding setting
 * @param[in] egress_virtual_flag
 *   virtual connection flag
 * @param[in] egress_blocking_flag
 *   blocking forwarding flag
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   fpga_chain_connect_ingress/egress().
 *   Function chain is set for FPGA.
 *   See the respective functions for details.
 *   If either cannot be set correctly, do not set both settings.
 */
int fpga_chain_connect(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t ingress_extif_id,
        uint32_t ingress_cid,
        uint32_t egress_extif_id,
        uint32_t egress_cid,
        uint8_t ingress_active_flag,
        uint8_t egress_active_flag,
        uint8_t direct_flag,
        uint8_t egress_virtual_flag,
        uint8_t egress_blocking_flag);

/**
 * @brief API which configure Function chain for ingress
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @param[in] ingress_extif_id
 *   external IFID for ingress side
 * @param[in] ingress_cid
 *   Target connection id for ingress chain
 * @param[in] active_flag
 *   transfer permission flag
 * @param[in] direct_flag
 *   direct transfer flag
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 * @retval TABLE_UPDATE_TIMEOUT
 *   Function chain setting in FPGA failed
 *
 * @details
 *   For the kernel in the FPGA of the device specified by the argument:
 *   The connection ID of ingress and the function in the function section are specified.
 *   Update the function chain table in the FPGA to be connected.
 */
int fpga_chain_connect_ingress(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t ingress_extif_id,
        uint32_t ingress_cid,
        uint8_t active_flag,
        uint8_t direct_flag);

/**
 * @brief API which connect Function chain for egress
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @param[in] egress_extif_id
 *   external IFID of egress side
 * @param[in] egress_cid
 *   Target connection id for egress chain
 * @param[in] active_flag
 *   transfer permission flag
 * @param[in] direct_flag
 *   direct transfer enable flag
 * @param[in] virtual_flag
 *   virtual connection flag
 * @param[in] blocking_flag
 *   blocking forwarding flag
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 * @retval TABLE_UPDATE_TIMEOUT
 *   Function chain setting in FPGA failed
 *
 * @details
 *   For the kernel in the FPGA of the device specified by the argument:
 *   The connection ID of egress and the function in the function section are specified.
 *   Update the function chain table in the FPGA to be connected.
 */
int fpga_chain_connect_egress(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t egress_extif_id,
        uint32_t egress_cid,
        uint8_t active_flag,
        uint8_t virtual_flag,
        uint8_t blocking_flag);

/**
 * @brief API which disconnect function chain for ingress and egress
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   fpga_chain_disconnect_ingress/egress()
 *   To cancel setting of a function chain to an FPGA.
 *   See the respective functions for details.
 *   If either one of them cannot be reset normally, the reset is stopped at that point.
 */
int fpga_chain_disconnect(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid);

/**
 * @brief API which disconnect function chain for ingress
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 * @retval TABLE_UPDATE_TIMEOUT
 *   Function chain setting in FPGA failed
 *
 * @details
 *   For the kernel in the FPGA of the device specified by the argument:
 *   on the ingress side of the function in the function part represented by fchid
 *   Clears the function chain table in the FPGA.
 */
int fpga_chain_disconnect_ingress(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid);

/**
 * @brief API which disconnect egress function chain
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 * @retval TABLE_UPDATE_TIMEOUT
 *   Function chain setting in FPGA failed
 *
 * @details
 *   For the kernel in the FPGA of the device specified by the argument:
 *   on the egress side of the function in the function part represented by fchid
 *   Clears the function chain table in the FPGA.
 */
int fpga_chain_disconnect_egress(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid);

/**
 * @brief API which read ingress chain table entry
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] ingress_extif_id
 *   external IFID for ingress side
 * @param[in] ingress_cid
 *   Target connection id for ingress chain
 * @param[out] enable_flag
 *   chain table enable flag
 * @param[out] active_flag
 *   Forwarding permission setting
 * @param[out] direct_flag
 *   direct transfer setting
 * @param[out] fchid
 *   Function channel ID
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   read fchid connected to ingress_cid
 */
int fpga_chain_read_table_ingress(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t ingress_extif_id,
        uint32_t ingress_cid,
        uint8_t *enable_flag,
        uint8_t *active_flag,
        uint8_t *direct_flag,
        uint32_t *fchid);

/**
 * @brief API which read egress chain table entry
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @param[out] enable_flag
 *   table enable flag
 * @param[out] active_flag
 *   Forwarding permission setting
 * @param[out] virtual_flag
 *   virtual connection flag
 * @param[out] blocking_flag
 *   blocking forwarding flag
 * @param[out] egress_extif_id
 *   external IFID of egress side
 * @param[out] egress_cid
 *   connection ID on egress side
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   read cid on egress side connected to function fchid
 */
int fpga_chain_read_table_egress(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint8_t *enable_flag,
        uint8_t *active_flag,
        uint8_t *virtual_flag,
        uint8_t *blocking_flag,
        uint32_t *egress_extif_id,
        uint32_t *egress_cid);

/**
 * @brief API which read function chain table entries in driver(not device)
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @param[out] ingress_extif_id
 *   Pointer variable to get extif_id on ingress side
 * @param[out] ingress_cid
 *   Pointer variable to get connection_id on ingress side
 * @param[out] egress_extif_id
 *   Pointer variable to get extif_id on egress side
 * @param[out] egress_cid
 *   Pointer variable to get connection_id on egress side
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   e.g.) xpcie_device.h used in driver and lib differ
 *
 * @details
 *   Get extif/connection ids on both of the ingress/egress side
 *    from lane and fchid.@n
 *   The values are not read from device really,
 *    this API read from driver, so it may differ from the values set in the device.@n
 *   When you want to get the values in the device, please use below APIs:
 *    @li fpga_chain_read_table_ingress()
 *    @li fpga_chain_read_table_egress()
 *   Please allocate memory for `ingress_extif_id`, `ingress_cid`, `egress_extif_id`
 *    and `egress_cid` by user.
 *    When this API fails, the entities of the pointer variables are not defined.@n
 *   The defalut value of each ids will be (uint32_t)-1 when there is no function chain.
 *   This API allows `NULL` in some cases:
 *    both of extif_id and cid on the one side are NULL,
 *    and both of extif_id and cid on the other side are not NULL.@n
 *    @li NULL(ingress_extif_id,ingress_cid)
 *    @li NULL(egress_extif_id,egress_cid)
 */
int fpga_chain_read_soft_table(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        uint32_t *ingress_extif_id,
        uint32_t *ingress_cid,
        uint32_t *egress_extif_id,
        uint32_t *egress_cid);

/**
 * @brief API which wait for ingress chain connection established
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @param[in] timeout
 *   Timeout for polling
 * @param[in] interval
 *   Interval for polling
 * @param[out] is_success
 *   The result if ingress chain connection established or not@n
 *   if chain found, return true, otherwise return false
 * @retval 0
 *   Success(Don't care if chain found or not)
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @return
 *   @sa fpga_chain_read_soft_table
 *
 * @details
 *   This API calls fpga_chain_read_soft_table() periodically to check
 *    if ingress chain connection established or not.@n
 *   The interval of API call is defined by `interval`,
 *    and the timeout of API polling is defined by `timeout`.
 *    If you set `NULL` for them, they will be considered as 0 sec.@n
 *   The retval means only if API calling succeed or not,
 *    so it doesn't care if ingress chain connection established or not.
 *    When you want to check if ingress chain connection established,
 *    please check `is_success` too.@n
 *   Please allocate memory for `is_success` by user.@n
 *   When this API fails, the value of `is_success` is not defined,
 *    so if you want to use `is_success`, please check retval first.
 */
int fpga_chain_wait_connection_ingress(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        const struct timeval *timeout,
        const struct timeval *interval,
        uint32_t *is_success);

/**
 * @brief API which wait for egress chain connection established
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @param[in] timeout
 *   Timeout for polling
 * @param[in] interval
 *   Interval for polling
 * @param[out] is_success
 *   The result if egress chain connection established or not@n
 *   if chain found, return true, otherwise return false
 * @retval 0
 *   Success(Don't care if chain found or not)
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @return
 *   @sa fpga_chain_read_soft_table
 *
 * @details
 *   This API calls fpga_chain_read_soft_table() periodically to check
 *    if egress chain connection established or not.@n
 *   The interval of API call is defined by `interval`,
 *    and the timeout of API polling is defined by `timeout`.
 *    If you set `NULL` for them, they will be considered as 0 sec.@n
 *   The retval means only if API calling succeed or not,
 *    so it doesn't care if egress chain connection established or not.
 *    When you want to check if egress chain connection established,
 *    please check `is_success` too.@n
 *   Please allocate memory for `is_success` by user.@n
 *   When this API fails, the value of `is_success` is not defined,
 *    so if you want to use `is_success`, please check retval first.
 */
int fpga_chain_wait_connection_egress(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        const struct timeval *timeout,
        const struct timeval *interval,
        uint32_t *is_success);

/**
 * @brief API which wait for ingress chain connection deleted
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @param[in] timeout
 *   Timeout for polling
 * @param[in] interval
 *   Interval for polling
 * @param[out] is_success
 *   The result if ingress chain connection deleted or not@n
 *   if chain found, return true, otherwise return false
 * @retval 0
 *   Success(Don't care if chain found or not)
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @return
 *   @sa fpga_chain_read_soft_table
 *
 * @details
 *   This API calls fpga_chain_read_soft_table() periodically to check
 *    if ingress chain connection deleted or not.@n
 *   The interval of API call is defined by `interval`,
 *    and the timeout of API polling is defined by `timeout`.
 *    If you set `NULL` for them, they will be considered as 0 sec.@n
 *   The retval means only if API calling succeed or not,
 *    so it doesn't care if ingress chain connection deleted or not.
 *    When you want to check if ingress chain connection deleted,
 *    please check `is_success` too.@n
 *   Please allocate memory for `is_success` by user.@n
 *   When this API fails, the value of `is_success` is not defined,
 *    so if you want to use `is_success`, please check retval first.
 */
int fpga_chain_wait_disconnection_ingress(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        const struct timeval *timeout,
        const struct timeval *interval,
        uint32_t *is_success);

/**
 * @brief API which wait for egress chain connection deleted
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] fchid
 *   Target function channel id
 * @param[in] timeout
 *   Timeout for polling
 * @param[in] interval
 *   Interval for polling
 * @param[out] is_success
 *   The result if egress chain connection deleted or not@n
 *   if chain found, return true, otherwise return false
 * @retval 0
 *   Success(Don't care if chain found or not)
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @return
 *   @sa fpga_chain_read_soft_table
 *
 * @details
 *   This API calls fpga_chain_read_soft_table() periodically to check
 *    if egress chain connection deleted or not.@n
 *   The interval of API call is defined by `interval`,
 *    and the timeout of API polling is defined by `timeout`.
 *    If you set `NULL` for them, they will be considered as 0 sec.@n
 *   The retval means only if API calling succeed or not,
 *    so it doesn't care if egress chain connection deleted or not.
 *    When you want to check if egress chain connection deleted,
 *    please check `is_success` too.@n
 *   Please allocate memory for `is_success` by user.@n
 *   When this API fails, the value of `is_success` is not defined,
 *    so if you want to use `is_success`, please check retval first.
 */
int fpga_chain_wait_disconnection_egress(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t fchid,
        const struct timeval *timeout,
        const struct timeval *interval,
        uint32_t *is_success);

/**
 * @brief API which get control value for Chain Control
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] control
 *   control register value
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   get control value
 */
int fpga_chain_get_control(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *control);

/**
 * @brief API which get module_id value for Chain Control
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] module_id
 *   module_id register value
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get module_id value of Chain Control
 */
int fpga_chain_get_module_id(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t *module_id);

/**
 * @brief API which get connection status of external IF
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] extif_id
 *   external IFID
 * @param[in] cid
 *   Target connection id
 * @param[out] status
 *   status register value
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   Bad argument@n
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -FAILURE_IOCTL
 *   ioctl failure
 *
 * @details
 *   Get the connection establishment status of the external IF.
 */
int fpga_chain_get_con_status(
        uint32_t dev_id,
        uint32_t lane,
        uint32_t extif_id,
        uint32_t cid,
        uint32_t *status);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBCHAIN_H_
