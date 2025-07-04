/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _LIBTOE_H__
#define _LIBTOE_H__

#include <stdint.h>
#include <libutil.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* fpga_toe_t;

// User API

/**
 * @brief Initialize the object for tcp connection
 * @param[in] dev_id device number
 * @param[in] is_tx direction
 * @param[in] ch using channel
 * @param[out] info the object initialized
 * @param[in] instance_id instance index in the device.
 * @return status code
 **/
xse_status_t fpga_toe_init(int dev_id, bool is_tx, int ch, fpga_toe_t* info, int instance_id = 0);

/**
 * @brief Set tranfer information to the hardware module.
 * @param[in] info the object of the specified channel
 * @param[in] buffer_size transfer size in byte (fixed size only now)
 * @param[in] credit_num max transferring frames (if direction is tx, not used)
 * @return status code
 **/
xse_status_t fpga_toe_set_buffer_info(fpga_toe_t info,
                                      uint32_t buffer_size, uint32_t credit_num);

/**
 * @brief Connect to target IP address and port number.
 * @param[in] info the object of the specified channel
 * @param[in] target_ip target IP address
 * @param[in] target_port target port number
 * @param[in] timeout timeout seconds (0 means no timeout)
 * @return status code
 **/
xse_status_t fpga_toe_connect(fpga_toe_t info,
                              const char* target_ip, uint16_t target_port, uint32_t timeout = 0);

/**
 * @brief Listen from target IP address and port number with masks.
 * @param[in] info the object of the specified channel
 * @param[in] self_port self port number
 * @param[in] target_ip target IP address
 * @param[in] target_port target port number
 * @param[in] target_ip_mask target IP address mask (NULL means 255.255.255.255)
 * @param[in] target_port_mask target port number mask (default: ignore target port)
 * @return status code
 **/
xse_status_t fpga_toe_listen(fpga_toe_t info, uint16_t self_port,
                             const char* target_ip, uint16_t target_port,
                             const char* target_ip_mask = NULL, uint16_t target_port_mask = 0);

/**
 * @brief Accept listening connection
 * @param[in] info the object of the specified channel
 * @param[in] timeout timeout seconds (0 means no timeout)
 * @return status code
 **/
xse_status_t fpga_toe_accept(fpga_toe_t info, uint32_t timeout = 0);

/**
 * @brief Disconnect the connection.
 * @param[in] info the object of the specified channel
 * @return status code
 **/
xse_status_t fpga_toe_disconnect(fpga_toe_t info);

/**
 * @brief Activate streaming.
 * @param[in] info the object of the specified channel
 * @return status code
 **/
xse_status_t fpga_toe_activate(fpga_toe_t info);

/**
 * @brief Get toe hardware status.
 * @param[in] info the object of the specified channel
 * @param[out] ptr the pointer of toe_ctrl_ioctl_status_t buffer
 * @return status code
 **/
xse_status_t fpga_toe_get_status(fpga_toe_t info, void* ptr = NULL);

/**
 * @brief Discard the object of the connection.
 * @param[in] info the object of the specified channel
 * @return status code
 **/
xse_status_t fpga_toe_finish(fpga_toe_t info);

#ifdef __cplusplus
}
#endif

#endif // _LIBTOE_H__
