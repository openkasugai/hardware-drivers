/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <tcp_ctrl_tunnel.hpp>
#include <tcp_cmd.hpp>
#include <sstream>
#include <cstring>

tcp_ctrl_tunnel::tcp_ctrl_tunnel(int port, const std::string& my_ip)
    : tcp_cmd_(new TcpCmd(port, my_ip)),
      is_connected_(false),
      credit_num_(0) {}

tcp_ctrl_tunnel::tcp_ctrl_tunnel(const std::string& ip, int port, const std::string& my_ip)
    : tcp_cmd_(new TcpCmd(ip, port, my_ip)),
      is_connected_(false),
      credit_num_(0) {}

tcp_ctrl_tunnel::~tcp_ctrl_tunnel(void) {
    tcp_cmd_.reset();
}

iddma_status tcp_ctrl_tunnel::listen(iddma_queue_set* queue_set, iddma_device_type dev_type,
                                     int acceptor_dev_id, int acceptor_ch_id) {
    bool stat = tcp_cmd_->listen();
    return stat ? KIDDMA_SUCCESS : KIDDMA_ERROR_NOT_CONNECTED;
}

iddma_status tcp_ctrl_tunnel::accept(iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                                     int& connector_dev_id, int& connector_ch_id) {
    bool stat = tcp_cmd_->accept();
    if (stat) is_connected_ = true;
    return stat ? KIDDMA_SUCCESS : KIDDMA_ERROR_CONNECTION_TIMEOUT;
}

iddma_status tcp_ctrl_tunnel::connect(iddma_queue_set* queue_set, iddma_device_type dev_type,
                                      iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                                      int connector_dev_id, int connector_ch_id,
                                      int& acceptor_dev_id, int& acceptor_ch_id) {
    bool stat = tcp_cmd_->connect();
    if (stat) is_connected_ = true;
    return stat ? KIDDMA_SUCCESS : KIDDMA_ERROR_CONNECTION_TIMEOUT;
}

iddma_status tcp_ctrl_tunnel::close_pre(void) {
    //nop
    return KIDDMA_SUCCESS;
}

iddma_status tcp_ctrl_tunnel::close_post(void) {
    tcp_cmd_->close();
    is_connected_ = false;
    return KIDDMA_SUCCESS;
}

bool tcp_ctrl_tunnel::is_connected(void) const
{
    return is_connected_;
}

iddma_status tcp_ctrl_tunnel::mmap_import(int& map_size) {
    map_size = 0;
    return KIDDMA_SUCCESS;
}

iddma_status tcp_ctrl_tunnel::mmap_export(int map_size) {
    credit_num_ = std::max(0, map_size - 1);
    return KIDDMA_SUCCESS;
}

iddma_status tcp_ctrl_tunnel::execute_mmap_import(struct mmap_share_data& map_data) {
    // nop
    return KIDDMA_SUCCESS;
}

iddma_status tcp_ctrl_tunnel::execute_mmap_export(struct mmap_share_data& tmp_shdat) {
    // nop
    return KIDDMA_SUCCESS;
}

bool tcp_ctrl_tunnel::process_synchronize(void) {
    return is_connected_;
}

void tcp_ctrl_tunnel::finalize(void) {
    // nop;
}

std::shared_ptr<TcpCmd> tcp_ctrl_tunnel::get_tcp_cmd(void) {
    return tcp_cmd_;
}

int tcp_ctrl_tunnel::get_credit_num(void) {
    return credit_num_;
}
