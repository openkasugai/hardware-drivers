/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _MEM_MANAGE_H__
#define _MEM_MANAGE_H__

#include <stdint.h>
#include <memory.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  kMemManage_Success = 0,
  kMemManage_Exception = -1,
  kMemManage_Error = -2,
  kMemManage_OpenDeviceFailed = -3,
  kMemManage_InvalidOperation = -4,
  kMemManage_PinBufferFailed = -5,
  kMemManage_UnpinBufferFailed = -6,
  kMemManage_InvalidPinnedBuffer = -7,
  kMemManage_MapPinnedBufferFailed = -8,
  kMemManage_FindMappedPinnedBufferFailed = -9,
  kMemManage_GetPaddrNumFailed = -10,
  kMemManage_GetPaddrFailed = -11,
  kMemManage_LackOfArgsSize = -12,
  kMemManage_GetDevTypeFailed = -13,
} MemManageStatus;

typedef void* MemManageObj;

MemManageObj memManageCreate(void);
MemManageStatus memManageDestroy(MemManageObj obj);

MemManageStatus memManagePinCudaDevBuffer(MemManageObj obj, void* dev_ptr, size_t size, uint64_t* token);
MemManageStatus memManageUnpinCudaDevBuffer(MemManageObj obj, void* dev_ptr, bool force);

MemManageStatus memManagePinHipDevBuffer(MemManageObj obj, void* dev_ptr, size_t size, uint64_t* token);
MemManageStatus memManageUnpinHipDevBuffer(MemManageObj obj, void* dev_ptr, bool force);

MemManageStatus memManagePinHostDevBuffer(MemManageObj obj, void* dev_ptr, size_t size, uint64_t* token);
MemManageStatus memManageUnpinHostDevBuffer(MemManageObj obj, void* dev_ptr, bool force);

MemManageStatus memManageGetPinnedDevBuffer(MemManageObj obj, uint64_t token, size_t size, void** dev_ptr);
MemManageStatus memManageReleasePinnedDevBuffer(MemManageObj obj, void* dev_ptr);

MemManageStatus memManageGetPinnedDevPhysAddrs(MemManageObj obj, void* dev_ptr, uint32_t max,
                                               uint32_t* num, uint64_t* phys_addrs, uint32_t* sizes);

#ifdef __cplusplus
}
#endif

#endif // _MEM_MANAGE_H__
