/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_tcp.hpp
 * @brief TCP DMA engine class object of iddma.
 *
 */

#ifndef __IDDMA_TCP_HPP__
#define __IDDMA_TCP_HPP__

#include <iddma_engine.hpp>
#include <string>
#include <mutex>
#include <condition_variable>
#include <thread>

class ctrl_tunnel;
class TcpCmd;

class iddma_tcp : public iddma_engine {
public:
    iddma_tcp(iddma_queue_set* queue_set, bool send_enable, bool recv_enable);
    iddma_tcp(iddma_queue_set* queue_set, bool send_enable, bool recv_enable, float limit_gbps);
    virtual ~iddma_tcp(void);

    virtual void wakeup_workers(void);
    virtual iddma_status write_queue_element(iddma_transfer_destination type, int idx, const iddma_queue_element* src);
    virtual iddma_status write_queue_head(iddma_transfer_destination type, uint32_t head);
    virtual iddma_status write_queue_tail(iddma_transfer_destination type, uint32_t tail);

protected:
    virtual iddma_status read_queue_element(iddma_queue_element* dst, const iddma_queue_element* src);
    virtual iddma_status read_queue_head_tail(uint32_t* head, uint32_t *tail, const iddma_queue* queue);
    virtual iddma_status read_buffer_data(void* dst, const void* src, uint32_t size);
    virtual iddma_status write_buffer_data(void* dst, const void* src, uint32_t size);
    virtual bool is_accessible(const void* src);

    static const uint32_t s_network_header_size = 0;
    static const uint32_t s_work_buffer_size = 16384;
private:
    void initialize(bool send_enable, bool recv_enable);
    void check_tcp_cmd(void);

    void dma_send(void);
    void dma_recv(void);
    void dma_send_credit(void);
    void dma_recv_credit(void);

    iddma_queue* get_queue(iddma_transfer_destination type);
    bool check_transfer_available(const iddma_queue* req_queue, const iddma_queue* cpl_queue,
                                  uint32_t& req_tail, uint32_t& cpl_head, iddma_queue_element& req);

    std::mutex mutex_;
    std::condition_variable cond_;
    std::shared_ptr<TcpCmd> dma_tcp_;
    std::unique_ptr<std::thread> worker_send_;
    std::unique_ptr<std::thread> worker_recv_;
    bool finalize_;
    float limit_gbps_;
    iddma_status worker_status_;
};

#endif // __IDDMA_TCP_HPP__
