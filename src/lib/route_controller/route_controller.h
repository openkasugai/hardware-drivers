/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _ROUTE_CONTROLLER_H__
#define _ROUTE_CONTROLLER_H__

#include <module_base.h>
#include <libutil.h>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <mutex> // NOLINT

class RouteController : public ModuleBase {
public:
    RouteController(void);
    explicit RouteController(int dev_id);
    virtual ~RouteController(void);

    xse_status_t set_tx_stream(int ch, int dest, bool need_relation, bool& connected);
    xse_status_t set_rx_stream(int ch, int dest, bool need_relation, bool& connected);
    xse_status_t set_tx_stream(int instance_id, int ch, int dest, bool need_relation, bool& connected);
    xse_status_t set_rx_stream(int instance_id, int ch, int dest, bool need_relation, bool& connected);
    void unset_rx_stream(int ch);
    void unset_rx_stream(int instance_id, int ch);
    void unset_tx_stream(int ch);
    void unset_tx_stream(int instance_id, int ch);
    xse_status_t unset_streams(int dest);

    xse_status_t allocate_mem(bool is_tx, int instance_id, int ch, uint32_t size, uint64_t& token);
    xse_status_t allocate_mem(bool is_tx, int instance_id, int ch, int mem_id, uint32_t size, uint64_t& token);
    xse_status_t free_mem(uint64_t token);
    xse_status_t set_buffer_size(bool is_tx, int instance_id, int ch, uint32_t size);
    xse_status_t set_mem(bool is_tx, int instance_id, int ch, int idx, uint64_t token, uint32_t offset);

private:
    struct PortIdentify {
        int instance_id;
        int channel;
        int fd_idx;
        bool is_mmapped;
    };

    enum CtrlOffset {
        kCtrlControl = 0,
        kCtrlBufSize = 1,
        kCtrlBufAddrValid = 2,
        kCtrlBufUsed = 3,
        kCtrlRelation = 4,
        kCtrlRelationExt = 5,
        kCtrlBufAddrPage8 = 8,
        kCtrlBufAddrPage31 = 31,
    };

    xse_status_t initialize(bool is_tx, int instance_id, int ch, int& port_idx, int& fd_idx);
    void finalize(void);
    xse_status_t set_stream(bool is_tx, int instance_id, int ch, int dest, bool need_relation, bool& connected);
    xse_status_t set_relation(int dest, bool& connected);
    void unset_stream(bool is_tx, int instance_id, int ch, std::map<int, bool>& config, std::map<int, int>& dests,
                      std::map<int, std::set<int>>& self_chs, std::map<int, std::set<int>>& target_chs);

    int m_dev_id;
    bool m_initialized;
    std::vector<PortIdentify> m_tx_ports;
    std::vector<PortIdentify> m_rx_ports;

    static int s_ch_max;
    static int s_ch_step;
    static int s_rx_offset;
    static int s_tx_offset;
    static int s_dest_bits;
    static int s_instance_bits;

    std::map<int, bool> m_rx_config; // port_idx -> mmap
    std::map<int, bool> m_tx_config; // port_idx -> mmap
    std::map<int, int> m_rx_dests; // port_idx -> dest
    std::map<int, int> m_tx_dests; // port_idx -> dest
    std::map<int, std::set<int>> m_dest_rxs; // dest -> port_idx
    std::map<int, std::set<int>> m_dest_txs; // dest -> port_idx

    std::map<int, std::vector<uint64_t>> m_tx_tokens;
    std::map<int, std::vector<uint64_t>> m_rx_tokens;

    std::map<uint64_t, int> m_tx_token_ports;
    std::map<uint64_t, int> m_rx_token_ports;

    std::mutex m_mutex;
};

#endif // _ROUTE_CONTROLLER_H__
