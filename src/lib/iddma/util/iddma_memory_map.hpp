/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef __INTERVAL_TREE_HPP__
#define __INTERVAL_TREE_HPP__

#include <utility>
#include <vector>
#include <map>
#include <stdint.h>
/**
 * @file iddma_memory_map.hpp
 * @author your name (you@domain.com)
 * @brief
 * Memory map utility for iddma.
 * This implementation just used for checking if the address is registered and getting an offset from the head address.
 */

#include <stdlib.h>
#include <iddma_def.h>

class iddma_memory_map {
public:
    struct iddma_memory_map_data {
        void *addr;
        size_t size;
        int type;
        iddma_memory_map_data(void)
            : addr(0),
              size(0),
              type(0) {};
    };
    iddma_memory_map();
    ~iddma_memory_map();

    iddma_status add_map(void *key, void *ptr, size_t size, int type = 0);
    // iddma_status add_map(void *key, void *ptr, size_t size);
    void *translate_addr(void *addr) const;
    int get_mem_type(void *addr) const;
    std::pair<void *, int> get_translated_data(void *addr) const;
    void clear(void* ptr);
    void clear(void);
private:
    std::map<void*, iddma_memory_map_data> memory_map_;
    // std::map<void*, std::pair<void*, size_t> > memory_map_;
    std::vector<void*> translated_addresses_;
};

#endif  // __INTERVAL_TREE_HPP__
