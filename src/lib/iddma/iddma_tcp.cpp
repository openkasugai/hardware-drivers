/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <tcp_ctrl_tunnel.hpp>
#include <ctrl_tunnel.hpp>
#include <tcp_cmd.hpp>
#include <iddma_tcp.hpp>
#include <cstring>
#include <sstream>

static const int s_credit_buf_size = 8;
static const int s_activate_buf_size = 10;
static const int s_credit_magic_size = 4;


iddma_tcp::iddma_tcp(iddma_queue_set* queue_set, bool send_enable, bool recv_enable)
    : iddma_engine(queue_set),
      finalize_(false),
      limit_gbps_(0.f),
      worker_status_(KIDDMA_SUCCESS) {
    initialize(send_enable, recv_enable);
}

iddma_tcp::iddma_tcp(iddma_queue_set* queue_set, bool send_enable, bool recv_enable, float limit_gbps)
    : iddma_engine(queue_set),
      finalize_(false),
      limit_gbps_(limit_gbps),
      worker_status_(KIDDMA_SUCCESS) {
    initialize(send_enable, recv_enable);
}

iddma_tcp::~iddma_tcp(void) {
    finalize_ = true;
    wakeup_workers();
    if (worker_send_.get()) {
        worker_send_->join();
        worker_send_.reset();
    }
    if (worker_recv_.get()) {
        worker_recv_->join();
        worker_recv_.reset();
    }
    dma_tcp_.reset();
}

void iddma_tcp::initialize(bool send_enable, bool recv_enable) {
    if (send_enable) {
        worker_send_.reset(new std::thread(&iddma_tcp::dma_send, this));
        worker_recv_.reset(new std::thread(&iddma_tcp::dma_recv_credit, this));
        if (recv_enable) {
            throw std::runtime_error("unsupported both direction");
        }
    } else if (recv_enable) {
        worker_send_.reset(new std::thread(&iddma_tcp::dma_send_credit, this));
        worker_recv_.reset(new std::thread(&iddma_tcp::dma_recv, this));
    }
}

void iddma_tcp::wakeup_workers(void) {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.notify_all();
}

iddma_status iddma_tcp::read_queue_element(iddma_queue_element* dst, const iddma_queue_element* src) {
    if (!src || !dst) return KIDDMA_ERROR_INVALID_ARGUMENT;
    memcpy(dst, src, sizeof(iddma_queue_element));
    return KIDDMA_SUCCESS;
}

iddma_queue* iddma_tcp::get_queue(iddma_transfer_destination type) {
    switch(type) {
    case KIDDMA_TRANSFER_DESTINATION_SRQ: return &queue_set_->srq; break;
    case KIDDMA_TRANSFER_DESTINATION_RRQ: return &queue_set_->rrq; break;
    case KIDDMA_TRANSFER_DESTINATION_SCQ: return &queue_set_->scq; break;
    case KIDDMA_TRANSFER_DESTINATION_RCQ: return &queue_set_->rcq; break;
    default: return nullptr;
    }
}

iddma_status iddma_tcp::write_queue_element(iddma_transfer_destination type, int idx, const iddma_queue_element* src) {
    if (idx < 0 || idx >= MAX_QUEUE_SIZE) return KIDDMA_ERROR_INVALID_ARGUMENT;

    iddma_queue* queue(get_queue(type));
    if (!queue) return KIDDMA_ERROR_INVALID_ARGUMENT;

    iddma_queue_element* dst = &queue->queue_elements[idx];
    return read_queue_element(dst, src);
}

iddma_status iddma_tcp::read_queue_head_tail(uint32_t* head, uint32_t* tail, const iddma_queue* queue) {
    if (!queue) return KIDDMA_ERROR_INVALID_ARGUMENT;
    if (head) *head = queue->head;
    if (tail) *tail = queue->tail;
    return KIDDMA_SUCCESS;
}

iddma_status iddma_tcp::write_queue_head(iddma_transfer_destination type, uint32_t head) {
    iddma_queue* queue(get_queue(type));
    if (!queue) return KIDDMA_ERROR_INVALID_ARGUMENT;
    queue->head = head;
    return KIDDMA_SUCCESS;
}

iddma_status iddma_tcp::write_queue_tail(iddma_transfer_destination type, uint32_t tail) {
    iddma_queue* queue(get_queue(type));
    if (!queue) return KIDDMA_ERROR_INVALID_ARGUMENT;
    queue->tail = tail;
    return KIDDMA_SUCCESS;
}

iddma_status iddma_tcp::read_buffer_data(void* dst, const void* src, uint32_t size) {
    if (!dst || !src) return KIDDMA_ERROR_INVALID_ARGUMENT;
    if (size) memcpy((char*)dst, (const char*)src, size);
    return KIDDMA_SUCCESS;
}

iddma_status iddma_tcp::write_buffer_data(void* dst, const void* src, uint32_t size) {
    return read_buffer_data(dst, src, size);
}

bool iddma_tcp::is_accessible(const void* src) {
    return true;
}

void iddma_tcp::dma_send(void) {
    while (!ctrl_tunnel_set_ && !finalize_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock);
    }
    while (!finalize_) {
        uint32_t send_req_tail, send_cpl_head, recv_req_tail, recv_cpl_head;
        iddma_queue_element recv_req, send_req;
        if (!check_transfer_available(&queue_set_->rrq, &queue_set_->rcq, recv_req_tail, recv_cpl_head, recv_req) ||
            !check_transfer_available(&queue_set_->srq, &queue_set_->scq, send_req_tail, send_cpl_head, send_req)) {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock);
            continue;
        }
        if (send_req.status == KIDDMA_QUEUE_STATUS_VALID) {
            iddma_status status = KIDDMA_SUCCESS;
            int ssize = dma_tcp_->send((char*)send_req.addr, send_req.size);
            if (ssize < 0) status = KIDDMA_ERROR_NETWORK_SEND_FAILED;
            if (status) {
                worker_status_ = status;
                break;
            }
            send_req.status = KIDDMA_QUEUE_STATUS_SUCCESS;
            if (write_queue_element(KIDDMA_TRANSFER_DESTINATION_SCQ, send_cpl_head, &send_req)) break;

            send_req_tail = (send_req_tail + 1) & (MAX_QUEUE_SIZE-1);
            send_cpl_head = (send_cpl_head + 1) & (MAX_QUEUE_SIZE-1);
            recv_req_tail = (recv_req_tail + 1) & (MAX_QUEUE_SIZE-1);
            if (write_queue_tail(KIDDMA_TRANSFER_DESTINATION_SRQ, send_req_tail) ||
                write_queue_head(KIDDMA_TRANSFER_DESTINATION_SCQ, send_cpl_head) ||
                write_queue_tail(KIDDMA_TRANSFER_DESTINATION_RRQ, recv_req_tail)) break;
            wakeup_workers();
        }
    }
}

void iddma_tcp::dma_send_credit(void) {
    while (!ctrl_tunnel_set_ && !finalize_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock);
    }
    int credit_num;
    if (ctrl_tunnel_set_) {
        tcp_ctrl_tunnel* tunnel = static_cast<tcp_ctrl_tunnel*>(ctrl_tunnel_);
        dma_tcp_ = tunnel->get_tcp_cmd();
        credit_num = tunnel->get_credit_num();
        if (credit_num == 0) credit_num = 1;
    }
    bool activated = false;
    char credit_buf[s_credit_buf_size] = {'C', 'R', 'D', 'T', '\0', '\0', '\0', '\0'};
    char activate_buf[s_activate_buf_size] = {'A', 'C', 'T', 'V', '\0', '\0', '\0', '\0', '\0', '\0'};
    while (!finalize_) {
        uint32_t req_tail, crd_head;
        iddma_queue_element req;
        if (!check_transfer_available(&queue_set_->rrq, &queue_set_->srq, req_tail, crd_head, req)) {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock);
            continue;
        }
        if (req.status == KIDDMA_QUEUE_STATUS_VALID) {
            iddma_status status = KIDDMA_SUCCESS;
            if (!activated) {
                *((uint32_t*)(activate_buf + 4)) = req.size;
                *((uint16_t*)(activate_buf + 8)) = credit_num;
                int ssize = dma_tcp_->send((char*)activate_buf, s_activate_buf_size);
                if (ssize < 0) {
                    status = KIDDMA_ERROR_NETWORK_SEND_FAILED;
                } else {
                    activated = true;
                    continue;
                }
            } else {
                *((uint32_t*)(credit_buf + 4)) = req.size;
                int ssize = dma_tcp_->send((char*)credit_buf, s_credit_buf_size);
                if (ssize < 0) status = KIDDMA_ERROR_NETWORK_SEND_FAILED;
            }
            if (status) {
                worker_status_ = status;
                break;
            }
            if (write_queue_element(KIDDMA_TRANSFER_DESTINATION_SRQ, crd_head, &req)) {
                break;
            }
            req_tail = (req_tail + 1) & (MAX_QUEUE_SIZE-1);
            crd_head = (crd_head + 1) & (MAX_QUEUE_SIZE-1);
            if (write_queue_tail(KIDDMA_TRANSFER_DESTINATION_RRQ, req_tail) ||
                write_queue_head(KIDDMA_TRANSFER_DESTINATION_SRQ, crd_head)) {
                break;
            }
            wakeup_workers();
        }
    }
}

void iddma_tcp::dma_recv(void) {
    while (!ctrl_tunnel_set_ && !finalize_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock);
    }
    std::vector<char> buffer;
    while (!finalize_) {
        uint32_t req_tail, cpl_head;
        iddma_queue_element req;
        if (!check_transfer_available(&queue_set_->srq, &queue_set_->rcq, req_tail, cpl_head, req)) {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock);
            continue;
        }
        int32_t rsize = 0;
        int header_offset = 0;
        bool accessible = is_accessible((const void*)req.addr);
        if (!accessible && buffer.size() < s_work_buffer_size) buffer.resize(s_work_buffer_size);
        bool failed = false;

        while (rsize < req.size && !finalize_) {
            int stat = dma_tcp_->wait(1000);
            if (stat < 0) {
                worker_status_ = KIDDMA_ERROR_POLL_TIMEOUT;
                failed = true;
                break;
            } else if (stat == 0) {
                continue;
            }
            int read_size = rsize < s_network_header_size ? s_network_header_size - rsize :
                accessible ? req.size - rsize + s_network_header_size : buffer.size();
            char* buf_ptr;
            if (accessible) {
                buf_ptr = (char*)(req.addr + rsize - header_offset);
            } else {
                buf_ptr = rsize < s_network_header_size ? &buffer[rsize] : &buffer[0];
            }
            int cur_rsize = dma_tcp_->recv(buf_ptr, read_size);
            if (cur_rsize < 0) {
                worker_status_ = KIDDMA_ERROR_NETWORK_RECV_FAILED;
                failed = true;
                break;
            } else if (cur_rsize == 0 || rsize + cur_rsize < s_network_header_size) {
                rsize += cur_rsize;
                continue;
            }
            if (rsize + cur_rsize == s_network_header_size) {
                header_offset = s_network_header_size;
            } else if (!accessible) {
                iddma_status status = write_buffer_data((void*)(req.addr + rsize - header_offset), buf_ptr, cur_rsize);
                if (status) {
                    worker_status_ = status;
                    failed = true;
                    break;
                }
            }
            rsize += cur_rsize;
        }
        if (!worker_status_) {
            req.status = KIDDMA_QUEUE_STATUS_SUCCESS;
            if (write_queue_element(KIDDMA_TRANSFER_DESTINATION_RCQ, cpl_head, &req)) break;
            req_tail = (req_tail + 1) & (MAX_QUEUE_SIZE-1);
            cpl_head = (cpl_head + 1) & (MAX_QUEUE_SIZE-1);
            if (write_queue_tail(KIDDMA_TRANSFER_DESTINATION_SRQ, req_tail) ||
                write_queue_head(KIDDMA_TRANSFER_DESTINATION_RCQ, cpl_head)) break;
            wakeup_workers();
        }
    }
}

void iddma_tcp::dma_recv_credit(void) {
    while (!ctrl_tunnel_set_ && !finalize_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock);
    }
    int credit_max;
    int credit_num = 0;
    uint32_t buffer_size = 0;
    if (ctrl_tunnel_set_) {
        tcp_ctrl_tunnel* tunnel = static_cast<tcp_ctrl_tunnel*>(ctrl_tunnel_);
        dma_tcp_ = tunnel->get_tcp_cmd();
        credit_max = tunnel->get_credit_num();
        if (credit_max == 0) credit_max = 1;
        dma_tcp_->set_limit_gbps(limit_gbps_);
    }
    bool failed = false;
    bool activated = false;
    char recv_buf[s_activate_buf_size];
    static const char activate_magic[4] = {'A', 'C', 'T', 'V'};
    static const char credit_magic[4] = {'C', 'R', 'D', 'T'};
    while (!finalize_) {
        int rsize = 0;
        int need_size = 4;
        bool is_activate = true;
        bool is_credit = true;

        uint32_t req_head, req_tail;
        iddma_status stat = read_queue_head_tail(&req_head, &req_tail, &queue_set_->rrq);
        if (!stat && ((req_head + 1) & (MAX_QUEUE_SIZE-1)) != req_tail) {
            while (rsize < need_size && !finalize_) {
                int stat = dma_tcp_->wait(1000);
                if (stat < 0) {
                    worker_status_ = KIDDMA_ERROR_POLL_TIMEOUT;
                    failed = true;
                    break;
                } else if (stat == 0) {
                    continue;
                }
                int cur_rsize = dma_tcp_->recv(recv_buf + rsize, need_size - rsize);
                if (cur_rsize < 0) {
                    worker_status_ = KIDDMA_ERROR_NETWORK_RECV_FAILED;
                    failed = true;
                    break;
                } else {
                    rsize += cur_rsize;
                    if (rsize == s_credit_magic_size) {
                        is_activate = true;
                        is_credit = true;
                        for (int i=0; i<s_credit_magic_size; i++) {
                            if (recv_buf[i] != activate_magic[i]) is_activate = false;
                            if (recv_buf[i] != credit_magic[i]) is_credit = false;
                        }
                        if (is_activate) need_size += 6;
                        if (is_credit) need_size += 4;
                        continue;
                    } else if (rsize == need_size) {
                        if (is_activate) {
                            buffer_size = *((uint32_t*)(recv_buf + 4));
                            credit_num = *((uint16_t*)(recv_buf + 8));
                        } else if (is_credit && !worker_status_) {
                            iddma_queue_element req;
                            uint64_t frame_size = *((uint32_t*)(recv_buf + 4));
                            req.size = frame_size;
                            req.status = KIDDMA_QUEUE_STATUS_VALID;
                            if (write_queue_element(KIDDMA_TRANSFER_DESTINATION_RRQ, req_head, &req)) break;
                            req_head = (req_head + 1) & (MAX_QUEUE_SIZE-1);
                            if (write_queue_head(KIDDMA_TRANSFER_DESTINATION_RRQ, req_head)) break;
                            wakeup_workers();
                        }
                    }
                }
            }
        } else {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock);
        }
    }
}

bool iddma_tcp::check_transfer_available(const iddma_queue* req_queue, const iddma_queue* cpl_queue,
                                            uint32_t& req_tail, uint32_t& cpl_head, iddma_queue_element& req) {
    uint32_t req_head, cpl_tail;
    iddma_status status = read_queue_head_tail(&req_head, &req_tail, req_queue);
    if (status) {
        worker_status_ = status;
        return false;
    }
    status = read_queue_head_tail(&cpl_head, &cpl_tail, cpl_queue);
    if (status) {
        worker_status_ = status;
        return false;
    }
    if (req_head == req_tail || ((cpl_head + 1) & (MAX_QUEUE_SIZE-1)) == cpl_tail) return false;

    status = read_queue_element(&req, &req_queue->queue_elements[req_tail]);
    if (status) {
        worker_status_ = status;
        return false;
    }
    return true;
}
