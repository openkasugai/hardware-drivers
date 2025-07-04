/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _TCP_XSE_CTRL_TUNNEL_HPP__
#define _TCP_XSE_CTRL_TUNNEL_HPP__

#include <ctrl_tunnel.hpp>
#include <memory>
#include <libtoe.h>

class tcp_xse_ctrl_tunnel : public ctrl_tunnel {
public:
    tcp_xse_ctrl_tunnel(bool is_tx, int dev, int instance, int ch, int timeout_sec,
                        int port, const std::string& my_ip);
    tcp_xse_ctrl_tunnel(bool is_tx, int dev, int instance, int ch, int timeout_sec,
                        const std::string& ip, int port, const std::string& my_ip);
    ~tcp_xse_ctrl_tunnel(void);

    iddma_status listen(iddma_queue_set* queue_set, iddma_device_type dev_type,
                        int acceptor_dev_id, int acceptor_ch_id);
    iddma_status accept(iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                        int& connector_dev_id, int& connector_ch_id);
    iddma_status connect(iddma_queue_set* queue_set, iddma_device_type dev_type,
                         iddma_queue_set*& cp_queue_set, iddma_device_type& cp_dev_type,
                         int connector_dev_id, int connector_ch_id,
                         int& acceptor_dev_id, int& acceptor_ch_id);
    iddma_status close_pre(void);
    iddma_status close_post(void);
    bool is_connected(void) const;

    iddma_status mmap_import(int& map_size);
    iddma_status mmap_export(int map_size);
    iddma_status execute_mmap_import(struct mmap_share_data& map_data);
    iddma_status execute_mmap_export(struct mmap_share_data& tmp_shdat);
    bool process_synchronize(void);

    void finalize(void);

    fpga_toe_t get_toe_object(void);

private:
    void initialize(bool is_tx, int dev, int instance, int ch);

    std::string target_ip_;
    std::string self_ip_;
    uint16_t port_;
    std::string target_ip_mask_;
    int timeout_sec_;
    bool is_connected_;
    fpga_toe_t toe_obj_;
};

#endif // _TCP_XSE_CTRL_TUNNEL_HPP__
