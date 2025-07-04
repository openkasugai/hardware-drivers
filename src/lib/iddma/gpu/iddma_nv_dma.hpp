/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_nv_dma.hpp
 * @brief Nv DMA engine class object of iddma.
 *
 */

#ifndef __IDDMA_NV_DMA_HPP__
#define __IDDMA_NV_DMA_HPP__

#include <iddma_engine.hpp>
#include <cuda_runtime.h>
#include <string>
#include <thread>
#include <vector>
#include <mutex> // NOLINT
#include <condition_variable> // NOLINT

struct mmap_share_data;
class iddma_memory_map;

class iddma_nv_dma : public iddma_engine {
public:
    iddma_nv_dma(iddma_queue_set* queue_set, iddma_queue_set* cp_queue_set,
                 const iddma_memory_map& memmap,
                 bool& closed,
                 const std::vector<mmap_share_data>& mmap_exports,
                 const std::vector<mmap_share_data>& mmap_imports,
                 bool send_enable, bool recv_enable, uint32_t poll_interval_ns);
    virtual ~iddma_nv_dma(void);

    virtual void wakeup_workers(void);
    virtual iddma_status write_queue_element(iddma_transfer_destination type, int idx, const iddma_queue_element* src);
    virtual iddma_status write_queue_head(iddma_transfer_destination type, uint32_t head);
    virtual iddma_status write_queue_tail(iddma_transfer_destination type, uint32_t tail);

protected:
    virtual bool update(void* dst, const void* src, uint32_t size,
                        cudaStream_t stream = nullptr, bool async = false, bool* fail = nullptr);

    const iddma_memory_map& memmap_;
    const std::vector<mmap_share_data>& mmap_exports_;
    const std::vector<mmap_share_data>& mmap_imports_;

private:
    void initialize(bool send_enable, bool recv_enable);

    void inc_pos(uint32_t& pos);
    bool check_valid(iddma_queue& queue, uint32_t* head_tail);
    bool check_ready(iddma_queue& queue, uint32_t* head_tail);
    bool transfer(iddma_queue& srq, iddma_queue& scq, iddma_queue& rrq, iddma_queue& rcq,
                  uint32_t* sr_head_tail, uint32_t* sc_head_tail, uint32_t* rr_head_tail, uint32_t* rc_head_tail,
                  iddma_queue_element *sqe, iddma_queue_element *rqe,
                  bool src_valid, bool src_ready, bool dst_valid, bool dst_ready, bool rev_dir_closed);

    void worker_func(bool send_enable, bool recv_enable);

    iddma_queue_set* queue_set_;
    iddma_queue_set* cp_queue_set_;

    void* pinned_buf_;
    cudaStream_t stream_;
    bool finalize_;
    bool& closed_;
    std::unique_ptr<std::thread> worker_;
    uint32_t poll_interval_us_;

    std::mutex mutex_;
    std::condition_variable cond_;
};

#endif // __IDDMA_NV_DMA_HPP__
