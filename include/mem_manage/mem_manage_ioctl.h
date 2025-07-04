/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License with an explicit syscall exception, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note
*************************************************/

#ifndef WHITEBOX_MEM_MANAGE_IOCTL_FPGA_IOCTL_H_
#define WHITEBOX_MEM_MANAGE_IOCTL_FPGA_IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mem_manage_nv_addr_ {
  void* dev_addr;
  size_t size;
  uint64_t token;
  unsigned id;
  bool force;
} mem_manage_nv_addr;

typedef struct mem_manage_amd_addr_ {
  void* dev_addr;
  size_t size;
  uint64_t token;
  unsigned id;
  bool force;
} mem_manage_amd_addr;


typedef struct mem_manage_host_addr_ {
  void* dev_addr;
  size_t size;
  uint64_t token;
  unsigned id;
  bool force;
} mem_manage_host_addr;

typedef struct mem_manage_paddr_ {
    uint64_t token;
    uint32_t id;
    uint64_t paddr;
    uint32_t size;
} mem_manage_paddr;

typedef struct mem_manage_dev_type_ {
    uint64_t token;
    uint32_t dev_type;
} mem_manage_dev_type;

enum {
  MEM_MANAGE_IOCTL_GET_NV_DEV_ADDR = _IOWR('S', 0x87, mem_manage_nv_addr),
  MEM_MANAGE_IOCTL_PUT_NV_DEV_ADDR = _IOW('S', 0x88, mem_manage_nv_addr),
  MEM_MANAGE_IOCTL_GET_AMD_DEV_ADDR = _IOWR('S', 0x89, mem_manage_amd_addr),
  MEM_MANAGE_IOCTL_PUT_AMD_DEV_ADDR = _IOW('S', 0x8a, mem_manage_amd_addr),
  MEM_MANAGE_IOCTL_GET_HOST_PHYS_ADDR = _IOWR('S', 0x8b, mem_manage_host_addr),
  MEM_MANAGE_IOCTL_PUT_HOST_PHYS_ADDR = _IOW('S', 0x8c, mem_manage_host_addr),

  MEM_MANAGE_IOCTL_GET_PADDR_NUM = _IOWR('S', 0x90, mem_manage_paddr),
  MEM_MANAGE_IOCTL_GET_PADDR = _IOWR('S', 0x91, mem_manage_paddr),
  MEM_MANAGE_IOCTL_GET_DEV_TYPE = _IOWR('S', 0x98, mem_manage_dev_type)
};

#ifdef __cplusplus
}
#endif

#endif // WHITEBOX_MEM_MANAGE_IOCTL_FPGA_IOCTL_H_
