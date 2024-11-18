/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libshmem_manager.h
 * @brief Header file for shared memory manager
 */

#ifndef LIBFPGA_INCLUDE_LIBSHMEM_MANAGER_H_
#define LIBFPGA_INCLUDE_LIBSHMEM_MANAGER_H_

#include <libshmem.h>

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @typedef shmem_func
 * @brief shmem manager's callback function type
 */
typedef int (*shmem_func)(void*);

/**
 * @typedef sig_func
 * @brief shmem manager's signal setting function type
 */
typedef void (*sig_func)(int);


/**
 * @struct fpga_shmem_manager_info_t
 * @brief Struct of shmem manager's managemet information
 * @var fpga_shmem_manager_info_t::hp_limit
 *      Num of hugepages
 * @var fpga_shmem_manager_info_t::pid
 *      pid of shmem manager
 * @var fpga_shmem_manager_info_t::is_initialized
 *      Whether shmem manager is launched in success
 * @var fpga_shmem_manager_info_t::socket_limit
 *      Memory limit per socket
 * @var fpga_shmem_manager_info_t::file_prefix
 *      Prefix corresponding to shmem manager
 */
typedef struct fpga_shmem_manager_info {
  int hp_limit;
  pid_t pid;
  int is_initialized;
  int socket_limit[SHMEM_MAX_NUMA_NODE];
  char file_prefix[SHMEM_MAX_HUGEPAGE_PREFIX];
}fpga_shmem_manager_info_t;


/**
 * @brief API which register signal for finish shmem manager safely
 * @param void
 * @retval 0
 *   Success
 * @retval -ALREADY_INITIALIZED
 *   e.g.) signal can be set by this API only once
 * @retval -FAILURE_REGISTER_SIGNAL
 *   e.g.) Failed to sigaction()
 *
 * @details
 *   When SIGUSR1 is sent, register the function and SIGNAL together
 *    so that the function to set the flag which terminates
 *    the launched shmem manager normally is executed.@n
 *   Initialize the management information manager_infos.@n
 *   If this API is not called, fpga_shmem_manager_init() fails.
 */
int fpga_shmem_set_signal(void);

/**
 * @brief API which launch shmem_manager
 * @param[in] file_prefix
 *   DPDK's file_prefix
 * @param[in] socket_limit
 *   Request Hugepage size(GB) per a socket
 * @param[in] lcore_mask
 *   Request logic cpu core
 * @param[in] clb
 *   Callback function for detecting secondary process finish
 * @param[in] arg
 *   Argument for call back function
 * @retval 0
 *   Success
 * @retval -NOT_REGISTERED_SIGNAL
 *   e.g.) fpga_shmem_set_signal() is not called yet
 * @retval -FAILURE_MKDIR
 *   e.g.) Failed to mkdir()
 * @retval -FAILURE_FORK
 *   e.g.) Failed to fork()
 * @retval -INVALID_ARGUMENT
 *   e.g.) The same `file_prefix` exist, `socket_limit` is over avaliable pages
 *
 * @details
 *   Fork process and initialize DPDK at the child process(i.e. background).@n
 *   The parent (own) process judges whether the DPDK initialization
 *    in the child process has started successfully, If successful,
 *    it registers child process's info as managed data and returns to
 *    the caller of the function.@n
 *   For normal termination of the child process,
 *    fpga_shmem_manager_finish() should be called.@n
 *   The child process executes the callback function in 500ms cycles
 *    to check whether the signal has been notified,
 *    after DPDK initialization and notification of the result to the parent process.@n
 *   When the callback function returns a non-zero value or a signal is notified,
 *    the DPDK is finalized and the child process is terminated.@n
 *   If the callback function requires an argument, pass it as a generic pointer(arg).@n
 *   If the callback function is NULL, only the signal is acknowledged.@n
 */
int fpga_shmem_manager_init(
        const char *file_prefix,
        const uint32_t socket_limit[],
        const bool lcore_mask[],
        const shmem_func clb,
        void *arg);

/**
 * @brief API which finish shmem manager
 * @param[in] file_prefix
 *   DPDK's file_prefix
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `file_prefix` is null.
 * @retval -MISMATCH_FILE_PREFIX
 *   e.g.) `file_prefix` not found.
 * @retval -LIBFPGA_FATAL_ERROR
 *   e.g.) pid is invalid, normally not set value.
 * @retval -FAILURE_SEND_SIGNAL
 *   e.g.) Failed to kill()
 *
 * @details
 *   Send SIGUSER1 to the shmem manager associated with file_prefix.
 */
int fpga_shmem_manager_finish(
        const char *file_prefix);

/**
 * @brief API which finish all shmem manager
 * @param void
 * @retval 0
 *   Success
 * @retval -LIBFPGA_FATAL_ERROR
 *   e.g.) pid is invalid, normally not set value.@n
 *   e.g.) Failed to verify that all child processes died.
 * @retval -FAILURE_SEND_SIGNAL
 *   e.g.) Failed to kill()
 *
 * @details
 *   Call fpga_shmem_manager_finish() using all currently managed file_prefixes.
 *   When error has occured in fpga_shmem_manager_finish(),
 *    stop this API and return the value without stopping the other processes.
 */
int fpga_shmem_manager_finish_all(void);


/**
 * @brief Function which print management data or copy to arg
 */
void fpga_shmem_dump_manager_infos(
        fpga_shmem_manager_info_t *data);

/**
 * @brief Function which get pid from managed infomation
 */
pid_t fpga_shmem_get_pid_from_prefix(
        const char* file_prefix);

/**
 * @brief Function which get Numa node num
 */
int fpga_shmem_get_socket_num(void);

/**
 * @brief Function which set lcore num
 */
int fpga_shmem_set_lcore_limit(
        int lcore_limit);

/**
 * @brief Function which get lcore num
 */
int fpga_shmem_get_lcore_limit(void);

/**
 * @brief Function which change TOTAL available page for manager
 */
int fpga_shmem_set_available_limit(
        const uint32_t socket_limit[]);

/**
 * @brief Function which check how many page are TOTAL available for manager
 */
int fpga_shmem_get_available_limit(void);

/**
 * @brief Function which check how many page are NOW available for manager
 */
int fpga_shmem_get_available_pages(void);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBSHMEM_MANAGER_H_
