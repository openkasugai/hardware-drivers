/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include <linux/string.h>
#include <linux/device.h>
#include <linux/fs.h>
#include "libxse.h"
#include "device_info.h"

extern struct class *g_xse_class;

const char* s_dev_name_template = DEVICE_NAME "%d_%s";
static const int s_reg_bar_id = 0;
static const int s_axi_bypass_bar_id = 4;

static const mode_t s_xse_perms = S_IRUSR | S_IWUSR |  S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
static struct attribute s_dev_attr_perm = {
    .name = "root",
    .mode = s_xse_perms
};
static struct attribute* s_dev_attr_perms[] = {&s_dev_attr_perm, NULL};
static struct attribute_group s_xse_group = {
    .name = "root",
    .attrs = &s_dev_attr_perms[0]
};
static struct attribute_group* s_xse_groups[] = {&s_xse_group, NULL};

struct xse_device_info* get_device_info(struct xse_pci_dev* xse_pdev, int idx)
{
    unsigned int* dev_info_ptr = NULL;
    char* dev_info_str_ptr = NULL;
    unsigned int value;
    unsigned int entry_size = 0;
    struct xse_device_info* info = NULL;

    struct xse_dev* dev = xse_pdev->dev;
    if (!dev->bar[s_reg_bar_id]) return info;

    dev_info_ptr = (unsigned int*)((char*)dev->bar[s_reg_bar_id] + DEVICE_INFO_OFFSET);
    entry_size = *dev_info_ptr;
    xse_log("device info: entry_size=%u\n", entry_size);

    dev_info_ptr = (unsigned int*)((char*)dev->bar[s_reg_bar_id] + DEVICE_INFO_OFFSET + entry_size * idx);
    dev_info_str_ptr = (char*)(dev_info_ptr + 3);

    if (!dev_info_ptr[0]) return info;

    info = kzalloc(sizeof(*info), GFP_KERNEL);
    if (!info) return info;

    strcpy(info->name, dev_info_str_ptr);
    value = dev_info_ptr[1];
    info->offset = (uint64_t)(value & 0xffffff) << (value >> 24);
    value = dev_info_ptr[2];
    info->size = (uint64_t)(value & 0xffffff) << (value >> 24);
    xse_log("module: %llx %llx %s\n", info->offset, info->size, info->name);

    return info;
}

void release_device_info(struct xse_device_info* info)
{
    if (!info) return;
    kfree(info);
}

static int get_bar_info(struct xse_cdev* cdev, int bar_id, uint64_t* paddr, uint64_t* size, bool need_offset)
{
    struct xse_pci_dev* xse_pdev = NULL;
    if (!cdev) return -EINVAL;
    xse_pdev = cdev->xse_pdev;
    if (!xse_pdev) return -EINVAL;
    if (!xse_pdev->dev) return -EINVAL;
    if (paddr) {
        void __iomem* vaddr = cdev->base;
        *paddr = xse_pdev->dev->bar_phys_addr[bar_id];
        if (need_offset) *paddr = *paddr + vaddr - xse_pdev->dev->bar[bar_id];
    }
    if (size) {
        *size = cdev->size;
    }
    return 0;
}

int get_axi_bypass_bar_info(struct xse_cdev* cdev, uint64_t* paddr, uint64_t* size)
{
    return get_bar_info(cdev, s_axi_bypass_bar_id, paddr, size, false);
}

int get_register_bar_info(struct xse_cdev* cdev, uint64_t* paddr, uint64_t* size)
{
    return get_bar_info(cdev, s_reg_bar_id, paddr, size, true);
}

int xse_cdev_init(struct xse_pci_dev* xse_pdev, struct xse_cdev* cdev, const char* name, int minor,
                  struct xse_device_info* info, const struct file_operations* fops, int id, int info_owner)
{
    int ret = 0;
    if (!xse_pdev->major) {
        dev_t dev;
        int ret = alloc_chrdev_region(&dev, DEVICE_MINOR, DEVICE_MINOR_COUNT, DEVICE_NAME);
        if (ret) return ret;
        xse_pdev->major = MAJOR(dev);
    }

    ret = kobject_set_name(&cdev->cdev.kobj, s_dev_name_template, xse_pdev->dev->dev_id, name);
    if (ret) return ret;

    cdev->cdev.owner = THIS_MODULE;
    cdev->xse_pdev = xse_pdev;
    cdev->base = (char*)xse_pdev->dev->bar[0] + info->offset;
    cdev->size = info->size;
    cdev->module_info = info;
    cdev->module_info_owner = info_owner;
    cdev->id = id;
    cdev_init(&cdev->cdev, fops);

    cdev->cdev_no = MKDEV(xse_pdev->major, minor);
    ret = cdev_add(&cdev->cdev, cdev->cdev_no, 1);
    if (ret) {
        unregister_chrdev_region(cdev->cdev_no, DEVICE_MINOR_COUNT);
        return ret;
    }
    if (g_xse_class) {
        //const struct attribute_group** p_groups = (const struct attribute_group**)(&s_xse_groups[0]);
        //cdev->sys_dev = device_create_with_groups(g_xse_class, &xse_pdev->pci_dev->dev, cdev->cdev_no, NULL,
        //                                          p_groups, s_dev_name_template, xse_pdev->dev->dev_id, name);
        cdev->sys_dev = device_create(g_xse_class, &xse_pdev->pci_dev->dev, cdev->cdev_no, NULL,
                                      s_dev_name_template, xse_pdev->dev->dev_id, name);
        if (!cdev->sys_dev) {
            cdev_del(&cdev->cdev);
            unregister_chrdev_region(cdev->cdev_no, DEVICE_MINOR_COUNT);
            return -EINVAL;
        }
    }
    return 0;
}

void xse_cdev_exit(struct xse_cdev* cdev)
{
    if (!cdev) return;
    cdev_del(&cdev->cdev);
    if (cdev->sys_dev) {
        device_destroy(g_xse_class, cdev->cdev_no);
        cdev->sys_dev = NULL;
    }
}

int xse_device_file_open(struct inode *inode, struct file *file)
{
    struct xse_cdev *cdev = NULL;
    cdev = container_of(inode->i_cdev, struct xse_cdev, cdev);
    if (cdev->cdev.count > 1) {
        struct xse_device_info* module_info = cdev->module_info;
        xse_err("xse%d_%s_%x already used. %d\n", cdev->xse_pdev->dev->dev_id,
                module_info->name, cdev->id, cdev->cdev.count);
        return -EINVAL;
    }

    file->private_data = cdev;
    return 0;
}

loff_t xse_device_file_seek(struct file *file, loff_t offset, int whence)
{
    loff_t pos;
    switch (whence) {
    case 0: /* SEEK_SET */
        pos = offset;
        break;
    case 1: /* SEEK_CUR */
        pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END */
        // unsupported
        // not break
    default:
        return -EINVAL;
    }

    file->f_pos = pos;
    return pos;
}

int xse_device_file_close(struct inode *inode, struct file *file)
{
    struct xse_cdev *cdev = (struct xse_cdev*)file->private_data;
    if (!cdev) return -EINVAL;

    return 0;
}

ssize_t xse_device_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    int i;
    unsigned int data;
    void __iomem* ptr;
    loff_t ofs = *pos;
    struct xse_cdev *cdev = file->private_data;

    if (!cdev) return -EINVAL;
    if (ofs + count > cdev->size) return -EINVAL;
    if (count & 3) return -EINVAL;

    for (i=0; i<count; i+=4) {
        int ret;
        ptr = cdev->base + ofs + i;
        data = ioread32(ptr);
        xse_log("%s: read[%llx]: %x\n",
                ((struct xse_device_info*)cdev->module_info)->name, ofs+i, data);
        ret = copy_to_user(buf + i, &data, 4);
        if (ret) {
            xse_err("%s: read: copy to user failed\n",
                    ((struct xse_device_info*)cdev->module_info)->name);
        }
        *pos+=4;
    }
    return count;
}

ssize_t xse_device_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    int i;
    unsigned int data;
    void __iomem* ptr;
    loff_t ofs = *pos;
    struct xse_cdev *cdev = file->private_data;

    if (!cdev) return -EINVAL;
    if (ofs + count > cdev->size) return -EINVAL;
    if (count & 3) return -EINVAL;

    for (i=0; i<count; i+=4) {
        int ret;
        ptr = cdev->base + ofs + i;
        ret = copy_from_user(&data, buf + i, 4);
        if (ret) {
            xse_err("%s: write: copy from user failed\n",
                    ((struct xse_device_info*)cdev->module_info)->name);
        } else {
            iowrite32(data, ptr);
            xse_log("%s: write[%llx]: %x\n",
                    ((struct xse_device_info*)cdev->module_info)->name, ofs+i, data);
        }
        *pos+=4;
    }
    return count;
}
