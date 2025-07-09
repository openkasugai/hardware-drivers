/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _FUNCTION_BASE_H__
#define _FUNCTION_BASE_H__

#include "module_base.h"
#include <libutil.h>

class FunctionBase : public ModuleBase
{
public:
    explicit FunctionBase(const std::string& func_name);
    explicit FunctionBase(int dev_id, const std::string& func_name);
    virtual ~FunctionBase(void);

    xse_status_t init(void);
    xse_status_t regWrite(uint32_t offset, uint32_t value);
    xse_status_t regRead(uint32_t offset, uint32_t& value);

private:
    int m_dev_id;
    std::string m_func_name;
};

#endif // _FUNCTION_BASE_H__
