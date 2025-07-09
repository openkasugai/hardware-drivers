/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_common.cpp
 * @brief Implementation of iddma_common class.
 *
 */

#include <iddma_common.hpp>
#include <iddma_engine.hpp>
#include <iddma_cuda_util.hpp>
#include <tcp_ctrl_tunnel.hpp>
#include <shmem_ctrl_tunnel.hpp>
#include <ftok_util.hpp>
#include <typeinfo>
#include <sstream>

iddma_common::base_config::base_config(void)
    : server_mode(false),
      protocol(KIDDMA_DMA_PROTOCOL_DEFAULT),
      transfer_mode(KIDDMA_TRANSFER_MODE_BOTH),
      poll_interval_ns(100000),
      ftok_id (124),
      timeout_sec(30),
      counter_tcp_port(0),
      limit_gbps(0.f) {}

iddma_common::iddma_common(iddma_device_type device_type)
    : queue_set_(nullptr),
      counterpart_queue_set_(nullptr),
      device_type_(device_type),
      counterpart_device_type_(KIDDMA_DEVICE_TYPE_ANY),
      is_initialized_(false),
      is_connector_(false),
      is_acceptor_(false) {
    cuda_util_.reset(new iddma_cuda_util());
}

iddma_common::~iddma_common(void) {
    finalize();
    cuda_util_.reset();
}

void iddma_common::parse_options(const char **options) {
    while (options != NULL && *options) {
        std::string option(*options);
        std::string value;
        auto pos = option.find("=");
        if (pos != std::string::npos) {
            value = option.substr(pos+1);
            option = option.substr(0, pos);
        }
        if (option.empty()) break;
        check_option(option, value);
        ++options;
    }
    check_protocol();
}

iddma_status iddma_common::initialize(const char **options) {
    iddma_status status;
    if (check_initialization()) return KIDDMA_ERROR_INVALID_OPERATION;
    parse_options(options); // put any initialize function after this function.

    if ((status = allocate_queue_set(&queue_set_)) != KIDDMA_SUCCESS)
        return status;

    is_connector_ = false;
    is_acceptor_ = false;
    is_initialized_ = true;

    return status;
}

void iddma_common::finalize(void) {
    if (!is_initialized_)
        return;
    close();
    free_queue_set(queue_set_);
    queue_set_ = nullptr;

    is_initialized_ = false;
}

iddma_status iddma_common::send(const void *addr, size_t size_byte, uint64_t imm) {
    auto ret = send_and_recv(const_cast<void*>(addr), size_byte, imm, true);
    if (!ret && dma_engine_.get()) dma_engine_->wakeup_workers();
    return ret;
}

iddma_status iddma_common::recv(void *addr, size_t size_byte) {
    auto ret = send_and_recv(addr, size_byte, 0, false);
    if (!ret && dma_engine_.get()) dma_engine_->wakeup_workers();
    return ret;
}

iddma_status iddma_common::poll_send(uint64_t timeout_sec, iddma_queue_element *data) {
    if (data == nullptr)
        return KIDDMA_ERROR_INVALID_OPERATION;
    auto ret = poll_send_and_recv(timeout_sec, data, true);
    if (!ret && dma_engine_.get()) dma_engine_->wakeup_workers();
    return ret;
}

iddma_status iddma_common::poll_recv(uint64_t timeout_sec, iddma_queue_element *data) {
    if (data == nullptr)
        return KIDDMA_ERROR_INVALID_OPERATION;
    auto ret = poll_send_and_recv(timeout_sec, data, false);
    if (!ret && dma_engine_.get()) dma_engine_->wakeup_workers();
    return ret;
}

iddma_status iddma_common::listen(int id, const std::string &directory) {
    if (is_acceptor_) return KIDDMA_SUCCESS;
    if (is_connector_) return KIDDMA_ERROR_ALREADY_CONNECTOR;

    iddma_status status = create_ctrl_tunnel(id, directory, true);
    if (status) return status;

    int acceptor_dev_id, acceptor_ch_id;
    get_acceptor_device_info(acceptor_dev_id, acceptor_ch_id);
    status = ctrl_tunnel_->listen(queue_set_, device_type_, acceptor_dev_id, acceptor_ch_id);
    if (status) return status;

    is_acceptor_ = true;
    return status;
}

iddma_status iddma_common::accept(void) {
    iddma_status status;
    if (!is_acceptor_) return KIDDMA_ERROR_INVALID_OPERATION;
    if (is_connector_) return KIDDMA_ERROR_ALREADY_CONNECTOR;

    if (!ctrl_tunnel_) return KIDDMA_ERROR_INVALID_OPERATION;

    int connector_dev_id, connector_ch_id;
    status = ctrl_tunnel_->accept(counterpart_queue_set_, counterpart_device_type_,
                                  connector_dev_id, connector_ch_id);
    if (status) return status;

    update_connector_device_info(connector_dev_id, connector_ch_id);

    mmap_populate(queue_set_, sizeof(iddma_queue_set));
    return KIDDMA_SUCCESS;
}

iddma_status iddma_common::connect(int id, const std::string &directory) {
    iddma_status status;
    if (is_connector_) return KIDDMA_SUCCESS;
    if (is_acceptor_) return KIDDMA_ERROR_ALREADY_ACCEPTOR;

    status = create_ctrl_tunnel(id, directory, false);
    if (status) return status;

    int acceptor_dev_id, acceptor_ch_id;
    int connector_dev_id, connector_ch_id;
    get_connector_device_info(connector_dev_id, connector_ch_id);

    status = ctrl_tunnel_->connect(queue_set_, device_type_, counterpart_queue_set_, counterpart_device_type_,
                                   connector_dev_id, connector_ch_id, acceptor_dev_id, acceptor_ch_id);

    if (status) return status;

    update_acceptor_device_info(acceptor_dev_id, acceptor_ch_id);
    is_connector_ = true;

    mmap_populate(queue_set_, sizeof(iddma_queue_set));
    return KIDDMA_SUCCESS;
}

iddma_status iddma_common::close(void) {
    if (!is_acceptor_ && !is_connector_)
        return KIDDMA_ERROR_INVALID_OPERATION;

    iddma_status status;
    if (!ctrl_tunnel_.get()) return KIDDMA_ERROR_INVALID_OPERATION;

    status = ctrl_tunnel_->close_pre();

    on_close();
    disable_dma_engine();

    if (!status) status = ctrl_tunnel_->close_post();

    status = release_ctrl_tunnel();
    if (is_acceptor_) {
        is_acceptor_ = false;
    } else if (is_connector_) {
        is_connector_ = false;
    }
    post_close();
    return status;
}

iddma_status iddma_common::mmap_share(void) {
    if (!check_connection())
        return KIDDMA_ERROR_NOT_CONNECTED;
    iddma_status status;

    if (is_connector_) {
        status = mmap_import();
        if (status) return status;
        status = mmap_export();
    } else {
        status = mmap_export();
        if (status) return status;
        status = mmap_import();
    }
    if (status) return status;
    status = post_mmap_share();
    if (status) return status;
    status = enable_dma_engine();
    if (status) return status;
    if (!ctrl_tunnel_->process_synchronize()) return KIDDMA_ERROR_SYNC_FAILED;
    return status;
}

iddma_status iddma_common::mmap_import(void) {
    iddma_status status = KIDDMA_SUCCESS;;
    if (!ctrl_tunnel_) return KIDDMA_ERROR_INVALID_OPERATION;

    int map_size;
    status = ctrl_tunnel_->mmap_import(map_size);
    if (status) return status;
    for (int i = 0; i < map_size; i++) {
        struct mmap_share_data tmp_shdat;
        status = ctrl_tunnel_->execute_mmap_import(tmp_shdat);
        if (status) return status;
        memory_map_import_.push_back(tmp_shdat);
    }
    return status;
}

iddma_status iddma_common::mmap_export(void) {
    iddma_status status = KIDDMA_SUCCESS;
    if (!ctrl_tunnel_.get()) return KIDDMA_ERROR_INVALID_OPERATION;

    uint64_t map_size = memory_map_export_.size();
    status = ctrl_tunnel_->mmap_export(map_size);
    if (status) return status;
    for (int i = 0; i < map_size; i++) {
        struct mmap_share_data tmp_shdat;
        status = get_mmap_export_info(i, tmp_shdat);
        if (status) return status;
        status = ctrl_tunnel_->execute_mmap_export(tmp_shdat);
        if (status) return status;
    }
    return status;
}

bool iddma_common::is_tcp_ctrl(void) const {
    return ctrl_tunnel_.get();
}

iddma_status iddma_common::get_mmap_export_info(int idx, struct mmap_share_data& cur_map) {
    if (idx >= 0 && idx < memory_map_export_.size()) {
        cur_map = memory_map_export_[idx];
        return KIDDMA_SUCCESS;
    }
    return KIDDMA_ERROR_INVALID_ARGUMENT;
}

iddma_status iddma_common::create_ctrl_tunnel(int id, const std::string& directory, bool is_listen) {
    iddma_status status;
    if (!base_config_.counter_ip_addr.empty() || base_config_.server_mode) {
        status = create_ctrl_tunnel_tcp(base_config_.server_mode, ctrl_tunnel_);
    } else {
        status = create_ctrl_tunnel_shmem(id, directory, is_listen);
    }
    return status;
}

iddma_status iddma_common::create_ctrl_tunnel_shmem(int id, const std::string& directory, bool is_listen) {
    try {
        ctrl_tunnel_.reset(new shmem_ctrl_tunnel(id, directory, is_listen,
                                                 base_config_.timeout_sec, base_config_.ftok_id));
    } catch (std::runtime_error& e) {
        return KIDDMA_ERROR_NOT_CONNECTED;
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_common::create_ctrl_tunnel_tcp(bool is_listen, std::unique_ptr<ctrl_tunnel>& tunnel) {
    try {
        if (is_listen) {
            tunnel.reset(new tcp_ctrl_tunnel(base_config_.counter_tcp_port, base_config_.self_ip_addr));
        } else {
            tunnel.reset(new tcp_ctrl_tunnel(base_config_.counter_ip_addr, base_config_.counter_tcp_port,
                                             base_config_.self_ip_addr));
        }
    } catch (std::runtime_error& e) {
        return KIDDMA_ERROR_NOT_CONNECTED;
    }
    return KIDDMA_SUCCESS;
}

iddma_status iddma_common::release_ctrl_tunnel(void) {
    iddma_status status = KIDDMA_SUCCESS;
    ctrl_tunnel_.reset();
    return status;
}

bool iddma_common::check_connection(void) {
    if (!is_connector_ && !is_acceptor_)
        return false;
    if (ctrl_tunnel_.get()) return ctrl_tunnel_->is_connected();
    return false;
}

bool iddma_common::check_initialization(void) {
    return is_initialized_;
}

void iddma_common::on_close(void) {}

void iddma_common::post_close(void) {
    if (ctrl_tunnel_.get()) ctrl_tunnel_->process_synchronize();
}

iddma_status iddma_common::post_mmap_share(void) {
    return KIDDMA_SUCCESS;
}

void iddma_common::get_acceptor_device_info(int& acceptor_dev_id, int& acceptor_ch_id) {
    acceptor_dev_id = 0;
    acceptor_ch_id = 0;
}

void iddma_common::update_connector_device_info(int connector_dev_id, int connector_ch_id) {
    // nop
}

void iddma_common::get_connector_device_info(int& connector_dev_id, int& connector_ch_id) {
    connector_dev_id = 0;
    connector_ch_id = 0;
}

void iddma_common::update_acceptor_device_info(int acceptor_dev_id, int acceptor_ch_id) {
    // nop
}

iddma_status iddma_common::enable_dma_engine(void) {
    if (dma_engine_.get()) {
        dma_engine_->set_ctrl_tunnel(ctrl_tunnel_.get());
    }
    return KIDDMA_SUCCESS;
}

void iddma_common::disable_dma_engine(void) {
    if (ctrl_tunnel_.get()) {
        ctrl_tunnel_->finalize();
    }
    dma_engine_.reset();
}

bool iddma_common::is_num_value(const std::string& value, int64_t& value_num) {
    try {
        value_num = std::stoll(value, nullptr, 0);
        return true;
    } catch (...) {
        return false;
    }
}

bool iddma_common::is_fp_num_value(const std::string& value, float& value_num) {
    try {
        value_num = std::stof(value);
        return true;
    } catch (...) {
        return false;
    }
}

bool iddma_common::check_option(const std::string& key, const std::string& value) {
    int64_t value_num = 0;
    float fp_value_num = 0.f;
    bool is_num = is_num_value(value, value_num);
    bool is_fp_num = (value.find(".") != std::string::npos) && is_fp_num_value(value, fp_value_num);

    if (key == "server") {
        base_config_.server_mode = true;
    } else if (key == "protocol") {
        if (value == "default" || value == "mmap") base_config_.protocol = KIDDMA_DMA_PROTOCOL_DEFAULT;
        else if (value == "tcp") base_config_.protocol = KIDDMA_DMA_PROTOCOL_TCP;
        else return false;
    } else if (key == "ip_addr") {
        base_config_.counter_ip_addr = value;
    } else if (key == "tcp_port" && is_num) {
        base_config_.counter_tcp_port = value_num;
    } else if (key == "self_ip_addr") {
        base_config_.self_ip_addr = value;
    } else if (key == "proj_id" && is_num) {
        base_config_.ftok_id = value_num;
    } else if (key == "poll_interval_ns" && is_num) {
        base_config_.poll_interval_ns = value_num;
    } else if (key == "timeout_sec" && is_num) {
        base_config_.timeout_sec = value_num;
    } else if (key == "transfer_mode" && value == "send") {
        base_config_.transfer_mode = KIDDMA_TRANSFER_MODE_SEND;
    } else if (key == "transfer_mode" && value == "recv") {
        base_config_.transfer_mode = KIDDMA_TRANSFER_MODE_RECV;
    } else if (key == "transfer_mode" && value == "both") {
        base_config_.transfer_mode = KIDDMA_TRANSFER_MODE_BOTH;
    } else if (key == "limit_gbps" && (is_num || is_fp_num)) {
        if (is_fp_num) {
            base_config_.limit_gbps = fp_value_num;
        } else {
            base_config_.limit_gbps = (float)value_num;
        }
    } else {
        return false;
    }
    return true;
}

void iddma_common::check_protocol(void) {
    if (base_config_.counter_ip_addr.empty() && !base_config_.server_mode) {
        base_config_.protocol = KIDDMA_DMA_PROTOCOL_DEFAULT;
    }
}
