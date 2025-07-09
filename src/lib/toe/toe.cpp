/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <toe.h>
#include <toe_ctrl_ioctl.h>
#include <libutil.h>
#include <unistd.h>
#include <string.h>
#include <chrono>

#define TOE_STR "toe"

const static int s_status_ofs = 4;
const static int s_session_id_ofs = 8;

TOE::TOE(int dev_id, int instance_id, bool is_tx, int ch)
    : m_ch_num(16),
      m_dev_id(dev_id),
      m_instance_id(instance_id),
      m_chid(ch),
      m_is_tx(is_tx),
      m_connected(false),
      m_is_server(false),
      m_is_client(false),
      m_started(false),
      m_session_id(0)
{}

TOE::~TOE(void)
{
    disconnect();
}

xse_status_t TOE::init(void)
{
    if (m_fds.empty()) {
        std::string dev_name("/dev/xse");
        dev_name += std::to_string(m_dev_id) + "_toe_ctrl_";
        dev_name += std::to_string(m_instance_id) + (m_is_tx ? "_tx_" : "_rx_");
        dev_name += std::to_string(m_chid);
        xse_status_t ret = openDev(dev_name);
        if (ret) return ret;
    }
    uint32_t config;
    regRead(0, config, TOE_STR);
    m_ch_num = 1 << (config & 0xff);

    if (m_chid >= m_ch_num) return kErrorInvalidArgument;

    return kErrorSuccess;
}

xse_status_t TOE::setBuffers(uint32_t size, uint32_t credit_num)
{
    if (m_fds.empty()) return kErrorNotInitialized;

    xse_status_t ret;
    int offset = ((m_is_tx ? 0 : m_ch_num) + m_chid) * s_channel_size + s_global_size;
    ret = regWrite(offset + s_frame_size_reg, size, TOE_STR);
    if (m_is_tx) {
        return ret;
    } else {
        if (ret) return ret;
        return regWrite(offset + s_max_credit_reg, credit_num, TOE_STR);
    }
}

xse_status_t TOE::connect(const std::string& target_ip, uint16_t target_port, uint32_t timeout_sec)
{
    uint32_t ip_value;
    xse_status_t ret = convertStr2int(target_ip, ip_value, ".", "");
    if (ret) return ret;
    if (m_fds.empty()) return kErrorNotInitialized;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_is_server || m_is_client) return kErrorInvalidOperation;
        m_is_client = true;
    }
    toe_ctrl_ioctl_connection_t info;
    info.target_ip = ip_value;
    info.target_port = target_port;
    auto now = std::chrono::system_clock::now();
    auto limit = now + std::chrono::seconds(timeout_sec);
    int ioctl_stat = 0;
    while (now < limit || timeout_sec==0) {
        if ((ioctl_stat = ioctl(m_fds[0], XSE_TOE_CTRL_CONNECT, &info)) == 0 && info.result == 0) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_session_id = info.session_id;
            m_connected = true;
            m_is_client = false;

            return kErrorSuccess;
        }
        usleep(1000000);
        auto now = std::chrono::system_clock::now();
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    m_is_client = false;
    if (ioctl) return kErrorIoctlFailed;
    return kErrorConnectFailed;
}

xse_status_t TOE::listen(uint16_t self_port, const std::string& target_ip, uint16_t target_port,
                         const std::string& target_ip_mask, uint16_t target_port_mask)
{
    uint32_t ip_value, ip_mask_value;
    xse_status_t ret = convertStr2int(target_ip, ip_value, ".", "");
    if (ret) return ret;
    ret = convertStr2int(target_ip_mask, ip_mask_value, ".", "");
    if (ret) return ret;
    if (m_fds.empty()) return kErrorNotInitialized;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_is_server || m_is_client) return kErrorInvalidOperation;
        m_is_server = true;
    }
    toe_ctrl_ioctl_connection_t info;
    info.self_port = self_port;
    info.target_ip = ip_value;
    info.target_ip_mask = ip_mask_value;
    info.target_port = target_port;
    info.target_port_mask = target_port_mask;
    int ioctl_stat;
    if ((ioctl_stat=ioctl(m_fds[0], XSE_TOE_CTRL_LISTEN, &info)) < 0) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_is_server = false;
        return kErrorIoctlFailed;
    }
    if (info.result < 0) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_is_server = false;
        return kErrorListenFailed;
    }
    return kErrorSuccess;
}

xse_status_t TOE::accept(uint32_t timeout_sec)
{
    if (m_fds.empty()) return kErrorNotInitialized;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_is_server) return kErrorInvalidOperation;
    }
    auto now = std::chrono::system_clock::now();
    auto limit = now + std::chrono::seconds(timeout_sec);
    while (now < limit) {
        toe_ctrl_ioctl_connection_t info;
        int ioctl_stat;
        if ((ioctl_stat=ioctl(m_fds[0], XSE_TOE_CTRL_ACCEPT, &info)) == 0 && !info.result) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_is_server = false;
            m_connected = true;
            m_session_id = info.session_id;
            return kErrorSuccess;
        }
        now = std::chrono::system_clock::now();
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    m_is_server = false;
    return kErrorAcceptFailed;
}

xse_status_t TOE::disconnect(void)
{
    if (m_fds.empty()) return kErrorNotInitialized;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_connected) return kErrorSuccess;
        m_connected = false;
    }
    toe_ctrl_ioctl_connection_t info;
    info.session_id = m_session_id;
    if (ioctl(m_fds[0], XSE_TOE_CTRL_DISCONNECT, &info) < 0) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_connected = true;
        return kErrorDisconnectFailed;
    }
    return kErrorSuccess;
}

xse_status_t TOE::activate(void)
{
    if (m_fds.empty()) return kErrorNotInitialized;
    if (m_is_tx || !m_connected) return kErrorInvalidOperation;

    int offset = ((m_is_tx ? 0 : m_ch_num) + m_chid) * s_channel_size + s_global_size;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        uint32_t cmd, status;
        regRead(offset, cmd, TOE_STR);
        regRead(offset + s_status_ofs, status, TOE_STR);
        if (!(status & 0x110) && !(cmd & 0x10)) {
            // not activated and not active, activate
            regWrite(offset, 0x11, TOE_STR);
        }
    }

    auto now = std::chrono::system_clock::now();
    auto limit = now + std::chrono::seconds(5);
    while (now < limit) {
        uint32_t data;
        regRead(offset + s_status_ofs, data, TOE_STR);
        if ((data & 0x111) == 0x111) {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_started = true;
            return kErrorSuccess;
        }
        usleep(100000);
        now = std::chrono::system_clock::now();
    }
#ifdef _DEBUG
    uint32_t data[6];
    for (int i=0; i<6; i++) {
        regRead(offset + i * 4, data[i], TOE_STR);
    }
    for (int i=0; i<6; i+=2) {
        printf("toe[%s]::start failed: %8x, %8x\n", m_is_tx ? "tx" : "rx", data[i], data[i+1]);
    }
#endif
    return kErrorInitializationFailed;
}

xse_status_t TOE::getInfo(void* ptr)
{
    toe_ctrl_ioctl_status_t info;
    int ioctl_stat = ioctl(m_fds[0], XSE_TOE_CTRL_GET_STATUS, &info);
    if (ioctl_stat < 0) return kErrorInvalidData;
    if (ptr) memcpy(ptr, &info, sizeof(info));
    return kErrorSuccess;
}

TOEDevInstanceDirCh TOE::getDevInstanceDirCh(void)
{
    TOEDevInstanceDirCh ret{m_dev_id, m_instance_id, m_is_tx, m_chid};
    return ret;
}

xse_status_t TOE::convertStr2int(const std::string& str, uint32_t& val, const std::string& delimiter,
                                 const std::string& prefix)
{
    std::string cur(str);
    val = 0;
    int shift = 0;
    while (!cur.empty()) {
        std::string digit;
        size_t pos = cur.find(delimiter);
        if (pos != std::string::npos) {
            digit = prefix + cur.substr(0, pos);
            cur = cur.substr(pos + 1);
        } else {
            digit = prefix + cur;
            cur.clear();
        }
        try {
            int cur_val = std::stoul(digit, nullptr, 0);
            val = val | (cur_val << shift);
            shift += 8;
        } catch (...) {
            return kErrorInvalidArgument;
        }
    }
    return kErrorSuccess;
}

bool operator < (const TOEDevInstanceDirCh& a, const TOEDevInstanceDirCh& b)
{
    return a.dev_id < b.dev_id ||
        (a.dev_id == b.dev_id && a.instance_id < b.instance_id) ||
        (a.dev_id == b.dev_id && a.instance_id == b.instance_id && !a.is_tx && b.is_tx) ||
        (a.dev_id == b.dev_id && a.instance_id == b.instance_id && a.is_tx == b.is_tx && a.ch < b.ch);
}

bool operator == (const TOEDevInstanceDirCh& a, const TOEDevInstanceDirCh& b)
{
    return a.dev_id == b.dev_id &&
        a.instance_id == b.instance_id &&
        a.is_tx == b.is_tx &&
        a.ch == b.ch;
}
