/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License with an explicit syscall exception, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note
*************************************************/

#ifndef __TOE_NETWORK_IOCTL_H__
#define __TOE_NETWORK_IOCTL_H__

#include <linux/ioctl.h>
#ifndef __KERNEL__
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

typedef struct toe_network_ioctl_config {
    uint32_t ip;
    uint32_t subnet_mask;
    uint32_t default_gateway;
} toe_network_ioctl_config_t;

#define XSE_TOE_NETWORK_MAGIC 'n'
#define	XSE_TOE_NETWORK_CONFIG _IOW(XSE_TOE_NETWORK_MAGIC, 40, toe_network_ioctl_config_t)

#endif // __TOE_NETWORK_IOCTL_H__
