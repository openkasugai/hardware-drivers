/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxse.h"
#include "device_info.h"
#include "cms.h"
#include <linux/delay.h>

enum {
    kSensorId_CardSerial = 0x21,
    kSensorId_Mac0 = 0x22,
    kSensorId_Mac1 = 0x23,
    kSensorId_Mac2 = 0x24,
    kSensorId_Mac3 = 0x25,
    kSensorId_CardRev = 0x26,
    kSensorId_CardName = 0x27,
    kSensorId_SateliteVersion = 0x28,
    kSensorId_TotalPowerAvail = 0x29,
    kSensorId_FanPresence = 0x2a,
    kSensorId_ConfigMode = 0x2b,
    kSensorId_NewMac = 0x4b,
    kSensorId_CageType0 = 0x50,
    kSensorId_CageType1 = 0x51,
    kSensorId_CageType2 = 0x52,
    kSensorId_CageType3 = 0x53
} sensor_id_key;

enum {
    kRegReset = 0x20000,
    kRegMapId = 0x28000,
    kControl = 0x28018,
    kMBoxOffset = 0x28300,
    kMBoxError = 0x28304,
} cms_register;

static const unsigned int s_reg_map_id_magic = 0x74736574;

static const struct file_operations s_cms_ops = {
    .owner = THIS_MODULE,
    .open = xse_device_file_open,
    .release = xse_device_file_close,
    .llseek = xse_device_file_seek,
    .read = xse_device_read,
    .write = xse_device_write,
};

int xse_cms_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    int i;
    int ok = 0;
    int ret = 0;
    char name[MAX_NAME_SIZE] = "cms";
    void __iomem* ptr;
    unsigned int data = 0;

    if (!info) return 0;
    if (xse_pdev->cms_num) {
        release_device_info(info);
        return 0;
    }

    ret = xse_cdev_init(xse_pdev, &xse_pdev->cms, name,
                        DEVICE_MINOR_CMS, info, &s_cms_ops, 0, 1);
    if (ret) return ret;

    ptr = xse_pdev->cms.base;
    //iowrite32(0, ptr + kRegReset); // reset assert;
    //data = ioread32(ptr + kRegReset);
    //xse_log("cms: reset assert: %x\n", data);
    //msleep(10);
    iowrite32(1, ptr + kRegReset); // reset deassert;
    data = ioread32(ptr + kRegReset);
    xse_log("cms: reset deassert: %x\n", data);
    for (i=0; i<100; i++) {
        data = ioread32(ptr + kRegMapId);
        if (data == s_reg_map_id_magic) {
            ok = 1;
            break;
        }
        msleep(10);
    }
    if (!ok) {
        xse_err("cms: magic is not read: %x\n", data);
    }
    xse_pdev->cms_num++;

    return 0;
}

void xse_cms_exit(struct xse_pci_dev* xse_pdev)
{
    int cms_num;
    int id;
    cms_num = xse_pdev->cms_num;
    for (id = 0; id<cms_num; id++) {
        struct xse_cdev* cdev = &xse_pdev->cms;
        xse_cdev_exit(cdev);
        if (cdev->module_info_owner) {
            struct xse_device_info *info = cdev->module_info;
            kfree(info);
        }
    }
}

int xse_cms_get_network_mac(struct xse_pci_dev* xse_pdev, int id, unsigned long long* mac)
{
    int i, j;
    int ok = 0;
    int search_key = kSensorId_Mac0 + id;
    unsigned int mbox_ofs;
    unsigned int data;
    unsigned int len;
    unsigned int ofs;
    void __iomem* ptr;
    struct xse_cdev* cdev = &xse_pdev->cms;

    if (!xse_pdev->cms_num) return -EPERM;
    if (id >= 4) return -EINVAL;

    ptr = cdev->base;
    for (i=0; i<100; i++) { // wait control_reg[5] is 0
        unsigned int data = ioread32(ptr + kControl);
        if ((data & 0x20) == 0) {
            ok = 1;
            break;
        }
        msleep(10);
    }
    if (!ok) {
        xse_err("cms: control_reg[5] is not ready\n");
        return -EFAULT;
    }
    mbox_ofs = ioread32(ptr + kMBoxOffset);
    xse_log("cms: mbox offset: %x\n", mbox_ofs);
    mbox_ofs += kRegMapId;
    iowrite32(0x04000000, ptr + mbox_ofs); // read card info
    iowrite32(0x20, ptr + kControl); // send request
    for (i=0; i<100; i++) { // wait control_reg[5] is 0
        unsigned int data = ioread32(ptr + kControl);
        if ((data & 0x20) == 0) {
            ok = 1;
            break;
        }
        msleep(10);
    }
    if (!ok) {
        xse_err("cms: control_reg[5] is not deassert\n");
        return -EFAULT;
    }
    data = ioread32(ptr + kMBoxError);
    if (data) {
        xse_err("cms: mbox error: %x\n", data);
        return -EFAULT;
    }
    len = ioread32(ptr + mbox_ofs) & 0xfff;
    ofs = 4;
    xse_log("cms: card info len: %x\n", len);
    unsigned int remain_data = 0;
    int remain = 0;
    while (ofs < len) {
        unsigned char key, cur_len;
        data = ioread32(ptr + mbox_ofs + ofs);
        remain_data |= data << (remain*8);
        if (remain) {
            data >>= 32 - remain*8;
        } else {
            data = 0;
        }
        remain += 4;
        key = remain_data & 0xff;
        cur_len = (remain_data >> 8) & 0xff;
        ofs += 4;
        remain -= 2;
        remain_data = (remain_data >> 16) | (data << 16);
        data >>= 16;
        //xse_log("cms: ofs: %x, key: %x, len: %x, data: %8x, remain: %8x(%d), search_key: %x\n",
        //        ofs, key, cur_len, data, remain_data, remain, search_key);
        if (key == search_key) {
            unsigned long long cur_mac = 0;
            unsigned char d1;
            unsigned long long cur_digit = 0;
            int digit = 0;
            while (digit < 6) {
                if (remain < 3 && ofs < len) {
                    data = ioread32(ptr + mbox_ofs + ofs);
                    ofs += 4;
                    remain_data = remain_data | (data << (remain*8));
                    if (remain) data >>= 32 - remain*8;
                    else data = 0;
                    remain += 4;
                }
                //xse_log("cms: data: %8x %8x(%d)\n", data, remain_data, remain);
                cur_digit = 0;
                for (j=0; j<2; j++) {
                    d1 = remain_data & 0xff;
                    remain_data = (remain_data >> 8) | (data << 24);
                    remain--;
                    data >>= 8;
                    if (0x30 <= d1 && d1 < 0x3a) {
                        d1 -= 0x30;
                    } else if (0x41 <= d1 && d1 < 0x46) {
                        d1 -= 0x37;
                    } else {
                        d1 = 0;
                    }
                    cur_digit = (cur_digit << 4) | d1;
                }
                cur_digit <<= 8*digit;
                cur_mac |= cur_digit;
                xse_log("cms: mac[%d] %llx\n", id, cur_mac);
                digit++;
                remain_data = (remain_data >> 8) | (data << 24); // skip delimiter
                data >>= 8;
                remain--;
            }
            if (!remain && ofs < len) {
                remain_data = ioread32(ptr + mbox_ofs + ofs);
                ofs += 4;
                remain += 4;
            }
            // skip delimiter
            remain_data >>= 8;
            remain--;
            *mac = cur_mac;
            return 0;
        } else {
            ofs += cur_len - remain;
            if (cur_len > remain) {
                remain = 0;
                remain_data = 0;
            } else {
                remain -= cur_len;
                remain_data >>= cur_len*8;
            }
            if ((ofs & 3) && ofs < len) {
                data = ioread32(ptr + mbox_ofs + (ofs & ~3));
                remain = 4 - (ofs & 3);
                remain_data = data >> ((ofs & 3)*8);
            }
            ofs = (ofs + 3) & ~3;
        }
    }
    return -EFAULT;
}
