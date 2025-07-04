/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef __STREAM_ENGINE_H__
#define __STREAM_ENGINE_H__

#include "xse_def.h"

extern int xse_stream_engine_tx_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info);
extern int xse_stream_engine_rx_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info);
extern void xse_stream_engine_tx_exit(struct xse_pci_dev* xse_pdev);
extern void xse_stream_engine_rx_exit(struct xse_pci_dev* xse_pdev);

#endif
