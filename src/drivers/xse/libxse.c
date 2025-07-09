/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/aer.h>

#include "libxse.h"
#include "device_info.h"
#include "devmem.h"
#include "function.h"
#include "stream_engine.h"
#include "route_controller.h"
#include "toe.h"
#include "cms.h"
#include "xdma.h"

struct class *g_xse_class = NULL;

static LIST_HEAD(s_dev_list);
static DEFINE_MUTEX(s_dev_mutex);

#ifndef list_last_entry
#define list_last_entry(ptr, type, member) list_entry((ptr)->prev, type, member)
#endif

static void dev_list_add(struct xse_dev* dev)
{
    mutex_lock(&s_dev_mutex);
    if (list_empty(&s_dev_list)) {
        dev->dev_id = 0;
    } else {
        struct xse_dev* last = list_last_entry(&s_dev_list, struct xse_dev, list_head);
        dev->dev_id = last->dev_id + 1;
    }
    list_add(&dev->list_head, &s_dev_list);
    mutex_unlock(&s_dev_mutex);
}

static void dev_list_del(struct xse_dev* dev)
{
    mutex_lock(&s_dev_mutex);
    list_del(&dev->list_head);
    mutex_unlock(&s_dev_mutex);
}

static int map_bars(struct xse_dev* dev)
{
    int i = 0;
    struct pci_dev* pdev = dev->pci_dev;
    for (i=0; i<6; i++) {
        resource_size_t bar_start;
        resource_size_t bar_size;
        resource_size_t map_size;
        bar_start = pci_resource_start(pdev, i);
        bar_size = pci_resource_len(pdev, i);
        map_size = bar_size;
        if (bar_size == 0) continue;
        if (bar_size <= MAP_BAR_MAX) {
            dev->bar[i] = pci_iomap(pdev, i, map_size);
            if (!dev->bar[i]) {
                return -EINVAL;
            }
        } else {
            dev->bar[i] = NULL;
        }
        dev->bar_phys_addr[i] = bar_start;
        dev->bar_size[i] = bar_size;
        xse_log("BAR[%d]: %llx %llx %lx\n", i, bar_start, bar_size, (uintptr_t)(dev->bar[i]));
    }
    return 0;
}

static void unmap_bars(struct xse_dev* dev)
{
    int i = 0;
    struct pci_dev* pdev = dev->pci_dev;
    for (i=0; i<6; i++) {
        if (dev->bar[i]) {
            pci_iounmap(pdev, dev->bar[i]);
            dev->bar[i] = NULL;
        }
    }
}

static int xse_device_init(struct xse_pci_dev* xse_pdev)
{
    int ret = 0;
    struct pci_dev* pdev = xse_pdev->pci_dev;

    struct xse_dev* dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev) return -ENOMEM;

    dev->pci_dev = pdev;

    ret = pci_enable_device(pdev);
    if (ret) {
        kfree(dev);
        return ret;
    }
    pcie_capability_set_word(pdev, PCI_EXP_DEVCTL, PCI_EXP_DEVCTL_EXT_TAG);
    pci_set_master(pdev);

    ret = pci_request_regions(pdev, DEVICE_NAME);
    if (ret) {
        dev->is_busy = 1;
    } else {
        dev->is_busy = 0;
    }
    // map bar
    ret = map_bars(dev);
    if (ret) {
        unmap_bars(dev);
        kfree(dev);
        return ret;
    }
    // dma mode
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0)
    if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(64))) {
        pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
        xse_log("dma is 64bit mode\n");
    } else {
        xse_err("dma is 32bit mode\n");
    }
#else
    if (!dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64))) {
        xse_log("dma is 64bit mode\n");
    } else {
        xse_err("dma is 32bit mode\n");
    }
#endif
    // add dev
    dev_list_add(dev);
    xse_pdev->dev = dev;
    xse_pdev->id = dev->dev_id;

    xse_log("xse dev[%d] initialized\n", xse_pdev->id);

    return 0;
}

static void xse_device_exit(struct xse_pci_dev* xse_pdev)
{
    struct pci_dev* pdev = xse_pdev->pci_dev;
    struct xse_dev* dev = xse_pdev->dev;

    if (dev) {
        dev_list_del(dev);
        unmap_bars(dev);
        if (dev->is_busy) {
            pci_release_regions(pdev);
        } else {
            pci_disable_device(pdev);
        }
        kfree(dev);
        xse_pdev->dev = NULL;
    }
}

static int xse_module_init(struct xse_pci_dev* xse_pdev)
{
    int ret = 0;
    int pos = 0;
    int max_module_num = MODULE_MAX;
    while (pos < max_module_num) {
        struct xse_device_info *info = get_device_info(xse_pdev, pos);
        if (!info) break;

        if (!strncmp(info->name, "stream_engine_tx", 16)) {
            ret = xse_stream_engine_tx_init(xse_pdev, info);
        } else if (!strncmp(info->name, "stream_engine_rx", 16)) {
            ret = xse_stream_engine_rx_init(xse_pdev, info);
        } else if (!strncmp(info->name, "route_controller", 16)) {
            ret = xse_route_controller_init(xse_pdev, info);
        } else if (!strncmp(info->name, "xdma", 4)) {
            ret = xse_xdma_init(xse_pdev, info);
        } else if (!strncmp(info->name, "ddr", 3)) {
            ret = xse_dev_mem_init(xse_pdev, info);
        } else if (!strncmp(info->name, "toe_ctrl", 8)) {
            ret = xse_toe_ctrl_init(xse_pdev, info);
        } else if (!strncmp(info->name, "toe_network", 8)) {
            ret = xse_toe_network_init(xse_pdev, info);
        } else if (!strncmp(info->name, "cms_subsystem", 13)) {
            ret = xse_cms_init(xse_pdev, info);
        } else if (!strncmp(info->name, "cmac_usplus", 11)) {
            ret = xse_cmac_init(xse_pdev, info);
        } else {
            ret = xse_function_init(xse_pdev, info);
        }
        if (ret) {
            release_device_info(info);
            return ret;
        }
        pos++;
    }
    if (xse_pdev->cms_num && xse_pdev->toe_network_num && xse_pdev->cmac_num) {
        int i;
        for (i=0; i<xse_pdev->cmac_num; i++) {
            unsigned long long mac;
            int ret = xse_cmac_start(xse_pdev, i);
            if (ret) continue;
            ret = xse_cms_get_network_mac(xse_pdev, i, &mac);
            if (!ret) {
                ret = xse_toe_network_mac(xse_pdev, i, mac);
            }
        }
    }

    return 0;
}

static void xse_module_exit(struct xse_pci_dev* xse_pdev)
{
    if (!xse_pdev) return;

    if (xse_pdev->cms_num) {
        xse_cms_exit(xse_pdev);
        xse_pdev->cms_num = 0;
    }
    if (xse_pdev->toe_network_num) {
        xse_toe_network_stop(xse_pdev);
        xse_toe_ctrl_exit(xse_pdev);
        xse_toe_network_exit(xse_pdev);
        xse_pdev->toe_network_num = 0;
    }
    if (xse_pdev->stream_engine_tx_ch) {
        xse_stream_engine_tx_exit(xse_pdev);
        xse_pdev->stream_engine_tx_ch = 0;
    }
    if (xse_pdev->stream_engine_rx_ch) {
        xse_stream_engine_rx_exit(xse_pdev);
        xse_pdev->stream_engine_rx_ch = 0;
    }
    if (xse_pdev->route_controller_num) {
        xse_route_controller_exit(xse_pdev);
        xse_pdev->route_controller_num = 0;
    }
    if (xse_pdev->function_num) {
        xse_function_exit(xse_pdev);
        xse_pdev->function_num = 0;
    }
    if (xse_pdev->cmac_num) {
        xse_cmac_exit(xse_pdev);
        xse_pdev->cmac_num = 0;
    }
    if (xse_pdev->xdma_num) {
        xse_xdma_exit(xse_pdev);
        xse_pdev->xdma_num = 0;
    }
    xse_dev_mem_exit(xse_pdev);
    if (xse_pdev && xse_pdev->major) {
        unregister_chrdev_region(MKDEV(xse_pdev->major, DEVICE_MINOR), DEVICE_MINOR_COUNT);
        xse_pdev->major = 0;
    }
}

static const struct pci_device_id pci_ids[] = {
    { PCI_DEVICE(0x10ee, 0x903f) },
    { 0 },
};
MODULE_DEVICE_TABLE(pci, pci_ids);

static pci_ers_result_t xse_error_detected(struct pci_dev *pdev, pci_channel_state_t error)
{
    struct xse_pci_dev *xse_pdev = dev_get_drvdata(&pdev->dev);

    switch (error) {
    case pci_channel_io_normal:
        return PCI_ERS_RESULT_CAN_RECOVER;
    case pci_channel_io_frozen:
        pr_warn("dev 0x%p,0x%p, frozen state error, reset controller\n",
                pdev, xse_pdev);
        pci_disable_device(pdev);
        return PCI_ERS_RESULT_NEED_RESET;
    case pci_channel_io_perm_failure:
        pr_warn("dev 0x%p,0x%p, failure state error, req. disconnect\n",
                pdev, xse_pdev);
        return PCI_ERS_RESULT_DISCONNECT;
    }
    return PCI_ERS_RESULT_NEED_RESET;
}

static pci_ers_result_t xse_slot_reset(struct pci_dev *pdev)
{
    struct xse_pci_dev *xse_pdev = dev_get_drvdata(&pdev->dev);

    xse_log("0x%p restart after slot reset\n", xse_pdev);
    if (pci_enable_device_mem(pdev)) {
        xse_log("0x%p failed to renable after slot reset\n", xse_pdev);
        return PCI_ERS_RESULT_DISCONNECT;
    }

    pci_set_master(pdev);
    pci_restore_state(pdev);
    pci_save_state(pdev);

    return PCI_ERS_RESULT_RECOVERED;
}

static void xse_error_resume(struct pci_dev *pdev)
{
    struct xse_pci_dev *xse_pdev = dev_get_drvdata(&pdev->dev);

    xse_log("error resume: dev 0x%p,0x%p.\n", pdev, xse_pdev);
#if KERNEL_VERSION(5, 7, 0) <= LINUX_VERSION_CODE
    pci_aer_clear_nonfatal_status(pdev);
#else
    pci_cleanup_aer_uncorrect_error_status(pdev);
#endif
}


#if KERNEL_VERSION(4, 13, 0) <= LINUX_VERSION_CODE
static void xse_reset_prepare(struct pci_dev *pdev)
{
    struct xse_pci_dev *xse_pdev = dev_get_drvdata(&pdev->dev);

    xse_log("reset prepare: dev 0x%p,0x%p.\n", pdev, xse_pdev);
}

static void xse_reset_done(struct pci_dev *pdev)
{
    struct xse_pci_dev *xse_pdev = dev_get_drvdata(&pdev->dev);

    xse_log("reset done: dev 0x%p,0x%p.\n", pdev, xse_pdev);
}
#elif KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
static void xse_reset_notify(struct pci_dev *pdev, bool prepare)
{
    struct xse_pci_dev *xse_pdev = dev_get_drvdata(&pdev->dev);
    if (prepare) {
        xse_log("reset prepare: dev 0x%p,0x%p.\n", pdev, xse_pdev);
    } else {
        xse_log("reset done: dev 0x%p,0x%p.\n", pdev, xse_pdev);
    }
}
#endif

static const struct pci_error_handlers xse_err_handler = {
    .error_detected = xse_error_detected,
    .slot_reset     = xse_slot_reset,
    .resume         = xse_error_resume,
#if KERNEL_VERSION(4, 13, 0) <= LINUX_VERSION_CODE
    .reset_prepare  = xse_reset_prepare,
    .reset_done     = xse_reset_done,
#elif KERNEL_VERSION(3, 16, 0) <= LINUX_VERSION_CODE
    .reset_notify   = xse_reset_notify,
#endif
};

static int xse_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    int ret;
    struct xse_pci_dev * xse_pdev = kmalloc(sizeof(*xse_pdev), GFP_KERNEL);
    if (!xse_pdev) return -ENOMEM;
    xse_log("xse_probe malloc\n");

    memset(xse_pdev, 0, sizeof(*xse_pdev));
    xse_pdev->pci_dev = dev;

    xse_log("xse_probe device_init\n");
    ret = xse_device_init(xse_pdev);
    if (ret) return ret;

    xse_log("xse_probe module_init\n");
    ret = xse_module_init(xse_pdev);
    if (ret) return ret;

    xse_log("xse_probe drvdata set\n");
    dev_set_drvdata(&dev->dev, xse_pdev);
    return 0;
}

static void xse_remove(struct pci_dev *dev)
{
    struct xse_pci_dev* xse_pdev = NULL;
    if (!dev) return;

    xse_log("xse_remove start\n");
    xse_pdev = dev_get_drvdata(&dev->dev);
    if (xse_pdev) {
        xse_log("xse_remove module_exit\n");
        xse_module_exit(xse_pdev);
        xse_log("xse_remove device_exit\n");
        xse_device_exit(xse_pdev);
        xse_log("xse_remove drvdata reset\n");
        kfree(xse_pdev);
        dev_set_drvdata(&dev->dev, NULL);
    }
    xse_log("xse_remove end\n");
}

static struct pci_driver pci_driver = {
    .name = DEVICE_NAME,
    .id_table = pci_ids,
    .probe = xse_probe,
    .remove = xse_remove,
    .err_handler = &xse_err_handler
};

static int xse_init(void)
{
    xse_log("xse_init start\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
    g_xse_class = class_create(THIS_MODULE, DEVICE_NAME);
#else
    g_xse_class = class_create(DEVICE_NAME);
#endif
    if (IS_ERR(g_xse_class)) {
        xse_err("failed to create class\n");
        return -EINVAL;
    }
    xse_log("xse_init register driver\n");
    return pci_register_driver(&pci_driver);
}

static void xse_exit(void)
{
    xse_log("xse_exit start\n");
    pci_unregister_driver(&pci_driver);
    if (g_xse_class) class_destroy(g_xse_class);
    xse_log("xse_exit end\n");
}

module_init(xse_init);
module_exit(xse_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("XDMA-based StreamEngine Device Driver");
