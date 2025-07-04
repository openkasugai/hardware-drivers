/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_memory.hpp
 * @brief memory utility functions.
 *
 */

#ifndef __BUFFER_UTIL_H__

#define __BUFFER_UTIL_H__
#include <iddma_def.h>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <set>

#define KPAGE_SIZE_4KB 4096       // = 4KB (= 4 * 1024)
#define KPAGE_SIZE_2MB 2097152    // = 2MB (= 2 * 1024 * 1024)
#define KPAGE_SIZE_1GB 1073741824 // = 1GB (= 1 * 1024 * 1024)

class iddma_allocator;

class iddma_memory final {
private:
    iddma_memory();
    ~iddma_memory();

    std::mutex mutex_;
    std::map<void *, iddma_memory_type> allocated_buffers_;
    std::map<iddma_memory_type, std::unique_ptr<iddma_allocator> > allocators_;

public:
    iddma_memory(const iddma_memory&) = delete;
    iddma_memory& operator=(const iddma_memory&) = delete;
    iddma_memory(iddma_memory&&) = delete;
    iddma_memory& operator=(iddma_memory&&) = delete;

    static iddma_memory *get_instance() {
        // this initialization is thread safe from c++11.
        // destruction is done after all threads finish.
        static iddma_memory instance;
        return &instance;
    }

    void *allocate_top(size_t size, iddma_memory_type type, size_t align = 64);
    void free_top(void *addr);
};

class iddma_allocator {
public:
    iddma_allocator() = default;
    virtual ~iddma_allocator() = default;

protected:
    std::mutex mutex_;
    std::map<void *, size_t> region_info_;
    std::map<void *, size_t> alloc_info_;
    std::map<void *, size_t> free_info_;
    std::multimap<size_t, void *> candidates_;

    virtual void *allocate_region(size_t &aligned_size, size_t align) = 0;
    virtual void free_region(void *addr, size_t size) = 0;
    void register_candidate(void *addr, size_t size);
    void unregister_candidate(void *addr, size_t size);

private:
    void *allocate_from_regions(size_t aligned_size, size_t align = 64);

public:
    void *allocate_buffer(size_t size, size_t align = 64);
    void free_buffer(void *base_addr);
};

class iddma_cpu_4kb_mem_allocator : public iddma_allocator {
public:
    ~iddma_cpu_4kb_mem_allocator();
private:
    void *allocate_region(size_t &aligned_size, size_t align);
    void free_region(void *addr, size_t size);
};

class iddma_cpu_shmem_allocator : public iddma_allocator {
public:
    iddma_cpu_shmem_allocator(iddma_memory_type type)
        : type_(type) {}
    ~iddma_cpu_shmem_allocator();

private:
    void *allocate_region(size_t &aligned_size, size_t align);
    void free_region(void *addr, size_t size);
    iddma_memory_type type_;
    std::map<void *, int> ids_;
};

class iddma_cpu_mmap_allocator : public iddma_allocator {
public:
    iddma_cpu_mmap_allocator(iddma_memory_type type)
        : type_(type) {}
    ~iddma_cpu_mmap_allocator();

private:
    void *allocate_region(size_t &aligned_size, size_t align);
    void free_region(void *addr, size_t size);
    iddma_memory_type type_;
};

#endif