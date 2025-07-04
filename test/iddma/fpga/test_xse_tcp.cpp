/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <test_xse_tcp.hpp>
#include <libfunction_base.h>

#include <test_config.h>
#include <tcp_cmd.hpp>

static const int s_tcp_ctrl_ch = 9;

iddma_xse_tcp_test::iddma_xse_tcp_test(void) {
    rx_buf_sizes_.push_back(2048);
    tx_buf_sizes_.push_back(2048);
    tx_is_tcp_ = true;
    rx_is_tcp_ = true;
}

iddma_xse_tcp_test::iddma_xse_tcp_test(int num)
    : iddma_xse_test(num) {
    for (int i=0; i<num; i++) {
        rx_buf_sizes_.push_back(2048);
        tx_buf_sizes_.push_back(2048);
    }
    tx_is_tcp_ = true;
    rx_is_tcp_ = true;
}

iddma_xse_tcp_test::iddma_xse_tcp_test(int num, int buf_size)
    : iddma_xse_test(num, buf_size) {
    tx_is_tcp_ = true;
    rx_is_tcp_ = true;
}

iddma_xse_tcp_test::iddma_xse_tcp_test(int tx_ch, int rx_ch, int func_id, int buf_size)
    : iddma_xse_test(tx_ch, rx_ch, func_id, buf_size) {
    tx_is_tcp_ = true;
    rx_is_tcp_ = true;
}

void iddma_xse_tcp_test::SetUp(){
    iddma_test::SetUp();
    if (!fpga_initialized_) {
        for (int i=0; i<func_num_; i++) {
            std::vector<std::string> rx_options;
            int rx_base_port = std::stoul(getenv("XSE_RX_PORT_BASE"));
            rx_options.push_back("protocol=tcp");
            rx_options.push_back(std::string("dev_id=") + std::to_string(dev_ids_[i]));
            rx_options.push_back(std::string("h2d_ch=") + std::to_string(rx_ch_ids_[i]));
            rx_options.push_back(std::string("h2d_valid=1"));
            rx_options.push_back(std::string("transfer_mode=recv"));
            rx_options.push_back(std::string("server"));
            rx_options.push_back(std::string("tcp_port=") +
                                 std::to_string(id_ + i * s_ch_step_ + rx_base_port));
#ifdef _DEBUG
            printf("rx[%d]: port=%d\n", i, id_ + i * s_ch_step_ + rx_base_port);
#endif
            rx_options.push_back(std::string("frame_size=") + std::to_string(rx_buf_sizes_[i]));
            bool ret = create_iddma_object(KIDDMA_DEVICE_TYPE_FPGA_XSE, rx_options);
            ASSERT_TRUE(ret);

            std::vector<std::string> tx_options;
            int tx_base_port = std::stoul(getenv("XSE_TX_PORT_BASE"));
            tx_options.push_back("protocol=tcp");
            tx_options.push_back(std::string("dev_id=") + std::to_string(dev_ids_[i]));
            tx_options.push_back(std::string("d2h_ch=") + std::to_string(tx_ch_ids_[i]));
            tx_options.push_back(std::string("d2h_valid=1"));
            tx_options.push_back(std::string("transfer_mode=send"));
            tx_options.push_back(std::string("ip_addr=") + getenv("XSE_TX_TARGET_IP"));
#ifdef _DEBUG
            printf("tx[%d]: %s\n", i, tx_options.back().c_str());
#endif
            tx_options.push_back(std::string("tcp_port=") +
                                 std::to_string(id_ + i * s_ch_step_ + tx_base_port));
#ifdef _DEBUG
            printf("tx[%d]: port=%d\n", i, id_ + i * s_ch_step_ + tx_base_port);
#endif
            tx_options.push_back(std::string("frame_size=") + std::to_string(tx_buf_sizes_[i]));
            ret = create_iddma_object(KIDDMA_DEVICE_TYPE_FPGA_XSE, tx_options);
            ASSERT_TRUE(ret);

            connect_function(i);
        }
        fpga_initialized_ = true;
    }
}

void iddma_xse_tcp_test::execute_pre(void)
{
    char* target_ip = getenv("XSE_TX_TARGET_IP");
    char* target_port_base = getenv("XSE_TX_PORT_BASE");
    int target_port = std::stol(target_port_base);
    target_port += id_ + s_tcp_ctrl_ch * s_ch_step_;
    tcp_cmd_.reset(new TcpCmd(target_ip, target_port));
    ASSERT_TRUE(tcp_cmd_->connect());
}

void iddma_xse_tcp_test::execute_wait(void)
{
    int size = 0;
    std::vector<char> buf(128);
    while ((size = tcp_cmd_->recv(buf)) == 0);
    tcp_cmd_->close();
}

iddma_xse_tcp_nop2_test::iddma_xse_tcp_nop2_test(void)
    : iddma_xse_tcp_test() {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_TCP_NOP;
}

iddma_xse_tcp_nop16_test::iddma_xse_tcp_nop16_test(void)
    : iddma_xse_tcp_test(1, 8192) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_TCP_NOP + 1;
}

iddma_xse_tcp_nop1240_test::iddma_xse_tcp_nop1240_test(void)
    : iddma_xse_tcp_test(1, 1024*1024) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_TCP_NOP_PERF;
}

#ifdef ENABLE_TCP_NOP
TEST_F(iddma_xse_tcp_nop2_test, TestTcpNop2) {
    wait_max_ = 5;
    execute();
}

TEST_F(iddma_xse_tcp_nop16_test, TestTcpNop16) {
    wait_max_ = 5;
    execute();
}

TEST_F(iddma_xse_tcp_nop1240_test, TestTcp_Nop1240) {
    wait_max_ = 5;
    execute();
}
#endif
