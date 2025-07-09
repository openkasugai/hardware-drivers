/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxse.h"
#include "devmem.h"
#include "devmem_ioctl.h"

#define TOKEN_BASE 0x444d000000000000llu
#define TOKEN_DEV_SHIFT 44

struct dev_mem_region {
    struct list_head head;
    uint32_t dev_id;
    int mem_id;
    uint64_t start;
    uint64_t size;
    uint64_t token;
    dev_t cdev_no;
};

static LIST_HEAD(s_dev_mem_list);
static LIST_HEAD(s_dev_mem_alloc_list);
static DEFINE_MUTEX(s_dev_mem_mutex);

static int dev_mem_add(uint32_t dev_id, int mem_id, uint64_t start, uint64_t size)
{
    struct dev_mem_region* region = kzalloc(sizeof(*region), GFP_KERNEL);
    if (!region) return -ENOMEM;

    region->dev_id = dev_id;
    region->mem_id = mem_id;
    region->start = start;
    region->size = size;

    xse_log("devmem: add %2d %2d %016llx %016llx\n", dev_id, mem_id, start, size);
    mutex_lock(&s_dev_mem_mutex);
    list_add(&region->head, &s_dev_mem_list);
    mutex_unlock(&s_dev_mem_mutex);
    return 0;
}

static void dev_mem_clear(uint32_t dev_id)
{
    struct list_head* pos = NULL;
    struct list_head* pos_tmp = NULL;
    mutex_lock(&s_dev_mem_mutex);
    list_for_each_safe(pos, pos_tmp, &s_dev_mem_alloc_list) {
        struct dev_mem_region* region = list_entry(pos, struct dev_mem_region, head);
        if (region->dev_id == dev_id) {
            list_del(pos);
            kfree(region);
        }
    }
    list_for_each_safe(pos, pos_tmp, &s_dev_mem_list) {
        struct dev_mem_region* region = list_entry(pos, struct dev_mem_region, head);
        if (region->dev_id == dev_id) {
            list_del(pos);
            kfree(region);
        }
    }
    mutex_unlock(&s_dev_mem_mutex);
}

static uint64_t dev_mem_alloc(struct xse_cdev* cdev, int mem_id, uint32_t size)
{
    uint64_t token = 0;
    int dev_id = cdev->xse_pdev->id;
    struct dev_mem_region* region = NULL;
    struct dev_mem_region* target = NULL;
    mutex_lock(&s_dev_mem_mutex);
    list_for_each_entry(region, &s_dev_mem_list, head) {
        if (region->dev_id == dev_id && (mem_id < 0 || region->mem_id == mem_id) && region->size >= size) {
            target = region;
            break;
        }
    }
    if (target) {
        struct dev_mem_region* alloc = kmalloc(sizeof(*alloc), GFP_KERNEL);;
        if (alloc) {
            uint64_t alloc_start;
            uint64_t alloc_end = target->start + target->size;
            alloc_start = alloc_end - size;
            alloc->cdev_no = cdev->cdev_no;
            alloc->dev_id = dev_id;
            alloc->mem_id = target->mem_id;
            alloc->start = alloc_start;
            alloc->size = size;
            target->size -= size;
            token = TOKEN_BASE + ((uint64_t)dev_id << TOKEN_DEV_SHIFT) + alloc->start;
            alloc->token = token;
            list_add(&alloc->head, &s_dev_mem_alloc_list);
            xse_log("devmem: alloc  %2d %2d %16llx %16llx : %16llx\n",
                    alloc->dev_id, alloc->mem_id, alloc->start, alloc->size, alloc->token);
            xse_log("devmem: remain %2d %2d %16llx %16llx\n",
                    target->dev_id, target->mem_id, target->start, target->size);
        }
        if (target->size == 0) {
            list_del(&target->head);
            kfree(target);
        }
    }
    mutex_unlock(&s_dev_mem_mutex);
    return token;
}

static void dev_mem_free_core(struct dev_mem_region* target)
{
    struct dev_mem_region* lbound = NULL;
    struct dev_mem_region* prev = NULL;
    struct dev_mem_region* next = NULL;
    uint64_t target_end = target->start + target->size;
    xse_log("devmem: free   %2d %2d %16llx %16llx : %16llx\n",
            target->dev_id, target->mem_id, target->start, target->size, target->token);
    list_for_each_entry(next, &s_dev_mem_list, head) {
        uint64_t next_end = next->start + next->size;
        if (target && target->dev_id == next->dev_id && target->mem_id == next->mem_id) {
            if (target->start == next_end) {
                xse_log("devmem: gc(succ): %16llx %16llx + %16llx %16llx\n",
                        next->start, next->size, target->start, target->size);
                next->size += target->size;
                kfree(target);
                target = NULL;
            } else if (target_end == next->start) {
                xse_log("devmem: gc(pred): %16llx %16llx + %16llx %16llx\n",
                        target->start, target->size, next->start, next->size);
                next->start = target->start;
                next->size += target->size;
                kfree(target);
                target = NULL;
            } else if (!lbound || (next->start < target->start && lbound->start < next->start)) {
                lbound = next;
            }
        }
        if (!target && prev && prev->dev_id == next->dev_id && prev->mem_id == next->mem_id) {
            if (prev->start + prev->size == next->start) {
                xse_log("devmem: gc(mid pred): %16llx %16llx + %16llx %16llx\n",
                        prev->start, prev->size, next->start, next->size);
                prev->size += next->size;
                list_del(&next->head);
                kfree(next);
                break;
            } else if (prev->start == next->start + next->size) {
                xse_log("devmem: gc(mid succ): %16llx %16llx + %16llx %16llx\n",
                        next->start, next->size, prev->start, prev->size);
                next->size += prev->size;
                list_del(&prev->head);
                kfree(prev);
                break;
            }
        }
        prev = next;
    }
    if (target) {
        if (lbound) {
            xse_log("devmem: no-gc(succ): %16llx %16llx > %16llx %16llx\n",
                    lbound->start, lbound->size, target->start, target->size);
            list_add_tail(&target->head, &lbound->head);
        } else {
            xse_log("devmem: no-gc(none): %16llx %16llx\n",
                    target->start, target->size);
            list_add(&target->head, &s_dev_mem_list);
        }
    }
}

int dev_mem_get_range(struct xse_cdev* cdev, uint64_t token, uint64_t* addr, uint32_t* size)
{
    struct dev_mem_region* region = NULL;
    int dev_id = cdev->xse_pdev->id;
    int ret = -EINVAL;
    mutex_lock(&s_dev_mem_mutex);
    list_for_each_entry(region, &s_dev_mem_alloc_list, head) {
        if (region->token == token && region->dev_id == dev_id) {
            *addr = region->start;
            *size = region->size;
            ret = 0;
            break;
        }
    }
    mutex_unlock(&s_dev_mem_mutex);
    return ret;
}

static int dev_mem_free(struct xse_cdev* cdev, uint64_t token)
{
    struct dev_mem_region* region = NULL;
    struct dev_mem_region* target = NULL;
    mutex_lock(&s_dev_mem_mutex);
    list_for_each_entry(region, &s_dev_mem_alloc_list, head) {
        if (region->token == token && region->cdev_no == cdev->cdev_no) {
            target = region;
            break;
        }
    }
    if (target) {
        list_del(&target->head);
        dev_mem_free_core(target);
    }
    mutex_unlock(&s_dev_mem_mutex);
    return 0;
}

int dev_mems_free(struct xse_cdev* cdev)
{
    struct list_head* pos = NULL;
    struct list_head* pos_tmp = NULL;
    mutex_lock(&s_dev_mem_mutex);
    list_for_each_safe(pos, pos_tmp, &s_dev_mem_alloc_list) {
        struct dev_mem_region* region = list_entry(pos, struct dev_mem_region, head);
        if (region->cdev_no == cdev->cdev_no) {
            list_del(&region->head);
            dev_mem_free_core(region);
        }
    }
    mutex_unlock(&s_dev_mem_mutex);
    return 0;
}

int xse_dev_mem_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    int mem_id = -1;
    int pos = 3;
    while (info->name[pos] != '\0') {
        if (info->name[pos] == '/') {
            // todo: exclusion list
            if (!strncmp(&info->name[pos+1], "C0_DDR4_MEMORY_MAP_CTRL", 23)) {
                xse_log("devmem: %2d ctrl is skipped. (%s)\n", xse_pdev->id, info->name);
                return 0;
            }
        }
        pos++;
    }
    pos = 3;
    while (info->name[pos] != '\0') {
        if (info->name[pos] == '_') {
            mem_id = info->name[pos+1] - '0';
            break;
        }
        pos++;
    }
    if (mem_id >= 0) {
        return dev_mem_add(xse_pdev->id, mem_id, info->offset, info->size);
    }
    xse_log("devmem: %2d unrecognize mem name: %s\n", xse_pdev->id, info->name);
    return 0;
}

void xse_dev_mem_exit(struct xse_pci_dev* xse_pdev)
{
    dev_mem_clear(xse_pdev->id);
}

long xse_dev_mem_ioctl_alloc_devmem(struct file *file, unsigned long arg)
{
    devmem_info_t data;
    struct xse_cdev* cdev = file->private_data;

    if (copy_from_user(&data, (void __user*)arg, sizeof(data))) return -EFAULT;

    data.token = dev_mem_alloc(cdev, data.mem_id, data.size);
    if (!data.token) return -ENOMEM;

    if (copy_to_user((void __user*)arg, &data, sizeof(data))) return -EFAULT;
    return 0;
}

long xse_dev_mem_ioctl_free_devmem(struct file *file, unsigned long arg)
{
    devmem_info_t data;
    struct xse_cdev* cdev = file->private_data;

    if (copy_from_user(&data, (void __user*)arg, sizeof(data))) return -EFAULT;

    dev_mem_free(cdev, data.token);
    return 0;
}
