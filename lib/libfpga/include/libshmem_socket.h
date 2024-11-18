/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libshmem_socket.h
 * @brief Header file for shared memory controller's socket connect function
 *        Normaly this file will be not included by user.
 */

#ifndef LIBFPGA_INCLUDE_LIBSHMEM_SOCKET_H_
#define LIBFPGA_INCLUDE_LIBSHMEM_SOCKET_H_

#include <sys/socket.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @enum shmem_socket_response_t
 *   Definition for shmem TCP connection response
 */
typedef enum SHMEM_SOCKET_RESPONSE {
  RES_OK = 0, /**< response which means ok */
  RES_NG,     /**< response which means ng */
  RES_QUIT,   /**< response which means accepting quit */
  RES_INIT,   /**< response which means shmem controller start */
}shmem_socket_response_t;


/**
 * @brief Function which listen and accept at addr:port
 */
int fpga_shmem_get_fd_server(
        struct sockaddr_in *data,
        unsigned short port,
        const char* addr);

/**
 * @brief Function which close fd opened by fpga_shmem_get_fd_server()
 */
void fpga_shmem_put_fd_server(void);

/**
 * @brief Function which connect to addr:port
 */
int fpga_shmem_get_fd_client(
        struct sockaddr_in *data,
        unsigned short port,
        const char *addr);

/**
 * @brief Function which send data
 */
int fpga_shmem_send(
        int fd,
        void *data,
        socklen_t len);

/**
 * @brief Function which recv data
 */
int fpga_shmem_recv(
        int fd,
        void **data,
        socklen_t *plen);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBSHMEM_SOCKET_H_
