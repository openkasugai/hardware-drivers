/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef __CMS_H__
#define __CMS_H__

#include "xse_def.h"

extern int xse_cms_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info);
extern void xse_cms_exit(struct xse_pci_dev* xse_pdev);

extern int xse_cms_get_network_mac(struct xse_pci_dev* xse_pdev, int id, unsigned long long* mac);

#endif
