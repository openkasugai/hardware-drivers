/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_cpu.cpp
 * @brief Implementation of iddma_cpu class.
 *
 */

#include <iddma_cpu.hpp>
#include <iddma_tcp.hpp>
#include <iddma_cuda_util.hpp>
#include <ftok_util.hpp>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <cmath>
#include <chrono>

iddma_cpu::iddma_cpu(iddma_device_type device_type)
    : iddma_common(device_type),
#ifdef __KTP_DEBUG__
      memcpy_time_(0.0),
      update_memcpy_time_(0.0),
      update_memcpy_count_(0),
#endif // __KTP_DEBUG__
      counterpart_close_(false) {}

iddma_cpu::~iddma_cpu(void) {
    finalize();
}

void iddma_cpu::finalize(void) {
#ifdef __KTP_DEBUG__
    printf("memcpy: %.06f s\n", memcpy_time_);
    printf("update memcpy: %.06f s\n", update_memcpy_time_);
    printf("update memcpy count: %d times\n", update_memcpy_count_);
#endif // __KTP_DEBUG__
    iddma_common::finalize();
    free_mmap_token();
}

iddma_device_object iddma_cpu::get_device_object(void) {
    return nullptr;
}

iddma_status iddma_cpu::mmap_populate(void *addr, size_t size) {
    if (!check_connection())
        return KIDDMA_ERROR_NOT_CONNECTED;

    void* token;
    size_t token_size;
    iddma_mmap_memory_type mem_type = KIDDMA_MEMTYPE_HOST;

    iddma_status ret = cuda_util_->register_memory(addr, size, &token, &token_size, &mem_type);
    if (ret != KIDDMA_SUCCESS) return ret;

    memory_map_export_.push_back(mmap_share_data({(uintptr_t)addr, size, (uintptr_t)token, token_size, mem_type}));
    return KIDDMA_SUCCESS;
}

void iddma_cpu::free_mmap_token(void) {
    memory_map_.clear();
}

iddma_status iddma_cpu::post_mmap_share(void) {
    return KIDDMA_SUCCESS;
}

iddma_status iddma_cpu::allocate_queue_set(iddma_queue_set **queue_set) {
    *queue_set = (iddma_queue_set*)aligned_alloc(4096, sizeof(iddma_queue_set));
    memset(*queue_set, 0, sizeof(iddma_queue_set));
    return KIDDMA_SUCCESS;
}

void iddma_cpu::free_queue_set(iddma_queue_set *queue_set) {
    if (queue_set) {
        free(queue_set);
    }
}

/* queue management */
iddma_status iddma_cpu::send_and_recv(void *addr, size_t size_byte, uint64_t imm, bool is_send) {
    if (!check_connection()) return KIDDMA_ERROR_NOT_CONNECTED;
    if (!check_initialization()) return KIDDMA_ERROR_INVALID_OPERATION;
    if (is_send && base_config_.transfer_mode == KIDDMA_TRANSFER_MODE_RECV)
        return KIDDMA_ERROR_NOT_SUPPORTED_TRANSFER_DIRECTION_BY_TRANSFER_MODE;
    if (!is_send && base_config_.transfer_mode == KIDDMA_TRANSFER_MODE_SEND)
        return KIDDMA_ERROR_NOT_SUPPORTED_TRANSFER_DIRECTION_BY_TRANSFER_MODE;

    iddma_queue* cur_queue = is_send ? &queue_set_->srq : &queue_set_->rrq;
    bool is_ready = ((cur_queue->head + 1) & (MAX_QUEUE_SIZE - 1)) != cur_queue->tail;
    if (!is_ready)
        return KIDDMA_ERROR_QUEUE_IS_FULL;

    iddma_queue_element qe;
    qe.addr = (uint64_t)addr;
    qe.size = size_byte;
    qe.status = KIDDMA_QUEUE_STATUS_VALID;
    qe.imm = imm;

    return add_queue_element((iddma_queue*)cur_queue, &qe);
}

iddma_status iddma_cpu::poll_send_and_recv(uint64_t timeout_sec, iddma_queue_element *data, bool is_send) {
    iddma_status status = KIDDMA_SUCCESS;
    iddma_transfer_ready_status t_status;
    if (!check_initialization()) return KIDDMA_ERROR_INVALID_OPERATION;

    const int64_t poll_timing = base_config_.poll_interval_ns;
    uint64_t timeout_maxcount = base_config_.timeout_sec * (1000000000 / poll_timing); // convert sec to how many count comes
    uint64_t time_count = 0;

    struct timespec ts = {.tv_sec = poll_timing / 1000000000, .tv_nsec = poll_timing % 1000000000};

    if (timeout_maxcount == 0)
        timeout_maxcount = 1;

    std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now();

    while (time_count < timeout_maxcount) {
        status = check_completion_queue(is_send, data);
        if (status != KIDDMA_ERROR_POLL_NO_VALID_ELEMENT) break;

        std::chrono::system_clock::time_point temp_time = std::chrono::system_clock::now();
        long time_elapsed = std::chrono::duration_cast<std::chrono::seconds>(temp_time - start_time).count();
        if (time_elapsed >= timeout_sec) break;

        if (timeout_sec == 0) break;
        nanosleep(&ts, NULL);
        time_count++;
    }

    if (status == KIDDMA_ERROR_POLL_NO_VALID_ELEMENT)
        status = KIDDMA_ERROR_POLL_TIMEOUT;
    if (data->status == KIDDMA_QUEUE_STATUS_CLOSE_REQUEST) {
        status = KIDDMA_ERROR_NOT_CONNECTED;
    }
    return status;
}

void iddma_cpu::on_close(void) {
    iddma_queue_element qe({0, 0, KIDDMA_QUEUE_STATUS_CLOSE_REQUEST, 0});
    const int64_t poll_timing = base_config_.poll_interval_ns;
    uint64_t timeout_maxcount = 100000;
    uint64_t time_count = 0;
    struct timespec ts = {.tv_sec = poll_timing / 1000000000, .tv_nsec = poll_timing % 1000000000};
    bool srq_ready = false;
    bool rrq_ready = false;
    while (time_count < timeout_maxcount) {
        srq_ready = ((queue_set_->srq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set_->srq.tail;
        rrq_ready = ((queue_set_->rrq.head + 1) & (MAX_QUEUE_SIZE - 1)) != queue_set_->rrq.tail;
        if (srq_ready || rrq_ready) break;
        nanosleep(&ts, NULL);
        ++time_count;
    }
    if (srq_ready) add_queue_element(&queue_set_->srq, &qe);
    if (rrq_ready) add_queue_element(&queue_set_->rrq, &qe);
}

iddma_status iddma_cpu::check_completion_queue(bool is_send, iddma_queue_element *c_data) {
    bool is_ready = false;
    if (is_send) {
        is_ready = (queue_set_->scq.head != queue_set_->scq.tail);
        if (!is_ready)
            return KIDDMA_ERROR_POLL_NO_VALID_ELEMENT;
        *c_data = queue_set_->scq.queue_elements[queue_set_->scq.tail];
        move_pointer_forward(&queue_set_->scq.tail);
    } else {
        is_ready = (queue_set_->rcq.head != queue_set_->rcq.tail);
        if (!is_ready)
            return KIDDMA_ERROR_POLL_NO_VALID_ELEMENT;
        *c_data = queue_set_->rcq.queue_elements[queue_set_->rcq.tail];
        move_pointer_forward(&queue_set_->rcq.tail);
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_cpu::add_queue_element(iddma_queue *buffer, iddma_queue_element *qe) {
    buffer->queue_elements[buffer->head] = *qe;
    move_pointer_forward(&buffer->head);
    return KIDDMA_SUCCESS;
}

void iddma_cpu::move_pointer_forward(uint32_t *ptr) {
    *ptr = (*ptr + 1) & (MAX_QUEUE_SIZE - 1);
}

iddma_status iddma_cpu::enable_dma_engine(void) {
    if (dma_engine_.get()) return KIDDMA_SUCCESS;

    if (base_config_.protocol == KIDDMA_DMA_PROTOCOL_DEFAULT) {
        // todo
        return KIDDMA_SUCCESS;
    } else if (base_config_.protocol == KIDDMA_DMA_PROTOCOL_TCP) {
        bool send_enable = base_config_.transfer_mode == KIDDMA_TRANSFER_MODE_SEND;
        bool recv_enable = base_config_.transfer_mode == KIDDMA_TRANSFER_MODE_RECV;

        // TODO: select engine type

        try {
            if (base_config_.server_mode || !base_config_.counter_ip_addr.empty()) {
                iddma_engine* engine = new iddma_tcp(queue_set_, send_enable, recv_enable, base_config_.limit_gbps);
                dma_engine_.reset(engine);
            }
        } catch (...) {
            return KIDDMA_ERROR_DMA_ENGINE_CREATION_FAILED;
        }
    } else {
        return KIDDMA_ERROR_UNSUPPORTED;
    }
    return iddma_common::enable_dma_engine();
}
