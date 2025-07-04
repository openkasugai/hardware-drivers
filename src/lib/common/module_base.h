/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _MODULE_BASE_H__
#define _MODULE_BASE_H__

#include <libutil.h>
#include <cstdint>
#include <string>
#include <vector>

class ModuleBase
{
protected:
    ModuleBase(void);

public:
    virtual ~ModuleBase(void);

protected:
    virtual xse_status_t openDev(const std::string& dev_name);
    std::string getDevBase(void) const;

    xse_status_t regWrite(uint32_t offset, int32_t value, const char* name);
    xse_status_t regWrite(uint32_t offset, uint32_t value, const char* name);
    xse_status_t regWrite(uint32_t offset, uint64_t value, const char* name);
    xse_status_t regWrite(int fid, uint32_t offset, int32_t value, const char* name);
    xse_status_t regWrite(int fid, uint32_t offset, uint32_t value, const char* name);
    xse_status_t regWrite(int fid, uint32_t offset, uint64_t value, const char* name);
    xse_status_t regRead(uint32_t offset, uint32_t& value, const char* name);
    xse_status_t regRead(int fid, uint32_t offset, uint32_t& value, const char* name);

    std::vector<int> m_fds;

private:
    template <typename T> xse_status_t regWrite(uint32_t offset, T value, const char* name);
    template <typename T> xse_status_t regWrite(int fid, uint32_t offset, T value, const char* name);
};

#endif // _MODULE_BASE_H__
