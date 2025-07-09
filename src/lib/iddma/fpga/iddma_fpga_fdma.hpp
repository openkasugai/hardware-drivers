/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef __IDDMA_FPGA_FDMA_H__

#define __IDDMA_FPGA_FDMA_H__

#include <iddma_fpga.hpp>
#include <libdmacommon.h>
#include <map>
#include <thread>
#include <mutex> // NO_LINT
#include <condition_variable> // NO_LINT

class iddma_fpga_fdma : public iddma_fpga {
public:
    explicit iddma_fpga_fdma(iddma_device_type device_type);
    virtual ~iddma_fpga_fdma(void);

    void finalize(void);

protected:
    // iddma_common functions
    virtual void on_close(void);
    virtual void post_close(void);
    virtual iddma_status allocate_queue_set(iddma_queue_set **queue_set);
    virtual void free_queue_set(iddma_queue_set *queue_set);
    virtual iddma_status poll_send_and_recv(uint64_t timeout_sec, iddma_queue_element *data, bool is_send);

    // iddma_fpga functions
    virtual iddma_status register_vaddress_translation(void* cp_vaddr, uint64_t vaddr, uint32_t size,
                                                       void* token, uint32_t token_size);
    virtual iddma_status register_paddress_translation(uint64_t vaddr, uint64_t paddr, uint32_t size);
    virtual bool check_option(const std::string& key, const std::string& value);

private:
    void worker(uint32_t prev_rsize);

protected:
    iddma_status initialize_h2d_fdma(void);
    iddma_status initialize_d2h_fdma(void);
    uint64_t conv_to_eaddr(void* paddr);

    iddma_status enable_tcp_ptu(void);
    virtual iddma_status enable_dma_engine(void);

    // static const variables
    static const uint32_t s_connector_name_max_ = 128;
    static const int s_dequeue_timeout_count_;
    static const int s_fdma_cid_base_ = 512;

    // variables
    uint32_t stream_dev_id_;
    uint32_t stream_h2d_ch_;
    uint32_t stream_d2h_ch_;
    std::string stream_h2d_name_;
    std::string stream_d2h_name_;
    dma_info_t stream_h2d_ch_info_;
    dma_info_t stream_d2h_ch_info_;
    dma_info_t stream_h2d_info_;
    dma_info_t stream_d2h_info_;
    int stream_ptu_id_;
    int stream_kernel_id_;
    int stream_func_id_;

    uint64_t counterpart_queue_set_paddr_;
    std::map<uint64_t, std::pair<uint64_t, uint32_t>> paddress_translation_table_; // vaddr -> paddr, size

    // variables for sw mode
    uint64_t counterpart_queue_set_vaddr_;
    std::map<uint64_t, std::pair<uint64_t, uint32_t>> vaddress_translation_table_; // vaddr -> vaddr, size
    std::unique_ptr<std::thread> worker_;
    bool finalize_;

    std::mutex mutex_;
    std::condition_variable cond_;
};

#endif
