/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <iddma_fpga_fdma.hpp>
#include <iddma_tcp_ptu.hpp>
#include <libfpgactl.h>
#include <libfdma.h>
#include <libdma.h>
#include <libshmem.h>
#include <libptu.h>
#include <libchain.h>
#include <chrono>
#include <deque>

//#define _DEBUG

#define ALIGN64(x) ((x) & ~63)

const int iddma_fpga_fdma::s_dequeue_timeout_count_ = 10000 / 100; // 10sec / 100msec

iddma_fpga_fdma::iddma_fpga_fdma(iddma_device_type device_type)
    : iddma_fpga(device_type),
      stream_dev_id_(0),
      stream_h2d_ch_(0),
      stream_d2h_ch_(0),
      stream_ptu_id_(-1),
      stream_kernel_id_(-1),
      stream_func_id_(-1),
      counterpart_queue_set_paddr_(0),
      counterpart_queue_set_vaddr_(0),
      finalize_(false) {
}

iddma_fpga_fdma::~iddma_fpga_fdma(void) {
    finalize();
}

void iddma_fpga_fdma::finalize(void) {
    on_close();
    iddma_fpga::finalize();
}

void iddma_fpga_fdma::on_close(void) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (worker_.get() && worker_->joinable()) {
            finalize_ = true;
            cond_.notify_all();
        }
    }
    if (worker_.get() && worker_->joinable()) {
        worker_->join();
        worker_.reset();

        int ret;
        if (!stream_h2d_name_.empty()) {
            ret = fpga_fdma_queue_finish(&stream_h2d_info_);
            ret = fpga_fdma_finish(&stream_h2d_ch_info_);
            if (stream_kernel_id_ >= 0 && stream_func_id_ >= 0) {
                fpga_chain_disconnect_ingress(stream_dev_id_, stream_kernel_id_, stream_func_id_);
            }
        }
        if (!stream_d2h_name_.empty()) {
            ret = fpga_fdma_queue_finish(&stream_d2h_info_);
            ret = fpga_fdma_finish(&stream_d2h_ch_info_);
            if (stream_kernel_id_ >= 0 && stream_func_id_ >= 0) {
                fpga_chain_disconnect_egress(stream_dev_id_, stream_kernel_id_, stream_func_id_);
            }
        }
    }
    for (auto& it : paddress_translation_table_) {
        fpga_shmem_unregister((void*)it.first);
    }
    paddress_translation_table_.clear();
}

void iddma_fpga_fdma::post_close(void) {
    iddma_common::post_close();
}

iddma_status iddma_fpga_fdma::allocate_queue_set(iddma_queue_set **queue_set) {
    return iddma_fpga::allocate_queue_set(queue_set);
}

void iddma_fpga_fdma::free_queue_set(iddma_queue_set *queue_set) {
    // todo: release queue set of lane and channel.
}

iddma_status iddma_fpga_fdma::poll_send_and_recv(uint64_t timeout_sec, iddma_queue_element *data, bool is_send) {
    auto now = std::chrono::system_clock::now();
    auto end = now + std::chrono::seconds(timeout_sec);
    while (now < end) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait_until(lock, end);
        if (finalize_) return KIDDMA_ERROR_NOT_CONNECTED;
    }
    return KIDDMA_ERROR_POLL_TIMEOUT;
}

iddma_status iddma_fpga_fdma::register_paddress_translation(uint64_t vaddr, uint64_t paddr, uint32_t size) {
    paddress_translation_table_[vaddr] = std::pair<uint64_t, uint32_t>(paddr, size);
    int ret = fpga_shmem_register((void*)vaddr, paddr, size);
    if (ret) return KIDDMA_ERROR_SETUP_FAILED;

    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_fdma::register_vaddress_translation(void* cp_vaddr, uint64_t vaddr, uint32_t size,
                                                            void* token, uint32_t token_size) {
    vaddress_translation_table_[reinterpret_cast<uint64_t>(cp_vaddr)] = std::pair<uint64_t, uint32_t>(vaddr, size);
    //fpga_shmem_register_by_token(token, token_size, size);
    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_fdma::initialize_h2d_fdma(void) {
    if (!stream_h2d_name_.empty() && tcp_info_.protocol == KIDDMA_DMA_PROTOCOL_DEFAULT) {
        int ret = fpga_fdma_init(stream_dev_id_, DMA_HOST_TO_DEV, stream_h2d_ch_,
                                 &stream_h2d_name_[0], &stream_h2d_ch_info_);
        if (ret) return KIDDMA_ERROR_OPEN_DEVICE_FAILED;
        ret = fpga_fdma_queue_setup(&stream_h2d_name_[0], &stream_h2d_info_);
        if (ret) return KIDDMA_ERROR_SETUP_FAILED;
        if (stream_kernel_id_ >= 0 && stream_func_id_ >=0) {
            ret = fpga_chain_connect_ingress(stream_dev_id_, stream_kernel_id_, stream_func_id_,
                                             stream_h2d_ch_ + s_fdma_cid_base_);
            if (ret < 0) return KIDDMA_ERROR_SETUP_FAILED;
        }
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_fdma::initialize_d2h_fdma(void) {
    if (!stream_d2h_name_.empty() && tcp_info_.protocol == KIDDMA_DMA_PROTOCOL_DEFAULT) {
        int ret = fpga_fdma_init(stream_dev_id_, DMA_DEV_TO_HOST, stream_d2h_ch_,
                                 &stream_d2h_name_[0], &stream_d2h_ch_info_);
        if (ret) return KIDDMA_ERROR_OPEN_DEVICE_FAILED;
        ret = fpga_fdma_queue_setup(&stream_d2h_name_[0], &stream_d2h_info_);
        if (ret) return KIDDMA_ERROR_SETUP_FAILED;
        if (stream_kernel_id_ >= 0 && stream_func_id_ >=0) {
            ret = fpga_chain_connect_egress(stream_dev_id_, stream_kernel_id_, stream_func_id_,
                                            stream_d2h_ch_ + s_fdma_cid_base_);
            if (ret < 0) return KIDDMA_ERROR_SETUP_FAILED;
        }
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_fdma::enable_tcp_ptu(void) {
    if (dma_engine_.get()) return KIDDMA_SUCCESS;

    bool send_enable = transfer_mode_ == KIDDMA_TRANSFER_MODE_SEND ||
        transfer_mode_ == KIDDMA_TRANSFER_MODE_BOTH;
    bool recv_enable = transfer_mode_ == KIDDMA_TRANSFER_MODE_RECV ||
        transfer_mode_ == KIDDMA_TRANSFER_MODE_BOTH;

    try {
        iddma_engine* engine = new iddma_tcp_ptu(tcp_info_.server_mode, tcp_info_.counter_ip_addr,
                                                 tcp_info_.counter_tcp_port, stream_dev_id_, stream_ptu_id_,
                                                 stream_kernel_id_, stream_func_id_, send_enable, recv_enable);
        dma_engine_.reset(engine);
    } catch (...) {
        return KIDDMA_ERROR_INVALID_ARGUMENT;
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_fpga_fdma::enable_dma_engine(void) {
    uint64_t vaddr(reinterpret_cast<uintptr_t>(counterpart_queue_set_));
    if (tcp_info_.protocol == KIDDMA_DMA_PROTOCOL_TCP &&
        (tcp_info_.server_mode || !tcp_info_.counter_ip_addr.empty()) && stream_ptu_id_ >= 0) {
        auto ret =  enable_tcp_ptu();
        if (ret) return ret;
    } else if (tcp_info_.protocol == KIDDMA_DMA_PROTOCOL_DEFAULT && (!worker_.get() || !worker_->joinable())) {
        {
            auto it = paddress_translation_table_.upper_bound(vaddr);
            if (it == paddress_translation_table_.begin()) {
                return KIDDMA_ERROR_BUFFER_NOT_MAPPED;
            }
            --it;
            uint64_t offset = vaddr - it->first;
            if (offset >= it->second.second) {
                return KIDDMA_ERROR_BUFFER_NOT_MAPPED;
            }
            counterpart_queue_set_paddr_ = it->second.first + offset;
        }
        {
            auto it = vaddress_translation_table_.upper_bound(vaddr);
            if (it == vaddress_translation_table_.begin()) {
                return KIDDMA_ERROR_BUFFER_NOT_MAPPED;
            }
            --it;
            uint64_t offset = vaddr - it->first;
            if (offset >= it->second.second) {
                return KIDDMA_ERROR_BUFFER_NOT_MAPPED;
            }
            counterpart_queue_set_vaddr_ = it->second.first + offset;
        }
        // FDMA2 transfer
        iddma_status stat = initialize_h2d_fdma();
        if (stat) return stat;
        stat = initialize_d2h_fdma();
        if (stat) return stat;
        uint32_t prev_rsize = 0;
        if (!stream_d2h_name_.empty()) {
            // ignore error
            if (!fpga_fdma_get_rsize(&stream_d2h_info_, &prev_rsize)) {
#ifdef _DEBUG
                printf("start rsize: %x\n", prev_rsize);
#endif
            }
        }

        worker_.reset(new std::thread(&iddma_fpga_fdma::worker, this, prev_rsize));
    } else {
        return KIDDMA_ERROR_UNSUPPORTED;
    }

    return iddma_common::enable_dma_engine();
}

bool iddma_fpga_fdma::check_option(const std::string& key, const std::string& value) {
    int64_t value_num = 0;
    bool is_num = is_num_value(value, value_num);
    if (key == "dev_id" && is_num && value_num >= 0) {
        stream_dev_id_ = value_num;
    } else if (key == "h2d_ch" && is_num && value_num >= 0) {
        stream_h2d_ch_ = value_num;
    } else if (key == "h2d_name" && value.size() < s_connector_name_max_) {
        stream_h2d_name_ = value;
    } else if (key == "d2h_ch" && is_num && value_num >= 0) {
        stream_d2h_ch_ = value_num;;
    } else if (key == "d2h_name" && value.size() < s_connector_name_max_) {
        stream_d2h_name_ = value;
    } else if (key == "ptu_id" && is_num && value_num >= 0) {
        stream_ptu_id_ = value_num;
    } else if (key == "kernel_id" && is_num && value_num >= 0) {
        stream_kernel_id_ = value_num;
    } else if (key == "func_id" && is_num && value_num >= 0) {
        stream_func_id_ = value_num;
    } else {
        return iddma_fpga::check_option(key, value);
    }
    return true;
}

void iddma_fpga_fdma::worker(uint32_t prev_rsize) {
    iddma_queue_set* cp_qset = reinterpret_cast<iddma_queue_set*>(counterpart_queue_set_vaddr_);
    iddma_queue* srq = &cp_qset->srq;
    iddma_queue* rrq = &cp_qset->rrq;
    iddma_queue* scq = &cp_qset->scq;
    iddma_queue* rcq = &cp_qset->rcq;

    std::deque<dmacmd_info_t> h2d_tasks;
    std::deque<dmacmd_info_t> d2h_tasks;
    int h2d_retry = 0;
    int d2h_retry = 0;
    uint16_t h2d_task_id = 0;
    uint16_t d2h_task_id = 0;

    uint32_t srq_tail = srq->tail;
    uint32_t rrq_tail = rrq->tail;
    uint32_t scq_head = scq->head;
    uint32_t rcq_head = rcq->head;
    while (!finalize_ || !h2d_tasks.empty() || !d2h_tasks.empty()) {
        uint32_t srq_head = srq->head;
        uint32_t rrq_head = rrq->head;
        uint32_t scq_tail = scq->tail;
        uint32_t rcq_tail = rcq->tail;
        bool sr_valid = srq_head != srq_tail;
        bool rr_valid = rrq_head != rrq_tail;
        bool sc_vacant = ((scq_head + h2d_tasks.size() + 1) & (MAX_QUEUE_SIZE-1)) != scq_tail;
        bool rc_vacant = ((rcq_head + d2h_tasks.size() + 1) & (MAX_QUEUE_SIZE-1)) != rcq_tail;
        if (sr_valid && sc_vacant && !finalize_) {
            iddma_queue_element sreq = srq->queue_elements[srq_tail];
            if (sreq.status == KIDDMA_QUEUE_STATUS_CLOSE_REQUEST) {
                std::unique_lock<std::mutex> lock(mutex_);
                finalize_ = true;
                cond_.notify_all();
            } else {
                h2d_tasks.push_back(dmacmd_info_t());
                if (sreq.addr && !set_dma_cmd(&h2d_tasks.back(), h2d_task_id, (void*)sreq.addr, sreq.size)) {
                    int ret = fpga_enqueue(&stream_h2d_info_, &h2d_tasks.back());
                    if (ret) {
                        h2d_tasks.pop_back();
                    } else {
#ifdef _DEBUG
                            printf("FDMA: enqueue h2d: %d, task_num=%lu, rrq=%u/%u, %u/%u, rcq=%u/%u, %u/%u\n",
                                   h2d_task_id, h2d_tasks.size(),
                                   srq->head, srq->tail, srq_head, srq_tail,
                                   scq->head, scq->tail, scq_head, scq_tail);
#endif
                        h2d_task_id++;
                        srq_tail = (srq_tail + 1) & (MAX_QUEUE_SIZE-1);
                        srq->tail = srq_tail;
                    }
                } else {
                    h2d_tasks.pop_back();
                    printf("FDMA: fail to set task cmd\n");
                }
            }
        }
        if (rr_valid && rc_vacant && !finalize_) {
            iddma_queue_element rreq = rrq->queue_elements[rrq_tail];
            if (rreq.status == KIDDMA_QUEUE_STATUS_CLOSE_REQUEST) {
                std::unique_lock<std::mutex> lock(mutex_);
                finalize_ = true;
                cond_.notify_all();
            } else {
                uint32_t rsize;
                int ret = fpga_fdma_get_rsize(&stream_d2h_info_, &rsize);
                if (ret == 0 && rsize && rsize != prev_rsize) {
#ifdef _DEBUG
                    printf("FDMA: get rsize: %x\n", rsize);
#endif
                    d2h_tasks.push_back(dmacmd_info_t());
                    if (rreq.size < ALIGN64(rsize)) {
                        printf("FDMA: lack of recv buffer: %lx < %x\n", rreq.size, ALIGN64(rsize));
                    } else if (rreq.addr && !set_dma_cmd(&d2h_tasks.back(), d2h_task_id, (void*)rreq.addr, rreq.size)) {
                        int ret = fpga_enqueue(&stream_d2h_info_, &d2h_tasks.back());
                        if (ret) {
                            d2h_tasks.pop_back();
                        } else {
#ifdef _DEBUG
                            printf("FDMA: enqueue d2h: %d, task_num=%lu, rrq=%u/%u, %u/%u, rcq=%u/%u, %u/%u\n",
                                   d2h_task_id, d2h_tasks.size(),
                                   rrq->head, rrq->tail, rrq_head, rrq_tail,
                                   rcq->head, rcq->tail, rcq_head, rcq_tail);
#endif
                            prev_rsize = rsize;
                            d2h_task_id++;
                            rrq_tail = (rrq_tail + 1) & (MAX_QUEUE_SIZE-1);
                            rrq->tail = rrq_tail;
                        }
                    } else {
                        d2h_tasks.pop_back();
                        printf("FDMA: fail to set d2h task cmd\n");
                    }
                }
            }
        }
        if (!(sr_valid && sc_vacant) && !(rr_valid && rc_vacant)) {
            if (h2d_tasks.empty() && d2h_tasks.empty()) {
                std::unique_lock<std::mutex> lock(mutex_);
                cond_.wait_for(lock, std::chrono::microseconds(100));
            } else if (!h2d_tasks.empty()) {
#ifdef _DEBUG
                printf("FDMA: dequeue h2d\n");
#endif
                iddma_queue_element sres;
                int ret = fpga_dequeue(&stream_h2d_info_, &h2d_tasks.front());
                if (ret) {
                    h2d_retry++;
                }
                if (!ret || h2d_retry >= s_dequeue_timeout_count_) {
                    sres.addr = reinterpret_cast<uint64_t>(h2d_tasks.front().data_addr);
                    sres.size = h2d_tasks.front().result_data_len;
                    sres.status = ret ? KIDDMA_QUEUE_STATUS_FAIL : KIDDMA_QUEUE_STATUS_SUCCESS;
#ifdef _DEBUG
                    printf("FDMA: dequeue h2d: %d, %d, %lx, remain:%lu, rrq=%u/%u, %u/%u, rcq=%u/%u, %u/%u\n",
                           ret, h2d_tasks.front().task_id, sres.status, h2d_tasks.size() - 1,
                           srq->head, srq->tail, srq_head, srq_tail,
                           scq->head, scq->tail, scq_head, scq_tail);
#endif
                    h2d_retry = 0;
                    scq->queue_elements[scq_head] = sres;
                    scq_head = (scq_head + 1) & (MAX_QUEUE_SIZE-1);
                    scq->head = scq_head;
                    h2d_tasks.pop_front();
                }
            } else {
#ifdef _DEBUG
                printf("FDMA: dequeue d2h\n");
#endif
                iddma_queue_element rres;
                int ret = fpga_dequeue(&stream_d2h_info_, &d2h_tasks.front());
                if (ret) {
                    d2h_retry++;
                }
                if (!ret || d2h_retry >= s_dequeue_timeout_count_) {
                    rres.addr = reinterpret_cast<uint64_t>(d2h_tasks.front().data_addr);
                    rres.size = d2h_tasks.front().result_data_len;
                    rres.status = ret ? KIDDMA_QUEUE_STATUS_FAIL : KIDDMA_QUEUE_STATUS_SUCCESS;
#ifdef _DEBUG
                    printf("FDMA: dequeue d2h: %d, %d, %lx, remain:%lu, rrq=%u/%u, %u/%u, rcq=%u/%u, %u/%u\n",
                           ret, d2h_tasks.front().task_id, rres.status, d2h_tasks.size() - 1,
                           rrq->head, rrq->tail, rrq_head, rrq_tail,
                           rcq->head, rcq->tail, rcq_head, rcq_tail);
#endif
                    d2h_retry = 0;
                    rcq->queue_elements[rcq_head] = rres;
                    rcq_head = (rcq_head + 1) & (MAX_QUEUE_SIZE-1);
                    rcq->head = rcq_head;
                    d2h_tasks.pop_front();
                }
            }
        }
    }
}
