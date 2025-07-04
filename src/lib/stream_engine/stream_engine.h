/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _STREAM_ENGINE_H__
#define _STREAM_ENGINE_H__

#include <memory>
#include <map>
#include "module_base.h"
#include <libutil.h>

struct StreamEngineDevDirCh {
    int dev_id;
    bool is_d2h;
    int ch;
};

class StreamEngine : public ModuleBase
{
public:
    StreamEngine(int dev_id, bool is_d2h, int ch, int check_interval);
    ~StreamEngine(void);

    xse_status_t init(void);
    xse_status_t setQueue(uint64_t req_q_token, uint32_t req_q_offset, uint64_t cpl_q_token, uint32_t cpl_q_offset,
                          uint64_t req_q_head_tail_token, uint32_t req_q_head_tail_offset,
                          uint64_t cpl_q_head_tail_token, uint32_t cpl_q_head_tail_offset, int req_q_depth, int cpl_q_depth);
    xse_status_t setBuffer(uint64_t vaddr, uint32_t size, uint64_t token);
    xse_status_t enable(bool d2d_mode);
    xse_status_t disable(void);
    StreamEngineDevDirCh getDevDirCh(void);
    xse_status_t setDoorbellAddr(int dev_id, int ch);
    xse_status_t setCounterpartQueue(int dev_id, int ch);
    xse_status_t setCounterpartVpmap(int dev_id, int ch);

private:
    enum CtrlRegEntry {
        kCtrlControl = 0,
        kCtrlDebugCount = 1,
        kCtrlReqQBaseLow = 2,
        kCtrlReqQBaseHigh = 3,
        kCtrlCplQBaseLow = 4,
        kCtrlCplQBaseHigh = 5,
        kCtrlReqQHeadTailLow = 6,
        kCtrlReqQHeadTailHigh = 7,
        kCtrlCplQHeadTailLow = 8,
        kCtrlCplQHeadTailHigh = 9,
        kCtrlQDepth = 10,
        kCtrlDebugIssueCompletion = 11,
        kCtrlDoorbellLow = 12,
        kCtrlDoorbellHigh = 13,
        kCtrlInterrupt = 14,
        kCtrlDoorbellTarget = 15,
    };


    StreamEngine(void);
    xse_status_t regWriteMemManageVpConv(bool is_vpmap, uint32_t entry, uint64_t token, uint64_t addr, uint32_t addr_offset, uint32_t& contiguous_size);

    static const int s_vpmap_entry_size_in_byte = 32;
    static const int s_agg_vpmap_entry_size_in_byte = 16;
    static const int s_stream_ch_entry_size_in_byte = 64;
    static const int s_queue_element_size = 32;
    static const int s_queue_head_tail_size = 8;

    int m_ch_num;
    int m_vpmap_num;
    bool m_vpmap_aggregate;
    int m_vpmap_size_in_byte;
    int m_stream_ctrl_size_in_byte;

    uint32_t m_dev_id;
    uint16_t m_chid;
    bool m_is_d2h;
    int m_interval_cycle;
    int m_enabled;

    uint32_t m_vpmap_offset;
    uint32_t m_stream_ctrl_offset;
    uint32_t m_ctrl_offset;

    std::map<uint64_t, uint32_t> m_vpmap;
};

bool operator < (const StreamEngineDevDirCh& a, const StreamEngineDevDirCh& b);
bool operator == (const StreamEngineDevDirCh& a, const StreamEngineDevDirCh& b);

#endif // _STREAM_ENGINE_H__
