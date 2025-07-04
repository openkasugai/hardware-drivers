/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include "libroute_controller.h"
#include "route_controller.h"
#include "libutil.h"
#include <map>
#include <memory>
#include <mutex> // NOLINT

static std::map<fpga_route_controller_t, std::unique_ptr<RouteController>> s_instances;
static std::mutex s_mutex;

xse_status_t fpga_route_controller_init(int dev_id, fpga_route_controller_t* info)
{
    try {
        std::unique_ptr<RouteController> instance(new RouteController(dev_id));
        fpga_route_controller_t ret = instance.get();
        std::unique_lock<std::mutex> lock(s_mutex);
        s_instances[ret].swap(instance);
        *info = ret;
        return kErrorSuccess;
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_route_controller_set_tx_stream(fpga_route_controller_t info,
                                                 int ch, int dest, bool need_relation, bool* connected, int instance_id)
{
    try {
        if (!info) return kErrorInvalidArgument;
        if (!connected) return kErrorInvalidArgument;
        RouteController* obj(reinterpret_cast<RouteController*>(info));
        return obj->set_tx_stream(instance_id, ch, dest, need_relation, *connected);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_route_controller_set_rx_stream(fpga_route_controller_t info,
                                                 int ch, int dest, bool need_relation, bool* connected, int instance_id)
{
    try {
        if (!info) return kErrorInvalidArgument;
        if (!connected) return kErrorInvalidArgument;
        RouteController* obj(reinterpret_cast<RouteController*>(info));
        return obj->set_rx_stream(instance_id, ch, dest, need_relation, *connected);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_route_controller_finish(fpga_route_controller_t info)
{
    try {
        if (!info) return kErrorInvalidArgument;
        std::unique_lock<std::mutex> lock(s_mutex);
        auto it = s_instances.find(info);
        if (it == s_instances.end()) return kErrorInvalidArgument;
        s_instances.erase(it);
        return kErrorSuccess;
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_route_controller_allocate_mem(fpga_route_controller_t info, bool is_tx, int ch, uint32_t size, uint64_t* token,
                                                int instance_id)
{
    try {
        if (!info) return kErrorInvalidArgument;
        if (!token) return kErrorInvalidArgument;
        RouteController* obj(reinterpret_cast<RouteController*>(info));
        return obj->allocate_mem(is_tx, instance_id, ch, size, *token);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_route_controller_allocate_mem_with_region(fpga_route_controller_t info,
                                                            bool is_tx, int ch, int mem_id, uint32_t size, uint64_t* token,
                                                            int instance_id)
{
    try {
        if (!info) return kErrorInvalidArgument;
        if (!token) return kErrorInvalidArgument;
        RouteController* obj(reinterpret_cast<RouteController*>(info));
        return obj->allocate_mem(is_tx, instance_id, ch, mem_id, size, *token);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_route_controller_free_mem(fpga_route_controller_t info, uint64_t token)
{
    try {
        if (!info) return kErrorInvalidArgument;
        if (!token) return kErrorInvalidArgument;
        RouteController* obj(reinterpret_cast<RouteController*>(info));
        return obj->free_mem(token);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_route_controller_set_buffer_size(fpga_route_controller_t info, bool is_tx, int ch, uint32_t size, int instance_id)
{
    try {
        if (!info) return kErrorInvalidArgument;
        RouteController* obj(reinterpret_cast<RouteController*>(info));
        return obj->set_buffer_size(is_tx, instance_id, ch, size);
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_route_controller_set_mem(fpga_route_controller_t info,
                                           bool is_tx, int ch, int idx, uint64_t token, uint32_t offset, int instance_id)
{
    try {
        if (!info) return kErrorInvalidArgument;
        if (!token) return kErrorInvalidArgument;
        RouteController* obj(reinterpret_cast<RouteController*>(info));
        return obj->set_mem(is_tx, instance_id, ch, idx, token, offset);
    } catch (...) {
        return kErrorUnknownException;
    }
}
