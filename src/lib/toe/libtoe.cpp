/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libtoe.h>
#include <toe.h>
#include <libutil.h>
#include <memory>
#include <map>
#include <mutex> //NOLINT

static std::map<TOEDevInstanceDirCh, std::unique_ptr<TOE>> s_instances;
static std::mutex s_mutex;

xse_status_t fpga_toe_init(int dev_id, bool is_tx, int ch, fpga_toe_t* info, int instance_id)
{
    try {
        TOEDevInstanceDirCh dev_instance_dir_ch{dev_id, instance_id, is_tx, ch};
        {
            std::unique_lock<std::mutex> lock(s_mutex);
            auto it = s_instances.find(dev_instance_dir_ch);
            if (it != s_instances.end()) {
                *info  = reinterpret_cast<fpga_toe_t>(it->second.get());
                return kErrorSuccess;
            }
        }
        TOE* instance(new TOE(dev_id, instance_id, is_tx, ch));
        xse_status_t ret = instance->init();
        if (ret) {
            delete instance;
            return ret;
        }
        std::unique_lock<std::mutex> lock(s_mutex);
        s_instances[dev_instance_dir_ch].reset(instance);
        *info = instance;
        return kErrorSuccess;
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_toe_finish(fpga_toe_t info)
{
    xse_status_t ret = fpga_toe_disconnect(info);
    if (ret) return ret;

    try {

        TOE* obj(reinterpret_cast<TOE*>(info));
        auto dev_instance_dir_ch = obj->getDevInstanceDirCh();
        std::unique_lock<std::mutex> lock(s_mutex);
        auto it = s_instances.find(dev_instance_dir_ch);
        if (it != s_instances.end()) {
            s_instances.erase(it);
        }
        return kErrorSuccess;
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_toe_set_buffer_info(fpga_toe_t info,
                                      uint32_t buffer_size, uint32_t credit_num)
{
    try {
        TOE* obj(reinterpret_cast<TOE*>(info));
        return obj->setBuffers(buffer_size, credit_num);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_toe_connect(fpga_toe_t info,
                              const char* target_ip, uint16_t target_port, uint32_t timeout)
{
    try {
        TOE* obj(reinterpret_cast<TOE*>(info));
        return obj->connect(std::string(target_ip), target_port, timeout);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_toe_listen(fpga_toe_t info, uint16_t self_port,
                             const char* target_ip, uint16_t target_port,
                             const char* target_ip_mask, uint16_t target_port_mask)
{
    try {
        TOE* obj(reinterpret_cast<TOE*>(info));
        return obj->listen(self_port, std::string(target_ip), target_port,
                           target_ip_mask ? std::string(target_ip_mask) : std::string(), target_port_mask);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_toe_accept(fpga_toe_t info, uint32_t timeout)
{
    try {
        TOE* obj(reinterpret_cast<TOE*>(info));
        return obj->accept(timeout);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_toe_disconnect(fpga_toe_t info)
{
    try {
        TOE* obj(reinterpret_cast<TOE*>(info));
        return obj->disconnect();
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_toe_activate(fpga_toe_t info)
{
    try {
        TOE* obj(reinterpret_cast<TOE*>(info));
        return obj->activate();
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_toe_get_status(fpga_toe_t info, void* ptr)
{
    try {
        TOE* obj(reinterpret_cast<TOE*>(info));
        return obj->getInfo(ptr);
    } catch (...) {
        return kErrorUnknownException;
    }
}
