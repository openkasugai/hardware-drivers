/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef __DEVICE_INFO_H__
#define __DEVICE_INFO_H__

#include "xse_def.h"

extern struct xse_device_info* get_device_info(struct xse_pci_dev* xse_pdev, int idx);
extern void release_device_info(struct xse_device_info* info);
extern int get_axi_bypass_bar_info(struct xse_cdev* cdev, uint64_t* paddr, uint64_t* size);
extern int get_register_bar_info(struct xse_cdev* cdev, uint64_t* paddr, uint64_t* size);

extern int xse_cdev_init(struct xse_pci_dev* xse_pdev, struct xse_cdev* cdev, const char* name, int minor,
                         struct xse_device_info* info, const struct file_operations* fops, int id, int info_owner);
extern void xse_cdev_exit(struct xse_cdev* cdev);

extern int xse_device_file_open(struct inode *inode, struct file *file);
extern loff_t xse_device_file_seek(struct file *file, loff_t offset, int whence);
extern int xse_device_file_close(struct inode *inode, struct file *file);
extern ssize_t xse_device_read(struct file *file, char __user *buf, size_t count, loff_t *pos);
extern ssize_t xse_device_write(struct file *file, const char __user *buf, size_t count, loff_t *pos);

#endif // __DEVICE_INFO_H__
