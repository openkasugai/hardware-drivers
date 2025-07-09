/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License with an explicit syscall exception, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note
*************************************************/

#ifndef __STREAM_ENGINE_IOCTL_H__
#define __STREAM_ENGINE_IOCTL_H__

#include <linux/ioctl.h>
#ifndef __KERNEL__
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#endif

typedef struct stream_engine_info {
    uint32_t ch_num;
    uint32_t vpmap_num;
    int aggregate;
} stream_engine_info_t;

typedef struct stream_engine_vpinfo {
    uint32_t entry;
    uint64_t vaddr;
    uint64_t token;
    uint32_t range_id;
    uint32_t voffset;
    uint32_t psize;
} stream_engine_vp_info_t;

typedef struct stream_engine_cpinfo {
    uint32_t dev_id;
    uint32_t ch_id;
    uint64_t paddr;
} stream_engine_cp_info_t;

#define XSE_STREAM_ENGINE_MAGIC 's'
#define	XSE_STREAM_ENGINE_RESET_VPMAP _IOW(XSE_STREAM_ENGINE_MAGIC, 80, stream_engine_vp_info_t)
#define	XSE_STREAM_ENGINE_SET_VPMAP   _IOWR(XSE_STREAM_ENGINE_MAGIC, 81, stream_engine_vp_info_t)
#define	XSE_STREAM_ENGINE_WRITE_PADDR _IOWR(XSE_STREAM_ENGINE_MAGIC, 82, stream_engine_vp_info_t)
#define	XSE_STREAM_ENGINE_GET_MEM_MANAGE_RANGE _IOW(XSE_STREAM_ENGINE_MAGIC, 83, stream_engine_vp_info_t)
#define	XSE_STREAM_ENGINE_GET_CFG	  _IOR(XSE_STREAM_ENGINE_MAGIC, 84, stream_engine_info_t)
#define	XSE_STREAM_ENGINE_CHECK_CTRL_REGS _IO(XSE_STREAM_ENGINE_MAGIC, 85)
#define	XSE_STREAM_ENGINE_SET_DOORBELL_ADDR _IOWR(XSE_STREAM_ENGINE_MAGIC, 86, stream_engine_cp_info_t)
#define	XSE_STREAM_ENGINE_SET_QUEUE_INFO _IOWR(XSE_STREAM_ENGINE_MAGIC, 87, stream_engine_cp_info_t)
#define	XSE_STREAM_ENGINE_SET_CP_VPMAP _IOWR(XSE_STREAM_ENGINE_MAGIC, 88, stream_engine_cp_info_t)

#ifdef XSE_STREAM_ENGINE_TRACE_LOG
typedef struct xse_stream_engine_name_ {
    unsigned long cmd;
    char *name;
} xse_stream_engine_name_t;
static const xse_stream_engine_name_t g_stream_engine_cmd_table[] = {
    {XSE_STREAM_ENGINE_GET, 	  "XSE_STREAM_ENGINE_GET"},
    {XSE_STREAM_ENGINE_SET, 	  "XSE_STREAM_ENGINE_SET"},
    {XSE_STREAM_ENGINE_GET_VPCONV,"XSE_STREAM_ENGINE_GET_VPCONV"},
    {XSE_STREAM_ENGINE_SET_VPCONV,"XSE_STREAM_ENGINE_SET_VPCONV"},
    {XSE_STREAM_ENGINE_ALLOC,     "XSE_STREAM_ENGINE_ALLOC"},
    {XSE_STREAM_ENGINE_START,     "XSE_STREAM_ENGINE_START"},
    {XSE_STREAM_ENGINE_STOP,      "XSE_STREAM_ENGINE_STOP"},
    {XSE_STREAM_ENGINE_FREE,      "XSE_STREAM_ENGINE_FREE"},
    {XSE_STREAM_ENGINE_GET_MEM_MANAGE_RANGE,    "XSE_STREAM_ENGINE_GET_MEM_MANAGE_RANGE"},
    {XSE_STREAM_ENGINE_SET_MMPA,  "XSE_STREAM_ENGINE_SET_MMPA"},
    {(unsigned long)-1,         ""}
};
static inline char *xse_stream_engine_get_cmd_name(unsigned long cmd){
    int i = 0;
    while (g_stream_engine_cmd_table[i].cmd != (unsigned long)-1){
        if (g_stream_engine_cmd_table[i].cmd == cmd) break;
        i++;
    }
    return g_stream_engine_cmd_table[i].name;
}
#endif

#endif // __STREAM_ENGINE_IOCTL_H__
