/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include "module_base.h"
#include "libutil.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#define MODULE_BASE_STR "module_base"

ModuleBase::ModuleBase(void) {}

ModuleBase::~ModuleBase(void)
{
    for (auto fd : m_fds) {
        close(fd);
    }
}

xse_status_t ModuleBase::openDev(const std::string& dev_name)
{
    int fd = open(dev_name.c_str(), O_RDWR);
    if (fd < 0) {
        xse_log(kLogError, MODULE_BASE_STR " Cannot open device file %s\n", dev_name.c_str());
        return kErrorDeviceOpenFailed;
    }
    m_fds.push_back(fd);
    return kErrorSuccess;
}

std::string ModuleBase::getDevBase(void) const
{
    return "/dev/xse";
}

template <typename T>
xse_status_t ModuleBase::regWrite(uint32_t offset, T value, const char* name)
{
    return regWrite(0, offset, value, name);
}

template <typename T>
xse_status_t ModuleBase::regWrite(int fid, uint32_t offset, T value, const char* name)
{
    if (m_fds.size() <= fid) return kErrorNotInitialized;

    off_t pos = lseek(m_fds[fid], offset, SEEK_SET);
    if (pos != offset) return kErrorSeekFailed;

    int ret = write(m_fds[fid], &value, sizeof(value));
    if (ret<0) {
        if (sizeof(value) == 4) {
            xse_log(kLogError, "%s Cannot write register[%4x]: %08x.\n", name, offset, value);
        } else {
            xse_log(kLogError, "%s Cannot write register[%4x]: %016lx.\n", name, offset, value);
        }
        return kErrorWriteRegFailed;
    }
    return kErrorSuccess;
}

xse_status_t ModuleBase::regWrite(uint32_t offset, uint32_t value, const char* name)
{
    return regWrite<uint32_t>(offset, value, name);
}

xse_status_t ModuleBase::regWrite(uint32_t offset, int32_t value, const char* name)
{
    return regWrite<int32_t>(offset, value, name);
}

xse_status_t ModuleBase::regWrite(uint32_t offset, uint64_t value, const char* name)
{
    return regWrite<uint64_t>(offset, value, name);
}

xse_status_t ModuleBase::regWrite(int fid, uint32_t offset, int32_t value, const char* name)
{
    return regWrite<int32_t>(fid, offset, value, name);
}

xse_status_t ModuleBase::regWrite(int fid, uint32_t offset, uint32_t value, const char* name)
{
    return regWrite<uint32_t>(fid, offset, value, name);
}

xse_status_t ModuleBase::regWrite(int fid, uint32_t offset, uint64_t value, const char* name)
{
    return regWrite<uint64_t>(fid, offset, value, name);
}

xse_status_t ModuleBase::regRead(uint32_t offset, uint32_t& value, const char* name)
{
    return regRead(0, offset, value, name);
}

xse_status_t ModuleBase::regRead(int fid, uint32_t offset, uint32_t& value, const char* name)
{
    if (m_fds.size() <= fid) return kErrorNotInitialized;

    off_t pos = lseek(m_fds[fid], offset, SEEK_SET);
    if (pos != offset) return kErrorSeekFailed;

    int ret = read(m_fds[fid], &value, sizeof(value));
    if (ret < 0) {
        xse_log(kLogError, "%s Cannot read StreamEngine register[%4x].\n", name, offset);
        return kErrorReadRegFailed;
    }
   return kErrorSuccess;
}
