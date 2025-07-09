/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef _MEM_MANAGE_EXT_H__
#define _MEM_MANAGE_EXT_H__

enum {
  STATUS_SUCCESS = 0,
  STATUS_ERROR = -1,
  STATUS_FAILED = -2,
  STATUS_NOT_FOUND = -3
};

extern long mem_manage_get_phys_addr_num(uint64_t token, uint32_t* num);
extern long mem_manage_get_phys_addr(uint64_t token, uint32_t id, uint64_t* paddr, uint32_t* size);

extern long mem_manage_get_phys_addr_num_gpl(uint64_t token, uint32_t* num);
extern long mem_manage_get_phys_addr_gpl(uint64_t token, uint32_t id, uint64_t* paddr, uint32_t* size);

#endif // _MEM_MANAGE_EXT_H__
