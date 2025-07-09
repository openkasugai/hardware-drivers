/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_common_def.hpp
 * @brief internal definitions of iddma.
 *
 */

#ifndef __IDDMA_COMMON_DEF_HPP__
#define __IDDMA_COMMON_DEF_HPP__

typedef enum {
    KIDDMA_MEMTYPE_DEVICE = 0,
    KIDDMA_MEMTYPE_HOST = 1,
    KIDDMA_MEMTYPE_HOST_PINNED = 2,
    KIDDMA_MEMTYPE_UNKNOWN = 100,
} iddma_mmap_memory_type;

typedef enum  {
    KIDDMA_TRANSFER_DESTINATION_SRQ = 0,
    KIDDMA_TRANSFER_DESTINATION_RRQ = 1,
    KIDDMA_TRANSFER_DESTINATION_SCQ = 2,
    KIDDMA_TRANSFER_DESTINATION_RCQ = 3,
} iddma_transfer_destination;

struct ipc_info {
    bool is_connected;
    bool is_accepted;
    bool connect_needs_close;
    bool accept_needs_close;
    bool connect_valid;
    bool accept_valid;
    bool connect_ready;
    bool accept_ready;
    uint32_t connect_magic;
    uint32_t accept_magic;
    uint64_t temp_data;
    uintptr_t connector_queue_set;
    uintptr_t acceptor_queue_set;
    iddma_device_type connector_type;
    iddma_device_type acceptor_type;
    uint32_t connector_dev_id;
    uint32_t acceptor_dev_id;
    uint32_t connector_ch_id;
    uint32_t acceptor_ch_id;
    iddma_dma_protocol protocol;
};

struct mmap_share_data {
    uintptr_t addr;
    uint64_t size;
    uintptr_t token_ptr;
    uint64_t token_size;
    iddma_mmap_memory_type mem_type;
};

#endif // __IDDMA_COMMON_DEF_HPP__
