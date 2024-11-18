/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libdpdkutil.h
 * @brief Header file for functions for wrap DPDK APIs
 */

#ifndef LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBDPDKUTIL_H_
#define LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBDPDKUTIL_H_

#include <rte_malloc.h>
#include <rte_common.h>
#include <rte_version.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function which wrap rte_eal_init() for google mock
 * @details See rte_eal_init().
 */
int fpgautil_rte_eal_init(int argc, char **argv);

/**
 * @brief Function which wrap rte_eal_cleanup() for google mock
 * @details See rte_eal_cleanup().
 */
int fpgautil_rte_eal_cleanup(void);

/**
 * @brief Function which wrap rte_malloc() for google mock
 * @details See rte_malloc().
 */
void *fpgautil_rte_malloc(const char *type, size_t size, unsigned align);

/**
 * @brief Function which wrap rte_free() for google mock
 * @details See rte_free().
 */
void fpgautil_rte_free(void *ptr);

/**
 * @brief Function which wrap rte_mem_virt2memseg_list() for google mock
 * @details See rte_mem_virt2memseg_list().
 */
struct rte_memseg_list *fpgautil_rte_mem_virt2memseg_list(const void *virt);

/**
 * @brief Function which wrap rte_mem_virt2memseg() for google mock
 * @details See rte_mem_virt2memseg().
 */
struct rte_memseg *fpgautil_rte_mem_virt2memseg(const void *virt, const struct rte_memseg_list *msl);

/**
 * @brief Function which wrap rte_mem_virt2phy() for google mock
 * @details See rte_mem_virt2phy().
 */
phys_addr_t fpgautil_rte_mem_virt2phy(const void *virt);

/**
 * @brief Function which wrap rte_version() for google mock
 * @details See rte_version().
 */
const char *fpgautil_rte_version(void);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFPGA_INTERNAL_LIBDPDKUTIL_H_
