/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxse.h"
#include "device_info.h"
#include "route_controller.h"
#include "route_controller_ioctl.h"
#include "devmem.h"

static const int s_route_controller_incoming_offset = 0;
static const int s_route_controller_outgoing_offset = 0x8000;
static const int s_route_controller_ch_entry = 128;
static const int s_route_controller_buf_size = 4;
static const int s_route_controller_mem_address_valid = 8;
static const int s_route_controller_mem_address_used = 12;
static const int s_route_controller_relation = 16;
static const int s_route_controller_relation_ext = 20;
static const int s_route_controller_mem_entry_base = 8;
static const int s_route_controller_mem_entry_offset = s_route_controller_mem_entry_base * 4;
static const int s_route_controller_mem_entry_max = 24;

struct route_controller_ext {
    int instance_id;
    int is_incoming;
};

static void xse_route_controller_get_range(int is_incoming, int ch, unsigned int* start, unsigned int* end)
{
    unsigned int pos = is_incoming ? s_route_controller_incoming_offset : s_route_controller_outgoing_offset;
    *start = pos + ch * s_route_controller_ch_entry;
    *end = *start + s_route_controller_ch_entry;
}

static int xse_route_controller_close(struct inode *inode, struct file *file)
{
    int i;
    void __iomem* ptr;
    unsigned int addr_start, addr_end;
    struct route_controller_ext *ext_data;
    struct xse_cdev *cdev = (struct xse_cdev*)file->private_data;

    if (!cdev) return -EINVAL;

    ext_data = cdev->ext_data;
    if (!ext_data) return -EINVAL;

    xse_route_controller_get_range(ext_data->is_incoming, cdev->id, &addr_start, &addr_end);
    ptr = cdev->base + addr_start;
    for (i=0; i<s_route_controller_ch_entry; i+=4) {
        uint32_t val = ioread32(ptr + i);
        xse_log("route_controller_%s[%d][%d][%d] read[%u](%4x,%lx): %08x\n",
                ext_data->is_incoming ? "in" : "out", cdev->xse_pdev->id, ext_data->instance_id, cdev->id,
                i, addr_start + i, (uintptr_t)(ptr + i), val);
    }
    iowrite32(0xffff0000, ptr); // valid, mmapped,...
    iowrite32(0, ptr + s_route_controller_buf_size); // buf size
    iowrite32(0, ptr + s_route_controller_mem_address_valid);
    iowrite32(0, ptr + s_route_controller_mem_address_used);
    iowrite32(0, ptr + s_route_controller_relation);

    dev_mems_free(cdev);

    return 0;
}


static int get_route_controller_cfg(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    // todo: add config register to ip.
    return 16; // ch num
}

static ssize_t xse_route_controller_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    int i;
    unsigned int data;
    void __iomem* ptr;
    loff_t ofs = *pos;
    unsigned int addr_start, addr_end;
    struct xse_cdev *cdev = file->private_data;
    struct route_controller_ext *ext_data = cdev->ext_data;

    if (!cdev) return -EINVAL;
    if (ofs + count > cdev->size) return -EINVAL;
    if (count & 3) return -EINVAL;

    xse_route_controller_get_range(ext_data->is_incoming, cdev->id, &addr_start, &addr_end);
    if (!(addr_start <= ofs && ofs + count <= addr_end)) return -EPROTO;

    for (i=0; i<count; i+=4) {
        int ret;
        ptr = cdev->base + addr_start + ofs + i;
        data = ioread32(ptr);
        xse_log("route_controller[%d] read[%llx]: %x (%s)\n", cdev->xse_pdev->id, ofs+i, data,
                ((struct xse_device_info*)cdev->module_info)->name);
        ret = copy_to_user(buf + i, &data, 4);
        if (ret) {
            xse_err("route_controller[%d][%x] read: copy to user failed\n", cdev->xse_pdev->id, cdev->id);
        }
        *pos+=4;
    }
    return count;
}

static ssize_t xse_route_controller_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    int i;
    unsigned int data;
    void __iomem* ptr;
    loff_t ofs = *pos;
    unsigned int addr_start, addr_end;
    struct xse_cdev *cdev = file->private_data;
    struct route_controller_ext *ext_data = cdev->ext_data;

    if (!cdev) return -EINVAL;
    if (ofs + count > cdev->size) return -EINVAL;
    if (count & 3) return -EINVAL;

    xse_route_controller_get_range(ext_data->is_incoming, cdev->id, &addr_start, &addr_end);
    if (!(addr_start <= ofs && ofs + count <= addr_end)) return -EPROTO;

    for (i=0; i<count; i+=4) {
        int ret;
        ptr = cdev->base + ofs + i;
        ret = copy_from_user(&data, buf + i, 4);
        if (ret) {
            xse_err("route_controller[%d][%x] write: copy from user failed\n", cdev->xse_pdev->id, cdev->id);
        } else {
            iowrite32(data, ptr);
            xse_log("route_controller[%d] write[%llx]: %x (%s)\n", cdev->xse_pdev->id, ofs+i, data,
                ((struct xse_device_info*)cdev->module_info)->name);
        }
        *pos+=4;
    }
    return count;
}

static long route_controller_ioctl_set_devmem_addr(struct file *file, unsigned long arg)
{
    route_controller_set_mem_info_t data;
    int ret;
    uint64_t paddr = 0;
    uint32_t size = 0;
    uint32_t set_val = 0;
    uint32_t read_val = 0;
    unsigned int addr_start, addr_end;
    void __iomem* ptr;
    struct xse_cdev* cdev = file->private_data;
    struct route_controller_ext *ext_data = cdev->ext_data;

    if (!ext_data) return -EINVAL;
    if (copy_from_user(&data, (void __user*)arg, sizeof(data))) return -EFAULT;
    if (data.idx >= s_route_controller_mem_entry_max) return -EINVAL;

    xse_route_controller_get_range(ext_data->is_incoming, cdev->id, &addr_start, &addr_end);

    ret = dev_mem_get_range(cdev, data.token, &paddr, &size);
    if (ret) return ret;
    if (data.mem_offset >= size) return -EINVAL;

    ptr = cdev->base + addr_start + s_route_controller_mem_entry_offset + data.idx * 4;
    set_val = (paddr + data.mem_offset) >> 12; // page size unit

    iowrite32(set_val, ptr);
    read_val = ioread32(ptr);
    xse_log("route_controller_%s[%d][%d][%d] mem[%u](%4x,%lx): %16llx %08x(%08x) (%s)\n",
            ext_data->is_incoming ? "in" : "out", cdev->xse_pdev->id, ext_data->instance_id, cdev->id,
            data.idx, addr_start + s_route_controller_mem_entry_offset + data.idx * 4, (uintptr_t)ptr,
            paddr + data.mem_offset, set_val, read_val,
            ((struct xse_device_info*)cdev->module_info)->name);

    ptr = cdev->base + addr_start + s_route_controller_mem_address_valid;
    set_val = ioread32(ptr);
    set_val |= 1u << (data.idx + s_route_controller_mem_entry_base);
    iowrite32(set_val, ptr);
    read_val = ioread32(ptr);
    xse_log("route_controller_%s[%d][%d][%d] vld[%u](%4x,%lx): %08x(%08x) (%s)\n",
            ext_data->is_incoming ? "in" : "out", cdev->xse_pdev->id, ext_data->instance_id, cdev->id,
            data.idx, addr_start + s_route_controller_mem_address_valid, (uintptr_t)ptr,
            set_val, read_val,
            ((struct xse_device_info*)cdev->module_info)->name);

    return 0;
}

static long xse_route_controller_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case XSE_ROUTE_CONTROLLER_ALLOC_DEVMEM:
        return xse_dev_mem_ioctl_alloc_devmem(file, arg);
    case XSE_ROUTE_CONTROLLER_FREE_DEVMEM:
        return xse_dev_mem_ioctl_free_devmem(file, arg);
    case XSE_ROUTE_CONTROLLER_SET_DEVMEM_ADDR:
        return route_controller_ioctl_set_devmem_addr(file, arg);
    default:
        return -EINVAL;
    }
    return 0;
}

static const struct file_operations s_route_controller_ops = {
    .owner = THIS_MODULE,
    .open = xse_device_file_open,
    .release = xse_route_controller_close,
    .llseek = xse_device_file_seek,
    .read = xse_route_controller_read,
    .write = xse_route_controller_write,
    .unlocked_ioctl = xse_route_controller_ioctl,
};

int xse_route_controller_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    int ch;
    int instance_id = 0;
    int ret = 0;
    int ch_num = 0;
    char in_name[] = "route_controller_0_in_00";
    char out_name[] = "route_controller_0_out_00";
    static const int s_rc_id_pos = 17;
    static const int s_ich_id_pos = 22;
    static const int s_och_id_pos = 23;
    struct route_controller_ext *ext_idata = NULL;
    struct route_controller_ext *ext_odata = NULL;

    if (!info) return 0;

    instance_id = xse_pdev->route_controller_num;
    if (instance_id == ROUTE_CONTROLLER_MAX) {
        release_device_info(info);
        return 0;
    }

    ch_num = get_route_controller_cfg(xse_pdev, info);

    in_name[s_rc_id_pos] = instance_id + 0x30;
    out_name[s_rc_id_pos] = instance_id + 0x30;

    for (ch = 0; ch<ch_num; ch++) {
        int rc_id = ch + CH_MAX * instance_id;
        if (ch < 10) {
            in_name[s_ich_id_pos] = ch + 0x30;
            in_name[s_ich_id_pos+1] = '\0';
            out_name[s_och_id_pos] = ch + 0x30;
            out_name[s_och_id_pos+1] = '\0';
        } else {
            in_name[s_ich_id_pos] = (ch / 10) + 0x30;
            in_name[s_ich_id_pos+1] = (ch % 10) + 0x30;
            out_name[s_och_id_pos] = (ch / 10) + 0x30;
            out_name[s_och_id_pos+1] = (ch % 10) + 0x30;
        }
        ret = xse_cdev_init(xse_pdev, &xse_pdev->route_controller_in[rc_id], in_name,
                            DEVICE_MINOR_ROUTE_CONTROLLER_IN + CH_MAX * instance_id + ch,
                            info, &s_route_controller_ops, ch, ch==0 ? 1 : 0);
        if (ret) return ret;
        if (!ext_idata) {
            ext_idata = kmalloc(sizeof(*ext_idata), GFP_KERNEL);
            ext_idata->is_incoming = 1;
            ext_idata->instance_id = instance_id;
        }
        xse_pdev->route_controller_in[rc_id].ext_data = ext_idata;
        ret = xse_cdev_init(xse_pdev, &xse_pdev->route_controller_out[rc_id], out_name,
                            DEVICE_MINOR_ROUTE_CONTROLLER_OUT + CH_MAX * instance_id + ch,
                            info, &s_route_controller_ops, ch, 0);
        if (ret) return ret;
        if (!ext_odata) {
            ext_odata = kmalloc(sizeof(*ext_odata), GFP_KERNEL);
            ext_odata->is_incoming = 0;
            ext_odata->instance_id = instance_id;
        }
        xse_pdev->route_controller_out[rc_id].ext_data = ext_odata;
        xse_pdev->route_controller_ch[instance_id] = ch+1;
    }
    xse_pdev->route_controller_num++;
    return 0;
}

void xse_route_controller_exit(struct xse_pci_dev* xse_pdev)
{
    int instance_num;
    int instance_id;
    int ch;
    struct xse_device_info *info = NULL;
    struct route_controller_ext* idata = NULL;
    struct route_controller_ext* odata = NULL;

    instance_num = xse_pdev->route_controller_num;
    for (instance_id = 0; instance_id < instance_num; instance_id++) {
        int ch_num = xse_pdev->route_controller_ch[instance_id];
        for (ch = 0; ch<ch_num; ch++) {
            int rc_id = ch + CH_MAX * instance_id;
            struct xse_cdev* cdev = &xse_pdev->route_controller_in[rc_id];
            xse_cdev_exit(cdev);
            if (cdev->module_info_owner) {
                info = cdev->module_info;
            }
            if (cdev->id == 0) {
                idata = cdev->ext_data;
            }
            cdev = &xse_pdev->route_controller_out[rc_id];
            xse_cdev_exit(cdev);
            if (cdev->module_info_owner) {
                info = cdev->module_info;
            }
            if (cdev->id == 0) {
                odata = cdev->ext_data;
            }
        }
        if (info) {
            kfree(info);
            info = NULL;
        }
        if (idata) {
            kfree(idata);
            idata = NULL;
        }
        if (odata) {
            kfree(odata);
            odata = NULL;
        }
    }
}
