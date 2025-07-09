/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <iddma_nv_dma.hpp>
#include <iddma_common_def.hpp>
#include <iddma_memory_map.hpp>
#include <unistd.h>
#include <chrono>
#include <cstring>

iddma_nv_dma::iddma_nv_dma(iddma_queue_set* queue_set, iddma_queue_set* cp_queue_set,
                           const iddma_memory_map& memmap,
                           bool& closed,
                           const std::vector<mmap_share_data>& mmap_exports,
                           const std::vector<mmap_share_data>& mmap_imports,
                           bool send_enable, bool recv_enable, uint32_t poll_interval_ns)
    : memmap_(memmap),
      mmap_exports_(mmap_exports),
      mmap_imports_(mmap_imports),
      queue_set_(queue_set),
      cp_queue_set_(cp_queue_set),
      pinned_buf_(nullptr),
      stream_(nullptr),
      finalize_(false),
      closed_(closed),
      poll_interval_us_(poll_interval_ns/1000) {
    initialize(send_enable, recv_enable);
}

iddma_nv_dma::~iddma_nv_dma(void) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        finalize_ = true;
        cond_.notify_all();
    }
    if (worker_.get() && worker_->joinable()) {
        worker_->join();
        worker_.reset();
    }
    if (stream_) cudaStreamDestroy(stream_);
    cudaFreeHost(pinned_buf_);
}

void iddma_nv_dma::wakeup_workers(void) {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.notify_all();
}

iddma_status iddma_nv_dma::write_queue_element(iddma_transfer_destination type, int idx, const iddma_queue_element* src) {
    // nop
    return KIDDMA_SUCCESS;
}

iddma_status iddma_nv_dma::write_queue_head(iddma_transfer_destination type, uint32_t head) {
    // nop
    return KIDDMA_SUCCESS;
}

iddma_status iddma_nv_dma::write_queue_tail(iddma_transfer_destination type, uint32_t tail) {
    // nop
    return KIDDMA_SUCCESS;
}

void iddma_nv_dma::initialize(bool send_enable, bool recv_enable) {
    cudaStreamCreate(&stream_);
    cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking);
    size_t pin_size = sizeof(uint32_t) * 16 + sizeof(iddma_queue_element) * 4;
    cudaHostAlloc(&pinned_buf_, pin_size, cudaHostAllocDefault);
    memset(pinned_buf_, 0, pin_size);
    std::thread* thread = new std::thread(&iddma_nv_dma::worker_func, this, send_enable, recv_enable);
    if (thread) {
        worker_.reset(thread);
    }
}

void iddma_nv_dma::inc_pos(uint32_t& pos) {
    pos = (pos + 1) % MAX_QUEUE_SIZE;
}

bool iddma_nv_dma::update(void* dst, const void* src, uint32_t size, cudaStream_t stream, bool async, bool* fail) {
    void* dst_translated = memmap_.translate_addr(dst);
    const void* src_translated = memmap_.translate_addr(const_cast<void*>(src));
    int dst_type = memmap_.get_mem_type(dst);
    int src_type = memmap_.get_mem_type(const_cast<void*>(src));
    if (!dst || !src || !size) return false;
    if (!dst_translated) {
        dst_translated = dst;
        dst_type = KIDDMA_MEMTYPE_HOST;
    }
    if (!src_translated) {
        src_translated = src;
        src_type = KIDDMA_MEMTYPE_HOST;
    }
    if (dst_type == KIDDMA_MEMTYPE_HOST && src_type == KIDDMA_MEMTYPE_HOST) {
        memcpy(dst_translated, src_translated, size);
    } else {
        cudaMemcpyKind kind =
            dst_type == KIDDMA_MEMTYPE_HOST ? cudaMemcpyDeviceToHost :
            src_type == KIDDMA_MEMTYPE_HOST ? cudaMemcpyHostToDevice : cudaMemcpyDeviceToDevice;
        cudaError_t err;
        if (stream) {
            err = cudaMemcpyAsync(dst_translated, src_translated, size, cudaMemcpyDefault, stream);
            if (!err && !async) {
                err = cudaStreamSynchronize(stream);
            }
        } else {
            err = cudaMemcpy(dst_translated, src_translated, size, kind);
        }
        if (err) {
            cudaGetLastError();
            if (fail) *fail = true;
            return false;
        }
    }
    return true;
}

bool iddma_nv_dma::check_valid(iddma_queue& queue, uint32_t* head_tail) {
    if (head_tail[0] != head_tail[1]) return true;
    bool stat = update(head_tail, &queue.head, sizeof(uint32_t), stream_);
    if (!stat) return false;
    return head_tail[0] != head_tail[1];
}

bool iddma_nv_dma::check_ready(iddma_queue& queue, uint32_t* head_tail) {
    if (head_tail[0] == head_tail[1]) return true;
    bool stat = update(head_tail+1, &queue.tail, sizeof(uint32_t), stream_);
    if (!stat) return false;
    return ((head_tail[0] + 1) % MAX_QUEUE_SIZE) != head_tail[1];
}

bool iddma_nv_dma::transfer(iddma_queue& srq, iddma_queue& scq, iddma_queue& rrq, iddma_queue& rcq,
                            uint32_t* sr_head_tail, uint32_t* sc_head_tail, uint32_t* rr_head_tail, uint32_t* rc_head_tail,
                            iddma_queue_element *sqe, iddma_queue_element *rqe,
                            bool src_valid, bool src_ready, bool dst_valid, bool dst_ready, bool rev_dir_closed) {
    bool stat = true;
    if (src_valid && sqe->status == 0) stat &= update(sqe, &srq.queue_elements[sr_head_tail[1]], sizeof(iddma_queue_element), stream_);
    if (dst_valid && rqe->status == 0) stat &= update(rqe, &rrq.queue_elements[rr_head_tail[1]], sizeof(iddma_queue_element), stream_);
    if (!stat) return true;

    bool ret = false;
    if ((src_valid && sqe->status == KIDDMA_QUEUE_STATUS_CLOSE_REQUEST) ||
        (dst_valid && rqe->status == KIDDMA_QUEUE_STATUS_CLOSE_REQUEST)) {
        if (!rev_dir_closed &&
            (sc_head_tail[0] != sc_head_tail[1] || rc_head_tail[0] != rc_head_tail[1])) return ret;
        sqe->status = KIDDMA_QUEUE_STATUS_CLOSE_REQUEST;
        rqe->status = KIDDMA_QUEUE_STATUS_CLOSE_REQUEST;
        ret = true;
    } else {
        bool src_completed = sc_head_tail[0] == sc_head_tail[1];
        bool dst_completed = rc_head_tail[0] == rc_head_tail[1];
        if (rev_dir_closed && src_completed && dst_completed) return true;
        if (!src_valid || !src_ready || !dst_valid || !dst_ready) {
            return ret;
       } else {
            uint64_t transfer_size = sqe->size < rqe->size ? sqe->size : rqe->size;
            bool fail = false;
            stat = update((void*)rqe->addr, (const void*)sqe->addr, transfer_size,
                          stream_, false/*sync*/, &fail);

            sqe->size = transfer_size;
            rqe->size = transfer_size;
            sqe->status = stat && !fail ? KIDDMA_QUEUE_STATUS_SUCCESS : KIDDMA_QUEUE_STATUS_FAIL;
            rqe->status = stat && !fail ? KIDDMA_QUEUE_STATUS_SUCCESS : KIDDMA_QUEUE_STATUS_FAIL;
            rqe->imm = sqe->imm;
        }
    }

    if (src_ready) {
        update(&scq.queue_elements[sc_head_tail[0]], sqe, sizeof(iddma_queue_element), stream_, true);
        inc_pos(sc_head_tail[0]);
        update(&scq.head, sc_head_tail, sizeof(uint32_t), stream_, true);
    }
    if (dst_ready) {
        update(&rcq.queue_elements[rc_head_tail[0]], rqe, sizeof(iddma_queue_element), stream_, true);
        inc_pos(rc_head_tail[0]);
        update(&rcq.head, rc_head_tail, sizeof(uint32_t), stream_, true);
    }
    if (src_valid) {
        inc_pos(sr_head_tail[1]);
        update(&srq.tail, sr_head_tail+1, sizeof(uint32_t), stream_, true);
    }
    if (dst_valid) {
        inc_pos(rr_head_tail[1]);
        update(&rrq.tail, rr_head_tail+1, sizeof(uint32_t), stream_, true);
    }
    if (stream_) cudaStreamSynchronize(stream_);
    sqe->status = KIDDMA_QUEUE_STATUS_INVALID;
    rqe->status = KIDDMA_QUEUE_STATUS_INVALID;

    return ret;
}

void iddma_nv_dma::worker_func(bool send_enable, bool recv_enable) {
    uint32_t* send_sr_head_tail = (uint32_t*)pinned_buf_;
    uint32_t* send_sc_head_tail = send_sr_head_tail + 2;
    uint32_t* send_rr_head_tail = send_sr_head_tail + 4;
    uint32_t* send_rc_head_tail = send_sr_head_tail + 6;
    uint32_t* recv_sr_head_tail = send_sr_head_tail + 8;
    uint32_t* recv_sc_head_tail = send_sr_head_tail + 10;
    uint32_t* recv_rr_head_tail = send_sr_head_tail + 12;
    uint32_t* recv_rc_head_tail = send_sr_head_tail + 14;

    iddma_queue_element *send_sqe = (iddma_queue_element*)((uint32_t*)pinned_buf_ + 16);
    iddma_queue_element *send_rqe = send_sqe + 1;
    iddma_queue_element *recv_sqe = send_sqe + 2;
    iddma_queue_element *recv_rqe = send_sqe + 3;

    if (send_enable) {
        update(send_sr_head_tail, &queue_set_->srq.head, sizeof(uint32_t)*2, stream_);
        update(send_sc_head_tail, &queue_set_->scq.head, sizeof(uint32_t)*2, stream_);
        update(send_rr_head_tail, &cp_queue_set_->rrq.head, sizeof(uint32_t)*2, stream_);
        update(send_rc_head_tail, &cp_queue_set_->rcq.head, sizeof(uint32_t)*2, stream_);
    }
    if (recv_enable) {
        update(recv_sr_head_tail, &cp_queue_set_->srq.head, sizeof(uint32_t)*2, stream_);
        update(recv_sc_head_tail, &cp_queue_set_->scq.head, sizeof(uint32_t)*2, stream_);
        update(recv_rr_head_tail, &queue_set_->rrq.head, sizeof(uint32_t)*2, stream_);
        update(recv_rc_head_tail, &queue_set_->rcq.head, sizeof(uint32_t)*2, stream_);
    }
    bool send_closed = false;
    bool recv_closed = false;
    while (!finalize_ || !closed_) {
        bool send_ready = false;
        bool recv_ready = false;
        if ((send_enable || finalize_) && !closed_ && !send_closed) {
            bool sr_valid = check_valid(queue_set_->srq, send_sr_head_tail);
            bool sc_ready = check_ready(queue_set_->scq, send_sc_head_tail);
            bool rr_valid = check_valid(cp_queue_set_->rrq, send_rr_head_tail);
            bool rc_ready = check_ready(cp_queue_set_->rcq, send_rc_head_tail);
            send_ready = sr_valid && sc_ready && rr_valid && rc_ready;
            if ((sr_valid || rr_valid) && sc_ready && rc_ready) {
                send_closed |= transfer(queue_set_->srq, queue_set_->scq, cp_queue_set_->rrq, cp_queue_set_->rcq,
                                        send_sr_head_tail, send_sc_head_tail, send_rr_head_tail, send_rc_head_tail,
                                        send_sqe, send_rqe,
                                        sr_valid, sc_ready, rr_valid, rc_ready, recv_closed);
            }
        }
        if ((recv_enable || finalize_) && !closed_ && !recv_closed) {
            bool sr_valid = check_valid(cp_queue_set_->srq, recv_sr_head_tail);
            bool sc_ready = check_ready(cp_queue_set_->scq, recv_sc_head_tail);
            bool rr_valid = check_valid(queue_set_->rrq, recv_rr_head_tail);
            bool rc_ready = check_ready(queue_set_->rcq, recv_rc_head_tail);
            recv_ready = sr_valid && sc_ready && rr_valid && rc_ready;
            if ((sr_valid || rr_valid) && rc_ready && sc_ready) {
                recv_closed |= transfer(cp_queue_set_->srq, cp_queue_set_->scq, queue_set_->rrq, queue_set_->rcq,
                                        recv_sr_head_tail, recv_sc_head_tail, recv_rr_head_tail, recv_rc_head_tail,
                                        recv_sqe, recv_rqe,
                                        sr_valid, sc_ready, rr_valid, rc_ready, send_closed);
            }
        }
        closed_ |= send_closed && recv_closed;
        if (!send_ready && !recv_ready && (!finalize_ || !closed_)) {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait_for(lock, std::chrono::microseconds(poll_interval_us_));
        }
    }
}
