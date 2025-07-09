/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <route_controller.h>
#include <route_controller_ioctl.h>
#include <libutil.h>
#include <vector>

//#define _DEBUG
#define ROUTE_CONTROLLER_STR "route_controller"

int RouteController::s_ch_max = 16;
int RouteController::s_ch_step = 32 * sizeof(int);
int RouteController::s_rx_offset = 0;
int RouteController::s_tx_offset = 0x8000;
int RouteController::s_dest_bits = 4;
int RouteController::s_instance_bits = 4;

RouteController::RouteController(void)
    : m_dev_id(0),
      m_initialized(false) {}

RouteController::RouteController(int dev_id)
    : m_dev_id(dev_id),
      m_initialized(false) {}

RouteController::~RouteController(void) {
    finalize();
}

xse_status_t RouteController::set_tx_stream(int ch, int dest, bool need_relation, bool& connected) {
    return set_stream(true, 0, ch, dest, need_relation, connected);
}

xse_status_t RouteController::set_tx_stream(int instance_id, int ch, int dest, bool need_relation, bool& connected) {
    return set_stream(true, instance_id, ch, dest, need_relation, connected);
}

xse_status_t RouteController::set_rx_stream(int ch, int dest, bool need_relation, bool& connected) {
    return set_stream(false, 0, ch, dest, need_relation, connected);
}

xse_status_t RouteController::set_rx_stream(int instance_id, int ch, int dest, bool need_relation, bool& connected) {
    return set_stream(false, instance_id, ch, dest, need_relation, connected);
}

void RouteController::unset_rx_stream(int ch) {
    unset_stream(false, 0, ch, m_rx_config, m_rx_dests, m_dest_rxs, m_dest_txs);
}

void RouteController::unset_rx_stream(int instance_id, int ch) {
    unset_stream(false, instance_id, ch, m_rx_config, m_rx_dests, m_dest_rxs, m_dest_txs);
}

void RouteController::unset_tx_stream(int ch) {
    unset_stream(true, 0, ch, m_tx_config, m_tx_dests, m_dest_txs, m_dest_rxs);
}

void RouteController::unset_tx_stream(int instance_id, int ch) {
    unset_stream(true, instance_id, ch, m_tx_config, m_tx_dests, m_dest_txs, m_dest_rxs);
}

xse_status_t RouteController::unset_streams(int dest) {
    std::vector<int> rx_chs;
    std::vector<int> tx_chs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto rx_it = m_dest_rxs.find(dest);
        if (rx_it != m_dest_rxs.end()) {
            for (auto& it : rx_it->second) rx_chs.push_back(it);
        }
        auto tx_it = m_dest_txs.find(dest);
        if (tx_it != m_dest_txs.end()) {
            for (auto& it : tx_it->second) tx_chs.push_back(it);
        }
    }

    for (auto& it : rx_chs) unset_rx_stream(it);
    for (auto& it : tx_chs) unset_tx_stream(it);

    return kErrorSuccess;
}

xse_status_t RouteController::allocate_mem(bool is_tx, int instance_id, int ch, uint32_t size, uint64_t& token) {
    return allocate_mem(is_tx, instance_id, ch, -1, size, token);
}

xse_status_t RouteController::allocate_mem(bool is_tx, int instance_id, int ch, int mem_id, uint32_t size, uint64_t& token) {
    int port_idx = -1;
    int fd_idx = -1;
    if (initialize(is_tx, instance_id, ch, port_idx, fd_idx)) return kErrorInitializationFailed;
    if (fd_idx < 0) return kErrorInvalidArgument;

    devmem_info_t info;
    info.mem_id = mem_id;
    info.size = size;
    int ret = ioctl(m_fds[fd_idx], XSE_ROUTE_CONTROLLER_ALLOC_DEVMEM, &info);
    if (ret) {
        return kErrorIoctlFailed;
    }

    token = info.token;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (is_tx) {
            m_tx_token_ports[token] = port_idx;
        } else {
            m_rx_token_ports[token] = port_idx;
        }
    }
    return kErrorSuccess;
}

xse_status_t RouteController::free_mem(uint64_t token) {
    devmem_info_t info;
    info.token = token;
    bool is_tx;
    int port_idx = -1;
    int fd_idx = -1;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto tx_it = m_tx_token_ports.find(token);
        auto rx_it = m_rx_token_ports.find(token);
        if (tx_it != m_tx_token_ports.end()) {
            is_tx = true;
            port_idx = tx_it->second;
            fd_idx = m_tx_ports[port_idx].fd_idx;
        } else if (rx_it != m_rx_token_ports.end()) {
            is_tx = false;
            port_idx = rx_it->second;
            fd_idx = m_rx_ports[port_idx].fd_idx;
        }
    }

    int ret = ioctl(m_fds[fd_idx], XSE_ROUTE_CONTROLLER_FREE_DEVMEM, &info);
    if (ret) return kErrorIoctlFailed;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (is_tx) {
            auto tx_it = m_tx_token_ports.find(token);
            if (tx_it != m_tx_token_ports.end()) m_tx_token_ports.erase(tx_it);
        } else {
            auto rx_it = m_rx_token_ports.find(token);
            if (rx_it != m_rx_token_ports.end()) m_rx_token_ports.erase(rx_it);
        }
    }
    return kErrorSuccess;
}

xse_status_t RouteController::set_buffer_size(bool is_tx, int instance_id, int ch, uint32_t size) {
    int port_idx = -1;
    int fd_idx = -1;
    if (initialize(is_tx, instance_id, ch, port_idx, fd_idx)) return kErrorInitializationFailed;
    if (fd_idx < 0) return kErrorInvalidArgument;

    int base = (is_tx ? s_tx_offset : s_rx_offset) + ch * s_ch_step;

    xse_status_t ret = regWrite(fd_idx, base + kCtrlBufSize * 4, size, ROUTE_CONTROLLER_STR);
    if (ret) return ret;

    return kErrorSuccess;
}

xse_status_t RouteController::set_mem(bool is_tx, int instance_id, int ch, int idx, uint64_t token, uint32_t offset) {
    route_controller_set_mem_info_t info;
    info.token = token;
    info.mem_offset = offset;
    info.idx = idx;
    int port_idx = -1;
    int fd_idx = -1;
    if (initialize(is_tx, instance_id, ch, port_idx, fd_idx)) return kErrorInitializationFailed;
    if (fd_idx < 0) return kErrorInvalidArgument;

    int ret = ioctl(m_fds[fd_idx], XSE_ROUTE_CONTROLLER_SET_DEVMEM_ADDR, &info);
    if (ret) return kErrorIoctlFailed;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (is_tx) {
        m_tx_ports[port_idx].is_mmapped = true;
    } else {
        m_rx_ports[port_idx].is_mmapped = true;
    }
    return kErrorSuccess;
}

xse_status_t RouteController::initialize(bool is_tx, int instance_id, int ch, int& port_idx, int& fd_idx) {
    std::lock_guard<std::mutex> lock(m_mutex);

    bool initialized = false;
    std::vector<PortIdentify>& ports(is_tx ? m_tx_ports : m_rx_ports);

    PortIdentify port_id{instance_id, ch, -1, false};
    for (port_idx = 0; port_idx < ports.size(); port_idx++) {
        auto& ref_port(ports[port_idx]);
        if (ref_port.instance_id == instance_id && ref_port.channel == ch) {
            initialized = true;
            port_id.fd_idx = ref_port.fd_idx;
            fd_idx = ref_port.fd_idx;
            break;
        }
    }
    if (!initialized) {
        std::string dev_name(getDevBase());
        dev_name += std::to_string(m_dev_id) + "_route_controller_" + std::to_string(instance_id);
        dev_name += is_tx ? "_out_" : "_in_";
        dev_name += std::to_string(ch);
        port_id.fd_idx = m_fds.size();

        xse_status_t ret = openDev(dev_name);
        if (ret) return ret;

        port_idx = ports.size();
        ports.push_back(port_id);
        fd_idx = port_id.fd_idx;
    }

    return kErrorSuccess;
}

void RouteController::finalize(void) {
    while (!m_rx_config.empty()) {
        PortIdentify& ports(m_rx_ports[m_rx_config.begin()->first]);
        unset_rx_stream(ports.instance_id, ports.channel);
    }
    while (!m_tx_config.empty()) {
        PortIdentify& ports(m_tx_ports[m_tx_config.begin()->first]);
        unset_tx_stream(ports.instance_id, ports.channel);
    }
}

xse_status_t RouteController::set_stream(bool is_tx, int instance_id, int ch, int dest, bool need_relation, bool& connected) {

    int port_idx = -1;
    int fd_idx = -1;
    bool is_mmapped = false;
    if (initialize(is_tx, instance_id, ch, port_idx, fd_idx)) return kErrorInitializationFailed;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (is_tx) {
        m_tx_config[port_idx] = false;
        m_tx_dests[port_idx] = dest;
        m_dest_txs[dest].insert(port_idx);
        is_mmapped = m_tx_ports[port_idx].is_mmapped;
    } else {
        m_rx_config[port_idx] = false;
        m_rx_dests[port_idx] = dest;
        m_dest_rxs[dest].insert(port_idx);
        is_mmapped = m_rx_ports[port_idx].is_mmapped;
    }

    int base = (is_tx ? s_tx_offset : s_rx_offset) + ch * s_ch_step;
    uint32_t val = 1 | (dest << 16) | (is_mmapped ? 0x10 : 0);
    xse_status_t ret = regWrite(fd_idx, base, val, ROUTE_CONTROLLER_STR);
#ifdef _DEBUG
    printf("set stream (%s): dev=%d, instance=%d, ch=%d port_idx=%d dest=%d : %x %08x, %d\n",
           is_tx ? "tx" : "rx", m_dev_id, instance_id, ch, port_idx, dest,
           base, val, ret);
#endif
    if (!need_relation) return ret;

    ret = set_relation(dest, connected);
#ifdef _DEBUG
    printf("set relation %s): dev=%d, instance=%d, ch=%d dest=%d, ret=%d\n", is_tx ? "tx" : "rx",
           m_dev_id, instance_id, ch, dest, ret);
#endif
    return ret;
}

xse_status_t RouteController::set_relation(int dest, bool& connected) {
    connected = false;
    auto rx_dest_it = m_dest_rxs.find(dest);
    auto tx_dest_it = m_dest_txs.find(dest);
    if (rx_dest_it == m_dest_rxs.end() || tx_dest_it == m_dest_txs.end()) return kErrorSuccess;
    if (rx_dest_it->second.size() > 3) return kErrorRelationOverflow;
    if (tx_dest_it->second.size() > 3) return kErrorRelationOverflow;;

    uint32_t rx_relations = 0;
    for (auto& rx_it : rx_dest_it->second) {
        const PortIdentify& port_id(m_rx_ports[rx_it]);
        uint32_t cur_dest = (port_id.instance_id & ((1 << s_instance_bits)-1)) << s_dest_bits;
        cur_dest |= port_id.channel & ((1 << s_dest_bits)-1);
        rx_relations = (rx_relations << 8) | cur_dest;
    }
    rx_relations = (rx_relations << 8) | rx_dest_it->second.size();
    uint32_t tx_relations = 0;
    for (auto& tx_it : tx_dest_it->second) {
        const PortIdentify& port_id(m_tx_ports[tx_it]);
        uint32_t cur_dest = (port_id.instance_id & ((1 << s_instance_bits)-1)) << s_dest_bits;
        cur_dest |= port_id.channel & ((1 << s_dest_bits)-1);
        tx_relations = (tx_relations << 8) | cur_dest;
    }
    tx_relations = (tx_relations << 8) | tx_dest_it->second.size();

#ifdef _DEBUG
    printf("relation: rx: %lu, tx: %lu\n", rx_dest_it->second.size(), tx_dest_it->second.size());
#endif
    for (auto& rx_it : rx_dest_it->second) {
        const PortIdentify& port_id(m_rx_ports[rx_it]);
        int rx_base = s_rx_offset + port_id.channel * s_ch_step;
        xse_status_t ret = regWrite(port_id.fd_idx, rx_base + kCtrlRelation*4, tx_relations, ROUTE_CONTROLLER_STR);
#ifdef _DEBUG
        printf("relation write(rx): [%d] %x, %08x : %d\n", port_id.fd_idx, rx_base + kCtrlRelation*4, tx_relations, ret);
#endif
    }
    for (auto& tx_it : tx_dest_it->second) {
        const PortIdentify& port_id(m_tx_ports[tx_it]);
        int tx_base = s_tx_offset + port_id.channel * s_ch_step;
        xse_status_t ret = regWrite(port_id.fd_idx, tx_base + kCtrlRelation*4, rx_relations, ROUTE_CONTROLLER_STR);
#ifdef _DEBUG
        printf("relation write(tx): [%d] %x, %08x : %d\n", port_id.fd_idx, tx_base + kCtrlRelation*4, rx_relations, ret);
#endif
    }
    connected = !tx_dest_it->second.empty() && !rx_dest_it->second.empty();
    return kErrorSuccess;
}

void RouteController::unset_stream(bool is_tx, int instance_id, int ch, std::map<int, bool>& config, std::map<int, int>& dests,
                                   std::map<int, std::set<int>>& self_chs, std::map<int, std::set<int>>& target_chs)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    int self_offset = is_tx ? s_tx_offset : s_rx_offset;
    int tar_offset = is_tx ? s_rx_offset : s_tx_offset;

    std::vector<PortIdentify>& ports(is_tx ? m_tx_ports : m_rx_ports);
    std::vector<PortIdentify>& tar_ports(is_tx ? m_rx_ports : m_tx_ports);
    int port_idx = -1;
#ifdef _DEBUG
    for (auto& it : ports) {
        printf("unset(%s) ports: instance:%d, channel:%d, fd_idx:%d, mmapped:%d\n",
               is_tx ? "tx" : "rx", it.instance_id, it.channel, it.fd_idx, it.is_mmapped);
    }
#endif
    for (int i = 0; i < ports.size(); i++) {
        if (ports[i].instance_id == instance_id && ports[i].channel == ch) {
            port_idx = i;
            break;
        }
    }
#ifdef _DEBUG
    printf("unset(%s) port_idx:%d, ch:%d\n", is_tx ? "tx" : "rx", port_idx, ch);
#endif
    if (port_idx < 0) return;

#ifdef _DEBUG
    printf("unset(%s) port_idx:%d\n", is_tx ? "tx" : "rx", port_idx);
#endif
    auto it = dests.find(port_idx);
    if (it == dests.end()) return;
    int dest = it->second;
    dests.erase(it);
#ifdef _DEBUG
    printf("unset(%s) port_idx:%d dest erased\n", is_tx ? "tx" : "rx", port_idx);
#endif
    auto self_dest_it = self_chs.find(dest);
    if (self_dest_it != self_chs.end()) {
        auto del_it = self_dest_it->second.find(port_idx);
        if (del_it != self_dest_it->second.end()) {
            self_dest_it->second.erase(del_it);
        }
    }
    uint32_t relations = 0;
    for (auto& rel_it : self_dest_it->second) {
        const PortIdentify& port_id(ports[rel_it]);
        uint32_t cur_dest = (port_id.instance_id & ((1 << s_instance_bits)-1)) << s_dest_bits;
        relations = (relations << 8) | cur_dest;
    }
    relations = (relations << 8) | self_dest_it->second.size();
    if (self_dest_it->second.empty()) {
        self_chs.erase(self_dest_it);
    }

    auto tar_dest_it = target_chs.find(dest);
    if (tar_dest_it != target_chs.end()) {
        for (auto& tar_it : tar_dest_it->second) {
            const PortIdentify& port_id(tar_ports[tar_it]);
            int base = tar_offset + port_id.channel * s_ch_step;
            regWrite(port_id.fd_idx, base + kCtrlRelation*4, relations, ROUTE_CONTROLLER_STR);
        }
    }

    PortIdentify& port_id(ports[port_idx]);

    int base = self_offset + port_id.channel * s_ch_step;
    regWrite(port_id.fd_idx, base + kCtrlControl*4, 0xffff0000, ROUTE_CONTROLLER_STR);
    regWrite(port_id.fd_idx, base + kCtrlRelation*4, 0, ROUTE_CONTROLLER_STR);
    auto cfg_it = config.find(port_idx);
#ifdef _DEBUG
    printf("unset(%s) port_idx:%d config erase? %d\n", is_tx ? "tx" : "rx", port_idx, cfg_it != config.end());
#endif
    if (cfg_it != config.end()) config.erase(cfg_it);
}
