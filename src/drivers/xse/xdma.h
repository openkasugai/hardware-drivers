/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef __XDMA_H__
#define __XDMA_H__

#include "xse_def.h"

extern int xse_xdma_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info);
extern void xse_xdma_exit(struct xse_pci_dev* xse_pdev);

#endif
