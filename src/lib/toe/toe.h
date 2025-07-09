/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _TOE_H__
#define _TOE_H__

#include <module_base.h>
#include <libutil.h>
#include <string>
#include <mutex> // NOLINT

struct TOEDevInstanceDirCh {
    int dev_id;
    int instance_id;
    bool is_tx;
    int ch;
};

class TOE : public ModuleBase {
public:
    explicit TOE(int dev_id, int instance_id, bool is_tx, int ch);
    virtual ~TOE(void);

    xse_status_t init(void);
    xse_status_t setBuffers(uint32_t size, uint32_t credit_num);
    xse_status_t connect(const std::string& target_ip, uint16_t target_port, uint32_t timeout_sec);
    xse_status_t listen(uint16_t self_port, const std::string& target_ip, uint16_t target_port,
                        const std::string& target_ip_mask, uint16_t target_port_mask);
    xse_status_t accept(uint32_t timeout_sec);
    xse_status_t disconnect(void);

    xse_status_t activate(void);
    xse_status_t getInfo(void* ptr);

    TOEDevInstanceDirCh getDevInstanceDirCh(void);
private:

    static const int s_global_size = 0x1000;
    static const int s_channel_size = 0x100;
    static const int s_frame_size_reg = 0x0c;
    static const int s_max_credit_reg = 0x10;

    xse_status_t convertStr2int(const std::string& str, uint32_t& val, const std::string& delimiter,
                                const std::string& prefix);

    int m_ch_num;
    int m_dev_id;
    int m_instance_id;
    int m_chid;
    bool m_is_tx;
    bool m_connected;
    bool m_is_server;
    bool m_is_client;
    bool m_started;
    uint32_t m_session_id;

    std::mutex m_mutex;
};

bool operator < (const TOEDevInstanceDirCh& a, const TOEDevInstanceDirCh& b);
bool operator == (const TOEDevInstanceDirCh& a, const TOEDevInstanceDirCh& b);

#endif // _TOE_H__
