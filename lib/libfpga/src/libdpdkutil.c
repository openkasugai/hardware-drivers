/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfpga_internal/libdpdkutil.h>

int fpgautil_rte_eal_init(int argc, char **argv) {
    return rte_eal_init(argc, argv);
}

int fpgautil_rte_eal_cleanup(void) {
    return rte_eal_cleanup();
}

void *fpgautil_rte_malloc(const char *type, size_t size, unsigned align) {
    return rte_malloc(type, size, align);
}

void fpgautil_rte_free(void *ptr) {
    return rte_free(ptr);
}

struct rte_memseg_list *fpgautil_rte_mem_virt2memseg_list(const void *virt) {
    return rte_mem_virt2memseg_list(virt);
}

struct rte_memseg *fpgautil_rte_mem_virt2memseg(const void *virt, const struct rte_memseg_list *msl) {
    return rte_mem_virt2memseg(virt, msl);
}

phys_addr_t fpgautil_rte_mem_virt2phy(const void *virt) {
    return rte_mem_virt2phy(virt);
}

