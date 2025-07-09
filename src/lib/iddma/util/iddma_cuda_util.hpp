/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _IDDMA_CUDA_UTIL_HPP__
#define _IDDMA_CUDA_UTIL_HPP__

#include <stddef.h>
#include <iddma_def.h>
#include <iddma_common_def.hpp>
#include <mem_manage.h>
#include <map>
#include <vector>
#include <mutex>

class iddma_cuda_util {
public:
    iddma_cuda_util();
    ~iddma_cuda_util();

    /**
     * register the device memory to share other process or container.
     * @param [in] dev_ptr virtual address for the device memory
     * @param [in] size sizeof the device memory region.
     * @param [out] token pointer for the token.
     * @param [out] token_size size of the token.
     * @return status of the function.
     */
    iddma_status register_memory(void* dev_ptr, size_t size,
                          void** token, size_t* token_size, iddma_mmap_memory_type *type);

    /**
     * unregister the device memory to share other process or container.
     * @param [in] dev_ptr virtual address for the device memory
     * @return status of the function.
     */
    iddma_status unregister_memory(void* dev_ptr);

    /**
     * get virtual address of the shared buffer for the process.
     * if cannot get the address by cudaIpcOpenMemHandle(), return KIDDMA_ERROR_BUFFER_IPC_OPEN_FAILED
     * and the virt_addr can be used to identify the buffer.
     *
     * @param [in] token pointer for the token.
     * @param [in] token_size size of the token.
     * @param [in] buf_size size of the buffer,
     * @param [out] virt_addr the virtual address for the process.
     * @return status of the function.
     */
    iddma_status get_virt_addr(void* token, size_t token_size,
                               size_t buf_size, void** virt_addr);

    /**
     * get physical address of the shared buffer.
     * need to call get_virt_addr before.
     * @param [in] token pointer for the token.
     * @param [in] token_size size of the token.
     * @param [in] max maximum count of physical addresses.
     * @param [out] num count of the physical addressses.
     * @param [out] phys_addrs list of the physical addresses.
     * @param [out] sizes list of the contiguous buffer sizes.
     * @param [out] vaddr virtual address of the process remapped.
     * @return status of the function.
     */
    iddma_status get_phys_addrs(void* token, size_t token_size,
                                uint32_t max, uint32_t* num, uint64_t* phys_addrs, uint32_t* sizes,
                                uint64_t* vaddr);

    /**
     * get the 64bit token of the shared buffer.
     * @param [in] token pointer for the token.
     * @param [in] token_size size of the token.
     * @param [out] token64 64bit token.
     * @return status of the function.
     */
    iddma_status get_token64(const void* token, size_t token_size, uint64_t* token64);

    /**
     * release the shared buffer with physical addresses.
     * @param [in] token pointer for the token.
     * @param [in] token_size size of the token.
     * @return status of the function.
     */
    iddma_status release_addrs(void* token, size_t token_size);

private:
    iddma_status get_mem_manage_token_iterator(void* token, size_t token_size,
                                               std::map<uint64_t, void*>::iterator* it);

    MemManageObj mem_manage_obj_;
    std::map<void*, std::vector<char> > ipc_tokens_;
    std::map<uint64_t, void*> mm_tokens_;
    std::map<void*, void*> ipc_ptrs_; // cudaIpcMemHandle_t typed token -> virt addr
    std::map<void*, iddma_mmap_memory_type> mm_types_;
    std::mutex mutex_;
};

#endif // _IDDMA_CUDA_UTIL_HPP__
