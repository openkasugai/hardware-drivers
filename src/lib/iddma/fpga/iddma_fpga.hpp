/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef __IDDMA_FPGA_H__
#define __IDDMA_FPGA_H__

#include <iddma_common.hpp>
#include <iddma_cuda_util.hpp>

class iddma_fpga : public iddma_common {
public:
    iddma_fpga(iddma_device_type device_type);
    virtual ~iddma_fpga(void); // for more subclass

    virtual void finalize(void);

    iddma_device_object get_device_object(void);
    iddma_status mmap_populate(void *addr, size_t size);

protected:
    // iddma_common functions
    virtual void on_close(void);
    virtual void post_close(void);
    virtual iddma_status allocate_queue_set(iddma_queue_set **queue_set);
    virtual void free_queue_set(iddma_queue_set *queue_set);
    virtual iddma_status poll_send_and_recv(uint64_t timeout_sec, iddma_queue_element *data, bool is_send);

    // iddma_fpga functions
    virtual iddma_status register_vaddress_translation(void* cp_vaddr, uint64_t vaddr, uint32_t size, void* token, uint32_t token_size);
    virtual iddma_status register_paddress_translation(uint64_t vaddr, uint64_t paddr, uint32_t size);
    virtual bool check_option(const std::string& key, const std::string& value);

private:
    // iddma_common functions
    iddma_status post_mmap_share(void);
    iddma_status send_and_recv(void *addr, size_t size_byte, uint64_t imm, bool is_send);

    // static const variables
    static const uint32_t s_max_paddrs_ = 8;

    // variables
    std::unique_ptr<iddma_queue_set> dummy_queue_set_;
};

#endif
