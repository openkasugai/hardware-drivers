/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libstream_engine.h>
#include <stream_engine.h>
#include <libutil.h>
#include <map>
#include <mutex> //NOLINT

static std::map<StreamEngineDevDirCh, std::unique_ptr<StreamEngine>> s_instances;
static std::mutex s_mutex;

xse_status_t fpga_stream_engine_init(int dev_id, bool is_d2h, int ch,
                                     fpga_stream_engine_t* info, int check_interval)
{
    try {
        StreamEngineDevDirCh dev_dir_ch{dev_id, is_d2h, ch};
        for (auto it = s_instances.begin(); it != s_instances.end(); ++it) {
            if (it->first == dev_dir_ch) {
                *info  = reinterpret_cast<fpga_stream_engine_t>(it->second.get());
                return kErrorSuccess;
            }
        }
        StreamEngine* instance(new StreamEngine(dev_id, is_d2h, ch, check_interval));
        xse_status_t ret = instance->init();
        if (ret) {
            delete instance;
            return ret;
        }
        std::unique_lock<std::mutex> lock(s_mutex);
        s_instances[dev_dir_ch].reset(instance);
        *info = instance;
        return kErrorSuccess;
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_stream_engine_finish(fpga_stream_engine_t info)
{
    try {
        xse_status_t ret = fpga_stream_engine_disable(info);
        if (ret) return ret;

        StreamEngine* obj(reinterpret_cast<StreamEngine*>(info));
        auto info = obj->getDevDirCh();
        std::unique_lock<std::mutex> lock(s_mutex);
        for (auto it = s_instances.begin(); it != s_instances.end(); ++it) {
            if (it->first == info) {
                s_instances.erase(it);
                return kErrorSuccess;
            }
        }
        return kErrorSuccess;
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_stream_engine_enable(fpga_stream_engine_t info, bool d2d_mode)
{
    try {
        StreamEngine* obj(reinterpret_cast<StreamEngine*>(info));
        if (!obj) return kErrorInvalidArgument;
        return obj->enable(d2d_mode);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_stream_engine_disable(fpga_stream_engine_t info)
{
    try {
        StreamEngine* obj(reinterpret_cast<StreamEngine*>(info));
        if (!obj) return kErrorInvalidArgument;
        return obj->disable();
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_stream_engine_set_queue_with_token(
    fpga_stream_engine_t info,
    uint64_t req_q_token,
    uint32_t req_q_offset,
    uint64_t cpl_q_token,
    uint32_t cpl_q_offset,
    uint64_t req_q_head_tail_token,
    uint32_t req_q_head_tail_offset,
    uint64_t cpl_q_head_tail_token,
    uint32_t cpl_q_head_tail_offset,
    int req_q_depth,
    int cpl_q_depth)
{
    try {
        StreamEngine* obj(reinterpret_cast<StreamEngine*>(info));
        if (!obj) return kErrorInvalidArgument;
        if (!req_q_token || !cpl_q_token || !req_q_head_tail_token || !cpl_q_head_tail_token) return kErrorInvalidArgument;
        if (req_q_depth < 2 || cpl_q_depth < 2) return kErrorInvalidArgument;
        return obj->setQueue(req_q_token, req_q_offset, cpl_q_token, cpl_q_offset,
                             req_q_head_tail_token, req_q_head_tail_offset, cpl_q_head_tail_token, cpl_q_head_tail_offset,
                             req_q_depth, cpl_q_depth);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_stream_engine_set_buffer(
    fpga_stream_engine_t info,
    uint64_t vaddr,
    uint32_t size,
    uint64_t token)
{
    try {
        StreamEngine* obj(reinterpret_cast<StreamEngine*>(info));
        if (!obj) return kErrorInvalidArgument;
        if (!vaddr || !size) return kErrorInvalidArgument;
        return obj->setBuffer(vaddr, size, token);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_stream_engine_set_doorbell_addr(fpga_stream_engine_t info, uint32_t counterpart_dev_id, uint32_t counterpart_ch_id)
{
    try {
        StreamEngine* obj(reinterpret_cast<StreamEngine*>(info));
        if (!obj) return kErrorInvalidArgument;
        return obj->setDoorbellAddr(counterpart_dev_id, counterpart_ch_id);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_stream_engine_set_counterpart_queue(fpga_stream_engine_t info, uint32_t counterpart_dev_id, uint32_t counterpart_ch_id)
{
    try {
        StreamEngine* obj(reinterpret_cast<StreamEngine*>(info));
        if (!obj) return kErrorInvalidArgument;
        return obj->setCounterpartQueue(counterpart_dev_id, counterpart_ch_id);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_stream_engine_set_counterpart_vpmap(fpga_stream_engine_t info, uint32_t counterpart_dev_id, uint32_t counterpart_ch_id)
{
    try {
        StreamEngine* obj(reinterpret_cast<StreamEngine*>(info));
        if (!obj) return kErrorInvalidArgument;
        return obj->setCounterpartVpmap(counterpart_dev_id, counterpart_ch_id);
    } catch (...) {
        return kErrorUnknownException;
    }
}
