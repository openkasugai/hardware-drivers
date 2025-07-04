/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License with an explicit syscall exception, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note
*************************************************/

#ifndef __DEVMEM_IOCTL_H__
#define __DEVMEM_IOCTL_H__

#include <linux/ioctl.h>
#ifndef __KERNEL__
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

typedef struct devmem_info {
    uint64_t token;
    int mem_id;
    uint32_t size;
} devmem_info_t;

#endif // __DEVMEM_IOCTL_H__
