/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libshmem.h
 * @brief Header file for shared memory
 */

#ifndef LIBFPGA_INCLUDE_LIBSHMEM_H_
#define LIBFPGA_INCLUDE_LIBSHMEM_H_

#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Max num of Hugepages avaliable by this library
 */
#define SHMEM_MAX_HUGEPAGES               32

/**
 * Max len of DPDK's prefix available by this library
 */
#define SHMEM_MAX_HUGEPAGE_PREFIX         64

/**
 * Max num of socket available by this library
 */
#define SHMEM_MAX_SOCKET_LIMIT            16

/**
 * Max num of NUMA node available by this library
 */
#define SHMEM_MAX_NUMA_NODE               16

/**
 * Max num of logic cpu core available by this library
 */
#define SHMEM_MAX_LCORE                   128

/**
 * Max len of file name available by this library
 */
#define SHMEM_MAX_FILE_NAME_LEN           MAXNAMLEN

/**
 * For alignment DMA's data memory
 */
#define SHMEM_BOUNDARY_SIZE               1024

/**
 * Minmum size of buffer for DMA data transfer
 */
#define SHMEM_MIN_SIZE_BUF                4096

/**
 * Definition of DPDK's default file_prefix
 */
#define SHMEM_DPDK_DEFAULT_PREFIX         "rte"

/**
 * Format for file to detect secondary process's dead
 */
#define SHMEM_FMT_FLOCK_FILE              "/var/run/dpdk/%s/.lock"

/**
 * Format for file to check if the primary/secondary process's DPDK version is the same
 */
#define SHMEM_FMT_VERSION_FILE            "/var/run/dpdk/%s/.version"

/**
 * Format for directory of NUMA node informations
 */
#define SHMEM_FMT_NUMA_NODE_DIRECTORY     "/sys/devices/system/node/node%d"

/**
 * Format for file of the num of free 1Gi Hugepage at one of the node
 */
#define SHMEM_FMT_NUMA_NODE_FREE_HUGEPAGE "/sys/devices/system/node/node%d/hugepages/hugepages-1048576kB/free_hugepages"

/**
 * Format for file of a cpulist at one of the node
 */
#define SHMEM_FMT_NUMA_NODE_CPULIST       "/sys/devices/system/node/node%d/cpulist"


/**
 * @brief API which initialize DPDK as secondary process
 * @param[in] file_prefix
 *   DPDK's file_prefix(if NULL, default "rte")
 * @param[in] lcore_mask
 *   Request logic cpu core(if NULL, not set this option)
 * @param[in] rte_log_flag
 *   DPDK's log flag(1:enabled/0:disabled)
 * @retval 0
 *   Success
 * @retval -LIBFPGA_FATAL_ERROR
 *   e.g.) Failed to detect NUMA Node num
 * @retval -FAILURE_OPEN
 *   e.g.) Failed to open system's file
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate temporary memory
 * @retval -FAILURE_INITIALIZE
 *   e.g.) Failed to create notification file for secondary process
 * @retval -INVALID_OPERATION
 *   e.g.) Failed to fseek()
 * @retval -INVALID_PARAMETER
 *   e.g.) The version file exist, but nothing is written
 * @retval -FAILURE_READ
 *   e.g.) Failed to read()
 * @retval -INVALID_DATA
 *   e.g.) DPDK's version is different
 * @return retval of rte_eal_init() if failed in rte_eal_init()
 *
 * @details
 *   Call rte_eal_init() as secondary process.@n
 *    1. Read DPDK's version file created by fpga_shmem_init_sys() and
 *        if DPDK's versions between primary and secondary are different,
 *        return failure before calling rte_eal_init().@n
 *    2. Call rte_eal_init() as secondary.@n
 *    3. Create file for locking and shared-lock it to notice primary process
 *        that this prefix's Hugepage is now using.@n
 *   DPDK options: --file-prefix,-c,--log-level=lib.eal:debug
 */
int fpga_shmem_init(
        const char *file_prefix,
        const bool lcore_mask[],
        int rte_log_flag);

/**
 * @brief API which initialize DPDK as primary process
 * @param[in] file_prefix
 *   DPDK's file_prefix(if NULL, default "rte")
 * @param[in] huge_dir
 *   Request hugepage directory(if NULL, default "/dev/hugepages")
 * @param[in] socket_limit
 *   Request Hugepage size(GB) per a socket(if NULL, no limit)
 * @param[in] lcore_mask
 *   Request logic cpu core(if NULL, not set this option)
 * @param[in] rte_log_flag
 *   DPDK's log flag(1:enabled/0:disabled)
 * @retval 0
 *   Success
 * @retval -LIBFPGA_FATAL_ERROR
 *   e.g.) Failed to detect NUMA Node num
 * @retval -FAILURE_OPEN
 *   e.g.) Failed to open system's file
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate temporary memory
 * @retval -FAILURE_WRITE
 *   Failed to write DPDK's version into the file
 * @return retval of rte_eal_init() if failed in rte_eal_init()
 *
 * @details
 *   Call rte_eal_init() as primary process.@n
 *    1. Call rte_eal_init() as primary.@n
 *    2. Create file for DPDK's version file,
 *        if failed to create, call rte_eal_cleanup() and return failure.@n
 *   DPDK options: --file-prefix,--huge-dir,--socket-limit,-c,--log-level=lib.eal:debug
 */
int fpga_shmem_init_sys(
        const char *file_prefix,
        const char *huge_dir,
        const uint32_t socket_limit[],
        const bool lcore_mask[],
        int rte_log_flag);

/**
 * @brief API which allocate memory from Hugepage
 * @param[in] length
 *   Request size
 * @retval address
 *   Allocated memory address
 * @retval NULL
 *   Failed to allocate memory
 *
 * @details
 *   Allocate shared memory(64bytes aligned) less than 1Gi.
 */
void *fpga_shmem_alloc(
        size_t length);

/**
 * @brief API which allocate memory from Hugepage
 * @param[in] length
 *   Request size
 * @retval address
 *   Allocated memory address
 * @retval NULL
 *   Failed to allocate memory
 *
 * @details
 *   Allocate shared memory(1024bytes aligned) less than 1Gi.
 */
void *fpga_shmem_aligned_alloc(
        size_t length);

/**
 * @brief API which free memory from Hugepage
 * @param[in] addr
 *   Allocated memory address
 * @retval void
 *
 * @details
 *   Free shared memory allocated by fpga_shmem_alloc(), fpga_shmem_aligned_alloc()
 */
void fpga_shmem_free(
        void *addr);

/**
 * @brief API which finalize DPDK
 * @param void
 * @return retval of rte_eal_cleanup()
 * @retval 0
 *   Success
 *
 * @details
 *   Call rte_eal_cleanup().@n
 *   When initialize API was fpga_shmem_init_sys(),
 *    this API delete DPDK's version file created by fpga_shmem_init_sys() after rte_eal_cleanup().@n
 *   When initialize API was fpga_shmem_init(),
 *    this API unlock the file create by fpga_shmem_init() after rte_eal_cleanup().@n
 */
int fpga_shmem_finish(void);

/**
 * @brief Function which register data by linking logical and physical addresses and size.
 */
int fpga_shmem_register(
        void* vaddr,
        uint64_t paddr,
        size_t size);

/**
 * @brief Function which update registerd data
 */
int fpga_shmem_register_update(
        void *addr,
        uint64_t paddr,
        size_t size);

/**
 * @brief Function which unregister data by logical addresses
 */
void fpga_shmem_unregister(
        void* vaddr);

/**
 * @brief Function which unregister all data
 */
void fpga_shmem_unregister_all(void);

/**
 * @brief Function which check if logical address is in the registerd regions
 */
int __fpga_shmem_register_check(
        void *addr);

/**
 * @brief Function which convert virtual addr to physical addr from local v2p map
 *        If the entity of the pointer variable `len` is too large,
 *         shorten it to fit within the registered size.
 */
uint64_t __fpga_shmem_mmap_v2p(
        void *va,
        uint64_t *len);

/**
 * @brief Function which convert physical addr to virtual addr from local p2v map
 */
void *__fpga_shmem_mmap_p2v(
        uint64_t pa64);

/**
 * @brief old name of __fpga_shmem_mmap_v2p()
 */
uint64_t __dma_pa_from_va(
        void *va,
        uint64_t *len);

/**
 * @brief old name of __fpga_shmem_mmap_p2v()
 */
void *local_phy2virt(
        uint64_t pa64);

/**
 * @brief Function which check alignment and call __dma_pa_from_va
 */
uint64_t dma_pa_from_va(
        void *va,
        uint64_t *len);

/**
 * @brief Debug function which call rte_eal_init() directly
 */
int fpga_shmem_init_arg(
        int argc,
        char **argv);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBSHMEM_H_
