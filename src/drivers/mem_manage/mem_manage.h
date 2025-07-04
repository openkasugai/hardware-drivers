/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef _MEM_MANAGE_H__
#define _MEM_MANAGE_H__

#define DEBUG_LVL 1 // 1: Error only, 2: Debug info

#include <linux/module.h>
#ifdef AMD_PLATFORM
#include <amd_rdma.h>
#endif

#include <mem_manage_ext.h>

extern int mem_manage_open(struct inode *inode, struct file *file);
extern int mem_manage_close(struct inode *inode, struct file *file);
extern long mem_manage_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
extern int mem_manage_mmap(struct file *file, struct vm_area_struct *vma);

extern long mem_manage_get_paddr_num(void __user *arg);
extern long mem_manage_get_paddr(void __user *arg);
extern long mem_manage_get_dev_type(void __user *arg);

extern long mem_manage_get_host_paddr(struct file* filp, void __user *arg);
extern long mem_manage_put_host_paddr(void __user *arg);

#ifdef NVIDIA_PLATFORM
extern long mem_manage_get_nv_dev_addr(struct file* filp, void __user *arg);
extern long mem_manage_put_nv_dev_addr(void __user *arg);
extern long mem_manage_get_nv_paddr_num(void __user *arg);
extern long mem_manage_get_nv_paddr(void __user *arg);
#endif
#ifdef AMD_PLATFORM
extern long mem_manage_get_amd_dev_addr(struct file* filp, void __user *arg);
extern long mem_manage_put_amd_dev_addr(void __user *arg);
#endif

extern int mem_manage_map_gpu_dev_addr(struct file *file, struct vm_area_struct *vma);
extern int mem_manage_unmap_gpu_dev_addr(struct vm_area_struct *vma);

#ifdef AMD_PLATFORM
extern const struct amd_rdma_interface *rdma_ops;
#endif

#endif // _MEM_MANAGE_H__
