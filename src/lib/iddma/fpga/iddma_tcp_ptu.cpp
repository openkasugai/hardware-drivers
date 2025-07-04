/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <iddma_tcp_ptu.hpp>
#include <arpa/inet.h>
#include <libchain.h>
#include <stdexcept>
#include <chrono>

static in_addr_t convert_ipv4(const char* addr) {
    struct in_addr iaddr;
    int ret = inet_aton(addr, &iaddr);
    if (ret == 0) return -1;

    in_addr_t ret_addr = iaddr.s_addr & 0xff;
    ret_addr = (ret_addr << 8) | ((iaddr.s_addr >> 8) & 0xff);
    ret_addr = (ret_addr << 8) | ((iaddr.s_addr >> 16) & 0xff);
    ret_addr = (ret_addr << 8) | ((iaddr.s_addr >> 24) & 0xff);
    return ret_addr;
}

iddma_tcp_ptu::iddma_tcp_ptu(bool is_server, const std::string counter_ip_addr, uint16_t counter_tcp_port,
                             int dev_id, int ptu_id, int kernel_id, int func_id, bool send_enable, bool recv_enable)
    : dev_id_(dev_id),
      ptu_id_(ptu_id),
      kernel_id_(kernel_id),
      func_id_(func_id),
      rport_(counter_tcp_port),
      cid_(-1),
      listening_(false),
      send_connected_(false),
      recv_connected_(false),
      timeout_sec_(30) {
    initialize(is_server, counter_ip_addr, counter_tcp_port, send_enable, recv_enable);
}

iddma_tcp_ptu::~iddma_tcp_ptu(void) {
    finalize();
}

void iddma_tcp_ptu::initialize(bool is_server, const std::string counter_ip_addr, uint16_t counter_tcp_port,
                               bool send_enable, bool recv_enable) {
    in_addr_t remote_ip(convert_ipv4(counter_ip_addr.c_str()));
    uint32_t cur_cid;
    if (is_server) {
        int ret = fpga_ptu_listen(dev_id_, ptu_id_, counter_tcp_port);
        if (ret < 0) std::runtime_error("tcp_listen failed");

        struct timeval timeout({timeout_sec_, 0});
        ret = fpga_ptu_accept(dev_id_, ptu_id_, counter_tcp_port, remote_ip, counter_tcp_port, &timeout, &cur_cid);
        if (ret < 0) {
            fpga_ptu_listen_close(dev_id_, ptu_id_, counter_tcp_port);
            std::runtime_error("tcp_accept failed");
        }
        listening_ = true;
    } else {
        auto now = std::chrono::system_clock::now();
        auto limit = now + std::chrono::seconds(timeout_sec_);
        struct timeval timeout({1, 0});
        int ret;
        do {
            ret = fpga_ptu_connect(dev_id_, ptu_id_, counter_tcp_port, remote_ip, counter_tcp_port, &timeout, &cur_cid);
            if (!ret) break;
            now = std::chrono::system_clock::now();
        } while (now < limit);
        if (ret < 0) std::runtime_error("tcp_connect failed");
    }
    cid_ = cur_cid;
    if (send_enable) {
        int ret = fpga_chain_connect_egress(dev_id_, kernel_id_, func_id_, cid_);
        if (ret < 0) {
            fpga_ptu_disconnect(dev_id_, ptu_id_, cid_);
            std::runtime_error("connect egress failed");
        }
        send_connected_ = true;
    }
    if (recv_enable) {
        int ret = fpga_chain_connect_ingress(dev_id_, kernel_id_, func_id_, cid_);
        if (ret < 0) {
            fpga_ptu_disconnect(dev_id_, ptu_id_, cid_);
            std::runtime_error("connect ingress failed");
        }
        recv_connected_ = true;
    }
}

void iddma_tcp_ptu::finalize(void) {
    if (send_connected_) fpga_chain_disconnect_egress(dev_id_, kernel_id_, func_id_);
    if (recv_connected_) fpga_chain_disconnect_ingress(dev_id_, kernel_id_, func_id_);
    fpga_ptu_disconnect(dev_id_, ptu_id_, cid_);
    if (listening_) fpga_ptu_listen_close(dev_id_, ptu_id_, rport_);
}
