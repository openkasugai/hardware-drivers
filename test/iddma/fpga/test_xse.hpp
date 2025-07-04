/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef __TEST_XSE_HPP__
#define __TEST_XSE_HPP__

#include <gtest/gtest.h>
#include <test.h>
#include <libroute_controller.h>
#include <iddma.h>
#include <stdint.h>
#include <vector>
#include <mutex> // NOLINT
#include <condition_variable> // NOLINT

class iddma_xse_test : public iddma_test {
protected:
    int func_num_;
    std::vector<int> dev_ids_;
    std::vector<int> tx_ch_ids_;
    std::vector<int> rx_ch_ids_;
    std::vector<int> func_ids_;

    iddma_xse_test(void);
    iddma_xse_test(int num);
    iddma_xse_test(int num, int buf_size);
    iddma_xse_test(int tx_ch, int rx_ch, int func_id, int buf_size);

    virtual void SetUp();
    virtual void TearDown();

    void connect_function(int func_idx);
    virtual void additional_setup(int func_idx, bool& need_relation);

    void disconnect(void);
    void finalize_cp(void);

    void wait_transfer_finishing(void);

    void execute(void);
    virtual void execute_pre(void) {}
    virtual void execute_wait(void) {}

    void connect_worker(int obj_id, int conn_id_base, int* fail, int* listen_readys);
    void close_worker(int obj_id, int* fail);

    std::vector<fpga_route_controller_t> route_controllers_;

    std::vector<int> rx_buf_sizes_;
    std::vector<int> tx_buf_sizes_;

    bool fpga_initialized_;
    bool tx_is_tcp_;
    bool rx_is_tcp_;
    bool transferring_;
    bool finalized_;
    bool failed_;

    std::mutex mutex_;
    std::condition_variable cond_;

    int wait_max_;
};

class iddma_xse_nop_test : public iddma_xse_test {
public:
    iddma_xse_nop_test(void);
};

class iddma_xse_nop2_test : public iddma_xse_test {
public:
    iddma_xse_nop2_test(void);
    iddma_xse_nop2_test(int buf_size);
};

class iddma_xse_nop2_16_test : public iddma_xse_nop2_test {
public:
    iddma_xse_nop2_16_test(void);
};

class iddma_xse_nop2_10240_test : public iddma_xse_nop2_test {
public:
    iddma_xse_nop2_10240_test(void);
};

class iddma_xse_nop_mm_test : public iddma_xse_test {
public:
    iddma_xse_nop_mm_test(void);
    iddma_xse_nop_mm_test(int buf_size);

protected:
    virtual void additional_setup(int func_idx, bool& need_relation);
    void additional_setup_rx(int func_idx, int mem_idx);
    void additional_setup_tx(int func_idx, int mem_idx);

    std::vector<int> rx_buf_nums_;
    std::vector<int> tx_buf_nums_;

    std::vector<std::vector<uint64_t>> rx_buf_tokens_;
    std::vector<std::vector<uint64_t>> tx_buf_tokens_;
};

class iddma_xse_nop_mm_16_test : public iddma_xse_nop_mm_test {
public:
    iddma_xse_nop_mm_16_test(void);
};

class iddma_xse_nop_mm_10240_test : public iddma_xse_nop_mm_test {
public:
    iddma_xse_nop_mm_10240_test(void);
};

class iddma_xse_nop_mm_2ddr_10240_test : public iddma_xse_nop_mm_test {
public:
    iddma_xse_nop_mm_2ddr_10240_test(void);

protected:
    void additional_setup(int func_idx, bool& need_relation);
};

class iddma_xse_vec_fp_inc_test : public iddma_xse_test {
public:
    iddma_xse_vec_fp_inc_test(void);
    iddma_xse_vec_fp_inc_test(float inc_val, int buf_size);

protected:
    virtual void additional_setup(int func_idx, bool& need_relation);

    float inc_val_;
};

class iddma_xse_vec_fp_inc_10240_test : public iddma_xse_vec_fp_inc_test {
public:
    iddma_xse_vec_fp_inc_10240_test(void);
};

#endif // __TEST_XSE_HPP__
