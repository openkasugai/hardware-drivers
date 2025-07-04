/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxse.h"
#include "device_info.h"
#include "toe.h"
#include "toe_network_ioctl.h"
#include "toe_ctrl_ioctl.h"
#include <linux/wait.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static DEFINE_MUTEX(s_network_mutex);
static DECLARE_WAIT_QUEUE_HEAD(s_network_wait_queue);

static const int s_toe_ctrl_cmd_pos = 0x0;
static const int s_toe_ctrl_cmd_status = 0x4;
static const int s_toe_ctrl_session_id = 0x8;
static const int s_toe_ctrl_frame_size = 0xc;
static const int s_toe_ctrl_credit_max = 0x10;
static const int s_toe_ctrl_credit_cur = 0x14;
static const int s_toe_ctrl_ip_pos = 0x18;
static const int s_toe_ctrl_ip_mask_pos = 0x1c;
static const int s_toe_ctrl_port_pos = 0x20;
static const int s_toe_ctrl_port_mask_pos = 0x24;
static const int s_toe_base = 0x100;
static const int s_global_end = 0x1000;

enum {
    kConnectMode_none = 0,
    kConnectMode_client = 1,
    kConnectMode_server = 2
} connect_mode;

enum {
    kEventId_open = 0,
    kEventId_listen = 1,
    kEventId_closed = 2,
    kEventId_accept = 3
} event_id;

struct toe_network_ext {
    struct task_struct *thread;
    int instance_id;
    int finalize;
    int is_listening;
    int listening_is_rx;
    int listening_ch;
    int ready;
    int io_ready;
};

struct toe_ctrl_ext {
    int instance_id;
    int is_rx;
    int ch_num;
    int connect_mode;
    int connected;
    int session_id;
    unsigned int self_port;
    unsigned int target_ip;
    unsigned int target_ip_mask;
    unsigned int target_port;
    unsigned int target_port_mask;
    int event_flag;
    int result;
};

typedef int (*update_function_t)(struct xse_cdev* cdev, unsigned int data[], int check_idx);

static int network_open_stat_update(struct xse_cdev* cdev, unsigned int data[], int check_idx)
{
    int i, j;
    unsigned int cur_ip;
    unsigned int cur_port;
    int session_id;
    struct toe_network_ext* ext_data = cdev->ext_data;
    struct xse_pci_dev* xse_pdev = cdev->xse_pdev;
    struct xse_cdev *ch_cdev = NULL;
    struct toe_ctrl_ext *ch_ext = NULL;

    cur_ip = (data[0] >> 24) | (data[1] << 8);
    cur_port = ((data[2] << 8) & 0xff00) | (data[1] >> 24);
    session_id = (data[0] & 0xffff);

    xse_log("toe_network[%d] open: %8x %8x %8x > ip:%8x, port:%4x: session:%4d\n",
            ext_data->instance_id,
            data[2], data[1], data[0], cur_ip, cur_port, session_id);
    for (i=0; i<xse_pdev->toe_ctrl_ch[ext_data->instance_id]; i++) {
        int tc_id = ext_data->instance_id * CH_MAX + i;
        for (j=0; j<2; j++) {
            ch_cdev = j==0 ? &xse_pdev->toe_ctrl_tx[tc_id] : &xse_pdev->toe_ctrl_rx[tc_id];
            ch_ext = ch_cdev->ext_data;
            if (!ch_cdev || !ch_ext) continue;

            if (ch_ext->connect_mode == kConnectMode_client && cur_port == ch_ext->target_port) {
                mutex_lock(&s_network_mutex);
                if (data[0] & 0x10000) {
                    ch_ext->result = session_id;
                } else {
                    ch_ext->result = -1;
                }
                ch_ext->event_flag |= 1 << kEventId_open;
                mutex_unlock(&s_network_mutex);
                xse_log("toe_network[%d] open[%s][%d]: ip:%8x, port:%4x\n",
                        ext_data->instance_id, j == 0 ? "tx" : "rx", i,
                        ch_ext->target_ip, ch_ext->target_port);
                return 0;
            }
        }
    }
    return -EINVAL;
}

static int network_listen_stat_update(struct xse_cdev* cdev, unsigned int data[], int check_idx)
{
    int tc_id;
    struct toe_network_ext* ext_data = cdev->ext_data;
    struct xse_pci_dev* xse_pdev = cdev->xse_pdev;
    struct xse_cdev *ch_cdev = NULL;
    struct toe_ctrl_ext *ch_ext = NULL;

    tc_id = ext_data->instance_id * CH_MAX + ext_data->listening_ch;
    ch_cdev = ext_data->listening_is_rx ? &xse_pdev->toe_ctrl_rx[tc_id] : &xse_pdev->toe_ctrl_tx[tc_id];
    ch_ext = ch_cdev->ext_data;

    mutex_lock(&s_network_mutex);
    ch_ext->result = data[0] & 1 ? 0 : -1;
    ch_ext->event_flag |= 1 << kEventId_listen;
    mutex_unlock(&s_network_mutex);

    return 0;
}

static int network_notify_update(struct xse_cdev* cdev, unsigned int data[], int check_idx)
{
    int i, j;
    unsigned int cur_ip;
    unsigned int cur_port;
    int session_id;
    int closed;
    int ret = -EINVAL;
    struct toe_network_ext* ext_data = cdev->ext_data;
    struct xse_pci_dev* xse_pdev = cdev->xse_pdev;
    struct xse_cdev *ch_cdev = NULL;
    struct toe_ctrl_ext *ch_ext = NULL;

    if (check_idx == 2) { // notify
        cur_ip = data[1];
        cur_port = data[2] & 0xffff;
        session_id = data[0] & 0xffff;
        closed = data[2] & 0x10000;
    } else { // ht update
        cur_ip = data[0];
        cur_port = ((data[1] & 0xff) << 8) | ((data[1] & 0xff00)>>8);
        session_id = data[2] & 0xffff;
        closed = 0;
    }
    xse_log("toe_network[%d] notify[%d]: %8x %8x %8x > ip:%8x, port:%4x: session:%4d, closed: %d\n",
            ext_data->instance_id, check_idx,
            data[2], data[1], data[0], cur_ip, cur_port, session_id, closed);
    for (i=0; i<xse_pdev->toe_ctrl_ch[ext_data->instance_id]; i++) {
        int tc_id = ext_data->instance_id * CH_MAX + i;
        for (j=0; j<2; j++) {
            ch_cdev = j==0 ? &xse_pdev->toe_ctrl_tx[tc_id] : &xse_pdev->toe_ctrl_rx[tc_id];
            ch_ext = ch_cdev->ext_data;
            if (!ch_cdev || !ch_ext) continue;

            mutex_lock(&s_network_mutex);
            if (ch_ext->connected && ch_ext->session_id == session_id && closed) {
                ch_ext->result = session_id;
                ch_ext->event_flag |= 1 << kEventId_closed;
                ret = 0;
            } else if (ch_ext->connect_mode == kConnectMode_server &&
                       (cur_ip & ch_ext->target_ip_mask) == (ch_ext->target_ip & ch_ext->target_ip_mask) &&
                       (cur_port & ch_ext->target_port_mask) == (ch_ext->target_port & ch_ext->target_port_mask)) {
                ch_ext->result = session_id;
                ch_ext->event_flag |= 1 << kEventId_accept;
                ret = 0;
            }
            mutex_unlock(&s_network_mutex);
            if (!ret) {
                xse_log("toe_network[%d] notify[%d][%s][%d]: ip:%8x, %8x, port:%4x, %4x: session:%4d, closed: %d, session: %d\n",
                        ext_data->instance_id, check_idx, j==0 ? "tx" : "rx", i,
                        ch_ext->target_ip, cur_ip, ch_ext->target_port, cur_port, session_id, closed, ch_ext->session_id);
                return ret;
            }
        }
    }
    return ret;
}

static int network_poll_func(void* data)
{
    static const int s_open_stat_base = s_toe_base + 0x10;
    static const int s_listen_stat_base = s_toe_base + 0x40;
    static const int s_notify_base = s_toe_base + 0x50;
    static const int s_ht_update_base = s_toe_base + 0x60;
    static const int s_bases[4] = {s_open_stat_base, s_listen_stat_base, s_notify_base, s_ht_update_base};
    static update_function_t update_functions[4] = {
        network_open_stat_update, network_listen_stat_update,
        network_notify_update, network_notify_update};
    struct xse_cdev* cdev = data;
    struct toe_network_ext* ext_data = cdev->ext_data;

    set_current_state(TASK_INTERRUPTIBLE);
    while (!(ext_data->finalize) && !kthread_should_stop()) {
        int i, j;
        void __iomem* ptr;
        unsigned int stat;
        unsigned int data[4];
        schedule_timeout(HZ / 100);
        for (i=0; i<4; i++) {
            ptr = cdev->base + s_bases[i];
            stat = ioread32(ptr);
            if ((stat != 0xffffffff) && (stat & 1)) {
                for (j=0; j<3; j++) data[j] = ioread32(ptr + j*4 + 4);
                iowrite32(1, ptr);
                if (!update_functions[i](cdev, data, i)) wake_up_all(&s_network_wait_queue);
                xse_log("toe_network[%d] worker poll_event[%x]: %x, %x %x %x\n", ext_data->instance_id,
                        s_bases[i], stat, data[0], data[1], data[2]);
            }
        }
    }
    xse_log("toe worker exit\n");
    return 0;
}

static int get_toe_ctrl_cfg(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    void __iomem* ptr;
    unsigned int data;

    ptr = xse_pdev->dev->bar[0] + info->offset;
    data = ioread32(ptr);
    return 1 << (int)(data & 0xff);
}

static void get_toe_ch_cfg(struct xse_cdev* cdev, struct toe_ctrl_ext* ext_data,
                           int* start_ofs, int* end_ofs)
{
    static const int s_global_size = 4096;
    static const int s_ch_size = 256;
    int start = 0, end = 0;

    if (ext_data) {
        int ch_num = ext_data->ch_num;

        start = s_global_size + s_ch_size * ((ext_data->is_rx ? ch_num : 0) + cdev->id);
        end = start + s_ch_size;
    }

    if (start_ofs) *start_ofs = start;
    if (end_ofs) *end_ofs = end;
}

static long toe_network_ioctl_config(struct file *file, unsigned long arg)
{
    void __iomem* ptr;
    int id;
    toe_network_ioctl_config_t info;
    struct xse_cdev* cdev = file->private_data;
    struct toe_network_ext* ext = cdev ? cdev->ext_data : NULL;

    if (!cdev || !ext) return -EINVAL;
    if (!ext->io_ready || ext->ready) return -EPERM;

    if (copy_from_user(&info, (void __user*)arg, sizeof(info))) return -EFAULT;

    id = ext->instance_id;
    ptr = cdev->base;
    iowrite32(info.ip, ptr);
    iowrite32(info.subnet_mask, ptr + 0x4);
    iowrite32(info.default_gateway, ptr + 0x8);
    xse_log("toe_network[%d]: ip %08x, mask %08x, gateway %08x\n",
            id, info.ip, info.subnet_mask, info.default_gateway);

    ext->ready = 1;
    return 0;
}

static long xse_toe_network_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case XSE_TOE_NETWORK_CONFIG:
        return toe_network_ioctl_config(file, arg);
    default:
        return -EINVAL;
    }
}

static const struct file_operations s_toe_network_ops = {
    .owner = THIS_MODULE,
    .open = xse_device_file_open,
    .release = xse_device_file_close,
    .llseek = xse_device_file_seek,
    .read = xse_device_read,
    .write = xse_device_write,
    .unlocked_ioctl = xse_toe_network_ioctl,
};

static int toe_network_connect(struct xse_cdev *cdev, int instance_id, unsigned int ip, unsigned int port,
                               int* session_id)
{
    static const int s_open_req_base = s_toe_base;
    void __iomem* ptr;
    int ret;
    int i;
    unsigned int data;
    struct toe_ctrl_ext* ext_data = cdev->ext_data;
    struct xse_pci_dev* xse_pdev = cdev ? cdev->xse_pdev : NULL;
    struct xse_cdev* network_cdev = NULL;
    struct toe_network_ext* network_ext = NULL;

    if (xse_pdev && instance_id < xse_pdev->toe_network_num) {
        network_cdev = &xse_pdev->toe_network[instance_id];
    }
    if (!network_cdev) return -EINVAL;
    network_ext = network_cdev->ext_data;
    if (!network_ext->ready) return -EPERM;

    ptr = network_cdev->base + s_open_req_base;

    mutex_lock(&s_network_mutex);
    // check previous connect
    data = ioread32(ptr);
    if (data) {
        mutex_unlock(&s_network_mutex);
        xse_err("toe_network[%d]: connect port busy: %x\n", instance_id, data);
        return -EPERM;
    }
    // set connect cmd
    iowrite32(ip, ptr + 4);
    iowrite32(port, ptr + 8);
    iowrite32(1, ptr);
    data = ioread32(ptr);
    mutex_unlock(&s_network_mutex);

    xse_log("toe_network[%d]: connect wait\n", instance_id);
    for (i=0; i<200; i++) {
        int ret = wait_event_interruptible_timeout(s_network_wait_queue, ext_data->event_flag & (1<<kEventId_open), 10*HZ);
        if (ret > 0) break;
    }

    mutex_lock(&s_network_mutex);
    if (ext_data->event_flag & (1<<kEventId_open)) {
        *session_id = ext_data->result;
        ext_data->event_flag &= ~(1<<kEventId_open);
        ret = ext_data->result < 0;
    } else {
        ret = -1;
    }
    mutex_unlock(&s_network_mutex);
    xse_log("toe_network[%d]: connect wait: %x, %d, %d\n", instance_id, ext_data->event_flag, ext_data->result, ret);

    return ret;
}

static int toe_network_listen(struct xse_cdev *cdev)
{
    static const int s_listen_req_base = s_toe_base + 0x30;
    void __iomem* ptr;
    int ret;
    unsigned int data;
    struct toe_ctrl_ext* ext_data = cdev->ext_data;
    struct xse_pci_dev* xse_pdev = cdev ? cdev->xse_pdev : NULL;
    struct xse_cdev* network_cdev = NULL;
    struct toe_network_ext* network_ext = NULL;

    if (xse_pdev && ext_data->instance_id < xse_pdev->toe_network_num) {
        network_cdev = &xse_pdev->toe_network[ext_data->instance_id];
    }
    if (!network_cdev) return -EINVAL;
    network_ext = network_cdev->ext_data;
    if (!network_ext->ready) return -EPERM;

    ptr = network_cdev->base + s_listen_req_base;

    mutex_lock(&s_network_mutex);
    // check previous connect
    data = ioread32(ptr);
    if (data || network_ext->is_listening) {
        mutex_unlock(&s_network_mutex);
        xse_err("toe_network[%d]: listen port busy: %x\n", ext_data->instance_id, data);
        return -EPERM;
    }
    // set connect cmd
    network_ext->is_listening = 1;
    network_ext->listening_is_rx = ext_data->is_rx;
    network_ext->listening_ch = cdev->id;
    iowrite32(ext_data->self_port, ptr + 4);
    iowrite32(1, ptr);
    data = ioread32(ptr);
    mutex_unlock(&s_network_mutex);

    xse_log("toe_network[%d]: listen wait: %d\n", ext_data->instance_id, ext_data->self_port);
    wait_event_timeout(s_network_wait_queue, ext_data->event_flag & (1<<kEventId_listen), HZ);

    xse_log("toe_network[%d]: listen wait: %x\n", ext_data->instance_id, ext_data->event_flag);
    mutex_lock(&s_network_mutex);
    if (ext_data->event_flag & (1<<kEventId_listen)) {
        network_ext->is_listening = 0;
        ext_data->event_flag &= ~(1<<kEventId_listen);
        ret = 0;
    } else {
        ret = -1;
    }
    mutex_unlock(&s_network_mutex);

    return ret;
}

static int toe_network_accept(struct xse_cdev *cdev, struct toe_ctrl_ext* ext_data, int* session_id)
{
    int i;
    int ret = -1;
    for (i=0; i<10; i++) { // 10msec
        int done = 0;
        mutex_lock(&s_network_mutex);
        if (ext_data->event_flag & (1<<kEventId_accept)) {
            ret = ext_data->result < 0;
            *session_id = ext_data->result;
            ext_data->event_flag &= ~(1<<kEventId_accept);
            done = 1;
        }
        mutex_unlock(&s_network_mutex);
        if (!done) {
            wait_event_timeout(s_network_wait_queue, ext_data->event_flag & (1<<kEventId_accept), HZ/10);
        }
    }
    return ret;
}

static int toe_network_disconnect(struct xse_cdev *cdev, int instance_id, int session_id)
{
    static const int s_close_req_base = s_toe_base + 0x20;
    void __iomem* ptr;
    int ret = 0;
    unsigned int data;
    struct xse_pci_dev* xse_pdev = cdev ? cdev->xse_pdev : NULL;
    struct xse_cdev* network_cdev = NULL;

    if (xse_pdev && instance_id < xse_pdev->toe_network_num) {
        network_cdev = &xse_pdev->toe_network[instance_id];
    }
    if (!network_cdev) return -EINVAL;

    ptr = network_cdev->base + s_close_req_base;

    mutex_lock(&s_network_mutex);
    // check previous connect
    data = ioread32(ptr);
    if (data) {
        mutex_unlock(&s_network_mutex);
        return -EPERM;
    }
    // set connect cmd
    iowrite32(session_id, ptr + 4);
    iowrite32(1, ptr);
    data = ioread32(ptr);
    mutex_unlock(&s_network_mutex);

    return ret;
}

static int toe_ctrl_check_ctrl_regs_and_stop(struct xse_cdev *cdev, int disable)
{
    void __iomem* ptr;
    unsigned int data;
    int start, end;
    int i;
    struct toe_ctrl_ext *ext_data = cdev ? cdev->ext_data : NULL;
    static const int s_reg_size = 40;

    if (!cdev || !ext_data) return -EINVAL;

    get_toe_ch_cfg(cdev, ext_data, &start, &end);

    ptr = cdev->base + start;
    for (i=0; i<s_reg_size; i+=4) {
        data = ioread32(ptr + i);
        xse_log("toe_ctrl_%s[%d][%d]: check[%x]: %8x (%s)\n",
                ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id, i, data,
                ((struct xse_device_info*)cdev->module_info)->name);
    }
    if (disable) {
        int is_connected = 0;
        int is_closed = 0;
        iowrite32(0, ptr + s_toe_ctrl_cmd_pos);
        ioread32(ptr + s_toe_ctrl_cmd_pos);
        ioread32(ptr + s_toe_ctrl_cmd_status);
        xse_log("toe_ctrl_%s[%d][%d]: disabled (%s)\n",
                ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id,
                ((struct xse_device_info*)cdev->module_info)->name);

        mutex_lock(&s_network_mutex);
        is_connected = ext_data->connected;
        ext_data->connected = 0;
        if (ext_data->event_flag & (1<<kEventId_closed)) {
            ext_data->event_flag &= ~(1<<kEventId_closed);
            is_closed = 1;
        }
        mutex_unlock(&s_network_mutex);
        if (is_connected) {
            toe_network_disconnect(cdev, ext_data->instance_id, ext_data->session_id);
        }
        ext_data->connect_mode = kConnectMode_none;
    }

    return 0;
}

static long toe_ctrl_ioctl_connect(struct file *file, unsigned long arg)
{
    toe_ctrl_ioctl_connection_t info;
    int ret;
    int start, end;
    int session_id;
    void __iomem* ptr;
    unsigned data = 0;
    unsigned ip_swap = 0;
    struct xse_cdev* cdev = file->private_data;
    struct toe_ctrl_ext* ext_data = cdev ? cdev->ext_data : NULL;

    if (!cdev || !ext_data) return -EINVAL;
    xse_log("toe_ctrl_%s[%d][%d]: connect %d %d\n",
            ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id,
            ext_data->connected, ext_data->connect_mode);
    if (ext_data->connected || ext_data->connect_mode) return -EPERM;

    get_toe_ch_cfg(cdev, ext_data, &start, &end);
    ptr = cdev->base + start;

    // error at not enabled
    data = ioread32(ptr);
    xse_log("toe_ctrl_%s[%d][%d]: connect busy[%x]:%x\n",
            ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id, start, data);
    if (data & 1) return -EPERM;

    if (copy_from_user(&info, (void __user*)arg, sizeof(info))) return -EFAULT;

    // set ip, port with mask
    ip_swap = ((info.target_ip & 0xff) << 24) | ((info.target_ip & 0xff00) << 8) |
        ((info.target_ip & 0xff0000) >> 8) | (info.target_ip >> 24);
    iowrite32(info.target_ip, ptr + s_toe_ctrl_ip_pos);
    iowrite32(0xffffffff, ptr + s_toe_ctrl_ip_mask_pos);
    iowrite32(info.target_port, ptr + s_toe_ctrl_port_pos);
    iowrite32(0xffffffff, ptr + s_toe_ctrl_port_mask_pos);
    iowrite32(1, ptr + s_toe_ctrl_cmd_pos);
    ext_data->connect_mode = kConnectMode_client;
    ext_data->target_ip = ip_swap;
    ext_data->target_port = info.target_port;
    // connect
    xse_log("toe_ctrl_%s[%d][%d]: connect by toe\n",
            ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id);
    ret = toe_network_connect(cdev, ext_data->instance_id, ip_swap, info.target_port, &session_id);
    xse_log("toe_ctrl_%s[%d][%d]: connect by toe: %d\n",
            ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id, ret);
    if (!ret) {
        ext_data->connected = 1;
        ext_data->session_id = session_id;
        info.session_id = session_id;
    } else {
        iowrite32(0, ptr + s_toe_ctrl_cmd_pos);
    }
    info.result = ret;
    ext_data->connect_mode = kConnectMode_none;

    if (copy_to_user((void __user*)arg, &info, sizeof(info))) return -EFAULT;
    return 0;
}

static long toe_ctrl_ioctl_listen(struct file *file, unsigned long arg)
{
    toe_ctrl_ioctl_connection_t info;
    int ret;
    int start, end;
    void __iomem* ptr;
    unsigned data;
    struct xse_cdev* cdev = file->private_data;
    struct toe_ctrl_ext* ext_data = cdev ? cdev->ext_data : NULL;

    if (!cdev || !ext_data) return -EINVAL;
    if (ext_data->connected || ext_data->connect_mode) return -EPERM;

    get_toe_ch_cfg(cdev, ext_data, &start, &end);
    ptr = cdev->base + start;

    // error at not enabled
    data = ioread32(ptr);
    if (data & 1) return -EPERM;

    if (copy_from_user(&info, (void __user*)arg, sizeof(info))) return -EFAULT;

    // set ip, port with mask
    iowrite32(info.target_ip, ptr + s_toe_ctrl_ip_pos);
    iowrite32(info.target_ip_mask, ptr + s_toe_ctrl_ip_mask_pos);
    iowrite32(info.target_port, ptr + s_toe_ctrl_port_pos);
    iowrite32(info.target_port_mask, ptr + s_toe_ctrl_port_mask_pos);
    iowrite32(1, ptr + s_toe_ctrl_cmd_pos);

    // listen
    ext_data->self_port = info.self_port;
    ext_data->target_ip = info.target_ip;
    ext_data->target_ip_mask = info.target_ip_mask;
    ext_data->target_port = info.target_port;
    ext_data->target_port_mask = info.target_port_mask;
    ext_data->connect_mode = kConnectMode_server;
    xse_log("toe_ctrl_%s[%d][%d]: listen %d, %8x %8x, %d %d",
            ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id, info.self_port,
            info.target_ip, info.target_ip_mask, info.target_port, info.target_port_mask);
    ret = toe_network_listen(cdev);
    info.result = ret;

    if (copy_to_user((void __user*)arg, &info, sizeof(info))) return -EFAULT;
    return 0;
}

static long toe_ctrl_ioctl_accept(struct file *file, unsigned long arg)
{
    toe_ctrl_ioctl_connection_t info;
    int ret;
    int start, end;
    int session_id;
    void __iomem* ptr;
    struct xse_cdev* cdev = file->private_data;
    struct toe_ctrl_ext* ext_data = cdev ? cdev->ext_data : NULL;

    if (!cdev || !ext_data) return -EINVAL;
    if (ext_data->connected || ext_data->connect_mode != kConnectMode_server) return -EPERM;

    get_toe_ch_cfg(cdev, ext_data, &start, &end);
    ptr = cdev->base + start;

    // accept
    ret = toe_network_accept(cdev, ext_data, &session_id);
    if (!ret) {
        ext_data->connected = 1;
        ext_data->session_id = session_id;
        info.session_id = session_id;
        ext_data->connect_mode = kConnectMode_none;
    }
    info.result = ret;

    if (copy_to_user((void __user*)arg, &info, sizeof(info))) return -EFAULT;
    return 0;
}

static long toe_ctrl_ioctl_disconnect(struct file *file, unsigned long arg)
{
    toe_ctrl_ioctl_connection_t info;
    int start, end;
    void __iomem* ptr;
    int is_closed = 0;
    int is_connected = 0;
    struct xse_cdev* cdev = file->private_data;
    struct toe_ctrl_ext* ext_data = cdev ? cdev->ext_data : NULL;

    if (!cdev || !ext_data) return -EINVAL;
    if (!ext_data->connected) return -EPERM;

    get_toe_ch_cfg(cdev, ext_data, &start, &end);
    ptr = cdev->base + start;

    if (copy_from_user(&info, (void __user*)arg, sizeof(info))) return -EFAULT;

    mutex_lock(&s_network_mutex);
    iowrite32(0, ptr + s_toe_ctrl_cmd_pos);
    ioread32(ptr + s_toe_ctrl_cmd_pos);
    ioread32(ptr + s_toe_ctrl_cmd_status);
    is_connected = ext_data->connected;
    ext_data->connected = 0;
    if (ext_data->event_flag & (1<<kEventId_closed)) {
        ext_data->event_flag &= ~(1<<kEventId_closed);
        is_closed = 1;
    }
    mutex_unlock(&s_network_mutex);

    if (is_connected) {
        return toe_network_disconnect(cdev, ext_data->instance_id, info.session_id);
    } else {
        return 0;
    }
}

static long toe_ctrl_ioctl_get_status(struct file *file, unsigned long arg)
{
    toe_ctrl_ioctl_status_t info;
    int start, end;
    void __iomem* ptr;
    struct xse_cdev* cdev = file->private_data;
    struct toe_ctrl_ext* ext_data = cdev ? cdev->ext_data : NULL;

    if (!cdev || !ext_data) return -EINVAL;
    if (!ext_data->connected) return -EPERM;

    get_toe_ch_cfg(cdev, ext_data, &start, &end);
    ptr = cdev->base + start;

    info.setting = ioread32(ptr + s_toe_ctrl_cmd_pos);
    info.status = ioread32(ptr + s_toe_ctrl_cmd_status);
    info.session_id = ioread32(ptr + s_toe_ctrl_session_id);
    info.frame_size = ioread32(ptr + s_toe_ctrl_frame_size);
    info.credit_max = ioread32(ptr + s_toe_ctrl_credit_max);
    info.credit_cur = ioread32(ptr + s_toe_ctrl_credit_cur);
    xse_log("toe_ctrl_%s[%d][%d]: status: set:%4x stat:%4x session:%4x size:%8x max:%4x cur:%4x\n",
            ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id,
            info.setting, info.status, info.session_id, info.frame_size, info.credit_max, info.credit_cur);

    if (copy_to_user((void __user*)arg, &info, sizeof(info))) return -EFAULT;
    return 0;
}

static long xse_toe_ctrl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case XSE_TOE_CTRL_CONNECT:
        return toe_ctrl_ioctl_connect(file, arg);
    case XSE_TOE_CTRL_LISTEN:
        return toe_ctrl_ioctl_listen(file, arg);
    case XSE_TOE_CTRL_ACCEPT:
        return toe_ctrl_ioctl_accept(file, arg);
    case XSE_TOE_CTRL_DISCONNECT:
        return toe_ctrl_ioctl_disconnect(file, arg);
    case XSE_TOE_CTRL_GET_STATUS:
        return toe_ctrl_ioctl_get_status(file, arg);
    default:
        return -EINVAL;
    }
    return 0;
}

static int xse_toe_ctrl_close(struct inode *inode, struct file *file)
{
    int ret;
    struct xse_cdev *cdev = (struct xse_cdev*)file->private_data;
    if (!cdev) return -EINVAL;

    ret = toe_ctrl_check_ctrl_regs_and_stop(cdev, 1);
    toe_ctrl_check_ctrl_regs_and_stop(cdev, 0);
    return ret;
}

static ssize_t xse_toe_ctrl_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    int i;
    int start, end;
    struct xse_cdev *cdev = file->private_data;
    struct toe_ctrl_ext *ext_data = cdev ? cdev->ext_data : NULL;
    int ofs = *pos;

    if (!cdev || !ext_data) return -EINVAL;
    if (count & 3) return -EINVAL;
    if (ofs & 3) return -EINVAL;

    get_toe_ch_cfg(cdev, ext_data, &start, &end);

    if (!(start <= ofs && ofs < end) && ofs >= s_global_end) return -EPROTO;

    for (i=0; i<count; i+=4) {
        int ret;
        void __iomem* ptr = cdev->base + ofs + i;
        unsigned int data = ioread32(ptr);
        xse_log("toe_ctrl_%s[%d][%x]: read[%x]: %x (%s)\n",
                ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id, ofs+i, data,
                ((struct xse_device_info*)cdev->module_info)->name);
        ret = copy_to_user(buf + i, &data, 4);
        if (ret) {
            xse_err("toe_ctrl_%s[%d][%x]: read: copy to user failed\n",
                    ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id);
        }
        *pos+=4;
    }
    return count;
}

static ssize_t xse_toe_ctrl_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{
    int i;
    int start, end;
    struct xse_cdev *cdev = file->private_data;
    struct toe_ctrl_ext *ext_data = cdev ? cdev->ext_data : NULL;
    int ofs = *pos;

    if (!cdev || !ext_data) return -EINVAL;
    if (count & 3) return -EINVAL;
    if (ofs & 3) return -EINVAL;

    get_toe_ch_cfg(cdev, ext_data, &start, &end);

    if (!(start <= ofs && ofs < end)) return -EPROTO;

    for (i=0; i<count; i+=4) {
        int ret;
        unsigned int data;
        void __iomem* ptr = cdev->base + ofs + i;

        ret = copy_from_user(&data, buf + i, 4);
        if (ret) {
            xse_err("toe_ctrl_%s[%d][%x]: read: copy from user failed\n",
                    ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id);
        } else {
            iowrite32(data, ptr);
            xse_log("toe_ctrl_%s[%d][%x]: write[%x]: %x (%s)\n",
                    ext_data->is_rx ? "rx" : "tx", cdev->xse_pdev->id, cdev->id, ofs+i, data,
                    ((struct xse_device_info*)cdev->module_info)->name);
        }
        *pos+=4;
    }
    return count;
}

static const struct file_operations s_toe_ctrl_ops = {
    .owner = THIS_MODULE,
    .open = xse_device_file_open,
    .release = xse_toe_ctrl_close,
    .llseek = xse_device_file_seek,
    .read = xse_toe_ctrl_read,
    .write = xse_toe_ctrl_write,
    .unlocked_ioctl = xse_toe_ctrl_ioctl,
};

int xse_toe_network_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    int ret = 0;
    int instance_id = 0;
    char toe_name[] = "toe_network_0";
    static const int s_toe_id_pos = 12;
    struct xse_cdev* toe_cdev = NULL;
    struct toe_network_ext *ext_data = NULL;

    if (!info) return 0;

    instance_id = xse_pdev->toe_network_num;
    if (instance_id == TOE_MAX) {
        release_device_info(info);
        return 0;
    }

    toe_name[s_toe_id_pos] = instance_id + 0x30;
    toe_cdev = &xse_pdev->toe_network[instance_id];
    ret = xse_cdev_init(xse_pdev, toe_cdev, toe_name,
                        DEVICE_MINOR_ETH + instance_id,
                        info, &s_toe_network_ops, 0, 1);
    if (ret) return ret;

    ext_data = kzalloc(sizeof(*ext_data), GFP_KERNEL);
    if (ext_data) {
        ext_data->instance_id = instance_id;
        toe_cdev->ext_data = ext_data;
    }
    xse_pdev->toe_network_num++;
    if (ext_data) {
        ext_data->thread = kthread_run(network_poll_func, toe_cdev, "network_poll_func");
    }
    return ext_data ? 0 : -ENOMEM;
}

void xse_toe_network_stop(struct xse_pci_dev* xse_pdev)
{
    int instance_id;
    int instance_num = xse_pdev->toe_ctrl_num;
    for (instance_id = 0; instance_id<instance_num; instance_id++) {
        struct xse_cdev* cdev = &xse_pdev->toe_network[instance_id];
        struct toe_network_ext* ext_data = cdev->ext_data;
        if (ext_data && ext_data->thread) {
            ext_data->finalize = 1;
            kthread_stop(ext_data->thread);
            ext_data->thread = NULL;
            xse_log("toe_network[%d]: stopped worker\n", ext_data->instance_id);
        }
    }
}

void xse_toe_network_exit(struct xse_pci_dev* xse_pdev)
{
    int instance_num;
    int instance_id;
    instance_num = xse_pdev->toe_ctrl_num;
    for (instance_id = 0; instance_id<instance_num; instance_id++) {
        struct xse_cdev* cdev = &xse_pdev->toe_network[instance_id];
        xse_cdev_exit(cdev);
        if (cdev->ext_data) {
            kfree(cdev->ext_data);
        }
        if (cdev->module_info_owner) {
            kfree(cdev->module_info);
        }
    }
}

int xse_toe_network_mac(struct xse_pci_dev* xse_pdev, int id, unsigned long long mac)
{
    void __iomem* ptr;
    struct xse_cdev* cdev = NULL;
    struct toe_network_ext* ext_data = NULL;
    static const int s_network_mac_fs = 0x10;

    if (id < 0 || id >= xse_pdev->toe_network_num) return -EINVAL;

    cdev = &xse_pdev->toe_network[id];
    ext_data = cdev->ext_data;
    ptr = cdev->base + s_network_mac_fs;
    iowrite32(mac, ptr);
    iowrite32(mac >> 32, ptr + 4);
    xse_log("toe_network[%d]: mac %llx\n", id, mac);
    ext_data->io_ready = 1;
    return 0;
}

int xse_toe_ctrl_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    int ch;
    int ret = 0;
    int instance_id = 0;
    int ch_num = 0;
    char rx_name[] = "toe_ctrl_0_rx_00";
    char tx_name[] = "toe_ctrl_0_tx_00";
    static const int s_ctrl_id_pos = 9;
    static const int s_trx_id_pos = 14;
    struct toe_ctrl_ext *ext_rdata = NULL;
    struct toe_ctrl_ext *ext_tdata = NULL;

    if (!info) return 0;

    instance_id = xse_pdev->toe_ctrl_num;
    if (instance_id == TOE_MAX) {
        release_device_info(info);
        return 0;
    }

    ch_num = get_toe_ctrl_cfg(xse_pdev, info);
    rx_name[s_ctrl_id_pos] = instance_id + 0x30;
    tx_name[s_ctrl_id_pos] = instance_id + 0x30;

    for (ch = 0; ch < ch_num; ch++) {
        int tc_id = ch + CH_MAX * instance_id;
        if (ch < 10) {
            rx_name[s_trx_id_pos] = ch + 0x30;
            rx_name[s_trx_id_pos + 1] = '\0';
            tx_name[s_trx_id_pos] = ch + 0x30;
            tx_name[s_trx_id_pos + 1] = '\0';
        } else {
            rx_name[s_trx_id_pos] = (ch / 10) + 0x30;
            rx_name[s_trx_id_pos + 1] = (ch % 10) + 0x30;
            tx_name[s_trx_id_pos] = (ch / 10) + 0x30;
            tx_name[s_trx_id_pos + 1] = (ch % 10) + 0x30;
        }
        ret = xse_cdev_init(xse_pdev, &xse_pdev->toe_ctrl_rx[tc_id], rx_name,
                            DEVICE_MINOR_TOE_CTRL_RX + tc_id,
                            info, &s_toe_ctrl_ops, ch, ch==0 ? 1 : 0);
        if (ret) return ret;

        ext_rdata = kzalloc(sizeof(*ext_rdata), GFP_KERNEL);
        ext_rdata->is_rx = 1;
        ext_rdata->instance_id = instance_id;
        ext_rdata->ch_num = ch_num;
        xse_pdev->toe_ctrl_rx[tc_id].ext_data = ext_rdata;

        ret = xse_cdev_init(xse_pdev, &xse_pdev->toe_ctrl_tx[tc_id], tx_name,
                            DEVICE_MINOR_TOE_CTRL_TX + tc_id,
                            info, &s_toe_ctrl_ops, ch, 0);
        if (ret) return ret;

        ext_tdata = kzalloc(sizeof(*ext_tdata), GFP_KERNEL);
        ext_tdata->is_rx = 0;
        ext_tdata->instance_id = instance_id;
        ext_tdata->ch_num = ch_num;
        xse_pdev->toe_ctrl_tx[tc_id].ext_data = ext_tdata;

        xse_pdev->toe_ctrl_ch[instance_id] = ch + 1;
    }
    xse_pdev->toe_ctrl_num++;

    return 0;
}

void xse_toe_ctrl_exit(struct xse_pci_dev* xse_pdev)
{
    int instance_num;
    int instance_id;
    int ch;
    struct xse_device_info *info = NULL;
    instance_num = xse_pdev->toe_ctrl_num;
    for (instance_id = 0; instance_id<instance_num; instance_id++) {
        int ch_num = xse_pdev->toe_ctrl_ch[instance_id];
        for (ch = 0; ch < ch_num; ch++) {
            int tc_id = ch + instance_id * CH_MAX;
            struct xse_cdev* cdev = &xse_pdev->toe_ctrl_rx[tc_id];
            xse_cdev_exit(cdev);
            if (cdev->module_info_owner) {
                info = cdev->module_info;
            }
            if (cdev->ext_data) {
                kfree(cdev->ext_data);
                cdev->ext_data = NULL;
            }
            cdev = &xse_pdev->toe_ctrl_tx[tc_id];
            xse_cdev_exit(cdev);
            if (cdev->module_info_owner) {
                info = cdev->module_info;
            }
            if (cdev->ext_data) {
                kfree(cdev->ext_data);
                cdev->ext_data = NULL;
            }
        }
        if (info) {
            kfree(info);
            info = NULL;
        }
    }
}

static const struct file_operations s_cmac_ops = {
    .owner = THIS_MODULE,
    .open = xse_device_file_open,
    .release = xse_device_file_close,
    .llseek = xse_device_file_seek,
    .read = xse_device_read,
    .write = xse_device_write,
};

int xse_cmac_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    int ret = 0;
    int instance_id = 0;
    char cmac_name[] = "cmac0";
    static const int s_cmac_id_pos = 4;
    struct xse_cdev* cmac_cdev = NULL;

    if (!info) return 0;

    instance_id = xse_pdev->cmac_num;
    if (instance_id == TOE_MAX) {
        release_device_info(info);
        return 0;
    }

    cmac_name[s_cmac_id_pos] = instance_id + 0x30;
    cmac_cdev = &xse_pdev->cmac[instance_id];
    ret = xse_cdev_init(xse_pdev, cmac_cdev, cmac_name,
                        DEVICE_MINOR_CMAC + instance_id,
                        info, &s_cmac_ops, 0, 1);
    if (ret) return ret;

    xse_pdev->cmac_num++;
    return 0;
}

void xse_cmac_exit(struct xse_pci_dev* xse_pdev)
{
    int instance_id;
    int instance_num = xse_pdev->cmac_num;
    for (instance_id = 0; instance_id<instance_num; instance_id++) {
        struct xse_cdev* cdev = &xse_pdev->cmac[instance_id];
        xse_cdev_exit(cdev);
        if (cdev->module_info_owner) {
            kfree(cdev->module_info);
        }
    }
}

int xse_cmac_start(struct xse_pci_dev* xse_pdev, int id)
{
    int i;
    int ok = 0;
    void __iomem* ptr;
    struct xse_cdev* cdev = NULL;
    static const int s_rx_cfg = 0x14;
    static const int s_tx_cfg = 0x0c;
    static const int s_tx_stat = 0x200;
    static const int s_rx_stat = 0x204;
    static const int s_rsfec = 0x107c;

    if (id < 0 || id >= xse_pdev->cmac_num) return -EINVAL;

    cdev = &xse_pdev->cmac[id];
    ptr = cdev->base;
    iowrite32(0x3, ptr + s_rsfec);
    iowrite32(0x1, ptr + s_rx_cfg);
    iowrite32(0x10, ptr + s_tx_cfg);
    msleep(10);
    for (i=0; i<100; i++) {
        unsigned int data = ioread32(ptr + s_rx_stat);
        if ((data & 2) == 2) {
            ok = 1;
            break;
        }
        msleep(100);
    }
    if (!ok) {
        xse_err("cmac[%d]: rx not aligned, rx stat: %x, tx stat: %x\n", id,
            ioread32(ptr + s_rx_stat), ioread32(ptr + s_tx_stat));
        iowrite32(0, ptr + s_rx_cfg);
        iowrite32(0, ptr + s_tx_cfg);
        return -EFAULT;
    }
    iowrite32(0x1, ptr + s_tx_cfg);
    msleep(100);
    xse_log("cmac[%d]: rx stat: %x, tx stat: %x\n", id,
            ioread32(ptr + s_rx_stat), ioread32(ptr + s_tx_stat));

    return 0;
}
