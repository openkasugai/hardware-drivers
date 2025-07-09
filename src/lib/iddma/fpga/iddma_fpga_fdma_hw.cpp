/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <iddma_fpga_fdma_hw.hpp>
#include <iddma_cuda_util.hpp>
#include <libfpgactl.h>
#include <libfdma.h>
#include <libdma.h>
#include <libshmem.h>
#include <chrono>
#include <deque>

//#define _DEBUG

iddma_fpga_fdma_hw::iddma_fpga_fdma_hw(iddma_device_type device_type)
    : iddma_fpga_fdma(device_type),
      check_interval_(-1),
      started_(false) {}

iddma_fpga_fdma_hw::~iddma_fpga_fdma_hw(void) {
    finalize();
}

void iddma_fpga_fdma_hw::finalize(void) {
    on_close();
    iddma_fpga_fdma::finalize();
}

void iddma_fpga_fdma_hw::on_close(void) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (started_) {
            int ret;
            if (!stream_h2d_name_.empty()) {
                ret = fpga_stream_engine_disable(stream_engine_h2d_info_);
                if (!ret) ret = fpga_stream_engine_finish(stream_engine_h2d_info_);
                ret = fpga_fdma_queue_finish(&stream_h2d_info_);
                ret = fpga_fdma_finish(&stream_h2d_ch_info_);
            }
            if (!stream_d2h_name_.empty()) {
                ret = fpga_stream_engine_disable(stream_engine_d2h_info_);
                if (!ret) ret = fpga_stream_engine_finish(stream_engine_d2h_info_);
                ret = fpga_fdma_queue_finish(&stream_d2h_info_);
                ret = fpga_fdma_finish(&stream_d2h_ch_info_);
            }
            started_ = false;
        }
    }
}

iddma_status iddma_fpga_fdma_hw::enable_dma_engine(void) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!started_) {
        int ret;
        iddma_status stat;
        if (tcp_info_.protocol == KIDDMA_DMA_PROTOCOL_DEFAULT) {
            stat = initialize_h2d_fdma();
            if (stat) return stat;
            if (!stream_h2d_name_.empty()) {
                ret = fpga_stream_engine_init(&stream_h2d_info_, &stream_engine_h2d_info_, check_interval_);
                if (ret) return KIDDMA_ERROR_INIT_DMA_ENGINE_TO_DEV_FAILED;
                uint64_t req_q_base(reinterpret_cast<uintptr_t>(counterpart_queue_set_->srq.queue_elements));
                uint64_t cpl_q_base(reinterpret_cast<uintptr_t>(counterpart_queue_set_->scq.queue_elements));
                uint64_t req_q_head_tail(reinterpret_cast<uintptr_t>(&counterpart_queue_set_->srq.head));
                uint64_t cpl_q_head_tail(reinterpret_cast<uintptr_t>(&counterpart_queue_set_->scq.head));
                stat = set_queue(stream_engine_h2d_info_, req_q_base, cpl_q_base, req_q_head_tail, cpl_q_head_tail);
                if (stat) return stat;
                stat = set_buffer_map(stream_engine_h2d_info_);
                if (stat) return stat;
                ret = fpga_stream_engine_enable(stream_engine_h2d_info_);
                if (ret) return KIDDMA_ERROR_START_DMA_ENGINE_TO_DEV_FAILED;
            }
            stat = initialize_d2h_fdma();
            if (stat) return stat;
            if (!stream_d2h_name_.empty()) {
                ret = fpga_stream_engine_init(&stream_d2h_info_, &stream_engine_d2h_info_, check_interval_);
                if (ret) return KIDDMA_ERROR_INIT_DMA_ENGINE_FROM_DEV_FAILED;
                uint64_t req_q_base(reinterpret_cast<uintptr_t>(counterpart_queue_set_->rrq.queue_elements));
                uint64_t cpl_q_base(reinterpret_cast<uintptr_t>(counterpart_queue_set_->rcq.queue_elements));
                uint64_t req_q_head_tail(reinterpret_cast<uintptr_t>(&counterpart_queue_set_->rrq.head));
                uint64_t cpl_q_head_tail(reinterpret_cast<uintptr_t>(&counterpart_queue_set_->rcq.head));
                iddma_status stat = set_queue(stream_engine_d2h_info_, req_q_base, cpl_q_base, req_q_head_tail, cpl_q_head_tail);
                if (stat) return stat;
                stat = set_buffer_map(stream_engine_d2h_info_);
                if (stat) return stat;
                ret = fpga_stream_engine_enable(stream_engine_d2h_info_);
                if (ret) return KIDDMA_ERROR_START_DMA_ENGINE_FROM_DEV_FAILED;
            }
        } else if (tcp_info_.protocol == KIDDMA_DMA_PROTOCOL_TCP &&
                   (tcp_info_.server_mode || !tcp_info_.counter_ip_addr.empty()) && stream_ptu_id_ >= 0) {
            stat = enable_tcp_ptu();
            if (stat) return stat;
        } else {
            return KIDDMA_ERROR_UNSUPPORTED;
        }
        started_ = true;
    }

    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_fdma_hw::set_queue(fpga_stream_engine_t stream_info_, uint64_t req_q_base, uint64_t cpl_q_base,
                                           uint64_t req_q_head_tail, uint64_t cpl_q_head_tail)
{
    uint64_t req_q_token, cpl_q_token, req_q_head_tail_token, cpl_q_head_tail_token;
    uint32_t req_q_offset, cpl_q_offset, req_q_head_tail_offset, cpl_q_head_tail_offset;
    auto ret = get_token_offset(req_q_base, req_q_token, req_q_offset);
    if (ret) return ret;
    ret = get_token_offset(cpl_q_base, cpl_q_token, cpl_q_offset);
    if (ret) return ret;
    ret = get_token_offset(req_q_head_tail, req_q_head_tail_token, req_q_head_tail_offset);
    if (ret) return ret;
    ret = get_token_offset(cpl_q_head_tail, cpl_q_head_tail_token, cpl_q_head_tail_offset);
    if (ret) return ret;
    if (fpga_stream_engine_set_queue_with_token(stream_info_,
                                                req_q_token, req_q_offset, cpl_q_token, cpl_q_offset,
                                                req_q_head_tail_token, req_q_head_tail_offset,
                                                cpl_q_head_tail_token, cpl_q_head_tail_offset, MAX_QUEUE_SIZE, MAX_QUEUE_SIZE)) {
        return KIDDMA_ERROR_SETUP_FAILED;
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_fdma_hw::get_token_offset(uint64_t vaddr, uint64_t& token, uint32_t& offset)
{
    for (auto itr = memory_map_import_.begin(); itr != memory_map_import_.end(); itr++) {
        if ((uintptr_t)itr->addr <= vaddr && vaddr < (uintptr_t)itr->addr + itr->size) {
            offset = vaddr - (uintptr_t)itr->addr;
            return cuda_util_->get_token64((void*)itr->token_ptr, itr->token_size, &token);
        }
    }
    return KIDDMA_ERROR_INVALID_TOKEN;
}

bool iddma_fpga_fdma_hw::check_option(const std::string& key, const std::string& value) {
    int64_t value_num = 0;
    bool is_num = is_num_value(value, value_num);
    if (key == "check_interval" && is_num && value_num >= 0) {
        check_interval_ = value_num;
    } else {
        return iddma_fpga_fdma::check_option(key, value);
    }
    return true;
}

iddma_status iddma_fpga_fdma_hw::set_buffer_map(fpga_stream_engine_t& info)
{
    // under locked state
    for (auto itr = memory_map_import_.begin(); itr != memory_map_import_.end(); itr++) {
        uint64_t token64;
        iddma_status stat = cuda_util_->get_token64((void*)itr->token_ptr,
                                                        itr->token_size, &token64);
        if (stat != KIDDMA_SUCCESS) return stat;
        int ret = fpga_stream_engine_set_buffer(info, (uint64_t)itr->addr, itr->size, token64);
        if (ret) return KIDDMA_ERROR_ADDRESS_TRANSLATION;
    }
    return KIDDMA_SUCCESS;
}
