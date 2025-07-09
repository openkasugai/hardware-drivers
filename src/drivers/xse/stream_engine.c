/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxse.h"
#include "device_info.h"
#include "stream_engine.h"
#include "stream_engine_ioctl.h"
#include "mem_manage_ext.h"

#define STREAM_ENGINE_ID 0x6e457453

static const int s_vpmap_entry = 32;
static const int s_vpmap_agg_entry = 16;
static const int s_ctrl_entry = 64;
static const int s_vpmap_offset = 8;
static const int s_queue_depth = 8;
static const int s_queue_element_byte = 32;
static const int s_head_tail_size = 8;

static const uint64_t s_page_size = 4096;
static const uint64_t s_bypass_size = 0x40000000UL; // 1GiB
static const uint32_t s_req_q_offset = 0x08;
static const uint32_t s_doorbell_addr_offset = 0x30;
static const uint32_t s_interrupt_and_doorbell_enable_offset = 0x38;
static const uint32_t s_doorbell_offset = 0x3c;

struct stream_engine_ext {
    struct mutex mutex;
    int ch_num;
    int vpmap_num;
    int is_tx;
};

struct stream_engine_list_element {
    struct list_head head;
    int is_tx;
    int dev_id;
    int ch_id;
    struct xse_cdev* cdev;
};

static LIST_HEAD(s_engine_list);
static DEFINE_MUTEX(s_engine_mutex);

static void engine_list_add(struct stream_engine_list_element* element)
{
    mutex_lock(&s_engine_mutex);
    list_add(&element->head, &s_engine_list);
    mutex_unlock(&s_engine_mutex);
}

static struct xse_cdev* engine_list_find(int is_tx, int dev_id, int ch_id)
{
    struct xse_cdev* ret = NULL;
    struct stream_engine_list_element *elem;
    mutex_lock(&s_engine_mutex);
    list_for_each_entry(elem, &s_engine_list, head) {
        if (elem->is_tx == is_tx && elem->dev_id == dev_id && elem->ch_id == ch_id) {
            ret = elem->cdev;
            break;
        }
    }
    mutex_unlock(&s_engine_mutex);
    return ret;
}

static void engine_list_del(struct xse_cdev* cdev)
{
    struct stream_engine_list_element *elem;
    mutex_lock(&s_engine_mutex);
    list_for_each_entry(elem, &s_engine_list, head) {
        if (elem->cdev == cdev) {
            list_del(&elem->head);
            kfree(elem);
            break;
        }
    }
    mutex_unlock(&s_engine_mutex);
}

static void xse_stream_engine_get_range(int ch, int ch_num, int vpmap_num,
                                        unsigned int* vpmap_start, unsigned int* vpmap_end,
                                        unsigned int* ctrl_start, unsigned int* ctrl_end,
                                        unsigned int* vpmap_page, int* aggregate_ptr)
{
    int aggregate = vpmap_num * ch_num * s_vpmap_entry > 32768;
    unsigned int vpmap_ch = aggregate ? 0 : s_vpmap_entry * vpmap_num * ch;
    unsigned int vpmap_ch_end = aggregate ? s_vpmap_agg_entry * vpmap_num : s_vpmap_entry * vpmap_num * (ch + 1);
    unsigned int ctrl = (aggregate ? s_vpmap_agg_entry * vpmap_num : s_vpmap_entry * vpmap_num * ch_num);
    unsigned int ctrl_all_end = ctrl + s_ctrl_entry * ch_num;

    *vpmap_start = vpmap_ch;
    *vpmap_end = vpmap_ch_end;
    *ctrl_start = ctrl + s_ctrl_entry * ch;
    *ctrl_end = *ctrl_start + s_ctrl_entry;
    *vpmap_page = ctrl_all_end + s_vpmap_offset;
    *aggregate_ptr = aggregate;
}

static void xse_stream_engine_get_queue_range(int ch, int ch_num, int vpmap_num,
                                              unsigned int* req_q_start, unsigned int* cpl_q_start,
                                              unsigned int* req_q_head_tail, unsigned int* cpl_q_head_tail)
{
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    unsigned int queue_start, head_tail_start;
    int aggregate;
    xse_stream_engine_get_range(ch, ch_num, vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);
    queue_start = (vpmap_page + 63) & ~63;
    head_tail_start = queue_start + s_queue_depth * s_queue_element_byte * ch_num * 2;
    queue_start += s_queue_depth * s_queue_element_byte * ch * 2;
    head_tail_start += s_head_tail_size * ch * 2;
    *req_q_start = queue_start;
    *cpl_q_start = queue_start + s_queue_depth * s_queue_element_byte;
    *req_q_head_tail = head_tail_start;
    *cpl_q_head_tail = head_tail_start + s_head_tail_size;
}

static int xse_stream_engine_check_ctrl_regs_and_stop(struct xse_cdev *cdev, int disable)
{
    void __iomem* ptr;
    unsigned int data;
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    int aggregate;
    int i;
    struct stream_engine_ext *ext_data = NULL;
    ext_data = cdev->ext_data;
    xse_stream_engine_get_range(cdev->id, ext_data->ch_num, ext_data->vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);

    ptr = cdev->base + ctrl_start;
    for (i=0; i<s_ctrl_entry; i+=4) {
        data = ioread32(ptr + i);
        xse_log("stream_engine_%s[%d][%d]: check[%x]: %8x (%s)\n",
                ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id, i, data,
                ((struct xse_device_info*)cdev->module_info)->name);
    }
    if (disable) {
        iowrite32(0, ptr);
        iowrite32(0, ptr + s_doorbell_addr_offset);
        iowrite32(0, ptr + s_doorbell_addr_offset + 4);
        iowrite32(0, ptr + s_interrupt_and_doorbell_enable_offset);
        xse_log("stream_engine_%s[%d][%d]: disabled (%s)\n",
                ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id,
                ((struct xse_device_info*)cdev->module_info)->name);
    }

    return 0;
}

static int xse_stream_engine_close(struct inode *inode, struct file *file)
{
    struct xse_cdev *cdev = (struct xse_cdev*)file->private_data;
    if (!cdev) return -EINVAL;

    return xse_stream_engine_check_ctrl_regs_and_stop(cdev, 1);
}

static ssize_t xse_stream_engine_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    int i;
    unsigned int data;
    void __iomem* ptr;
    loff_t ofs = *pos;
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    int aggregate;
    struct xse_cdev *cdev = file->private_data;
    struct stream_engine_ext *ext_data = cdev->ext_data;

    if (!cdev) return -EINVAL;
    if (ofs + count > cdev->size) return -EINVAL;
    if (count & 3) return -EINVAL;

    xse_stream_engine_get_range(cdev->id, ext_data->ch_num, ext_data->vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);
    if (!((vpmap_start <= ofs && ofs + count <= vpmap_end) ||
          (ctrl_start <= ofs && ofs + count <= ctrl_end) ||
          ((vpmap_page == ofs || vpmap_page - 8 == ofs) && count == 4))) return -EPROTO;

    for (i=0; i<count; i+=4) {
        int ret;
        ptr = cdev->base + ofs + i;
        data = ioread32(ptr);
        xse_log("stream_engine_%s[%d][%x]: read[%llx]: %x (%s)\n",
                ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id, ofs+i, data,
                ((struct xse_device_info*)cdev->module_info)->name);
        ret = copy_to_user(buf + i, &data, 4);
        if (ret) {
            xse_err("stream_engine_%s[%d][%x]: read: copy to user failed\n",
                    ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id);
        }
        *pos+=4;
    }
    return count;
}

static ssize_t xse_stream_engine_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    int i;
    unsigned int data;
    void __iomem* ptr;
    loff_t ofs = *pos;
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    int aggregate;
    struct xse_cdev *cdev = file->private_data;
    struct stream_engine_ext *ext_data = cdev->ext_data;

    if (!cdev) return -EINVAL;
    if (ofs + count > cdev->size) return -EINVAL;
    if (count & 3) return -EINVAL;

    xse_stream_engine_get_range(cdev->id, ext_data->ch_num, ext_data->vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);
    if (!((ctrl_start <= ofs && ofs + count <= ctrl_end) ||
          (vpmap_page - 8 == ofs && count == 4))) return -EPROTO;

    for (i=0; i<count; i+=4) {
        int ret;
        ptr = cdev->base + ofs + i;
        ret = copy_from_user(&data, buf + i, 4);
        if (ret) {
            xse_err("stream_engine_%s[%x]: write: copy from user failed\n",
                    ext_data->is_tx ? "tx" : "rx", cdev->id);
        } else {
            iowrite32(data, ptr);
            xse_log("stream_engine_%s[%d][%x]: write[%llx]: %x (%s)\n",
                    ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id, ofs+i, data,
                    ((struct xse_device_info*)cdev->module_info)->name);
        }
        *pos+=4;
    }
    return count;
}

static long stream_engine_ioctl_vpmap_reset(struct file * file, unsigned long arg)
{
    stream_engine_vp_info_t data;
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    int aggregate;
    struct xse_cdev* cdev = file->private_data;
    struct stream_engine_ext* ext_data = cdev->ext_data;

    if (!ext_data) return -EINVAL;
    if (copy_from_user(&data, (void __user*)arg, sizeof(data))) return -EFAULT;

    xse_stream_engine_get_range(cdev->id, ext_data->ch_num, ext_data->vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);
    if (data.entry >= ext_data->vpmap_num) return -EINVAL;
    if (aggregate) {
        // 0: vaddr[43:12] (32)
        // 1: paddr[23:12] (12), vaddr[63:44] (20)
        // 2: paddr[55:24] (32)
        // 3: valid, size[31:12] (20), paddr[63:56] (8)
        int i;
        void __iomem* page_ptr;
        void __iomem* entry_ptr;
        page_ptr = cdev->base + vpmap_page;
        entry_ptr = cdev->base + vpmap_start + data.entry * s_vpmap_agg_entry;
        mutex_lock(&ext_data->mutex);
        iowrite32(cdev->id, page_ptr);
        for (i=0; i<4; i++) {
            iowrite32(0, entry_ptr + i*4);
        }
        mutex_unlock(&ext_data->mutex);
    } else {
        int i;
        void __iomem* ptr;
        ptr = cdev->base + vpmap_start + data.entry * s_vpmap_entry;
        for (i=0; i<6; i++) iowrite32(0, ptr + i*4);
    }
    return 0;
}

static long set_vpmap_entry(struct xse_cdev* cdev, int aggregate, unsigned int vpmap_start, unsigned int vpmap_page,
                            uint32_t entry, uint64_t vaddr, uint64_t paddr, uint32_t psize)
{
    struct stream_engine_ext* ext_data = cdev->ext_data;
    if (!ext_data) return -EINVAL;
    if (aggregate) {
        // 0: vaddr[43:12] (32)
        // 1: paddr[23:12] (12), vaddr[63:44] (20)
        // 2: paddr[55:24] (32)
        // 3: valid, size[31:12] (20), paddr[63:56] (8)
        uint32_t write_data[4];
        int i;
        void __iomem* page_ptr;
        void __iomem* entry_ptr;
        page_ptr = cdev->base + vpmap_page;
        entry_ptr = cdev->base + vpmap_start + entry * s_vpmap_agg_entry;
        psize = (psize + 4095) & ~4095;
        write_data[0] = vaddr >> 12;
        write_data[1] = (vaddr >> 44) | ((paddr << 8) & 0xfff00000);
        write_data[2] = paddr >> 24;
        write_data[3] = (paddr >> 56) | ((psize >> 4) & 0x0fffff00) | (1 << 28);
        mutex_lock(&ext_data->mutex);
        iowrite32(cdev->id, page_ptr);
        for (i=0; i<4; i++) {
            iowrite32(write_data[i], entry_ptr + i*4);
        }
        mutex_unlock(&ext_data->mutex);
        xse_log("stream_engine_%s[%d][%x]: vpmap[%x]: vaddr=%llx, paddr=%llx, size=%x\n",
                ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id,
                vpmap_start + entry * s_vpmap_agg_entry, vaddr, paddr, psize);
    } else {
        void __iomem* ptr;
        ptr = cdev->base + vpmap_start + entry * s_vpmap_entry;
        iowrite32(vaddr & 0xffffffff, ptr);
        iowrite32(vaddr >> 32, ptr+4);
        iowrite32(paddr & 0xffffffff, ptr+8);
        iowrite32(paddr >> 32, ptr+12);
        iowrite32(psize, ptr+16);
        iowrite32(1, ptr+20);
        xse_log("stream_engine_%s[%d][%x]: vpmap[%x]: vaddr=%llx, paddr=%llx, size=%x\n",
                ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id,
                vpmap_start + entry * s_vpmap_entry, vaddr, paddr, psize);
    }
    return 0;
}

static long stream_engine_ioctl_vpmap_set(struct file * file, unsigned long arg)
{
    stream_engine_vp_info_t data;
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    int aggregate;
    uint64_t paddr;
    uint32_t psize, psize_ceil;
    long stat;
    struct xse_cdev* cdev = file->private_data;
    struct stream_engine_ext* ext_data = cdev->ext_data;

    if (!ext_data) return -EINVAL;
    if (copy_from_user(&data, (void __user*)arg, sizeof(data))) return -EFAULT;

    xse_stream_engine_get_range(cdev->id, ext_data->ch_num, ext_data->vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);
    if (data.entry >= ext_data->vpmap_num) return -EINVAL;

    stat = mem_manage_get_phys_addr_gpl(data.token, data.range_id, &paddr, &psize);
    if (stat < 0) return -EINVAL;

    // modify to 4k boundary
    psize_ceil = psize + (data.vaddr & (s_page_size - 1));
    data.vaddr = data.vaddr & ~(s_page_size - 1);
    psize_ceil = (psize_ceil + s_page_size - 1) & ~(s_page_size-1);

    stat = set_vpmap_entry(cdev, aggregate, vpmap_start, vpmap_page,
                           data.entry, data.vaddr, paddr, psize_ceil);
    if (stat) return stat;
    data.psize = psize;

    if (copy_to_user((void __user*)arg, &data, sizeof(data))) return -EFAULT;

    return 0;
}

static long stream_engine_ioctl_paddr_write(struct file * file, unsigned long arg)
{
    stream_engine_vp_info_t data;
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    int aggregate;
    uint64_t paddr;
    uint32_t psize;
    long stat;
    void __iomem* ptr;
    struct xse_cdev* cdev = file->private_data;
    struct stream_engine_ext* ext_data = cdev->ext_data;

    if (!ext_data) return -EINVAL;
    if (copy_from_user(&data, (void __user*)arg, sizeof(data))) return -EFAULT;

    xse_stream_engine_get_range(cdev->id, ext_data->ch_num, ext_data->vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);
    if (data.entry >= s_ctrl_entry/4) return -EINVAL;

    stat = mem_manage_get_phys_addr(data.token, data.range_id, &paddr, &psize);
    if (stat < 0 || (psize && psize < data.voffset)) return -EINVAL;

    paddr += data.voffset;
    ptr = cdev->base + ctrl_start + data.entry * 4;
    iowrite32(paddr & 0xffffffff, ptr);
    iowrite32(paddr >> 32, ptr+4);
    data.psize = psize;
    xse_log("stream_engine_%s[%d][%x]: offset=%x, paddr=%llx\n",
            ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id, ctrl_start + data.entry*4, paddr);

    if (copy_to_user((void __user*)arg, &data, sizeof(data))) return -EFAULT;

    return 0;
}

static long stream_engine_ioctl_cfg_get(struct file *file, unsigned long arg)
{
    stream_engine_info_t data;
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    int aggregate;
    struct xse_cdev* cdev = file->private_data;
    struct stream_engine_ext* ext_data = cdev->ext_data;

    if (!ext_data) return -EINVAL;
    xse_stream_engine_get_range(cdev->id, ext_data->ch_num, ext_data->vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);
    data.ch_num = ext_data->ch_num;
    data.vpmap_num = ext_data->vpmap_num;
    data.aggregate = aggregate;

    if (copy_to_user((void __user*)arg, &data, sizeof(data))) return -EFAULT;

    return 0;
}

static long stream_engine_ioctl_mem_manage_range_id_get(struct file *file, unsigned long arg)
{
    stream_engine_vp_info_t data;
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    int aggregate;
    uint32_t id = 0;
    uint64_t paddr;
    uint32_t psize;
    struct xse_cdev* cdev = file->private_data;
    struct stream_engine_ext* ext_data = cdev->ext_data;

    if (!ext_data) return -EINVAL;
    if (copy_from_user(&data, (void __user*)arg, sizeof(data))) return -EFAULT;

    xse_stream_engine_get_range(cdev->id, ext_data->ch_num, ext_data->vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);
    while (true) {
        int stat = mem_manage_get_phys_addr(data.token, id, &paddr, &psize);
        xse_log("stream_engine_%s[%d][%x]: get phys addr: token=%llx, voffset=%x, range_id=%u, paddr=%llx, psize=%x, stat=%d\n",
                ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id,
                data.token, data.voffset, id, paddr, psize, stat);
        if (stat < 0) return -EFAULT;
        if (data.voffset < psize) {
            data.range_id = id;
            data.psize = psize - data.voffset;
            xse_log("stream_engine_%s[%d][%x]: token=%llx, voffset=%x, range_id=%u, psize=%x\n",
                    ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id,
                    data.token, data.voffset, id, data.psize);
            break;
        }
        if (data.voffset == 0) return -EINVAL;
        data.voffset -= psize;
        id++;
    }
    if (copy_to_user((void __user*)arg, &data, sizeof(data))) return -EFAULT;
    return 0;
}

static long stream_engine_ioctl_check_ctrl_regs(struct file *file, unsigned long arg)
{
    struct xse_cdev* cdev = file->private_data;
    if (!cdev) return -EINVAL;

    return xse_stream_engine_check_ctrl_regs_and_stop(cdev, 0);
}

static long stream_engine_ioctl_doorbell_addr_set(struct file *file, unsigned long arg)
{
    stream_engine_cp_info_t data;
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    int aggregate;
    int is_tx_cp;
    struct xse_cdev* cdev_cp = NULL;
    struct xse_cdev* cdev = file->private_data;
    struct stream_engine_ext* ext_data = NULL;
    if (!cdev) return -EINVAL;

    ext_data = cdev->ext_data;
    if (!ext_data) return -EINVAL;

    if (copy_from_user(&data, (void __user*)arg, sizeof(data))) return -EFAULT;

    xse_stream_engine_get_range(cdev->id, ext_data->ch_num, ext_data->vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);

    is_tx_cp = ext_data->is_tx ? 0 : 1;
    if (!is_tx_cp) return -EPERM;

    cdev_cp = engine_list_find(is_tx_cp, data.dev_id, data.ch_id);
    if (cdev_cp) {
        int ret;
        void __iomem* ptr;
        uint64_t paddr, psize;
        uint64_t doorbell_paddr;

        unsigned int cp_vpmap_start, cp_vpmap_end, cp_ctrl_start, cp_ctrl_end, cp_vpmap_page;
        int cp_aggregate;
        struct stream_engine_ext* cp_ext_data = cdev_cp->ext_data;

        xse_stream_engine_get_range(cdev_cp->id, cp_ext_data->ch_num, cp_ext_data->vpmap_num,
                                    &cp_vpmap_start, &cp_vpmap_end, &cp_ctrl_start, &cp_ctrl_end, &cp_vpmap_page,
                                    &cp_aggregate);

        ret = get_register_bar_info(cdev_cp, &paddr, &psize);
        if (ret) return -EINVAL;

        ptr = cdev->base + ctrl_start + s_doorbell_addr_offset;
        doorbell_paddr = paddr + cp_ctrl_start + s_doorbell_offset;

        iowrite32(doorbell_paddr & 0xffffffff, ptr);
        iowrite32(doorbell_paddr >> 32, ptr + 4);
        xse_log("stream_engine_%s[%d][%x]: doorbell addr set: %llx\n",
                ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id, doorbell_paddr);
        return 0;
    }
    return -EPERM;
}

static long stream_engine_ioctl_counterpart_queue_info_set(struct file *file, unsigned long arg)
{
    stream_engine_cp_info_t data;
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    int aggregate;
    int is_tx_cp;
    struct xse_cdev* cdev_cp = NULL;
    struct xse_cdev* cdev = file->private_data;
    struct stream_engine_ext* ext_data = NULL;
    if (!cdev) return -EINVAL;

    ext_data = cdev->ext_data;
    if (!ext_data) return -EINVAL;

    if (copy_from_user(&data, (void __user*)arg, sizeof(data))) return -EFAULT;

    xse_stream_engine_get_range(cdev->id, ext_data->ch_num, ext_data->vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);

    is_tx_cp = ext_data->is_tx ? 0 : 1;
    if (is_tx_cp) {
        xse_err("stream_engine_%s[%x]: cp type is tx\n",
                ext_data->is_tx ? "tx" : "rx", cdev->id);
        return -EPERM;
    }

    cdev_cp = engine_list_find(is_tx_cp, data.dev_id, data.ch_id);
    if (cdev_cp) {
        int ret;
        void __iomem* ptr;
        uint64_t paddr, psize;
        unsigned int req_q_start, cpl_q_start, req_q_head_tail, cpl_q_head_tail;
        uint64_t req_q_start_addr, cpl_q_start_addr, req_q_head_tail_addr, cpl_q_head_tail_addr;
        struct stream_engine_ext* cp_ext_data = cdev_cp->ext_data;

        xse_stream_engine_get_queue_range(cdev_cp->id, cp_ext_data->ch_num, cp_ext_data->vpmap_num,
                                          &req_q_start, &cpl_q_start, &req_q_head_tail, &cpl_q_head_tail);

        ret = get_axi_bypass_bar_info(cdev_cp, &paddr, &psize);
        if (ret) return -EINVAL;

        req_q_start_addr = paddr + req_q_start;
        cpl_q_start_addr = paddr + cpl_q_start;
        req_q_head_tail_addr = paddr + req_q_head_tail;
        cpl_q_head_tail_addr = paddr + cpl_q_head_tail;

        ptr = cdev->base + ctrl_start + s_req_q_offset;
        iowrite32(req_q_start_addr & 0xffffffff, ptr);
        iowrite32(req_q_start_addr >> 32, ptr + 4);
        iowrite32(cpl_q_start_addr & 0xffffffff, ptr + 8);
        iowrite32(cpl_q_start_addr >> 32, ptr + 12);
        iowrite32(req_q_head_tail_addr & 0xffffffff, ptr + 16);
        iowrite32(req_q_head_tail_addr >> 32, ptr + 20);
        iowrite32(cpl_q_head_tail_addr & 0xffffffff, ptr + 24);
        iowrite32(cpl_q_head_tail_addr >> 32, ptr + 28);
        iowrite32(0x0808, ptr + 32); // queue depth
        iowrite32(0x10, ptr + 48); // enable to check with doorbell
        xse_log("stream_engine_%s[%d][%x]: queue: %llx %llx %llx %llx\n",
                ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id,
                req_q_start_addr, cpl_q_start_addr, req_q_head_tail_addr, cpl_q_head_tail_addr);
        return 0;
    }
    xse_err("stream_engine_%s[%d][%x]: cp cdev not found: %d %d\n",
            ext_data->is_tx ? "tx" : "rx", cdev->xse_pdev->id, cdev->id, data.dev_id, data.ch_id);
    return -EPERM;

}

static long stream_engine_ioctl_counterpart_vpmap_set(struct file *file, unsigned long arg)
{
    stream_engine_cp_info_t data;
    unsigned int vpmap_start, vpmap_end, ctrl_start, ctrl_end, vpmap_page;
    int aggregate;
    int is_tx_cp;
    struct xse_cdev* cdev_cp = NULL;
    struct xse_cdev* cdev = file->private_data;
    struct stream_engine_ext* ext_data = NULL;
    if (!cdev) return -EINVAL;

    ext_data = cdev->ext_data;
    if (!ext_data) return -EINVAL;

    if (copy_from_user(&data, (void __user*)arg, sizeof(data))) return -EFAULT;

    is_tx_cp = ext_data->is_tx ? 0 : 1;
    if (is_tx_cp) return -EPERM;

    xse_stream_engine_get_range(cdev->id, ext_data->ch_num, ext_data->vpmap_num,
                                &vpmap_start, &vpmap_end, &ctrl_start, &ctrl_end, &vpmap_page, &aggregate);

    cdev_cp = engine_list_find(is_tx_cp, data.dev_id, data.ch_id);
    if (cdev_cp) {
        uint64_t vaddr, paddr, psize;
        int ret = get_axi_bypass_bar_info(cdev_cp, &paddr, &psize);
        if (ret) return -EINVAL;

        vaddr = s_bypass_size * (1 + cdev->id);
        paddr += vaddr;
        psize = s_bypass_size;
        return set_vpmap_entry(cdev, aggregate, vpmap_start, vpmap_page,
                               0, vaddr, paddr, psize & 0xffffffff);
    }
    return -EPERM;
}

static long xse_stream_engine_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case XSE_STREAM_ENGINE_GET_CFG:
        return stream_engine_ioctl_cfg_get(file, arg);
    case XSE_STREAM_ENGINE_GET_MEM_MANAGE_RANGE:
        return stream_engine_ioctl_mem_manage_range_id_get(file, arg);
    case XSE_STREAM_ENGINE_RESET_VPMAP:
        return stream_engine_ioctl_vpmap_reset(file, arg);
    case XSE_STREAM_ENGINE_SET_VPMAP:
        return stream_engine_ioctl_vpmap_set(file, arg);
    case XSE_STREAM_ENGINE_WRITE_PADDR:
        return stream_engine_ioctl_paddr_write(file, arg);
    case XSE_STREAM_ENGINE_CHECK_CTRL_REGS:
        return stream_engine_ioctl_check_ctrl_regs(file, arg);
    case XSE_STREAM_ENGINE_SET_DOORBELL_ADDR:
        return stream_engine_ioctl_doorbell_addr_set(file, arg);
    case XSE_STREAM_ENGINE_SET_QUEUE_INFO:
        return stream_engine_ioctl_counterpart_queue_info_set(file, arg);
    case XSE_STREAM_ENGINE_SET_CP_VPMAP:
        return stream_engine_ioctl_counterpart_vpmap_set(file, arg);
    default:
        return -EINVAL;
    }
    return 0;
}

static const struct file_operations s_stream_engine_ops = {
    .owner = THIS_MODULE,
    .open = xse_device_file_open,
    .release = xse_stream_engine_close,
    .llseek = xse_device_file_seek,
    .read = xse_stream_engine_read,
    .write = xse_stream_engine_write,
    .unlocked_ioctl = xse_stream_engine_ioctl,
};

static int get_stream_engine_cfg(struct xse_pci_dev* xse_pdev, struct xse_device_info* info,
                                 unsigned int* ch_num, unsigned int* vpmap_num)
{
    int ch_log = 0;
    int vpmap_log = 0;
    int aggregate = 0;
    int id_offset = 0;
    void __iomem* ptr;
    if (!xse_pdev || !xse_pdev->dev) return -EINVAL;

    *ch_num = 0;
    *vpmap_num = 0;
    ptr = xse_pdev->dev->bar[0] + info->offset;
    for (ch_log = 3; ch_log < 6; ch_log++) {
        for (vpmap_log = 3; vpmap_log < 11; vpmap_log++) {
            unsigned int data;
            aggregate = vpmap_log + ch_log + 5 > 15;
            if (aggregate) {
                id_offset = (s_vpmap_entry << vpmap_log) + (s_ctrl_entry << ch_log) + 16;
            } else {
                id_offset = (s_vpmap_entry << (vpmap_log + ch_log)) + (s_ctrl_entry << ch_log) + 12;
            }
            data = ioread32(ptr + id_offset + 4);
            xse_log("stream_engine[%d] cfg: ch=%2d, vpmap=%4d id=%08x\n", xse_pdev->id, 1<<ch_log, 1<<vpmap_log, data);
            if (data == STREAM_ENGINE_ID) {
                data = ioread32(ptr + id_offset);
                if ((data & 0xff) == ch_log && ((data >> 16) & 0xff) == vpmap_log) {
                    *ch_num = 1 << (data & 0xff);
                    *vpmap_num = 1 << ((data >> 16) & 0xff);
                    break;
                }
            }
        }
        if (*ch_num || *vpmap_num) break;
    }
    return (*ch_num || *vpmap_num) ? 0 : -EINVAL;
}

static int xse_stream_engine_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info, int is_tx,
                                  int* stream_engine_ch_num, int* stream_engine_vpmap_num,
                                  struct xse_cdev* cdevs, char* name, int minor)
{
    int ch;
    int ret = 0;
    int ch_num = 0;
    int vpmap_num = 0;
    struct stream_engine_ext *ext_data = NULL;

    if (!info) return 0;

    ret = get_stream_engine_cfg(xse_pdev, info, &ch_num, &vpmap_num);
    if (ret) {
        release_device_info(info);
        return 0; // disable stream egine
    }

    ch_num = ch_num > CH_MAX ? CH_MAX : ch_num;
    *stream_engine_vpmap_num = vpmap_num;

    for (ch = 0; ch < ch_num; ch++) {
        struct stream_engine_list_element *elem = NULL;
        if (ch < 10) {
            name[17] = ch + 0x30;
            name[18] = '\0';
        } else {
            name[17] = (ch / 10) + 0x30;
            name[18] = (ch % 10) + 0x30;
        }
        ret = xse_cdev_init(xse_pdev, &cdevs[ch], name,
                            minor + ch, info, &s_stream_engine_ops, ch, ch==0 ? 1 : 0);
        if (ret) return ret;
        if (!ext_data) {
            ext_data = kmalloc(sizeof(*ext_data), GFP_KERNEL);
            mutex_init(&ext_data->mutex);
            ext_data->ch_num = ch_num;
            ext_data->vpmap_num = vpmap_num;
            ext_data->is_tx = is_tx;
        }
        cdevs[ch].ext_data = ext_data;

        elem = kmalloc(sizeof(*elem), GFP_KERNEL);
        if (elem) {
            elem->is_tx = is_tx;
            elem->dev_id = xse_pdev->id;
            elem->ch_id = ch;
            elem->cdev = &cdevs[ch];
            engine_list_add(elem);
        }
        *stream_engine_ch_num = ch+1;
    }
    return 0;
}

static void xse_stream_engine_exit(struct xse_pci_dev* xse_pdev, int ch_num, struct xse_cdev* cdevs)
{
    int ch;
    struct stream_engine_ext *ext_data = NULL;
    struct xse_device_info *info = NULL;

    for (ch = 0; ch<ch_num; ch++) {
        engine_list_del(&cdevs[ch]);
        xse_cdev_exit(&cdevs[ch]);
        if (cdevs[ch].module_info_owner) {
            ext_data = cdevs[ch].ext_data;
            info = cdevs[ch].module_info;
        }
    }
    if (info) kfree(info);
    if (ext_data) kfree(ext_data);
}

int xse_stream_engine_tx_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    char name[] = "stream_engine_tx_00";
    int is_tx = 1;
    return xse_stream_engine_init(xse_pdev, info, is_tx,
                                  &xse_pdev->stream_engine_tx_ch, &xse_pdev->stream_engine_tx_vpmap,
                                  &xse_pdev->stream_engine_tx[0], name, DEVICE_MINOR_STREAM_ENGINE_TX);
}

int xse_stream_engine_rx_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    char name[] = "stream_engine_rx_00";
    int is_tx = 0;
    return xse_stream_engine_init(xse_pdev, info, is_tx,
                                  &xse_pdev->stream_engine_rx_ch, &xse_pdev->stream_engine_rx_vpmap,
                                  &xse_pdev->stream_engine_rx[0], name, DEVICE_MINOR_STREAM_ENGINE_RX);
}

void xse_stream_engine_tx_exit(struct xse_pci_dev* xse_pdev)
{
    xse_stream_engine_exit(xse_pdev, xse_pdev->stream_engine_tx_ch, &xse_pdev->stream_engine_tx[0]);
}

void xse_stream_engine_rx_exit(struct xse_pci_dev* xse_pdev)
{
    xse_stream_engine_exit(xse_pdev, xse_pdev->stream_engine_rx_ch, &xse_pdev->stream_engine_rx[0]);
}
