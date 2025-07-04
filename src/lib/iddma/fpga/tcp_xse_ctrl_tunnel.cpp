/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <tcp_xse_ctrl_tunnel.hpp>
#include <stdexcept>

tcp_xse_ctrl_tunnel::tcp_xse_ctrl_tunnel(bool is_tx, int dev, int instance, int ch, int timeout_sec,
                                         int port, const std::string& my_ip)
    : target_ip_("0.0.0.0"),
      self_ip_(my_ip),
      port_(port),
      target_ip_mask_("0.0.0.0"),
      timeout_sec_(timeout_sec),
      is_connected_(false),
      toe_obj_(nullptr) {
    initialize(is_tx, dev, instance, ch);
}

tcp_xse_ctrl_tunnel::tcp_xse_ctrl_tunnel(bool is_tx, int dev, int instance, int ch, int timeout_sec,
                                         const std::string& ip, int port, const std::string& my_ip)
    : target_ip_(ip),
      self_ip_(my_ip),
      port_(port),
      target_ip_mask_("0.0.0.0"),
      timeout_sec_(timeout_sec),
      is_connected_(false),
      toe_obj_(nullptr) {
    initialize(is_tx, dev, instance, ch);
}

tcp_xse_ctrl_tunnel::~tcp_xse_ctrl_tunnel(void) {
    if (toe_obj_) {
        fpga_toe_finish(toe_obj_);
        toe_obj_ = nullptr;
    }
}

iddma_status tcp_xse_ctrl_tunnel::listen(iddma_queue_set* queue_set, iddma_device_type dev_type,
                                         int acceptor_dev_id, int acceptor_ch_id) {
    xse_status_t stat = fpga_toe_listen(toe_obj_, port_, target_ip_.c_str(), port_, target_ip_mask_.c_str());
    if (stat) return KIDDMA_ERROR_NOT_CONNECTED;
    return KIDDMA_SUCCESS;
}

iddma_status tcp_xse_ctrl_tunnel::accept(iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                                         int& connector_dev_id, int& connector_ch_id) {
    xse_status_t stat = fpga_toe_accept(toe_obj_, timeout_sec_);
    if (stat) return KIDDMA_ERROR_CONNECTION_TIMEOUT;
    is_connected_ = true;
    return KIDDMA_SUCCESS;
}

iddma_status tcp_xse_ctrl_tunnel::connect(iddma_queue_set* queue_set, iddma_device_type dev_type,
                                          iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                                          int connector_dev_id, int connector_ch_id,
                                          int& acceptor_dev_id, int& acceptor_ch_id) {
    xse_status_t stat = fpga_toe_connect(toe_obj_, target_ip_.c_str(), port_, timeout_sec_);
    if (stat) return KIDDMA_ERROR_CONNECTION_TIMEOUT;
    is_connected_ = true;
    return KIDDMA_SUCCESS;
}

iddma_status tcp_xse_ctrl_tunnel::close_pre(void) {
    // nop
    return KIDDMA_SUCCESS;
}

iddma_status tcp_xse_ctrl_tunnel::close_post(void) {
    xse_status_t stat = fpga_toe_disconnect(toe_obj_);
    if (stat) return KIDDMA_ERROR_NOT_CONNECTED;
    is_connected_ = false;
    return KIDDMA_SUCCESS;
}

bool tcp_xse_ctrl_tunnel::is_connected(void) const {
    return is_connected_;
}

iddma_status tcp_xse_ctrl_tunnel::mmap_import(int& map_size) {
    map_size = 0;
    return KIDDMA_SUCCESS;
}

iddma_status tcp_xse_ctrl_tunnel::mmap_export(int map_size) {
    // nop
    return KIDDMA_SUCCESS;
}

iddma_status tcp_xse_ctrl_tunnel::execute_mmap_import(struct mmap_share_data& map_data) {
    // nop
    return KIDDMA_SUCCESS;
}

iddma_status tcp_xse_ctrl_tunnel::execute_mmap_export(struct mmap_share_data& tmp_shdat) {
    // nop
    return KIDDMA_SUCCESS;
}

bool tcp_xse_ctrl_tunnel::process_synchronize(void) {
    return is_connected_;
}

void tcp_xse_ctrl_tunnel::finalize(void) {
    // nop
}

fpga_toe_t tcp_xse_ctrl_tunnel::get_toe_object(void) {
    return toe_obj_;
}

void tcp_xse_ctrl_tunnel::initialize(bool is_tx, int dev, int instance, int ch) {
    xse_status_t stat = fpga_toe_init(dev, is_tx, ch, &toe_obj_, instance);
    if (stat) throw std::runtime_error("fpga toe init failed");
}
