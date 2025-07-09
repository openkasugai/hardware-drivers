/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _TCP_CTRL_TUNNEL_HPP__
#define _TCP_CTRL_TUNNEL_HPP__

#include <ctrl_tunnel.hpp>
#include <memory>
#include <mutex> // NOLINT

class TcpCmd;

class tcp_ctrl_tunnel : public ctrl_tunnel {
public:
    tcp_ctrl_tunnel(int port, const std::string& my_ip);
    tcp_ctrl_tunnel(const std::string& ip, int port, const std::string& my_ip);
    ~tcp_ctrl_tunnel(void);

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

    std::shared_ptr<TcpCmd> get_tcp_cmd(void);
    int get_credit_num(void);

private:
    std::shared_ptr<TcpCmd> tcp_cmd_;

    bool is_connected_;
    int credit_num_;

    std::mutex mutex_;
};

#endif // _CTRL_TUNNEL_HPP__
