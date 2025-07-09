/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <iddma_fpga.hpp>
#include <iddma_cuda_util.hpp>

iddma_fpga::iddma_fpga(iddma_device_type device_type)
    : iddma_common(device_type) {}

iddma_fpga::~iddma_fpga(void) {
    finalize();
}

void iddma_fpga::finalize(void) {
    iddma_common::finalize();
}

iddma_device_object iddma_fpga::get_device_object(void) {
    return nullptr;
}

iddma_status iddma_fpga::mmap_populate(void *addr, size_t size) {
    return KIDDMA_ERROR_UNSUPPORTED;
}

iddma_status iddma_fpga::register_vaddress_translation(void* cp_vaddr, uint64_t vaddr, uint32_t size,
                                                       void* token, uint32_t token_size) {
    return KIDDMA_ERROR_UNSUPPORTED;
}

iddma_status iddma_fpga::register_paddress_translation(uint64_t vaddr, uint64_t paddr, uint32_t size) {
    return KIDDMA_ERROR_UNSUPPORTED;
}

bool iddma_fpga::check_option(const std::string& key, const std::string& value) {
    return iddma_common::check_option(key, value);
}

iddma_status iddma_fpga::post_mmap_share(void) {
#ifdef _DEBUG
    printf("imported: %lu\n", memory_map_import_.size());
#endif
    for (auto itr = memory_map_import_.begin(); itr != memory_map_import_.end(); itr++) {
        void *cu_addr = nullptr;
#ifdef _DEBUG
        printf_debug("addr: 0x%lx, size: 0x%lx, token_ptr: 0x%lx, token_size: %ld\n",
                     itr->addr, itr->size, itr->token_ptr, itr->token_size);
#endif
        iddma_status ret;
        ret = cuda_util_->get_virt_addr((void*)itr->token_ptr, itr->token_size,
                                        itr->size, &cu_addr);
        if (ret != KIDDMA_SUCCESS) return ret;

        uint32_t max = s_max_paddrs_;
        uint32_t num;
        std::vector<uint64_t> paddrs(max);
        std::vector<uint32_t> psizes(max);
        uint64_t vaddr;
        ret = cuda_util_->get_phys_addrs((void*)itr->token_ptr, itr->token_size,
                                         max, &num, &paddrs[0], &psizes[0], &vaddr);
        if (ret != KIDDMA_SUCCESS) return ret;
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga::send_and_recv(void *addr, size_t size_byte, uint64_t imm, bool is_send) {
    return KIDDMA_ERROR_UNSUPPORTED;
}

iddma_status iddma_fpga::poll_send_and_recv(uint64_t timeout_sec, iddma_queue_element *data, bool is_send) {
    return KIDDMA_ERROR_UNSUPPORTED;
}

void iddma_fpga::on_close(void) {}

void iddma_fpga::post_close(void) {}

iddma_status iddma_fpga::allocate_queue_set(iddma_queue_set **queue_set) {
    dummy_queue_set_.reset(new iddma_queue_set());
    return KIDDMA_SUCCESS;
}

void iddma_fpga::free_queue_set(iddma_queue_set *queue_set) {}
