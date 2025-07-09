/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include "function_base.h"
#include "libutil.h"

#define FUNCTION_BASE_STR "function_base"

FunctionBase::FunctionBase(const std::string& func_name)
    : m_dev_id(0),
      m_func_name(func_name) {}

FunctionBase::FunctionBase(int dev_id, const std::string& func_name)
    : m_dev_id(dev_id),
      m_func_name(func_name) {}

FunctionBase::~FunctionBase(void) {}

xse_status_t FunctionBase::init(void)
{
    if (m_fds.empty()) {
        std::string dev_name(getDevBase());
        dev_name += std::to_string(m_dev_id) + "_" + m_func_name;

        return openDev(dev_name);
    }
    return kErrorSuccess;
}

xse_status_t FunctionBase::regWrite(uint32_t offset, uint32_t value)
{
    return ModuleBase::regWrite(offset, value, FUNCTION_BASE_STR);
}

xse_status_t FunctionBase::regRead(uint32_t offset, uint32_t& value)
{
    return ModuleBase::regRead(offset, value, FUNCTION_BASE_STR);
}
