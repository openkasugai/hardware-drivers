/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef __IDDMA_FPGA_FDMA_HW_H__
#define __IDDMA_FPGA_FDMA_HW_H__

#include <iddma_fpga_fdma.hpp>
#include <libstream_engine.h>
#include <map>
#include <thread>
#include <mutex> // NO_LINT
#include <condition_variable> // NO_LINT

class iddma_fpga_fdma_hw : public iddma_fpga_fdma {
public:
    explicit iddma_fpga_fdma_hw(iddma_device_type device_type);
    ~iddma_fpga_fdma_hw(void);

    void finalize(void);

private:
    // iddma_common functions
    void on_close(void);
    //void post_close(void);
    //iddma_status allocate_queue_set(iddma_queue_set **queue_set);
    //void free_queue_set(iddma_queue_set *queue_set);

    // iddma_fpga functions
    iddma_status enable_dma_engine(void);
    iddma_status set_queue(fpga_stream_engine_t stream_info_, uint64_t req_q_base, uint64_t cpl_q_base,
                           uint64_t req_q_head_tail, uint64_t cpl_q_head_tail);
    iddma_status get_token_offset(uint64_t vaddr, uint64_t& token, uint32_t& offset);
    bool check_option(const std::string& key, const std::string& value);

    iddma_status set_buffer_map(fpga_stream_engine_t& info);

    // variables
    fpga_stream_engine_t stream_engine_h2d_info_;
    fpga_stream_engine_t stream_engine_d2h_info_;

    int check_interval_;
    bool started_;

};
#endif
