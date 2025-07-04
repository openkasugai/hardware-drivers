/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libfunction_base.h>
#include <function_base.h>
#include <libutil.h>
#include <memory>
#include <map>
#include <mutex> // NOLINT

static std::map<fpga_function_base_t, std::unique_ptr<FunctionBase>> s_instances;
static std::mutex s_mutex;

xse_status_t fpga_function_base_init(int dev_id, const char* func_name, fpga_function_base_t* info)
{
    try {
        FunctionBase* instance(new FunctionBase(dev_id, std::string(func_name)));
        xse_status_t ret = instance->init();
        if (ret) {
            delete instance;
            return ret;
        }
        std::unique_lock<std::mutex> lock(s_mutex);
        s_instances[instance].reset(instance);
        *info = instance;
        return kErrorSuccess;
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_function_base_finish(fpga_function_base_t info)
{
    try {
        std::unique_lock<std::mutex> lock(s_mutex);
        auto it = s_instances.find(info);
        if (it != s_instances.end()) {
            s_instances.erase(it);
        }
        return kErrorSuccess;
    } catch (...) {
        return kErrorUnknownException;
    }
}

template<typename T>
static xse_status_t function_reg_write(fpga_function_base_t info, uint32_t offset, T value)
{
    try {
        FunctionBase* obj(reinterpret_cast<FunctionBase*>(info));
        if (!obj) return kErrorInvalidArgument;
        for (int i=0; i<sizeof(T); i+=sizeof(uint32_t)) {
            const uint32_t* ptr((const uint32_t*)((uint8_t*)&value + i));
            uint32_t val = *ptr;
            xse_status_t ret = obj->regWrite(offset, val);
            if (ret) return ret;
        }
        return kErrorSuccess;
    } catch (...) {
        return kErrorUnknownException;
    }
}

template<typename T>
static xse_status_t function_reg_read(fpga_function_base_t info, uint32_t offset, T* value)
{
    try {
        FunctionBase* obj(reinterpret_cast<FunctionBase*>(info));
        if (!obj || !value) return kErrorInvalidArgument;
        for (int i=0; i<sizeof(T); i+=sizeof(uint32_t)) {
            uint32_t val;
            xse_status_t ret = obj->regRead(offset, val);
            if (ret) return ret;
            uint32_t* ptr((uint32_t*)((uint8_t*)value + i));
            *ptr = val;
        }
        return kErrorSuccess;
    } catch (...) {
        return kErrorUnknownException;
    }
}

xse_status_t fpga_function_base_reg_write_uint32(fpga_function_base_t info, uint32_t offset, uint32_t value)
{
    return function_reg_write<uint32_t>(info, offset, value);
}

xse_status_t fpga_function_base_reg_write_uint64(fpga_function_base_t info, uint32_t offset, uint64_t value)
{
    return function_reg_write<uint64_t>(info, offset, value);
}

xse_status_t fpga_function_base_reg_write_fp32(fpga_function_base_t info, uint32_t offset, float value)
{
    return function_reg_write<float>(info, offset, value);
}

xse_status_t fpga_function_base_reg_read_uint32(fpga_function_base_t info, uint32_t offset, uint32_t* value)
{
    return function_reg_read<uint32_t>(info, offset, value);
}

xse_status_t fpga_function_base_reg_read_uint64(fpga_function_base_t info, uint32_t offset, uint64_t* value)
{
    return function_reg_read<uint64_t>(info, offset, value);
}

xse_status_t fpga_function_base_reg_read_fp32(fpga_function_base_t info, uint32_t offset, float* value)
{
    return function_reg_read<float>(info, offset, value);
}
