/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <test_xse.hpp>
#include <libfunction_base.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <random>
#include <future>
#include <deque>
#include <chrono>
#include <fstream>

#include <test_config.h>

static bool s_finalize = false;
static std::mutex s_mutex;
static std::condition_variable s_cond;

void sig_handler(int sig, siginfo_t *info, void* context) {
    std::unique_lock<std::mutex> lock(s_mutex);
    s_finalize = true;
    s_cond.notify_all();
}

iddma_xse_test::iddma_xse_test(void)
    : func_num_(1),
      dev_ids_({0}),
      tx_ch_ids_({0}),
      rx_ch_ids_({0}),
      func_ids_({NOP_FUNC_ID}),
      fpga_initialized_(false),
      tx_is_tcp_(false),
      rx_is_tcp_(false),
      transferring_(false),
      finalized_(false),
      failed_(false),
      route_controllers_({nullptr}),
      wait_max_(3) {
}

iddma_xse_test::iddma_xse_test(int num)
    : func_num_(num),
      fpga_initialized_(false),
      tx_is_tcp_(false),
      rx_is_tcp_(false),
      transferring_(false),
      finalized_(false),
      failed_(false),
      wait_max_(3) {
    for (int i=0; i<num; i++) {
        dev_ids_.push_back(i);
        tx_ch_ids_.push_back(0);
        rx_ch_ids_.push_back(0);
        func_ids_.push_back(0);
        route_controllers_.push_back(nullptr);
    }
}

iddma_xse_test::iddma_xse_test(int num, int buf_size)
    : func_num_(num),
      fpga_initialized_(false),
      tx_is_tcp_(false),
      rx_is_tcp_(false),
      transferring_(false),
      finalized_(false),
      failed_(false),
      wait_max_(3) {
    for (int i=0; i<num; i++) {
        dev_ids_.push_back(i);
        tx_ch_ids_.push_back(0);
        rx_ch_ids_.push_back(0);
        func_ids_.push_back(0);
        route_controllers_.push_back(nullptr);
        rx_buf_sizes_.push_back(buf_size);
        tx_buf_sizes_.push_back(buf_size);
    }
}

iddma_xse_test::iddma_xse_test(int tx_ch, int rx_ch, int func_id, int buf_size)
    : func_num_(1),
      dev_ids_({0}),
      tx_ch_ids_({tx_ch}),
      rx_ch_ids_({rx_ch}),
      func_ids_({func_id}),
      fpga_initialized_(false),
      tx_is_tcp_(false),
      rx_is_tcp_(false),
      transferring_(false),
      finalized_(false),
      failed_(false),
      route_controllers_({nullptr}),
      wait_max_(3) {
    rx_buf_sizes_.push_back(buf_size);
    tx_buf_sizes_.push_back(buf_size);
}

iddma_xse_nop_test::iddma_xse_nop_test(void)
    : iddma_xse_test() {}

iddma_xse_nop2_test::iddma_xse_nop2_test(void)
    : iddma_xse_test(2, 2048) {}

iddma_xse_nop2_test::iddma_xse_nop2_test(int buf_size)
    : iddma_xse_test(2, buf_size) {}

iddma_xse_nop2_16_test::iddma_xse_nop2_16_test(void)
    : iddma_xse_nop2_test(8192) {}

iddma_xse_nop2_10240_test::iddma_xse_nop2_10240_test(void)
    : iddma_xse_nop2_test(1024*1024) {}

iddma_xse_nop_mm_test::iddma_xse_nop_mm_test(void)
    : iddma_xse_test(0, 0, NOP_MMAPPED_ID, 4096),
      rx_buf_nums_({4}),
      tx_buf_nums_({4}),
      rx_buf_tokens_(1),
      tx_buf_tokens_(1) {
}

iddma_xse_nop_mm_test::iddma_xse_nop_mm_test(int buf_size)
    : iddma_xse_test(0, 0, NOP_MMAPPED_ID, buf_size),
      rx_buf_nums_({4}),
      tx_buf_nums_({4}),
      rx_buf_tokens_(1),
      tx_buf_tokens_(1) {
}

iddma_xse_nop_mm_16_test::iddma_xse_nop_mm_16_test(void)
    : iddma_xse_nop_mm_test(8192) {}

iddma_xse_nop_mm_10240_test::iddma_xse_nop_mm_10240_test(void)
    : iddma_xse_nop_mm_test(1024*1024) {}

iddma_xse_nop_mm_2ddr_10240_test::iddma_xse_nop_mm_2ddr_10240_test(void)
    : iddma_xse_nop_mm_test(1024*1024) {}

iddma_xse_vec_fp_inc_test::iddma_xse_vec_fp_inc_test(void)
    : iddma_xse_test(1, 1, VEC_FP_INC_ID, 8192),
      inc_val_(1.0f) {}

iddma_xse_vec_fp_inc_test::iddma_xse_vec_fp_inc_test(float inc_val, int buf_size)
    : iddma_xse_test(1, 1, VEC_FP_INC_ID, buf_size),
      inc_val_(inc_val) {}

iddma_xse_vec_fp_inc_10240_test::iddma_xse_vec_fp_inc_10240_test(void)
    : iddma_xse_vec_fp_inc_test(3.f, 1024*1024) {}

void iddma_xse_test::SetUp(){
    iddma_test::SetUp();
    if (!fpga_initialized_) {
        for (int i=0; i<func_num_; i++) {
            std::vector<std::string> rx_options;
            rx_options.push_back(std::string("dev_id=") + std::to_string(dev_ids_[i]));
            rx_options.push_back(std::string("h2d_ch=") + std::to_string(rx_ch_ids_[i]));
            rx_options.push_back(std::string("h2d_valid=1"));
            //rx_options.push_back(std::string("check_interval=1000"));
            bool ret = create_iddma_object(KIDDMA_DEVICE_TYPE_FPGA_XSE, rx_options);
            ASSERT_TRUE(ret);

            std::vector<std::string> tx_options;
            tx_options.push_back(std::string("dev_id=") + std::to_string(dev_ids_[i]));
            tx_options.push_back(std::string("d2h_ch=") + std::to_string(tx_ch_ids_[i]));
            tx_options.push_back(std::string("d2h_valid=1"));
            //tx_options.push_back(std::string("check_interval=1000"));
            ret = create_iddma_object(KIDDMA_DEVICE_TYPE_FPGA_XSE, tx_options);
            ASSERT_TRUE(ret);

            connect_function(i);
        }
        fpga_initialized_ = true;
    }
}

void iddma_xse_test::TearDown(){
    iddma_test::TearDown();
    for (auto& rc : route_controllers_) {
        if (rc) {
            EXPECT_EQ(fpga_route_controller_finish(rc), 0);
            rc = nullptr;
        }
    }
}

void iddma_xse_test::connect_function(int func_idx) {
    int ret = fpga_route_controller_init(dev_ids_[func_idx], &route_controllers_[func_idx]);
    ASSERT_EQ(ret, 0);

    int rx_instance_id = rx_is_tcp_ ? 1 : 0;
    int tx_instance_id = tx_is_tcp_ ? 1 : 0;
    bool need_relation = false;
    additional_setup(func_idx, need_relation);

    bool connected = false;
    ret = fpga_route_controller_set_rx_stream(route_controllers_[func_idx], rx_ch_ids_[func_idx],
                                              func_ids_[func_idx], need_relation, &connected, rx_instance_id);
    ASSERT_EQ(ret, 0);
    ASSERT_FALSE(connected);
    ret = fpga_route_controller_set_tx_stream(route_controllers_[func_idx], tx_ch_ids_[func_idx],
                                              func_ids_[func_idx], need_relation, &connected, tx_instance_id);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(connected, need_relation);
}

void iddma_xse_test::additional_setup(int func_idx, bool& need_relation) {
    if (func_idx < tx_buf_sizes_.size()) {
        int tx_instance_id = tx_is_tcp_ ? 1 : 0;
        int ret = fpga_route_controller_set_buffer_size(route_controllers_[func_idx], true,
                                                        tx_ch_ids_[func_idx], tx_buf_sizes_[func_idx], tx_instance_id);
        ASSERT_EQ(ret, 0);
    }
    if (func_idx < rx_buf_sizes_.size()) {
        int rx_instance_id = rx_is_tcp_ ? 1 : 0;
        int ret = fpga_route_controller_set_buffer_size(route_controllers_[func_idx], false,
                                                        rx_ch_ids_[func_idx], rx_buf_sizes_[func_idx], rx_instance_id);
        ASSERT_EQ(ret, 0);
    }
    need_relation = true;
}

void iddma_xse_test::wait_transfer_finishing(void) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto start = std::chrono::system_clock::now();
    auto limit = start + std::chrono::seconds(wait_max_);
    while (!s_finalize) {
        cond_.wait_until(lock, limit);
        auto now = std::chrono::system_clock::now();
        if (now >= limit) break;
    }
}

void iddma_xse_test::connect_worker(int obj_id, int conn_id_base, int* fail, int* listen_readys) {
    *fail = false;
    while (!listen_readys[1]) usleep(10000);
    usleep(500000); // W.A. wait for port listening.
    for (int i=1; i>=0; i--) {
        bool is_connector = i==1;
        int conn_id = conn_id_base + i;
        if (i==0) listen_readys[i] = 1;
        iddma_status ret = connect_iddmas(is_connector, obj_id+i, conn_id);
        EXPECT_EQ(ret, KIDDMA_SUCCESS);
        if (ret) {
            *fail = 1;
            break;
        }
        ret = iddma_mmap_share(obj_[obj_id+i]);
        EXPECT_EQ(ret, KIDDMA_SUCCESS);
        if (ret) {
            *fail = 1;
            break;
        }
    }
}

void iddma_xse_test::close_worker(int obj_id, int* fail) {
    iddma_status ret;
    int retry = 3;
    while ((ret = iddma_close(obj_[obj_id])) == KIDDMA_ERROR_CONNECTION_TIMEOUT && retry) {
        retry--;
    }
    if (ret) *fail = true;
}

void iddma_xse_test::execute(void) {
    execute_pre();

    std::vector<int> fails(obj_.size(), 0);

    // at first, connect to cpu.
    // when parallelly try to establish connections, can not identify connections.
    // W.A. sequentially establish connections.
    bool is_sequential = true;
    bool fail = false;
    std::vector<std::unique_ptr<std::thread>> connect_threads(obj_.size()/2);
    std::vector<int> listen_readys(obj_.size()/2+1, 0);
    for (int i=0; i<obj_.size(); i+=2) {
        int* listen_ready_ptr = &listen_readys[i/2];
        std::unique_ptr<std::thread> th(new std::thread(&iddma_xse_test::connect_worker, this,
                                                        i, i/2, &fails[i/2], listen_ready_ptr));
        if (th) connect_threads[i/2].swap(th);
    }
    listen_readys[obj_.size()/2] = 1;
    for (int i=0; i<obj_.size()/2; i++) {
        connect_threads[i]->join();
        fail |= fails[i];
    }
    if (fail) {
        failed_ = true;
    } else {
        wait_transfer_finishing();
        execute_wait();
    }
    EXPECT_EQ(failed_, false);
    std::vector<std::unique_ptr<std::thread>> close_threads(obj_.size());
    for (int i=0; i<obj_.size(); i++) {
        std::unique_ptr<std::thread> th(new std::thread(&iddma_xse_test::close_worker, this, i, &fails[i]));
        if (th) close_threads[i].swap(th);
    }
    for (int i=0; i<obj_.size(); i++) {
        close_threads[i]->join();
        fail |= fails[i];
    }
    EXPECT_EQ(failed_, false);
}

void iddma_xse_nop_mm_test::additional_setup(int func_idx, bool& need_relation) {
    additional_setup_rx(func_idx, -1);
    additional_setup_tx(func_idx, -1);
    need_relation = false;
}

void iddma_xse_nop_mm_test::additional_setup_rx(int func_idx, int mem_idx) {
    // rx buffer size
    int ret = fpga_route_controller_set_buffer_size(route_controllers_[func_idx], false,
                                                    rx_ch_ids_[func_idx], rx_buf_sizes_[func_idx]);
    for (int i=0; i<rx_buf_nums_[func_idx]; i++) {
        // rx buffer
        uint64_t token;
        ret = fpga_route_controller_allocate_mem_with_region(route_controllers_[func_idx], false,
                                                             rx_ch_ids_[func_idx], mem_idx, rx_buf_sizes_[func_idx], &token);
        ASSERT_EQ(ret, 0);
        ret = fpga_route_controller_set_mem(route_controllers_[func_idx], false,
                                            rx_ch_ids_[func_idx], i, token, 0);
        ASSERT_EQ(ret, 0);
        rx_buf_tokens_[func_idx].push_back(token);
    }
}

void iddma_xse_nop_mm_test::additional_setup_tx(int func_idx, int mem_idx) {
    // tx buffer size
    int ret = fpga_route_controller_set_buffer_size(route_controllers_[func_idx], true,
                                                    tx_ch_ids_[func_idx], tx_buf_sizes_[func_idx]);
    for (int i=0; i<tx_buf_nums_[func_idx]; i++) {
        // tx buffer
        uint64_t token;
        ret = fpga_route_controller_allocate_mem_with_region(route_controllers_[func_idx], true,
                                                             tx_ch_ids_[func_idx], mem_idx, tx_buf_sizes_[func_idx], &token);
        ASSERT_EQ(ret, 0);
        ret = fpga_route_controller_set_mem(route_controllers_[func_idx], true,
                                            tx_ch_ids_[func_idx], i, token, 0);
        ASSERT_EQ(ret, 0);
        tx_buf_tokens_[func_idx].push_back(token);
    }
}

void iddma_xse_nop_mm_2ddr_10240_test::additional_setup(int func_idx, bool& need_relation) {
    additional_setup_rx(func_idx, 0);
    additional_setup_tx(func_idx, 1);
    need_relation = false;
}

void iddma_xse_vec_fp_inc_test::additional_setup(int func_idx, bool& need_relation) {
    fpga_function_base_t func;
    int ret = fpga_function_base_init(dev_ids_[func_idx], "vec_fp_inc_wrapper_0", &func);
    ASSERT_EQ(ret, 0);

    ret = fpga_function_base_reg_write_fp32(func, 0x10, inc_val_);
    ASSERT_EQ(ret, 0);

    ret = fpga_function_base_finish(func);
    ASSERT_EQ(ret, 0);

    iddma_xse_test::additional_setup(func_idx, need_relation);
}

#ifdef ENABLE_NOP
TEST_F(iddma_xse_nop_test, TestNop2) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_NOP;
    wait_max_ = 2;
    execute();
}

TEST_F(iddma_xse_nop_test, TestNop16) {
    wait_max_ = 3;
    execute();
}

TEST_F(iddma_xse_nop_test, TestNop10240) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_NOP_PERF;
    wait_max_ = 10;
    execute();
}
#endif

#ifdef ENABLE_NOP_DUAL
TEST_F(iddma_xse_nop2_test, TestNopX2_2) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_NOP_DUAL;
    wait_max_ = 2;
    execute();
}

TEST_F(iddma_xse_nop2_16_test, TestNopX2_16) {
    wait_max_ = 3;
    execute();
}

TEST_F(iddma_xse_nop2_10240_test, TestNopX2_10240) {
    wait_max_ = 10;
    execute();
}
#endif

#ifdef ENABLE_NOP_MMAPPED
TEST_F(iddma_xse_nop_mm_test, TestNopMM2) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_NOP_MMAPPED;
    wait_max_ = 2;
    execute();
}

TEST_F(iddma_xse_nop_mm_16_test, TestNopMM16) {
    wait_max_ = 3;
    execute();
}

TEST_F(iddma_xse_nop_mm_10240_test, TestNopMM16) {
    wait_max_ = 10;
    execute();
}

TEST_F(iddma_xse_nop_mm_2ddr_10240_test, TestNopMM16) {
    wait_max_ = 10;
    execute();
}
#endif

#ifdef ENABLE_VEC_FP_INC
TEST_F(iddma_xse_vec_fp_inc_test, TestVecFPInc16) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_VEC_FP_INC;
    wait_max_ = 3;
    execute();
}
TEST_F(iddma_xse_vec_fp_inc_10240_test, TestVecFPInc10240) {
    wait_max_ = 10;
    execute();
}
#endif
