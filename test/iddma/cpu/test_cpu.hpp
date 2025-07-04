/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef TEST_CPU_HPP_
#define TEST_CPU_HPP_

#include <test.h>

class iddma_cpu_test : public iddma_test {
protected:
    iddma_cpu_test(void);
    virtual void SetUp(void);
    virtual void test_set_up_opt(bool is_tx, std::vector<std::string>& options);
    void execute(void);

    void send_worker(int id, bool* send_start, bool* send_ok, bool* send_ready,
                     uint32_t* out_cpl, const uint32_t* in_cpl, uint32_t out_cpl_offset, float limit_gbps);
    void recv_worker(int id, bool* send_ok, bool* recv_ok, bool* recv_result_ok, bool *send_start, bool* recv_ready,
                     uint32_t* out_cpl, const uint32_t* in_cpl, uint32_t out_cpl_offset);
    void connector(bool is_connector, int obj_id, int ch_id, void** bufs, uint32_t buf_num, int* stat);

    typedef iddma_status (*send_function)(iddma_object obj, const void* ptr, size_t size);
    typedef iddma_status (*recv_function)(iddma_object obj, void* ptr, size_t size);
    typedef iddma_status (*poll_function)(iddma_object obj, uint64_t timeout, iddma_queue_element* data);
    typedef std::function<void(int, bool*)> check_function;

    void worker_core(iddma_object obj, bool* op_ok, bool* continous, bool* result_ok, bool* send_start, bool* ready,
                     send_function send_func, recv_function recv_func, poll_function poll_func, check_function check_func,
                     uint32_t* out_cpl, const uint32_t* in_cpl, uint32_t out_cpl_offset, float limit_gbps);
    void checker(int buf_id, bool* result_ok);

    void* rand_buf_;
    int func_num_;
    iddma_test_buffer_type buf_type_;
    uint32_t buf_size_;
    uint32_t buf_num_;
    uint32_t transfer_num_;
    std::vector<void*> tx_bufs_;
    std::vector<void*> rx_bufs_;
    std::vector<void*> exp_bufs_;
    bool is_cp_;

    bool throughput_mode_;
    bool latency_mode_;
    int32_t start_delay_usec_;
    float limit_gbps_;

    std::mutex mutex_;
    std::condition_variable cond_;
};

class iddma_cpu_nop_test : public iddma_cpu_test {
protected:
    iddma_cpu_nop_test(void);
    void create_data(void* data, void* expects, uint32_t size, const uint64_t* rand_buf, uint32_t rand_num);
};

class iddma_cpu_vec_fp_inc_test : public iddma_cpu_test {
protected:
    iddma_cpu_vec_fp_inc_test(void);
    void create_data(void* data, void* expects, uint32_t size, const uint64_t* rand_buf, uint32_t rand_num);

    float inc_val_;
};

#endif // TEST_CPU_HPP_
