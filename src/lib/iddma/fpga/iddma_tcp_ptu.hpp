/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_tcp_ptu.hpp
 * @brief TCP PTU engine class object of iddma.
 *
 */

#ifndef __IDDMA_TCP_PTU_HPP__
#define __IDDMA_TCP_PTU_HPP__

#include <iddma_engine.hpp>
#include <libptu.h>
#include <time.h>
#include <string>

class iddma_tcp_ptu : public iddma_engine {
public:
    iddma_tcp_ptu(bool is_server, const std::string counter_ip_addr, uint16_t counter_tcp_port,
                  int dev_id, int ptu_id, int kernel_id, int func_id, bool send_enable, bool recv_enable);
    ~iddma_tcp_ptu(void);

private:
    void initialize(bool is_server, const std::string counter_ip_addr, uint16_t counter_tcp_port,
                    bool send_enable, bool recv_enable);
    void finalize(void);

    int dev_id_;
    int ptu_id_;
    int kernel_id_;
    int func_id_;
    int rport_;
    int cid_;
    bool listening_;
    bool send_connected_;
    bool recv_connected_;
    int timeout_sec_;
};

#endif // __IDDMA_TCP_PTU_HPP__
