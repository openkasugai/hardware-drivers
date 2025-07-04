/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef __DEV_MEM_H__
#define __DEV_MEM_H__

#include "xse_def.h"

extern int dev_mems_free(struct xse_cdev* cdev);
extern int dev_mem_get_range(struct xse_cdev* cdev, uint64_t token, uint64_t* addr, uint32_t* size);

extern int xse_dev_mem_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info);
extern void xse_dev_mem_exit(struct xse_pci_dev* xse_pdev);

extern long xse_dev_mem_ioctl_alloc_devmem(struct file *file, unsigned long arg);
extern long xse_dev_mem_ioctl_free_devmem(struct file *file, unsigned long arg);

#endif
