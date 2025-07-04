/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <iddma_fpga_xse.hpp>
#include <chrono>
#include <deque>
#include <stdexcept>
#include <tcp_xse_ctrl_tunnel.hpp>
#include <unistd.h>

//#define _DEBUG

#define ALIGN64(x) ((x) & ~63)

iddma_fpga_xse::iddma_fpga_xse(iddma_device_type device_type)
    : iddma_fpga(device_type),
      stream_dev_id_(0),
      stream_instance_id_(0),
      stream_h2d_ch_(0),
      stream_d2h_ch_(0),
      stream_h2d_valid_(false),
      stream_d2h_valid_(false),
      stream_h2d_obj_(nullptr),
      stream_d2h_obj_(nullptr),
      frame_size_(0),
      credit_num_(8),
      toe_obj_(nullptr),
      check_interval_(-1),
      started_(false),
      finalize_(false) {}

iddma_fpga_xse::~iddma_fpga_xse(void) {
    finalize();
}

void iddma_fpga_xse::finalize(void) {
    on_close();
    iddma_fpga::finalize();
}

void iddma_fpga_xse::on_close(void) {
    xse_status_t ret;
    finalize_ = true;
    if (worker_.get() && worker_->joinable()) {
        worker_->join();
        worker_.reset();
    }
    std::unique_lock<std::mutex> lock(mutex_);
#ifdef _DEBUG
    printf("iddma_fpga_xse::on_close\n");
#endif
    if (stream_h2d_obj_) {
        ret = fpga_stream_engine_disable(stream_h2d_obj_);
#ifdef _DEBUG
        if (ret) printf("stream_engine h2d disable failed: %d\n", ret);
#endif
        ret = fpga_stream_engine_finish(stream_h2d_obj_);
#ifdef _DEBUG
        if (ret) printf("stream_engine h2d finish failed: %d\n", ret);
#endif
    }
    if (stream_d2h_obj_) {
        ret = fpga_stream_engine_disable(stream_d2h_obj_);
#ifdef _DEBUG
        if (ret) printf("stream_engine d2h finish failed: %d\n", ret);
#endif
        ret = fpga_stream_engine_finish(stream_d2h_obj_);
#ifdef _DEBUG
        if (ret) printf("stream_engine d2h finish failed: %d\n", ret);
#endif
    }
    started_ = false;
#ifdef _DEBUG
    printf("iddma_fpga_xse::on_close done\n");
#endif
}

void iddma_fpga_xse::post_close(void) {
    iddma_common::post_close();
}

iddma_status iddma_fpga_xse::allocate_queue_set(iddma_queue_set **queue_set) {
    return iddma_fpga::allocate_queue_set(queue_set);
}

void iddma_fpga_xse::free_queue_set(iddma_queue_set *queue_set) {
    // todo: release queue set of lane and channel.
}

iddma_status iddma_fpga_xse::create_ctrl_tunnel_tcp(bool is_listen, std::unique_ptr<ctrl_tunnel>& tunnel) {
    tcp_xse_ctrl_tunnel* tunnel_obj = nullptr;
    try {
        bool send_enable = base_config_.transfer_mode == KIDDMA_TRANSFER_MODE_SEND;
        bool recv_enable = base_config_.transfer_mode == KIDDMA_TRANSFER_MODE_RECV;
        if (!send_enable && !recv_enable) return KIDDMA_ERROR_UNSUPPORTED;

        int dev = stream_dev_id_;
        int instance = stream_instance_id_;
        int ch;
        if (send_enable) {
            if (stream_d2h_valid_) {
                ch = stream_d2h_ch_;
            } else {
                return KIDDMA_ERROR_INVALID_ARGUMENT;
            }
        } else {
            if (stream_h2d_valid_) {
                ch = stream_h2d_ch_;
            } else {
                return KIDDMA_ERROR_INVALID_ARGUMENT;
            }
        }
        if (is_listen) {
            tunnel_obj = new tcp_xse_ctrl_tunnel(send_enable, dev, instance, ch, base_config_.timeout_sec,
                                                 base_config_.counter_tcp_port, base_config_.self_ip_addr);
        } else {
            tunnel_obj = new tcp_xse_ctrl_tunnel(send_enable, dev, instance, ch, base_config_.timeout_sec,
                                                 base_config_.counter_ip_addr, base_config_.counter_tcp_port,
                                                 base_config_.self_ip_addr);
        }
        toe_obj_ = tunnel_obj->get_toe_object();
        tunnel.reset(tunnel_obj);
    } catch (std::runtime_error& e) {
        return KIDDMA_ERROR_NOT_CONNECTED;
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_xse::enable_tcp_xse(void) {
    if (!frame_size_) return KIDDMA_ERROR_UNSUPPORTED;
    if (!toe_obj_) return KIDDMA_ERROR_INVALID_OPERATION;

    xse_status_t ret;
    bool recv_enable = base_config_.transfer_mode == KIDDMA_TRANSFER_MODE_RECV;
    if (recv_enable) {
        ret = fpga_toe_set_buffer_info(toe_obj_, frame_size_, credit_num_);
        if (ret) return KIDDMA_ERROR_DMA_ENGINE_CREATION_FAILED;
        // W.A. wait until connection established signal arrives to the state table of toe.
        usleep(1000000);
        ret = fpga_toe_activate(toe_obj_);
#ifdef _DEBUG
        printf("activate streaming: %d\n", ret);
#endif
        if (ret) return KIDDMA_ERROR_DMA_ENGINE_CREATION_FAILED;
    }
    if (!worker_.get()) {
        // disable checker thread
        //worker_.reset(new std::thread(&iddma_fpga_xse::check_regs, this));
    }

    return KIDDMA_SUCCESS;
}

void iddma_fpga_xse::check_regs(void)
{
    while (!finalize_) {
        fpga_toe_get_status(toe_obj_);
        usleep(1000000);
    }
}

iddma_status iddma_fpga_xse::enable_pcie_xse(void) {
    iddma_status stat = KIDDMA_SUCCESS;
    bool d2d_mode = false;
    std::unique_lock<std::mutex> lock(mutex_);
    if (stream_h2d_valid_) {
        xse_status_t ret = fpga_stream_engine_init(stream_dev_id_, false, stream_h2d_ch_, &stream_h2d_obj_, check_interval_);
        if (ret) return KIDDMA_ERROR_INIT_DMA_ENGINE_TO_DEV_FAILED;
        if (counterpart_queue_set_) {
            uint64_t req_q_base(reinterpret_cast<uintptr_t>(counterpart_queue_set_->srq.queue_elements));
            uint64_t cpl_q_base(reinterpret_cast<uintptr_t>(counterpart_queue_set_->scq.queue_elements));
            uint64_t req_q_head_tail(reinterpret_cast<uintptr_t>(&counterpart_queue_set_->srq.head));
            uint64_t cpl_q_head_tail(reinterpret_cast<uintptr_t>(&counterpart_queue_set_->scq.head));
            stat = set_queue(stream_h2d_obj_, req_q_base, cpl_q_base, req_q_head_tail, cpl_q_head_tail);
            if (stat) return stat;
            stat = set_buffer_map(stream_h2d_obj_);
            if (stat) return stat;
        } else if (counterpart_device_type_ == KIDDMA_DEVICE_TYPE_FPGA_XSE) {
            d2d_mode = true;
            // set counterpart doorbell addr
            stat = set_counterpart_doorbell_addr(stream_h2d_obj_, counterpart_dev_id_, counterpart_h2d_ch_id_);
            if (stat) return stat;
        } else {
            return KIDDMA_ERROR_UNSUPPORTED;
        }
        ret = fpga_stream_engine_enable(stream_h2d_obj_, d2d_mode);
        if (ret) return KIDDMA_ERROR_START_DMA_ENGINE_TO_DEV_FAILED;
    }
    if (stat) return stat;
    if (stream_d2h_valid_) {
        d2d_mode = false;
        xse_status_t ret = fpga_stream_engine_init(stream_dev_id_, true, stream_d2h_ch_, &stream_d2h_obj_, check_interval_);
        if (ret) return KIDDMA_ERROR_INIT_DMA_ENGINE_FROM_DEV_FAILED;
        if (counterpart_queue_set_) {
            uint64_t req_q_base(reinterpret_cast<uintptr_t>(counterpart_queue_set_->rrq.queue_elements));
            uint64_t cpl_q_base(reinterpret_cast<uintptr_t>(counterpart_queue_set_->rcq.queue_elements));
            uint64_t req_q_head_tail(reinterpret_cast<uintptr_t>(&counterpart_queue_set_->rrq.head));
            uint64_t cpl_q_head_tail(reinterpret_cast<uintptr_t>(&counterpart_queue_set_->rcq.head));
            stat = set_queue(stream_d2h_obj_, req_q_base, cpl_q_base, req_q_head_tail, cpl_q_head_tail);
            if (stat) return stat;
            stat = set_buffer_map(stream_d2h_obj_);
            if (stat) return stat;
        } else if (counterpart_device_type_ == KIDDMA_DEVICE_TYPE_FPGA_XSE) {
            d2d_mode = true;
            // set counterpart vpmap
            stat = set_counterpart_queue(stream_d2h_obj_, counterpart_dev_id_, counterpart_d2h_ch_id_);
            if (stat) return stat;
            stat = set_counterpart_vpmap(stream_d2h_obj_, counterpart_dev_id_, counterpart_d2h_ch_id_);
            if (stat) return stat;
        } else {
            return KIDDMA_ERROR_UNSUPPORTED;
        }
        ret = fpga_stream_engine_enable(stream_d2h_obj_, d2d_mode);
        if (ret) return KIDDMA_ERROR_START_DMA_ENGINE_FROM_DEV_FAILED;
    }
    return stat;
}

iddma_status iddma_fpga_xse::get_token_offset(uint64_t vaddr, uint64_t& token, uint32_t& offset)
{
    for (auto itr = memory_map_import_.begin(); itr != memory_map_import_.end(); itr++) {
        if ((uintptr_t)itr->addr <= vaddr && vaddr < (uintptr_t)itr->addr + itr->size) {
            offset = vaddr - (uintptr_t)itr->addr;
            return cuda_util_->get_token64((void*)itr->token_ptr, itr->token_size, &token);
        }
    }
    return KIDDMA_ERROR_INVALID_TOKEN;
}

iddma_status iddma_fpga_xse::set_queue(fpga_stream_engine_t stream_info_, uint64_t req_q_base, uint64_t cpl_q_base,
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
#ifdef _DEBUG
        printf("failed set queue with token\n");
#endif
        return KIDDMA_ERROR_SETUP_FAILED;
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_xse::set_buffer_map(fpga_stream_engine_t& info)
{
    // under locked state
    std::map<uint64_t, mmap_share_data> mem_map;
    for (auto itr = memory_map_import_.begin(); itr != memory_map_import_.end(); itr++) {
        mem_map[itr->addr] = *itr;
    }
    for (auto itr = mem_map.begin(); itr != mem_map.end(); itr++) {
        uint64_t token64;
        iddma_status stat = cuda_util_->get_token64((void*)itr->second.token_ptr,
                                                        itr->second.token_size, &token64);
        if (stat != KIDDMA_SUCCESS) return stat;
        xse_status_t ret = fpga_stream_engine_set_buffer(info, (uint64_t)itr->second.addr, itr->second.size, token64);
        if (ret) return KIDDMA_ERROR_ADDRESS_TRANSLATION;
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_xse::enable_dma_engine(void) {
    uint64_t vaddr(reinterpret_cast<uintptr_t>(counterpart_queue_set_));
#ifdef _DEBUG
    printf("xse::enable_dma_engine\n");
#endif
    if (base_config_.protocol == KIDDMA_DMA_PROTOCOL_TCP &&
        (base_config_.server_mode || !base_config_.counter_ip_addr.empty())) {
        auto ret =  enable_tcp_xse();
#ifdef _DEBUG
        printf("xse::enable_dma_engine: %d\n", ret);
#endif
        if (ret) return ret;
        started_ = true;
    } else if (base_config_.protocol == KIDDMA_DMA_PROTOCOL_DEFAULT && !started_) {
#ifdef _DEBUG
        printf("xse::enable_dma_engine: pcie\n");
#endif
        auto ret = enable_pcie_xse();
#ifdef _DEBUG
        printf("xse::enable_dma_engine: pcie: %d\n", ret);
#endif
        if (ret) return ret;
        started_ = true;
    } else {
#ifdef _DEBUG
        printf("xse::enable_dma_engine: unsupported\n");
#endif
        return KIDDMA_ERROR_UNSUPPORTED;
    }

    return iddma_common::enable_dma_engine();
}

bool iddma_fpga_xse::check_option(const std::string& key, const std::string& value) {
    int64_t value_num = 0;
    bool is_num = is_num_value(value, value_num);
    if (key == "dev_id" && is_num && value_num >= 0) {
        stream_dev_id_ = value_num;
    } else if (key == "instance_id" && is_num && value_num >= 0) {
        stream_instance_id_ = value_num;
    } else if (key == "h2d_ch" && is_num && value_num >= 0) {
        stream_h2d_ch_ = value_num;
    } else if (key == "h2d_valid" && is_num) {
        stream_h2d_valid_ = value_num;
    } else if (key == "d2h_ch" && is_num && value_num >= 0) {
        stream_d2h_ch_ = value_num;;
    } else if (key == "d2h_valid" && is_num) {
        stream_d2h_valid_ = value_num;
    } else if (key == "frame_size" && is_num) {
        frame_size_ = value_num;
    } else if (key == "credit_num" && is_num) {
        credit_num_ = value_num;
    } else if (key == "check_interval" && is_num && value_num >= 0) {
        check_interval_ = value_num;
    } else {
        return iddma_fpga::check_option(key, value);
    }
    return true;
}

void iddma_fpga_xse::get_acceptor_device_info(int& acceptor_dev_id, int& acceptor_ch_id) {
    acceptor_dev_id = stream_dev_id_;
    if (stream_h2d_valid_) {
        acceptor_ch_id = stream_h2d_ch_;
    } else if (stream_d2h_valid_) {
        acceptor_ch_id = stream_d2h_ch_;
    }
}

void iddma_fpga_xse::update_connector_device_info(int connector_dev_id, int connector_ch_id) {
    if (stream_h2d_valid_ || stream_d2h_valid_) {
        counterpart_dev_id_ = connector_dev_id;
    }
    if (stream_h2d_valid_) {
        counterpart_h2d_ch_id_ = connector_ch_id;
    }
    if (stream_d2h_valid_) {
        counterpart_d2h_ch_id_ = connector_ch_id;
    }
}

void iddma_fpga_xse::get_connector_device_info(int& connector_dev_id, int& connector_ch_id) {
    connector_dev_id = stream_dev_id_;
    if (stream_h2d_valid_) {
        connector_ch_id = stream_h2d_ch_;
    } else if (stream_d2h_valid_) {
        connector_ch_id = stream_d2h_ch_;
    }
}

void iddma_fpga_xse::update_acceptor_device_info(int acceptor_dev_id, int acceptor_ch_id) {
    if (stream_h2d_valid_ || stream_d2h_valid_) {
        counterpart_dev_id_ = acceptor_dev_id;
    }
    if (stream_h2d_valid_) {
        counterpart_h2d_ch_id_ = acceptor_ch_id;
    }
    if (stream_d2h_valid_) {
        counterpart_d2h_ch_id_ = acceptor_ch_id;
    }
}

iddma_status iddma_fpga_xse::set_counterpart_doorbell_addr(fpga_stream_engine_t obj, uint32_t cp_dev_id, uint32_t cp_ch_id) {
    xse_status_t stat = fpga_stream_engine_set_doorbell_addr(obj, cp_dev_id, cp_ch_id);
    if (stat) {
        return KIDDMA_ERROR_SET_DOORBELL_ADDR_FAILED;
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_xse::set_counterpart_queue(fpga_stream_engine_t obj, uint32_t cp_dev_id, uint32_t cp_ch_id) {
    xse_status_t stat = fpga_stream_engine_set_counterpart_queue(obj, cp_dev_id, cp_ch_id);
    if (stat) {
        return KIDDMA_ERROR_SET_QUEUE_INFO_FAILED;
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_xse::set_counterpart_vpmap(fpga_stream_engine_t obj, uint32_t cp_dev_id, uint32_t cp_ch_id) {
    xse_status_t stat = fpga_stream_engine_set_counterpart_vpmap(obj, cp_dev_id, cp_ch_id);
    if (stat) {
        return KIDDMA_ERROR_SET_VPMAP_FAILED;
    }
    return KIDDMA_SUCCESS;
}
