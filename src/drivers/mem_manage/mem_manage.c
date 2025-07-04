/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include <mem_manage.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#ifdef AMD_PLATFORM
#include <amd_rdma.h>
#endif

#ifdef USE_PROPRIETARY
MODULE_LICENSE ("Proprietary");
#else
MODULE_LICENSE ("GPL");
#endif
#define DRIVER_NAME "mem_manage"
#define DRIVER_MINOR_NUM 1
#define DRIVER_MINOR_BASE 0

static unsigned int g_device_major;
static struct cdev g_device_cdev;

#ifdef AMD_PLATFORM
int (*query_rdma_interface)(const struct amd_rdma_interface **);
const struct amd_rdma_interface *rdma_ops = NULL;
#endif

static struct file_operations s_ops = {
  .open = mem_manage_open,
  .release = mem_manage_close,
  .unlocked_ioctl = mem_manage_ioctl,
  .mmap = mem_manage_mmap,
};

static int mem_manage_init(void) {
  int ret;
  dev_t dev;

#if DEBUG_LVL > 1
  printk("mem manage init\n");
#endif

  ret = alloc_chrdev_region(&dev, DRIVER_MINOR_BASE, DRIVER_MINOR_NUM, DRIVER_NAME);
  if (ret) {
#if DEBUG_LVL > 0
    printk("mem manage alloc_chrdev_region failed: %d\n", ret);
#endif
    return STATUS_ERROR;
  }

  g_device_major = MAJOR(dev);
  dev = MKDEV(g_device_major, DRIVER_MINOR_BASE);

  cdev_init(&g_device_cdev, &s_ops);
  g_device_cdev.owner = THIS_MODULE;

  ret = cdev_add(&g_device_cdev, dev, DRIVER_MINOR_NUM);
  if (ret) {
#if DEBUG_LVL > 0
    printk("mem manage cdev_add failed: %d\n", ret);
#endif
    unregister_chrdev_region(dev, DRIVER_MINOR_NUM);
    return STATUS_ERROR;
  }

#ifdef AMD_PLATFORM
  query_rdma_interface = symbol_get(amdkfd_query_rdma_interface);

  if (!query_rdma_interface) {
    printk("failed to find amdkfd_query_rdma_interface symbol\n");
    cdev_del(&g_device_cdev);
    unregister_chrdev_region(dev, DRIVER_MINOR_NUM);
    return STATUS_ERROR;
  }
  if (query_rdma_interface(&rdma_ops) < 0) {
    printk("failed to get rdma interface.\n");
    symbol_put(amdkfd_query_rdma_interface);
    cdev_del(&g_device_cdev);
    unregister_chrdev_region(dev, DRIVER_MINOR_NUM);
    return STATUS_ERROR;
  }
  if (!rdma_ops) {
    printk("rdma_ops is empty.\n");
    symbol_put(amdkfd_query_rdma_interface);
    cdev_del(&g_device_cdev);
    unregister_chrdev_region(dev, DRIVER_MINOR_NUM);
    return STATUS_ERROR;
  }
#if DEBUG_LVL > 1
  printk("rdma_ops found!\n");
#endif
#endif

  return STATUS_SUCCESS;
}

static void mem_manage_exit(void) {
  dev_t dev = MKDEV(g_device_major, DRIVER_MINOR_BASE);

  cdev_del(&g_device_cdev);
  unregister_chrdev_region(dev, DRIVER_MINOR_NUM);

#ifdef AMD_PLATFORM
  if (query_rdma_interface) {
    symbol_put(amdkfd_query_rdma_interface);
    query_rdma_interface = NULL;
    rdma_ops = NULL;
  }
#endif

#if DEBUG_LVL > 1
  printk("mem manage exit\n");
#endif
}

module_init(mem_manage_init);
module_exit(mem_manage_exit);
