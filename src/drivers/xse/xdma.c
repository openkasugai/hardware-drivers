/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxse.h"
#include "device_info.h"
#include "xdma.h"

static const struct file_operations s_xdma_ops = {
    .owner = THIS_MODULE,
    .open = xse_device_file_open,
    .release = xse_device_file_close,
    .llseek = xse_device_file_seek,
    .read = xse_device_read,
    .write = xse_device_write,
};

static int xdma_dma_enable(struct xse_pci_dev* xse_pdev)
{
    int i;
    void __iomem* ptr = xse_pdev->xdma.base;
    for (i=0; i<3; i++) {
        int offset = 0x0000 + 0x100 * i + 4; // h2c channel control
        iowrite32(1, ptr + offset);
        xse_log("xdma h2c[%d] run\n", i);
        offset = 0x1000 + 0x100 * i + 4; // c2h channel control
        iowrite32(1, ptr + offset);
        xse_log("xdma c2h[%d] run\n", i);
    }
    return 0;
}

static int xdma_dma_disable(struct xse_pci_dev* xse_pdev)
{
    int i;
    void __iomem* ptr = xse_pdev->xdma.base;
    for (i=0; i<3; i++) {
        unsigned int data;
        int offset = 0x0000 + 0x100 * i + 0x40; // h2c channel status
        data = ioread32(ptr + offset);
        xse_log("xdma h2c[%d] status: %08x\n", i, data);
        offset = 0x0000 + 0x100 * i + 4; // h2c channel control
        iowrite32(0, ptr + offset);
        xse_log("xdma h2c[%d] stop\n", i);
        offset = 0x1000 + 0x100 * i + 0x40; // c2h channel status
        data = ioread32(ptr + offset);
        xse_log("xdma c2h[%d] status: %08x\n", i, data);
        offset = 0x1000 + 0x100 * i + 4; // c2h channel control
        iowrite32(0, ptr + offset);
        xse_log("xdma c2h[%d] stop\n", i);
    }
    return 0;
}

int xse_xdma_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    int ret = 0;
    char name[] = "xdma";

    if (!info) return 0;
    if (xse_pdev->xdma_num) {
        release_device_info(info);
        return 0;
    }

    ret = xse_cdev_init(xse_pdev, &xse_pdev->xdma, name,
                        DEVICE_MINOR_XDMA + xse_pdev->xdma_num, info, &s_xdma_ops, 0, 1);
    if (ret) return ret;

    xse_pdev->xdma_num++;

    return xdma_dma_enable(xse_pdev);
}

void xse_xdma_exit(struct xse_pci_dev* xse_pdev)
{
    int xdma_num = xse_pdev->xdma_num;
    if (xdma_num) {
        struct xse_cdev* cdev = &xse_pdev->xdma;
        xdma_dma_disable(xse_pdev);
        xse_cdev_exit(cdev);
        if (cdev->module_info_owner) {
            struct xse_device_info *info = cdev->module_info;
            kfree(info);
        }
    }
}
