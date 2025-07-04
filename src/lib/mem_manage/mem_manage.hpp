/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _MEM_MANAGE_HPP__
#define _MEM_MANAGE_HPP__

#include <mem_manage.h>
#include <cstdint>
#include <map>
#include <vector>
#include <mutex> // NOLINT

class MemManage {
public:
  MemManage(void);
  ~MemManage(void);

  MemManageStatus pinCudaDevBuffer(void* dev_ptr, size_t size, uint64_t& token);
  MemManageStatus unpinCudaDevBuffer(void* dev_ptr, bool force);

  MemManageStatus pinHipDevBuffer(void* dev_ptr, size_t size, uint64_t& token);
  MemManageStatus unpinHipDevBuffer(void* dev_ptr, bool force);

  MemManageStatus pinHostDevBuffer(void* dev_ptr, size_t size, uint64_t& token);
  MemManageStatus unpinHostDevBuffer(void *dev_ptr, bool force);

  MemManageStatus getPinnedDevBuffer(uint64_t token, size_t size, void*& dev_ptr);
  MemManageStatus releasePinnedDevBuffer(void* dev_ptr);

  MemManageStatus getPinnedDevPhysAddrs(void* dev_ptr, uint32_t max, uint32_t& num,
                                        uint64_t* phys_addrs, uint32_t* sizes);

private:
  enum GpuType {
    kGpuType_ANY = 0,
    kGpuType_NV = 1,
    kGpuType_AMD = 2,
    kGpuType_CPU = 3,
  };

  void finalize(void);

  MemManage(MemManage& ref) = delete;
  MemManage& operator=(MemManage& ref) = delete;
  MemManage(MemManage&& ref) noexcept = delete;
  MemManage& operator=(MemManage&& ref) noexcept = delete;

  struct MemInfo {
    GpuType type;
    void* dev_ptr;
    void* dev_ptr_base;
    size_t size;
    uint64_t token;
    unsigned id;
  };

  std::map<void*, MemInfo> m_mem_info;
  std::map<void*, std::vector<std::pair<uint64_t, uint32_t>>> m_paddrs; // token -> vector(paddr, size)
  std::mutex m_mutex;
  int m_dev_fd;
};

#endif // _MEM_MANAGE_HPP__
