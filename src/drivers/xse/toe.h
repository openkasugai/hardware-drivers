/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef __TOE_H__
#define __TOE_H__

#include "xse_def.h"

extern int xse_toe_ctrl_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info);
extern void xse_toe_ctrl_exit(struct xse_pci_dev* xse_pdev);
extern int xse_toe_network_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info);
extern void xse_toe_network_stop(struct xse_pci_dev* xse_pdev);
extern void xse_toe_network_exit(struct xse_pci_dev* xse_pdev);
extern int xse_cmac_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info);
extern void xse_cmac_exit(struct xse_pci_dev* xse_pdev);

extern int xse_cmac_start(struct xse_pci_dev* xse_pdev, int id);
extern int xse_toe_network_mac(struct xse_pci_dev* xse_pdev, int id, unsigned long long mac);

#endif
