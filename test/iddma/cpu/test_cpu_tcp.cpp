/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <test_cpu_tcp.hpp>
#include <thread>
#include <test_config.h>
#include <tcp_cmd.hpp>

static const int s_tcp_ctrl_ch = 9;

iddma_cpu_tcp_test::iddma_cpu_tcp_test(void)
    : is_start_end_(true) {
    start_delay_usec_ = 2000000;
}

iddma_cpu_tcp_test::iddma_cpu_tcp_test(bool mode)
    : is_start_end_(mode) {
    start_delay_usec_ = 2000000;
}

void iddma_cpu_tcp_test::SetUp(void) {
    iddma_test::SetUp();
    allocate_buf(KIDDMA_TEST_BUFFER_MALLOC, &rand_buf_, sizeof(uint64_t) * RANDOM_IMM_NUM);
    create_random((uint64_t*)rand_buf_, RANDOM_IMM_NUM);

    std::vector<std::string> tx_options;
    std::vector<std::string> rx_options;
    tx_options.push_back("transfer_mode=send");
    rx_options.push_back("transfer_mode=recv");

    test_set_up_opt(true, tx_options);
    test_set_up_opt(false, rx_options);

    if (is_start_end_) {
        create_iddma_object(KIDDMA_DEVICE_TYPE_CPU, tx_options);
        create_iddma_object(KIDDMA_DEVICE_TYPE_CPU, rx_options);
    } else {
        create_iddma_object(KIDDMA_DEVICE_TYPE_CPU, rx_options);
        create_iddma_object(KIDDMA_DEVICE_TYPE_CPU, tx_options);
    }
}

void iddma_cpu_tcp_test::test_set_up_opt(bool is_tx, std::vector<std::string>& options) {
    options.push_back("protocol=tcp");
    char* target_ip = nullptr;
    char* target_port_base = nullptr;
    char* self_ip = nullptr;
    char* self_port_base = nullptr;
    int ch_id;
    if (is_start_end_) {
        target_ip = getenv("CPU_CP_IP");
        target_port_base = getenv("CPU_CP_PORT_BASE");
        self_ip = getenv("CPU_SELF_IP");
        self_port_base = getenv("CPU_PORT_BASE");
        ch_id = 0;
    } else {
        target_ip = getenv("CPU_IP");
        target_port_base = getenv("CPU_PORT_BASE");
        self_ip = getenv("CPU_CP_SELF_IP");
        self_port_base = getenv("CPU_CP_PORT_BASE");
        ch_id = 0;
    }
    std::string opt_port("tcp_port=");
    std::string opt_target_ip("ip_addr=");
    std::string opt_self_ip("self_ip_addr=");
    if (is_tx) {
        ASSERT_NE((char*)NULL, target_ip);
        ASSERT_NE((char*)NULL, target_port_base);
        int target_port = std::stol(target_port_base);
        target_port += id_ + ch_id * s_ch_step_;
#ifdef _DEBUG
        printf("target: %s %s -> %d (%d)\n", target_ip, target_port_base, target_port, id_);
#endif
        opt_target_ip += target_ip;
        opt_port += std::to_string(target_port);
        options.push_back(opt_target_ip);
        options.push_back(opt_port);
        if (self_ip) {
            opt_self_ip += self_ip;
            options.push_back(opt_self_ip);
        }
        if (limit_gbps_ > 0.f) {
            options.push_back(std::string("limit_gbps=") + std::to_string(limit_gbps_));
        }
    } else {
        ASSERT_NE((char*)NULL, self_port_base);
        int self_port = std::stol(self_port_base);
        self_port += id_ + ch_id * s_ch_step_;
#ifdef _DEBUG
        printf("self: %s -> %d (%d)\n", self_port_base, self_port, id_);
#endif
        opt_port += std::to_string(self_port);
        options.push_back("server");
        options.push_back(opt_port);
        if (self_ip) {
            opt_self_ip += self_ip;
            options.push_back(opt_self_ip);
#ifdef _DEBUG
            printf("self: ip %s\n", self_ip);
#endif
        }
    }
}

void iddma_cpu_tcp_test::execute(void) {
    if (is_start_end_) {
        char* self_port_base = getenv("CPU_PORT_BASE");
        int self_port = std::stol(self_port_base);
        self_port += id_ + s_tcp_ctrl_ch * s_ch_step_;
        TcpCmd tcp_cmd(self_port);
        ASSERT_TRUE(tcp_cmd.listen());
        ASSERT_TRUE(tcp_cmd.accept());
        iddma_cpu_test::execute();
        tcp_cmd.send("finished");
        tcp_cmd.close();
    } else {
        char* target_ip = getenv("CPU_IP");
        char* target_port_base = getenv("CPU_PORT_BASE");
        int target_port = std::stol(target_port_base);
        target_port += id_ + s_tcp_ctrl_ch * s_ch_step_;
        TcpCmd tcp_cmd(target_ip, target_port);
        ASSERT_TRUE(tcp_cmd.connect());
        execute_cp();
        int size = 0;
        std::vector<char> buf(128);
        while ((size = tcp_cmd.recv(buf)) == 0);
        tcp_cmd.close();
    }
}

void iddma_cpu_tcp_test::execute_cp(void) {
    tx_bufs_.resize(buf_num_);
    rx_bufs_.resize(buf_num_);
    for (int i=0; i<buf_num_; i++) {
        allocate_buf(buf_type_, &tx_bufs_[i], buf_size_);
        rx_bufs_[i] = tx_bufs_[i];
    }

    iddma_status status = connect_iddmas(true, 1, func_num_);
    EXPECT_EQ(status, KIDDMA_SUCCESS);
    for (int i=0; i<tx_bufs_.size(); i++) {
        status = iddma_mmap_populate(obj_[1], tx_bufs_[i], buf_size_);
        EXPECT_EQ(status, KIDDMA_SUCCESS);
    }
    status = iddma_mmap_share(obj_[1]);
    EXPECT_EQ(status, KIDDMA_SUCCESS);

    status = connect_iddmas(false, func_num_-1);
    EXPECT_EQ(status, KIDDMA_SUCCESS);
    for (int i=0; i<rx_bufs_.size(); i++) {
        status = iddma_mmap_populate(obj_[0], rx_bufs_[i], buf_size_);
        EXPECT_EQ(status, KIDDMA_SUCCESS);
    }
    status = iddma_mmap_share(obj_[0]);
    EXPECT_EQ(status, KIDDMA_SUCCESS);

    bool send_start = false;
    bool send_ok = true;
    bool recv_ok = true;
    bool recv_result_ok = true;
    bool send_ready = false;
    bool recv_ready = false;
    uint32_t recv_num = 0;
    uint32_t send_num = buf_num_;
    std::thread rx_worker(&iddma_cpu_tcp_test::recv_worker, this, 0, &send_ok, &recv_ok, &recv_result_ok, &send_start, &recv_ready,
                          &recv_num, &send_num, 0);
    std::thread tx_worker(&iddma_cpu_tcp_test::send_worker, this, 1, &send_start, &send_ok, &send_ready,
                          &send_num, &recv_num, buf_num_, 0.f);

    usleep(10000);
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!send_ready || !recv_ready) cond_.wait(lock);
        send_start = true;
        cond_.notify_all();
    }
    rx_worker.join();
    tx_worker.join();

    for (int i=0; i<2; i++) {
        while ((status = iddma_close(obj_[i])) == KIDDMA_ERROR_CONNECTION_TIMEOUT);
        EXPECT_EQ(KIDDMA_SUCCESS, status);
    }
}

void iddma_cpu_tcp_nop_test::create_data(void* data, void* expects, uint32_t size, const uint64_t* rand_buf, uint32_t rand_num) {
    // same as iddma_cpu_nop_test
    static uint32_t pos = 0;
    uint32_t i;
    for (i=0; i<(size & ~7); i+=8) {
        *((uint64_t*)((uint8_t*)data + i)) = rand_buf[pos];
        *((uint64_t*)((uint8_t*)expects + i)) = rand_buf[pos++];
        if (pos == rand_num) pos = 0;
        //printf("[%8x][%4x]: %016lx\n", pos, i, *((uint64_t*)((uint8_t*)data + i)));
    }
    if (i==size) return;

    uint64_t val = rand_buf[pos++];
    if (pos == rand_num) pos = 0;
    for (; i<size; i++) {
        *((uint8_t*)data + i) = val & 0xff;
        *((uint8_t*)expects + i) = val & 0xff;
        val >>= 8;
    }
}

iddma_cpu_tcp_nop_test::iddma_cpu_tcp_nop_test(void) {
    limit_gbps_ = 30.f;
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_TCP_NOP;
}

iddma_cpu_tcp_nop_test16::iddma_cpu_tcp_nop_test16(void) {
    limit_gbps_ = 30.f;
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_TCP_NOP + 1;
}

iddma_cpu_tcp_nop_test1240::iddma_cpu_tcp_nop_test1240(void) {
    limit_gbps_ = 30.f;
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_TCP_NOP_PERF;
}

iddma_cpu_tcp_nop_cp_test::iddma_cpu_tcp_nop_cp_test(void)
    : iddma_cpu_tcp_test(false) {
    is_cp_ = true;
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_TCP_NOP;
}

iddma_cpu_tcp_nop_cp_test16::iddma_cpu_tcp_nop_cp_test16(void) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_TCP_NOP + 1;
}

iddma_cpu_tcp_nop_cp_test1240::iddma_cpu_tcp_nop_cp_test1240(void) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_TCP_NOP_PERF;
}

#ifdef ENABLE_TCP_NOP
TEST_F(iddma_cpu_tcp_nop_test, TestTcpNop2) {
    execute();
}

TEST_F(iddma_cpu_tcp_nop_test16, TestTcpNop16) {
#ifdef USE_4KPAGES
    buf_type_ = KIDDMA_TEST_BUFFER_MALLOC;
#endif
    buf_size_ = 8192;
    buf_num_ = 5;
    transfer_num_ = 16;
    execute();
}

TEST_F(iddma_cpu_tcp_nop_test1240, TestTcpNop1240) {
    buf_size_ = 1024*1024;
    buf_num_ = 8;
    transfer_num_ = 1240;
    throughput_mode_ = true;
    execute();
}
#endif

#ifdef ENABLE_TCP_NOP_CP
TEST_F(iddma_cpu_tcp_nop_cp_test, TestTcpNopCp2) {
    throughput_mode_ = true;
    execute();
}

TEST_F(iddma_cpu_tcp_nop_cp_test16, TestTcpNopCp16) {
#ifdef USE_4KPAGES
    buf_type_ = KIDDMA_TEST_BUFFER_MALLOC;
#endif
    buf_size_ = 8192;
    buf_num_ = 5;
    transfer_num_ = 16;
    throughput_mode_ = true;
    execute();
}

TEST_F(iddma_cpu_tcp_nop_cp_test1240, TestTcpNopCp1240) {
    buf_size_ = 1024*1024;
    buf_num_ = 8;
    transfer_num_ = 1240;
    throughput_mode_ = true;
    execute();
}
#endif
