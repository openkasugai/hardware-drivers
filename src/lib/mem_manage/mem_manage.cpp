/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <mem_manage.hpp>
#include <memory>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <mem_manage_ioctl.h>
#include <unistd.h>
#include <cstdio>
#ifdef NVIDIA_PLATFORM
#include <cuda.h>
#endif

static const char* s_dev_file = "/dev/mem_manage0";
#define PAGE_SIZE_LOG 12
#define CPU_ALIGN_SIZE_LOG 12
#define NV_ALIGN_SIZE_LOG 8

MemManage::MemManage(void)
  : m_dev_fd(-1) {
  m_dev_fd = open(s_dev_file, O_RDWR | O_SYNC);
}

MemManage::~MemManage(void) {
  finalize();
}

MemManageStatus MemManage::pinCudaDevBuffer(void* dev_ptr, size_t size, uint64_t& token) {
#ifdef NVIDIA_PLATFORM
  unsigned flag = 1;
  CUresult ret = cuPointerSetAttribute(&flag, CU_POINTER_ATTRIBUTE_SYNC_MEMOPS, (CUdeviceptr)dev_ptr);
  if (ret != CUDA_SUCCESS) {
    printf("failed to cuPointerSetAttribute: %d\n", ret);
    return kMemManage_Error;
  }
#endif

  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_dev_fd < 0) {
    return kMemManage_OpenDeviceFailed;
  }
  mem_manage_nv_addr info{dev_ptr, size, 0, 0, false};
  int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_GET_NV_DEV_ADDR, &info);
  if (stat) {
    return kMemManage_PinBufferFailed;
  }
  token = info.token;
  m_mem_info[dev_ptr] = MemInfo({kGpuType_NV, dev_ptr, 0, size, token, info.id});

  return kMemManage_Success;
}

MemManageStatus MemManage::unpinCudaDevBuffer(void* dev_ptr, bool force) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_dev_fd < 0) {
    return kMemManage_OpenDeviceFailed;
  }
  auto it = m_mem_info.find(dev_ptr);
  if (it == m_mem_info.end()) {
    return kMemManage_InvalidPinnedBuffer;
  }
  if (it->second.dev_ptr_base) return kMemManage_InvalidOperation;
  mem_manage_nv_addr info{it->second.dev_ptr, it->second.size, it->second.token, it->second.id, force};
  int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_PUT_NV_DEV_ADDR, &info);
  if (stat) {
    return kMemManage_UnpinBufferFailed;
  }
  m_mem_info.erase(it);
  return kMemManage_Success;
}

MemManageStatus MemManage::pinHipDevBuffer(void* dev_ptr, size_t size, uint64_t& token) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_dev_fd < 0) {
    return kMemManage_OpenDeviceFailed;
  }
  mem_manage_amd_addr info{dev_ptr, size, 0, 0, false};
  int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_GET_AMD_DEV_ADDR, &info);
  if (stat) {
    return kMemManage_PinBufferFailed;
  }
  token = info.token;
  m_mem_info[dev_ptr] = MemInfo({kGpuType_AMD, dev_ptr, 0, size, token, info.id});
  return kMemManage_Success;
}

MemManageStatus MemManage::unpinHipDevBuffer(void* dev_ptr, bool force) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_dev_fd < 0) {
    return kMemManage_OpenDeviceFailed;
  }
  auto it = m_mem_info.find(dev_ptr);
  if (it == m_mem_info.end()) {
    return kMemManage_InvalidPinnedBuffer;
  }
  if (it->second.dev_ptr_base) return kMemManage_InvalidOperation;
  mem_manage_amd_addr info{it->second.dev_ptr, it->second.size, it->second.token, it->second.id, force};
  int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_PUT_AMD_DEV_ADDR, &info);
  if (stat) {
    return kMemManage_UnpinBufferFailed;
  }
  m_mem_info.erase(it);
  return kMemManage_Success;
}

MemManageStatus MemManage::pinHostDevBuffer(void* dev_ptr, size_t size, uint64_t& token) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_dev_fd < 0) {
    return kMemManage_OpenDeviceFailed;
  }
  mem_manage_host_addr info{dev_ptr, size, 0, 0, false};
  int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_GET_HOST_PHYS_ADDR, &info);
  if (stat) {
    return kMemManage_PinBufferFailed;
  }
  token = info.token;
  m_mem_info[dev_ptr] = MemInfo({kGpuType_CPU, dev_ptr, 0, size, token, info.id});

  return kMemManage_Success;
}

MemManageStatus MemManage::unpinHostDevBuffer(void* dev_ptr, bool force) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_dev_fd < 0) {
    return kMemManage_OpenDeviceFailed;
  }
  auto it = m_mem_info.find(dev_ptr);
  if (it == m_mem_info.end()) {
    return kMemManage_InvalidPinnedBuffer;
  }
  if (it->second.dev_ptr_base) return kMemManage_InvalidOperation;
  mem_manage_host_addr info{it->second.dev_ptr, it->second.size, it->second.token, it->second.id, force};
  int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_PUT_HOST_PHYS_ADDR, &info);
  if (stat) {
    return kMemManage_UnpinBufferFailed;
  }
  m_mem_info.erase(it);
  return kMemManage_Success;
}

MemManageStatus MemManage::getPinnedDevBuffer(uint64_t token, size_t size, void*& dev_ptr) {
  mem_manage_dev_type info{token, 0};
  int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_GET_DEV_TYPE, &info);
  if (stat) {
    return kMemManage_GetDevTypeFailed;
  }

  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_SHARED;
  uint64_t paddr;
  if ((GpuType)info.dev_type == kGpuType_CPU)
    paddr = token;
  else
    paddr = token << NV_ALIGN_SIZE_LOG;
  uint64_t offset = paddr & 4095;
  uint64_t map_ofs = token << PAGE_SIZE_LOG;

  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_dev_fd < 0) {
    return kMemManage_OpenDeviceFailed;
  }

  uint64_t map_size = size + offset;
  void* aligned_ptr = mmap(0, map_size, prot, flags, m_dev_fd, map_ofs);
  if (aligned_ptr == reinterpret_cast<void*>(-1)) return kMemManage_MapPinnedBufferFailed;

  dev_ptr = (char*)aligned_ptr + offset;

  m_mem_info[dev_ptr] = MemInfo({(GpuType)info.dev_type, dev_ptr, aligned_ptr, map_size, token, 0});
  return kMemManage_Success;
}

MemManageStatus MemManage::getPinnedDevPhysAddrs(void* dev_ptr, uint32_t max,
                                                 uint32_t& num, uint64_t* paddrs, uint32_t* sizes) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_dev_fd < 0) {
    return kMemManage_OpenDeviceFailed;
  }
  auto it = m_mem_info.find(dev_ptr);
  if (it == m_mem_info.end()) {
    return kMemManage_FindMappedPinnedBufferFailed;
  }

  mem_manage_paddr info{it->second.token, 0, 0, 0};
  int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_GET_PADDR_NUM, &info);
  if (stat) {
    return kMemManage_GetPaddrNumFailed;
  }
  num = info.id;
  std::vector<std::pair<uint64_t, uint32_t>> lpaddrs(num);
  for (int i=0; i<num; ++i) {
    if (i >= max) return kMemManage_LackOfArgsSize;
    info.id = i;
    int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_GET_PADDR, &info);
    if (stat) {
      return kMemManage_GetPaddrFailed;
    }
    lpaddrs[i] = std::pair<uint64_t, uint32_t>(info.paddr, info.size);
    paddrs[i] = info.paddr;
    sizes[i] = info.size;
  }
  m_paddrs[dev_ptr] = lpaddrs;

  return kMemManage_Success;
}

MemManageStatus MemManage::releasePinnedDevBuffer(void* dev_ptr) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_dev_fd < 0) {
    return kMemManage_OpenDeviceFailed;
  }
  auto it = m_mem_info.find(dev_ptr);
  if (it == m_mem_info.end()) {
    return kMemManage_InvalidPinnedBuffer;
  }
  if (!it->second.dev_ptr_base) return kMemManage_InvalidOperation;
  munmap(it->second.dev_ptr_base, it->second.size);

  m_mem_info.erase(it);
  return kMemManage_Success;
}

void MemManage::finalize(void) {
  std::lock_guard<std::mutex> lock(m_mutex);
  if (m_dev_fd < 0) {
    return;
  }
  for (auto it = m_mem_info.begin(); it != m_mem_info.end();) {
    auto cur_it = it++;
    if (cur_it->second.dev_ptr_base) {
      munmap(cur_it->second.dev_ptr_base, cur_it->second.size);
      m_mem_info.erase(cur_it);
    }
  }
  int ext_retry = 5;
  while (ext_retry) {
    for (auto it = m_mem_info.begin(); it != m_mem_info.end();) {
      auto cur_it = it++;
      if (cur_it->second.type == kGpuType_NV) {
        mem_manage_nv_addr info{
                cur_it->second.dev_ptr, cur_it->second.size,
                cur_it->second.token, cur_it->second.id, 0};
        int retry = 10;
        while (retry) {
          int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_PUT_NV_DEV_ADDR, &info);
          if (!stat) break;
          retry--;
          usleep(100000);
        }
        if (retry) {
          m_mem_info.erase(cur_it);
        }
      } else if (cur_it->second.type == kGpuType_AMD) {
        mem_manage_amd_addr info{
                cur_it->second.dev_ptr, cur_it->second.size,
                cur_it->second.token, cur_it->second.id, 0};
        int retry = 10;
        while (retry) {
          int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_PUT_AMD_DEV_ADDR, &info);
          if (!stat) break;
          retry--;
          usleep(100000);
        }
        if (retry) {
          m_mem_info.erase(cur_it);
        }
      } else if (cur_it->second.type == kGpuType_CPU) {
        mem_manage_host_addr info{
                cur_it->second.dev_ptr, cur_it->second.size,
                cur_it->second.token, cur_it->second.id, 0};
        int retry = 10;
        while (retry) {
          int stat = ioctl(m_dev_fd, MEM_MANAGE_IOCTL_PUT_HOST_PHYS_ADDR, &info);
          if (!stat) break;
          retry--;
          usleep(100000);
        }
        if (retry) {
          m_mem_info.erase(cur_it);
        }
      }
    }
    ext_retry--;
  }
  if (!m_mem_info.empty()) {
    printf("failed to unpin buffers\n");
  }
  close(m_dev_fd);
}
