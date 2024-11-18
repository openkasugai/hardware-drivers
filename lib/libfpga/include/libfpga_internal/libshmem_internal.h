/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libshmem_internal.h
 * @brief Header file for internal definition or function of libshmem
 */

#ifndef LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBSHMEM_INTERNAL_H_
#define LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBSHMEM_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get Host information
 */
int __fpga_shmem_init_host_info(void);

/**
 * @brief Get static global vairable's pointer in libshmem.c
 */
int *__fpga_shmem_get_socket_num(void);

/**
 * @brief Get static global vairable's pointer in libshmem.c
 */
int *__fpga_shmem_get_socket_limit(void);

/**
 * @brief Get static global vairable's pointer in libshmem.c
 */
int *__fpga_shmem_get_available_limit(void);

/**
 * @brief Get static global vairable's pointer in libshmem.c
 */
int *__fpga_shmem_get_lcore_limit(void);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBSHMEM_INTERNAL_H_
