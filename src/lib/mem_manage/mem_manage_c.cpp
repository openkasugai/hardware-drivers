/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <mem_manage.hpp>
#include <map>
#include <memory>
#include <mutex>

static std::mutex s_mutex;
static std::map<void*, std::unique_ptr<MemManage>> s_obj_map;

MemManageObj memManageCreate(void) {
  try {
    std::lock_guard<std::mutex> lock(s_mutex);
    MemManage* obj = new MemManage();
    s_obj_map[obj].reset(obj);
    return obj;
  } catch (...) {
    return nullptr;
  }
}

MemManageStatus memManageDestroy(MemManageObj obj) {
  try {
    MemManage* mm = reinterpret_cast<MemManage*>(obj);
    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_obj_map.find(mm);
    if (it == s_obj_map.end()) return kMemManage_InvalidOperation;
    s_obj_map.erase(it);
    return kMemManage_Success;
  } catch (...) {
    return kMemManage_Exception;
  }
}

MemManageStatus memManagePinCudaDevBuffer(MemManageObj obj, void* dev_ptr, size_t size, uint64_t* token) {
  try {
    MemManage* mm = reinterpret_cast<MemManage*>(obj);
    return mm->pinCudaDevBuffer(dev_ptr, size, *token);
  } catch (...) {
    return kMemManage_Exception;
  }
}

MemManageStatus memManageUnpinCudaDevBuffer(MemManageObj obj, void* dev_ptr, bool force) {
  try {
    MemManage* mm = reinterpret_cast<MemManage*>(obj);
    return mm->unpinCudaDevBuffer(dev_ptr, force);
  } catch (...) {
    return kMemManage_Exception;
  }
}

MemManageStatus memManagePinHipDevBuffer(MemManageObj obj, void* dev_ptr, size_t size, uint64_t* token) {
  try {
    MemManage* mm = reinterpret_cast<MemManage*>(obj);
    return mm->pinHipDevBuffer(dev_ptr, size, *token);
  } catch (...) {
    return kMemManage_Exception;
  }
}

MemManageStatus memManageUnpinHipDevBuffer(MemManageObj obj, void* dev_ptr, bool force) {
  try {
    MemManage* mm = reinterpret_cast<MemManage*>(obj);
    return mm->unpinHipDevBuffer(dev_ptr, force);
  } catch (...) {
    return kMemManage_Exception;
  }
}

MemManageStatus memManagePinHostDevBuffer(MemManageObj obj, void* dev_ptr, size_t size, uint64_t* token) {
  try {
    MemManage* mm = reinterpret_cast<MemManage*>(obj);
    return mm->pinHostDevBuffer(dev_ptr, size, *token);
  } catch (...) {
    return kMemManage_Exception;
  }
}

MemManageStatus memManageUnpinHostDevBuffer(MemManageObj obj, void* dev_ptr, bool force) {
  try {
    MemManage* mm = reinterpret_cast<MemManage*>(obj);
    return mm->unpinHostDevBuffer(dev_ptr, force);
  } catch (...) {
    return kMemManage_Exception;
  }
}

MemManageStatus memManageGetPinnedDevBuffer(MemManageObj obj, uint64_t token, size_t size, void** dev_ptr) {
  try {
    MemManage* mm = reinterpret_cast<MemManage*>(obj);
    return mm->getPinnedDevBuffer(token, size, *dev_ptr);
  } catch (...) {
    return kMemManage_Exception;
  }
}

MemManageStatus memManageReleasePinnedDevBuffer(MemManageObj obj, void* dev_ptr) {
  try {
    MemManage* mm = reinterpret_cast<MemManage*>(obj);
    return mm->releasePinnedDevBuffer(dev_ptr);
  } catch (...) {
    return kMemManage_Exception;
  }
}

MemManageStatus memManageGetPinnedDevPhysAddrs(MemManageObj obj, void* dev_ptr, uint32_t max,
                                               uint32_t* num, uint64_t* paddrs, uint32_t* sizes) {
  try {
    MemManage* mm = reinterpret_cast<MemManage*>(obj);
    return mm->getPinnedDevPhysAddrs(dev_ptr, max, *num, paddrs, sizes);
  } catch (...) {
    return kMemManage_Exception;
  }
}
