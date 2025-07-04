/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef __IDDMA_FPGA_XSE_H__
#define __IDDMA_FPGA_XSE_H__

#include <libstream_engine.h>
#include <libtoe.h>
#include <iddma_fpga.hpp>
#include <map>
#include <thread>
#include <mutex> // NO_LINT
#include <condition_variable> // NO_LINT

class ctrl_tunnel;

class iddma_fpga_xse : public iddma_fpga {
public:
    explicit iddma_fpga_xse(iddma_device_type device_type);
    virtual ~iddma_fpga_xse(void);

    void finalize(void);

protected:
    // iddma_common functions
    virtual void on_close(void);
    virtual void post_close(void);
    virtual iddma_status allocate_queue_set(iddma_queue_set **queue_set);
    virtual void free_queue_set(iddma_queue_set *queue_set);
    virtual void get_acceptor_device_info(int& acceptor_dev_id, int& acceptor_ch_id);
    virtual void get_connector_device_info(int& connector_dev_id, int& connector_ch_id);
    virtual void update_acceptor_device_info(int acceptor_dev_id, int acceptor_ch_id);
    virtual void update_connector_device_info(int connector_dev_id, int connector_ch_id);
    virtual iddma_status create_ctrl_tunnel_tcp(bool is_listen, std::unique_ptr<ctrl_tunnel>& tunnel);

    // iddma_fpga functions
    virtual bool check_option(const std::string& key, const std::string& value);

protected:
    iddma_status get_token_offset(uint64_t vaddr, uint64_t& token, uint32_t& offset);
    iddma_status set_queue(fpga_stream_engine_t stream_info_, uint64_t req_q_base, uint64_t cpl_q_base,
                           uint64_t req_q_head_tail, uint64_t cpl_q_head_tail);
    iddma_status set_buffer_map(fpga_stream_engine_t& info);

    iddma_status enable_tcp_xse(void);
    iddma_status enable_pcie_xse(void);
    virtual iddma_status enable_dma_engine(void);

    iddma_status set_counterpart_doorbell_addr(fpga_stream_engine_t obj, uint32_t cp_dev_id, uint32_t cp_ch_id);
    iddma_status set_counterpart_queue(fpga_stream_engine_t obj, uint32_t cp_dev_id, uint32_t cp_ch_id);
    iddma_status set_counterpart_vpmap(fpga_stream_engine_t obj, uint32_t cp_dev_id, uint32_t cp_ch_id);

    void check_regs(void);

    // variables
    uint32_t stream_dev_id_;
    uint32_t stream_instance_id_;
    uint32_t stream_h2d_ch_;
    uint32_t stream_d2h_ch_;
    bool stream_h2d_valid_;
    bool stream_d2h_valid_;
    fpga_stream_engine_t stream_h2d_obj_;
    fpga_stream_engine_t stream_d2h_obj_;
    uint32_t frame_size_;
    uint16_t credit_num_;
    fpga_toe_t toe_obj_;
    int check_interval_;
    bool started_;
    uint32_t counterpart_dev_id_;
    uint32_t counterpart_h2d_ch_id_;
    uint32_t counterpart_d2h_ch_id_;

    bool finalize_;
    std::unique_ptr<std::thread> worker_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

#endif // __IDDMA_FPGA_XSE_H__
