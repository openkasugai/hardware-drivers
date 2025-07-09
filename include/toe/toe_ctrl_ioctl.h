/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License with an explicit syscall exception, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note
*************************************************/

#ifndef __TOE_CTRL_IOCTL_H__
#define __TOE_CTRL_IOCTL_H__

#include <linux/ioctl.h>
#ifndef __KERNEL__
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

typedef struct toe_ctrl_ioctl_connection {
    uint16_t self_port;
    uint32_t target_ip;
    uint32_t target_ip_mask;
    uint16_t target_port;
    uint16_t target_port_mask;
    uint32_t session_id;
    int result;
} toe_ctrl_ioctl_connection_t;

typedef struct toe_ctrl_ioctl_status {
    uint32_t setting;
    uint32_t status;
    uint16_t credit_max;
    uint16_t credit_cur;
    uint32_t frame_size;
    uint16_t session_id;
} toe_ctrl_ioctl_status_t;

#define XSE_TOE_CTRL_MAGIC 't'
#define	XSE_TOE_CTRL_CONNECT _IOWR(XSE_TOE_CTRL_MAGIC, 50, toe_ctrl_ioctl_connection_t)
#define	XSE_TOE_CTRL_LISTEN  _IOWR(XSE_TOE_CTRL_MAGIC, 51, toe_ctrl_ioctl_connection_t)
#define	XSE_TOE_CTRL_ACCEPT  _IOR(XSE_TOE_CTRL_MAGIC, 52, toe_ctrl_ioctl_connection_t)
#define	XSE_TOE_CTRL_DISCONNECT _IOW(XSE_TOE_CTRL_MAGIC, 53, toe_ctrl_ioctl_connection_t)
#define	XSE_TOE_CTRL_GET_STATUS _IOR(XSE_TOE_CTRL_MAGIC, 54, toe_ctrl_ioctl_status_t)

#endif // __TOE_CTRL_IOCTL_H__
