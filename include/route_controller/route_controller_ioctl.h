/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License with an explicit syscall exception, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note
*************************************************/

#ifndef __ROUTE_CONTROLLER_IOCTL_H__
#define __ROUTE_CONTROLLER_IOCTL_H__

#include <linux/ioctl.h>
#ifndef __KERNEL__
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#endif
#include "../devmem/devmem_ioctl.h"

typedef struct route_controller_set_mem_info {
    uint64_t token;
    uint32_t mem_offset;
    uint32_t idx;
} route_controller_set_mem_info_t;

#define XSE_ROUTE_CONTROLLER_MAGIC 'r'
#define	XSE_ROUTE_CONTROLLER_ALLOC_DEVMEM _IOWR(XSE_ROUTE_CONTROLLER_MAGIC, 60, devmem_info_t)
#define	XSE_ROUTE_CONTROLLER_FREE_DEVMEM  _IOW(XSE_ROUTE_CONTROLLER_MAGIC, 61, devmem_info_t)
#define	XSE_ROUTE_CONTROLLER_SET_DEVMEM_ADDR  _IOW(XSE_ROUTE_CONTROLLER_MAGIC, 62, route_controller_set_mem_info_t)

#endif // __ROUTE_CONTROLLER_IOCTL_H__
