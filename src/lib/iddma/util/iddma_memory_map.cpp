/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_memory_map.cpp
 * @brief Implementation of iddma_memory_map class utility.
 *
 */

#include <iddma_memory_map.hpp>

iddma_memory_map::iddma_memory_map() {}
iddma_memory_map::~iddma_memory_map() {}

iddma_status iddma_memory_map::add_map(void *key, void *ptr, size_t size, int type) {
    try {
        iddma_memory_map_data data;
        data.addr = ptr;
        data.size = size;
        data.type = type;
        memory_map_[key] = data;
        translated_addresses_.push_back(ptr);
        return KIDDMA_SUCCESS;
    } catch (...) {
        return KIDDMA_ERROR_UNKNOWN_EXCEPTION;
    }
}

void *iddma_memory_map::translate_addr(void *addr) const {
    if (memory_map_.size() == 0)
        return nullptr;
    auto it = memory_map_.upper_bound(addr);
    if (it == memory_map_.begin())
        return nullptr;
    it--;

    size_t cur = (size_t) addr;
    size_t base = (size_t) it->first;
    size_t conv_base = (size_t) it->second.addr;
    size_t size = it->second.size;
    size_t offset = cur - base;
    if (offset >= size)
        return nullptr;

    return (void *)(conv_base + offset);
}

int iddma_memory_map::get_mem_type(void *addr) const {
    if (memory_map_.size() == 0)
        return -1;
    auto it = memory_map_.upper_bound(addr);
    if (it == memory_map_.begin())
        return -1;
    it--;

    size_t cur = (size_t) addr;
    size_t base = (size_t) it->first;
    size_t size = it->second.size;
    size_t offset = cur - base;
    if (offset >= size)
        return -1;

    return it->second.type;
}

std::pair<void *, int> iddma_memory_map::get_translated_data(void *addr) const{
    if (memory_map_.size() == 0)
        return std::make_pair(nullptr, 0);
    auto it = memory_map_.upper_bound(addr);
    if (it == memory_map_.begin())
        return std::make_pair(nullptr, 0);
    it--;

    size_t cur = (size_t) addr;
    size_t base = (size_t) it->first;
    size_t conv_base = (size_t) it->second.addr;
    size_t size = it->second.size;
    size_t offset = cur - base;
    if (offset >= size)
        return std::make_pair(nullptr, 0);

    return std::make_pair((void *)(conv_base + offset), it->second.type);
}

void iddma_memory_map::clear(void* ptr) {
    auto it = memory_map_.find(ptr);
    if (it != memory_map_.end()) memory_map_.erase(it);
}

void iddma_memory_map::clear(void) {
    translated_addresses_.clear();
    memory_map_.clear();
}
