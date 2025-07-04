/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include "xse_def.h"

extern int xse_function_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info);
extern void xse_function_exit(struct xse_pci_dev* xse_pdev);

#endif
