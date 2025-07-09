/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <stream_engine.h>
#include <stream_engine_ioctl.h>
#include <libutil.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <memory>
#include <string>
#include <chrono>

#define STREAM_ENGINE_STR "stream_engine"

static const uint64_t s_page_size = 4096;

// VPMAP
// - entry structure
//   byte |31       24|23       16|15        8| 7        0|
//   field|           |valid| size| phys addr | virt addr |
//   - valid is bit[160]
// - stream ctrl structure
//   byte |31       24|23       16|15        8| 7        0|
//   field|req-Q head |cpl-Q base |req-Q base |pend |valid|
//        |     tail  |           |           |           |
//   byte |63       56|55       48|47       40|39       32|
//   field| desc end  | desc base |    |Qdepth|cpl-Q head |
//        |           |           |           |       tail|
//   - pend is bit[32]
//   - valid is bit[0]
//   - Qdepth is consisted of req-Q depth (byte[41]) and cpl-Q depth (byte[40])

bool operator < (const StreamEngineDevDirCh& a, const StreamEngineDevDirCh& b) {
    return (a.dev_id < b.dev_id) ||
        (a.dev_id == b.dev_id && !a.is_d2h && b.is_d2h) ||
        (a.dev_id == b.dev_id && !b.is_d2h && a.ch < b.ch);
}

bool operator == (const StreamEngineDevDirCh& a, const StreamEngineDevDirCh& b) {
    return a.dev_id == b.dev_id && a.is_d2h == b.is_d2h && a.ch == b.ch;
}

StreamEngine::StreamEngine(int dev_id, bool is_d2h, int ch, int check_interval)
    : m_ch_num(16),
    m_vpmap_num(32),
    m_vpmap_aggregate(false),
    m_vpmap_size_in_byte(m_ch_num * m_vpmap_num * s_vpmap_entry_size_in_byte),
    m_stream_ctrl_size_in_byte(m_ch_num * s_stream_ch_entry_size_in_byte),
    m_dev_id(dev_id),
    m_chid(ch),
    m_is_d2h(is_d2h),
    m_interval_cycle(check_interval),
    m_enabled(false),
    m_vpmap_offset(0),
    m_stream_ctrl_offset(m_vpmap_size_in_byte),
    m_ctrl_offset(m_vpmap_size_in_byte + m_stream_ctrl_size_in_byte) {}

StreamEngine::~StreamEngine(void)
{
    disable();
}

xse_status_t StreamEngine::init(void)
{
    if (m_fds.empty()) {
        std::string dev_name("/dev/xse");
        dev_name += std::to_string(m_dev_id) + "_stream_engine_";
        dev_name += m_is_d2h ? "tx_" : "rx_";
        dev_name += std::to_string(m_chid);

        xse_status_t ret = openDev(dev_name);
        if (ret) return ret;
    }

    stream_engine_info_t engine_info;
    int ret = ioctl(m_fds[0], XSE_STREAM_ENGINE_GET_CFG, &engine_info);
    if (ret) return kErrorIoctlFailed;

    m_ch_num = engine_info.ch_num;
    m_vpmap_num = engine_info.vpmap_num;
    m_vpmap_aggregate = engine_info.aggregate;

    m_vpmap_size_in_byte = m_vpmap_num * (m_vpmap_aggregate ? s_agg_vpmap_entry_size_in_byte : m_ch_num * s_vpmap_entry_size_in_byte);
    m_stream_ctrl_size_in_byte = m_ch_num * s_stream_ch_entry_size_in_byte;
    m_stream_ctrl_offset = m_vpmap_size_in_byte;
    m_ctrl_offset = m_vpmap_size_in_byte + m_stream_ctrl_size_in_byte;

    // clear vpmap
    for (int i=0; i<m_vpmap_num; ++i) {
        stream_engine_vp_info_t vpmap_info;
        vpmap_info.entry = i;
        int ret = ioctl(m_fds[0], XSE_STREAM_ENGINE_RESET_VPMAP, &vpmap_info);
        if (ret) return kErrorIoctlFailed;
    }

    // set queue checking interval cycle
    if (m_interval_cycle >= 0) {
        return regWrite(m_ctrl_offset, (uint32_t)m_interval_cycle, STREAM_ENGINE_STR);
    } else {
        return kErrorSuccess;
    }
}

xse_status_t StreamEngine::setQueue(uint64_t req_q_token, uint32_t req_q_offset, uint64_t cpl_q_token, uint32_t cpl_q_offset,
                                    uint64_t req_q_head_tail_token, uint32_t req_q_head_tail_offset,
                                    uint64_t cpl_q_head_tail_token, uint32_t cpl_q_head_tail_offset, int req_q_depth, int cpl_q_depth)
{
    if (m_fds.empty()) return kErrorNotInitialized;

    uint32_t contiguous_size = 0;
    xse_status_t ret = regWriteMemManageVpConv(false, kCtrlReqQBaseLow, req_q_token, 0lu, req_q_offset, contiguous_size);
    xse_log(kLogInfo, "mem_manage_vp_conv: %d, %016lx, %8x, %8x, ret=%d\n",
            kCtrlReqQBaseLow, req_q_token, req_q_offset, contiguous_size, ret);
    if (ret) return ret;
    if (contiguous_size < req_q_depth * s_queue_element_size) return kErrorInvalidAddress;

    contiguous_size = 0;
    ret = regWriteMemManageVpConv(false, kCtrlCplQBaseLow, cpl_q_token, 0lu, cpl_q_offset, contiguous_size);
    xse_log(kLogInfo, "mem_manage_vp_conv: %d, %016lx, %8x, %8x, ret=%d\n",
            kCtrlCplQBaseLow, cpl_q_token, cpl_q_offset, contiguous_size, ret);
    if (ret) return ret;
    if (contiguous_size < cpl_q_depth * s_queue_element_size) return kErrorInvalidAddress;

    contiguous_size = 0;
    ret = regWriteMemManageVpConv(false, kCtrlReqQHeadTailLow, req_q_head_tail_token, 0lu, req_q_head_tail_offset, contiguous_size);
    xse_log(kLogInfo, "mem_manage_vp_conv: %d, %016lx, %8x, %8x, ret=%d\n",
            kCtrlReqQHeadTailLow, req_q_head_tail_token, req_q_head_tail_offset, contiguous_size, ret);
    if (ret) return ret;
    if (contiguous_size < s_queue_head_tail_size) return kErrorInvalidAddress;

    contiguous_size = 0;
    ret = regWriteMemManageVpConv(false, kCtrlCplQHeadTailLow, cpl_q_head_tail_token, 0lu, cpl_q_head_tail_offset, contiguous_size);
    xse_log(kLogInfo, "mem_manage_vp_conv: %d, %016lx, %8x, %8x, ret=%d\n",
            kCtrlCplQHeadTailLow, cpl_q_head_tail_token, cpl_q_head_tail_offset, contiguous_size, ret);
    if (contiguous_size < s_queue_head_tail_size) return kErrorInvalidAddress;
    if (ret) return ret;

    uint32_t qd_val = ((cpl_q_depth & 0xff) << 8) | (req_q_depth & 0xff);
    ret = regWrite(m_stream_ctrl_offset + m_chid * s_stream_ch_entry_size_in_byte + kCtrlQDepth*4, qd_val, STREAM_ENGINE_STR);
    xse_log(kLogInfo, "reg write: %x, %x, %d\n", m_stream_ctrl_offset + m_chid * s_stream_ch_entry_size_in_byte + kCtrlQDepth*4, qd_val, ret);
    return ret;
}

xse_status_t StreamEngine::setBuffer(uint64_t vaddr, uint32_t size, uint64_t token)
{
    if (m_fds.empty()) return kErrorNotInitialized;
    if (m_enabled) return kErrorAlreadyInitialized;
    uint32_t entry_id = m_vpmap.size();
    if (entry_id >= m_vpmap_num) return kErrorInvalidData;

    size += vaddr & (s_page_size - 1);
    vaddr &= ~(s_page_size - 1);

    uint32_t entry_id_start = entry_id;
    uint64_t addr_offset = 0;
    do {
        auto it = m_vpmap.find(vaddr);
        uint32_t ceil_size = 0;
        uint32_t contiguous_size = 0;
        if (it != m_vpmap.end()) {
            contiguous_size = it->second < size ? it->second : size;
            ceil_size = it->second;
        } else {
            xse_status_t ret = regWriteMemManageVpConv(true, entry_id, token, vaddr, addr_offset, contiguous_size);
            xse_log(kLogInfo, "mem_manage_vp_conv: entry:%d, token:%016lx, vaddr:%16lx offset:%8x, cont:%8x, ret=%d\n",
                    entry_id, token, vaddr, addr_offset, contiguous_size, ret);
            if (ret) return ret;

            ceil_size = (contiguous_size + s_page_size - 1) & ~(s_page_size - 1);
            m_vpmap[vaddr] = ceil_size;
        }
        vaddr += ceil_size;
        size -= contiguous_size;
        addr_offset += ceil_size;
        entry_id++;
    } while (size && m_vpmap.size() < m_vpmap_num);
#ifdef _DEBUG
    for (int i=entry_id_start; i<entry_id; i++) {
        uint32_t base = m_vpmap_aggregate ? 0 : m_chid * m_vpmap_num * s_vpmap_entry_size_in_byte;
        uint32_t entry_size = m_vpmap_aggregate ? s_agg_vpmap_entry_size_in_byte : s_vpmap_entry_size_in_byte;
        printf("[%4x]:", base + i * entry_size);
        for (int j=0; j<entry_size; j+=4) {
            uint32_t data;
            regRead(base + i*entry_size + j, data, STREAM_ENGINE_STR);
            printf(" %08x", data);
        }
        printf("\n");
    }
#endif
    return size ? kErrorInvalidData : kErrorSuccess;
}

xse_status_t StreamEngine::enable(bool d2d_mode)
{
    // todo: d2d
    if (m_fds.empty()) return kErrorNotInitialized;
    if (!d2d_mode && m_vpmap.empty()) return kErrorNotInitialized;
    uint32_t offset = m_stream_ctrl_offset + (m_chid * s_stream_ch_entry_size_in_byte);

    uint32_t enable_data = d2d_mode ? 0x11 : 1;

    xse_log(kLogInfo, STREAM_ENGINE_STR " enable: chid: %d offset: d2d_mode %d\n",
            m_chid, offset, d2d_mode);
    xse_status_t ret = regWrite(offset, enable_data, STREAM_ENGINE_STR);
    if (ret) return ret;

    if (!d2d_mode || m_is_d2h) {
        auto cur_time = std::chrono::system_clock::now();
        auto limit_time = cur_time + std::chrono::seconds(1);
        uint32_t stat = 0;
        do {
            ret = regRead(offset, stat, STREAM_ENGINE_STR);
            if (stat & 0x100) break;
            usleep(100);
            cur_time = std::chrono::system_clock::now();
        } while (!ret && cur_time < limit_time);

        if (!(stat & 0x100)) {
            xse_log(kLogInfo, STREAM_ENGINE_STR " enable: chid: %d offset: %x stat: %x (%x)\n", m_chid, offset, stat, s_stream_ch_entry_size_in_byte);
            return kErrorInitializationFailed;
        }
    }
    m_enabled = true;
    return ret;
}

xse_status_t StreamEngine::disable(void)
{
    if (m_fds.empty()) return kErrorNotInitialized;
    if (!m_enabled) return kErrorSuccess;

    if (ioctl(m_fds[0], XSE_STREAM_ENGINE_CHECK_CTRL_REGS, NULL) < 0) {
        xse_log(kLogError, STREAM_ENGINE_STR "[%d] Cannot check ctrl registers [%4x]\n", m_chid);
    }
    uint32_t offset = m_stream_ctrl_offset + (m_chid * s_stream_ch_entry_size_in_byte);

    xse_status_t ret = regWrite(offset, 0, STREAM_ENGINE_STR);
    if (!ret) m_enabled = false;
    return ret;
}

xse_status_t StreamEngine::regWriteMemManageVpConv(bool is_vpmap, uint32_t entry,
                                                   uint64_t token, uint64_t addr, uint32_t addr_offset, uint32_t& contiguous_size)
{
    stream_engine_vp_info_t info;
    info.entry = entry;
    info.token = token;
    info.vaddr = addr;
    info.voffset = (uint64_t)addr_offset;
    xse_log(kLogInfo, STREAM_ENGINE_STR " get MemManaged range [%4x]: token %016lx, offset %x.\n",
            entry, token, addr_offset);
    if (ioctl(m_fds[0], XSE_STREAM_ENGINE_GET_MEM_MANAGE_RANGE, &info) < 0) {
        xse_log(kLogError, STREAM_ENGINE_STR "[%d] Cannot get MemManaged range [%4x]: %016lx, %u %x.\n",
                m_chid, entry, token, info.range_id, info.psize);
        return kErrorIoctlFailed;
    }
    if (contiguous_size && info.psize < contiguous_size) return kErrorInvalidAddress;
    xse_log(kLogInfo, STREAM_ENGINE_STR " get MemManaged range [%4x]: token %016lx, offset %x, range_id %u.\n",
            entry, token, addr_offset, info.range_id);
    info.psize = 0;
    if (is_vpmap) {
        if (ioctl(m_fds[0], XSE_STREAM_ENGINE_SET_VPMAP, &info) < 0) {
            xse_log(kLogError, STREAM_ENGINE_STR "[%d] Cannot set StreamEngine register with MemManaged VP-Map [%4x]: %016lx, %016lx, %u, %x.\n",
                    m_chid, entry, token, info.vaddr, info.range_id, info.voffset);
            return kErrorIoctlFailed;
        }
    } else {
        int ret;
        if ((ret = ioctl(m_fds[0], XSE_STREAM_ENGINE_WRITE_PADDR, &info)) < 0) {
            xse_log(kLogError, STREAM_ENGINE_STR "[%d] Cannot set StreamEngine register with MemManaged Paddr ctrl[%4x]: %016lx, %u, %x, %d\n",
                    m_chid, entry, token, info.range_id, info.voffset, ret);
            return kErrorIoctlFailed;
        }
    }
    contiguous_size = info.psize;
    return kErrorSuccess;
}

StreamEngineDevDirCh StreamEngine::getDevDirCh(void)
{
    return StreamEngineDevDirCh{(int)m_dev_id, m_is_d2h, (int)m_chid};
}

xse_status_t StreamEngine::setDoorbellAddr(int dev_id, int ch)
{
    int ret;
    stream_engine_cp_info_t info;
    info.dev_id = dev_id;
    info.ch_id = ch;
    if ((ret = ioctl(m_fds[0], XSE_STREAM_ENGINE_SET_DOORBELL_ADDR, &info)) < 0) {
        xse_log(kLogError, STREAM_ENGINE_STR "[%d] Cannot set StreamEngine register with doorbell addr: %016lx, %d\n",
                m_chid, info.paddr, ret);
        return kErrorIoctlFailed;
    }
    return kErrorSuccess;
}

xse_status_t StreamEngine::setCounterpartQueue(int dev_id, int ch)
{
    int ret;
    stream_engine_cp_info_t info;
    info.dev_id = dev_id;
    info.ch_id = ch;
    if ((ret = ioctl(m_fds[0], XSE_STREAM_ENGINE_SET_QUEUE_INFO, &info)) < 0) {
        xse_log(kLogError, STREAM_ENGINE_STR "[%d] Cannot set StreamEngine register with queue info: %016lx, %d\n",
                m_chid, info.paddr, ret);
        return kErrorIoctlFailed;
    }
    return kErrorSuccess;
}

xse_status_t StreamEngine::setCounterpartVpmap(int dev_id, int ch)
{
    int ret;
    stream_engine_cp_info_t info;
    info.dev_id = dev_id;
    info.ch_id = ch;
    if ((ret = ioctl(m_fds[0], XSE_STREAM_ENGINE_SET_CP_VPMAP, &info)) < 0) {
        xse_log(kLogError, STREAM_ENGINE_STR "[%d] Cannot set StreamEngine register with counterpart vpmap: %016lx, %d\n",
                m_chid, info.paddr, ret);
        return kErrorIoctlFailed;
    }
    return kErrorSuccess;
}
