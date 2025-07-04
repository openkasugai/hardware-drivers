/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef __STREAM_ENGINE_EXT_H__
#define __STREAM_ENGINE_EXT_H__

typedef struct stream_engine_vpmap_ {
    uint64_t vaddr;
    uint64_t paddr; // unused
    uint32_t size;
    uint32_t valid;
    uint32_t entry;
} stream_engine_vpmap_t;

typedef struct stream_engine_paddr_ {
    uint64_t vaddr;
    uint64_t paddr;
    uint32_t offset;
} stream_engine_paddr_t;

#define STREAM_ENGINE_VPMAP_RESET _IOW('S', 11, stream_engine_vpmap_t)
#define STREAM_ENGINE_VPMAP_SET _IOW('S', 12, stream_engine_vpmap_t)
#define STREAM_ENGINE_PADDR_WRITE _IOW('S', 13, stream_engine_paddr_t)

#endif // __STREAM_ENGINE_EXT_H__
