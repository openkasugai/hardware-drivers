/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#if defined(__SUPPORT_GPU_NV__) || defined(__USE_GPU_NV__)
#include <cuda_runtime.h>
#endif
#include <mem_manage.h>
#include <cstdint>
#include <map>
#include <vector>
#include <memory>
#include <mutex> // NOLINT
#include <iddma_cuda_util.hpp>

#if defined(__SUPPORT_GPU_NV__) || defined(__USE_GPU_NV__)
bool operator<(const cudaIpcMemHandle_t& a, const cudaIpcMemHandle_t& b) {
    for (int i=0; i<sizeof(cudaIpcMemHandle_t); i+=sizeof(uint64_t)) {
        uint64_t la = *((const uint64_t*)((const uint8_t*)&a + i));
        uint64_t lb = *((const uint64_t*)((const uint8_t*)&b + i));
        if (la < lb) return true;
        if (la > lb) return false;
    }
    return false;
}
#endif

iddma_cuda_util::iddma_cuda_util(void)
    : mem_manage_obj_(memManageCreate()) {}

iddma_cuda_util::~iddma_cuda_util(void) {
#if defined(__SUPPORT_GPU_NV__) || defined(__USE_GPU_NV__)
    for (auto& it : ipc_ptrs_) {
        cudaIpcCloseMemHandle(it.second);
    }
#endif
    auto itr = ipc_tokens_.begin();
    while (itr != ipc_tokens_.end()) {
        auto citr = itr++;
        unregister_memory(citr->first);
    }
    memManageDestroy(mem_manage_obj_);
}

iddma_status iddma_cuda_util::register_memory(void* dev_ptr, size_t size,
                                              void** token, size_t* token_size, iddma_mmap_memory_type *type) {
    try {
        MemManageStatus mm_stat;
        uint64_t dev_token;
        bool is_ipc_handle = false;
#if defined(__SUPPORT_GPU_NV__) || defined(__USE_GPU_NV__)
        cudaIpcMemHandle_t handle;
        cudaError_t stat = cudaIpcGetMemHandle(&handle, dev_ptr);
        is_ipc_handle = stat == cudaSuccess;
        if (!is_ipc_handle) {
            // reset previous intended cudaIpcGetMemHandle Error (if this error remains, it may affect other libraries.)
            stat = cudaGetLastError();
            is_ipc_handle = stat == cudaSuccess;
        }
#endif
        if (!is_ipc_handle) { // This buffer is not on device, but might be on host mem.
            mm_stat = memManagePinHostDevBuffer(mem_manage_obj_, dev_ptr, size, &dev_token);
            if (mm_stat != kMemManage_Success)
                return KIDDMA_ERROR_BUFFER_IPC_GET_FAILED;
            std::lock_guard<std::mutex> lock(mutex_);
            ipc_tokens_[dev_ptr].resize(sizeof(uint64_t));
            memcpy(ipc_tokens_[dev_ptr].data(), &dev_token, sizeof(uint64_t));
            *token = ipc_tokens_[dev_ptr].data();
            *token_size = ipc_tokens_[dev_ptr].size();
            *type = KIDDMA_MEMTYPE_HOST;
            mm_types_[dev_ptr] = *type;
            return KIDDMA_SUCCESS;
        }

#if defined(__SUPPORT_GPU_NV__) || defined(__USE_GPU_NV__)
        // Buffer is allocated on device, so pin it and get ipc token.
        mm_stat = memManagePinCudaDevBuffer(mem_manage_obj_, dev_ptr, size, &dev_token);
        if (mm_stat != kMemManage_Success) return KIDDMA_ERROR_BUFFER_PIN_FAILED;

        std::lock_guard<std::mutex> lock(mutex_);
        ipc_tokens_[dev_ptr].resize(sizeof(cudaIpcMemHandle_t) + sizeof(uint64_t));
        memcpy(&ipc_tokens_[dev_ptr][0], &dev_token, sizeof(uint64_t));
        memcpy(&ipc_tokens_[dev_ptr][sizeof(uint64_t)], &handle, sizeof(cudaIpcMemHandle_t));
        *token = ipc_tokens_[dev_ptr].data();
        *token_size = ipc_tokens_[dev_ptr].size();
        *type = KIDDMA_MEMTYPE_DEVICE;
        mm_types_[dev_ptr] = *type;
        return KIDDMA_SUCCESS;
#else
        // unreachable
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
#endif
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_cuda_util::unregister_memory(void* dev_ptr) {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = ipc_tokens_.find(dev_ptr);
        if (it == ipc_tokens_.end()) return KIDDMA_ERROR_INVALID_OPERATION;
        MemManageStatus mm_stat;
        if (mm_types_[dev_ptr] == KIDDMA_MEMTYPE_HOST) {
            mm_stat = memManageUnpinHostDevBuffer(mem_manage_obj_, dev_ptr, false);
        } else {
            mm_stat = memManageUnpinCudaDevBuffer(mem_manage_obj_, dev_ptr, false);
        }
        if (mm_stat != kMemManage_Success) return KIDDMA_ERROR_BUFFER_UNPIN_FAILED;
        ipc_tokens_.erase(it);
        return KIDDMA_SUCCESS;
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_cuda_util::get_virt_addr(void* token, size_t token_size,
                                            size_t buf_size, void** virt_addr) {
    // Any token obtained by iddma_cuda_util::register_memory() has two patterns.
    // i ) buffer is on host, and thus only pinned memory token can be obtained.
    //     token size is just sizeof(uint64_t).
    // ii) buffer is on device, and thus cudaIpcMemHandle_t handle and pinned memory by GPUDirect RDMA API can be obtained.
    //     token size is sizeof(cudaIpcMemHandle_t) + sizeof(uint64_t)
    try {
#if defined(__SUPPORT_GPU_NV__) || defined(__USE_GPU_NV__)
        if (!token || (token_size != sizeof(uint64_t) && token_size != (sizeof(cudaIpcMemHandle_t) + sizeof(uint64_t))))
            return KIDDMA_ERROR_INVALID_TOKEN;
#else
        if (!token || token_size < sizeof(uint64_t)) return KIDDMA_ERROR_INVALID_TOKEN;
#endif

        std::lock_guard<std::mutex> lock(mutex_);
#if defined(__SUPPORT_GPU_NV__) || defined(__USE_GPU_NV__)
        cudaError_t stat;
        cudaIpcMemHandle_t handle;
        if (token_size == sizeof(cudaIpcMemHandle_t) + sizeof(uint64_t)) { // buffer is on device
            memcpy(&handle, (const char*)token + sizeof(uint64_t), sizeof(cudaIpcMemHandle_t));
            stat = cudaIpcOpenMemHandle(virt_addr, handle, cudaIpcMemLazyEnablePeerAccess);
            if (stat != cudaSuccess)
                return KIDDMA_ERROR_INVALID_TOKEN;
            ipc_ptrs_[token] = *virt_addr;
        }
#endif
        void* drv_virt_addr;
        uint64_t* cur_token = reinterpret_cast<uint64_t*>(reinterpret_cast<char*>(token));

        MemManageStatus mm_stat = memManageGetPinnedDevBuffer(mem_manage_obj_, *cur_token, buf_size, &drv_virt_addr);
        if (mm_stat != kMemManage_Success) {
#if defined(__SUPPORT_GPU_NV__) || defined(__USE_GPU_NV__)
            if (stat == cudaSuccess && token_size == sizeof(cudaIpcMemHandle_t) + sizeof(uint64_t)) return KIDDMA_SUCCESS;
#endif
            return KIDDMA_ERROR_BUFFER_GET_VADDR_FAILED;
        }

        mm_tokens_[*cur_token] = drv_virt_addr;
        if (token_size == sizeof(uint64_t)) {
            *virt_addr = drv_virt_addr;
            return KIDDMA_SUCCESS;
        }
#if defined(__SUPPORT_GPU_NV__) || defined(__USE_GPU_NV__)
        if (stat == cudaErrorInvalidValue) {
            *virt_addr = drv_virt_addr;
            return KIDDMA_ERROR_BUFFER_IPC_OPEN_FAILED;
        }
#endif
        return KIDDMA_SUCCESS;
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_cuda_util::get_mem_manage_token_iterator(void* token, size_t token_size,
                                                            std::map<uint64_t, void*>::iterator* it) {
    if (!token || token_size < sizeof(uint64_t)) return KIDDMA_ERROR_INVALID_TOKEN;

    uint64_t* cur_token = reinterpret_cast<uint64_t*>(reinterpret_cast<char*>(token));
    *it = mm_tokens_.find(*cur_token);
    return KIDDMA_SUCCESS;
}


iddma_status iddma_cuda_util::get_phys_addrs(void* token, size_t token_size,
                                             uint32_t max, uint32_t* num, uint64_t* phys_addrs, uint32_t* sizes,
                                             uint64_t* vaddr) {
    // for SW stream engine
    try {
        std::map<uint64_t, void*>::iterator it;

        std::lock_guard<std::mutex> lock(mutex_);
        iddma_status ret = get_mem_manage_token_iterator(token, token_size, &it);

        if (ret) return ret;
        if (it == mm_tokens_.end()) return KIDDMA_ERROR_INVALID_OPERATION;
        if (vaddr) *vaddr = reinterpret_cast<uint64_t>(it->second);

        MemManageStatus mm_stat = memManageGetPinnedDevPhysAddrs(mem_manage_obj_, it->second, max,
                                                                 num, phys_addrs, sizes);
        if (mm_stat != kMemManage_Success) return KIDDMA_ERROR_BUFFER_GET_PADDR_FAILED;
        return KIDDMA_SUCCESS;
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_cuda_util::get_token64(const void* token, size_t token_size, uint64_t* token64) {
    try {
        if (!token64) return KIDDMA_ERROR_INVALID_ARGUMENT;
        if (token_size < sizeof(uint64_t)) return KIDDMA_ERROR_INVALID_TOKEN;
        *token64 = *reinterpret_cast<const uint64_t*>(reinterpret_cast<const char*>(token));
        return KIDDMA_SUCCESS;
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

iddma_status iddma_cuda_util::release_addrs(void* token, size_t token_size) {
    try {
        std::map<uint64_t, void*>::iterator it;

        std::lock_guard<std::mutex> lock(mutex_);
        iddma_status ret = get_mem_manage_token_iterator(token, token_size, &it);
        if (ret || it == mm_tokens_.end()) return ret;

        MemManageStatus mm_stat = memManageReleasePinnedDevBuffer(mem_manage_obj_, it->second);
        if (mm_stat != kMemManage_Success) return KIDDMA_ERROR_BUFFER_RELEASE_ADDR_FAILED;
        mm_tokens_.erase(it);
        return KIDDMA_SUCCESS;
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}