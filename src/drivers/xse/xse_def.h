/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef __XSE_DEF_H__
#define __XSE_DEF_H__

#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/pci.h>

#define DEVICE_INFO_OFFSET 0x10000
#define IO_MAX 4
#define ETH_MAX 3
#define CH_MAX 16
#define ROUTE_CONTROLLER_MAX 4
#define TOE_MAX ETH_MAX
#define FUNCTION_MAX 16
#define MAP_BAR_MAX 0x1000000
#define MAX_NAME_SIZE 128
#define MODULE_MAX 16

#define DEVICE_NAME "xse"
#define DEVICE_MINOR 0
#define DEVICE_MINOR_IO (DEVICE_MINOR)
#define DEVICE_MINOR_XDMA (DEVICE_MINOR_IO)
#define DEVICE_MINOR_ETH  (DEVICE_MINOR_XDMA + 1)
#define DEVICE_MINOR_STREAM_ENGINE_TX (DEVICE_MINOR_IO + IO_MAX)
#define DEVICE_MINOR_STREAM_ENGINE_RX ((DEVICE_MINOR_STREAM_ENGINE_TX) + CH_MAX)
#define DEVICE_MINOR_ROUTE_CONTROLLER_IN ((DEVICE_MINOR_STREAM_ENGINE_RX) + CH_MAX)
#define DEVICE_MINOR_ROUTE_CONTROLLER_OUT ((DEVICE_MINOR_ROUTE_CONTROLLER_IN) + ROUTE_CONTROLLER_MAX * CH_MAX)
#define DEVICE_MINOR_TOE_CTRL_RX ((DEVICE_MINOR_ROUTE_CONTROLLER_OUT) + ROUTE_CONTROLLER_MAX * CH_MAX)
#define DEVICE_MINOR_TOE_CTRL_TX ((DEVICE_MINOR_TOE_CTRL_RX) + TOE_MAX * CH_MAX)
#define DEVICE_MINOR_FUNCTION ((DEVICE_MINOR_TOE_CTRL_TX) + TOE_MAX * CH_MAX)
#define DEVICE_MINOR_CMS ((DEVICE_MINOR_FUNCTION) + FUNCTION_MAX)
#define DEVICE_MINOR_CMAC ((DEVICE_MINOR_CMS) + 1)
#define DEVICE_MINOR_COUNT ((DEVICE_MINOR_CMAC) + ETH_MAX)

struct xse_dev {
    struct list_head list_head;
    struct pci_dev *pci_dev;
    int dev_id;
    int is_busy;
    void __iomem *bar[6];
    uint64_t bar_phys_addr[6];
    uint64_t bar_size[6];
    int msi_enabled;
};

struct xse_cdev {
    struct xse_pci_dev *xse_pdev;
    dev_t cdev_no;
    struct cdev cdev;
    struct device *sys_dev;
    void __iomem *base;
    unsigned long size;
    int id;
    int module_info_owner;
    void* ext_data;
    void* module_info;
};

struct xse_pci_dev {
    struct pci_dev *pci_dev;
    struct xse_dev *dev;
    int major;
    int id;
    int stream_engine_tx_ch;
    int stream_engine_tx_vpmap;
    int stream_engine_rx_ch;
    int stream_engine_rx_vpmap;
    int route_controller_ch[ROUTE_CONTROLLER_MAX];
    int route_controller_num;
    int toe_ctrl_ch[TOE_MAX];
    int toe_ctrl_num;
    int toe_network_num;
    int function_num;
    int xdma_num;
    int cmac_num;
    int cms_num;

    struct xse_cdev stream_engine_tx[CH_MAX];
    struct xse_cdev stream_engine_rx[CH_MAX];
    struct xse_cdev toe_network[TOE_MAX];
    struct xse_cdev route_controller_in[ROUTE_CONTROLLER_MAX*CH_MAX];
    struct xse_cdev route_controller_out[ROUTE_CONTROLLER_MAX*CH_MAX];
    struct xse_cdev toe_ctrl_rx[TOE_MAX*CH_MAX];
    struct xse_cdev toe_ctrl_tx[TOE_MAX*CH_MAX];
    struct xse_cdev custom_functions[FUNCTION_MAX];
    struct xse_cdev cms;
    struct xse_cdev cmac[TOE_MAX];
    struct xse_cdev xdma;

    void* ext_data;
};

struct xse_device_info {
    char name[MAX_NAME_SIZE];
    unsigned long long offset;
    unsigned long long size;
};

#endif
