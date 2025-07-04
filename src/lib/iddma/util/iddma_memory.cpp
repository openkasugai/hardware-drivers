/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file iddma_memory.cpp
 * @brief implement of memory utility functions.
 *
 */

#include <iddma_memory.hpp>
#include <sys/shm.h>
#include <sys/mman.h>
#include <linux/mman.h>
#include <cstring>
#include <cstdlib>
// these constants are defined in linux/shm.h but shm_info redefinition error occurs if including it, so define necessary constants here
#define SHM_HUGE_SHIFT    (26)
#define SHM_HUGE_2MB      (21 << SHM_HUGE_SHIFT)
#define SHM_HUGE_1GB      (30 << SHM_HUGE_SHIFT)

#define SHMEM_COMMON_FLAG (IPC_CREAT | 0666)
#define MMAP_COMMON_FLAG  (MAP_SHARED | MAP_ANONYMOUS)
#define ALIGN_64B         (64)

iddma_memory::iddma_memory() {
#ifdef _DEBUG
    printf("iddma_memory: constructor\n");
#endif
    allocators_[KIDDMA_MEMORY_TYPE_CPU] = std::make_unique<iddma_cpu_4kb_mem_allocator>();
    allocators_[KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_2MB] = std::make_unique<iddma_cpu_shmem_allocator>(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_2MB);
    allocators_[KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_1GB] = std::make_unique<iddma_cpu_shmem_allocator>(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_1GB);
    allocators_[KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_2MB] = std::make_unique<iddma_cpu_mmap_allocator>(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_2MB);
    allocators_[KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_1GB] = std::make_unique<iddma_cpu_mmap_allocator>(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_1GB);
}

iddma_memory::~iddma_memory() {
#ifdef _DEBUG
    printf("iddma_memory: destructor\n");
#endif
    auto itr = allocators_.begin();
    while ( itr != allocators_.end()) {
        itr = allocators_.erase(itr);
    }
}

std::vector<iddma_memory_type> get_type_candidates(iddma_memory_type type) {
    std::vector<iddma_memory_type> candidates;

    switch (type)
    {
    case KIDDMA_MEMORY_TYPE_CPU:
        candidates.push_back(KIDDMA_MEMORY_TYPE_CPU);
        break;
    case KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM:
    case KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_1GB:
        candidates.push_back(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_1GB);
        candidates.push_back(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_2MB);
        break;
    case KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_2MB:
        candidates.push_back(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_2MB);
        candidates.push_back(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_1GB);
        break;
    case KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP:
    case KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_1GB:
        candidates.push_back(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_1GB);
        candidates.push_back(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_2MB);
        break;
    case KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_2MB:
        candidates.push_back(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_2MB);
        candidates.push_back(KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_1GB);
        break;
    default:
        break;
    }
    return candidates;
}

void *iddma_memory::allocate_top(size_t size, iddma_memory_type type, size_t align) {
    std::lock_guard<std::mutex> lock(mutex_);
    void *ptr = nullptr;
    if (align < 64 || (align & (align-1)) != 0) {
        return nullptr;
    }
    size = (size + align - 1) & ~(align - 1);

    std::vector<iddma_memory_type> type_candidates = get_type_candidates(type);

    if (type_candidates.size() == 0) {
#ifdef _DEBUG
        printf("invalid or not supported type is specified\n");
#endif
        return nullptr;
    }

    for (auto itr = type_candidates.begin(); itr != type_candidates.end(); itr++) {
        ptr = allocators_[*itr]->allocate_buffer(size, align);
        if (ptr != nullptr) {
            allocated_buffers_[ptr] = *itr;
            break;
        }
    }

    return ptr;
}

void iddma_memory::free_top(void *addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (addr == nullptr) {
        return;
    }
    auto itr = allocated_buffers_.find(addr);
    if (itr == allocated_buffers_.end()) {
        return;
    }

    iddma_memory_type type = itr->second;
    allocators_[type]->free_buffer(addr);
    allocated_buffers_.erase(itr);
}

/* iddma_allocator base */
void iddma_allocator::register_candidate(void *addr, size_t size) {
    candidates_.emplace(size, addr);
}

void iddma_allocator::unregister_candidate(void *addr, size_t size) {
    auto itr_pair = candidates_.equal_range(size);
    for (auto itr = itr_pair.first; itr != itr_pair.second; itr++) {
        if (itr->second == addr) {
            candidates_.erase(itr);
            break;
        }
    }
}

void *iddma_allocator::allocate_from_regions(size_t aligned_size, size_t align) {
    void *ptr = nullptr;
    auto candidate_itr = candidates_.lower_bound(aligned_size);
    while (candidate_itr != candidates_.end()) {
        uint64_t candidate_addr = reinterpret_cast<uint64_t>(candidate_itr->second);
        auto free_itr = free_info_.find(reinterpret_cast<void *>(candidate_addr));
        size_t free_size = free_itr->second;
        uint64_t next_aligned_addr = (candidate_addr + free_size - aligned_size) & ~(align - 1);
        if (next_aligned_addr >= candidate_addr) {
            unregister_candidate(reinterpret_cast<void *>(candidate_addr), free_size);
            size_t next_free_size = next_aligned_addr - candidate_addr;
            uint64_t back_free_addr = next_aligned_addr + aligned_size;
            size_t back_free_size = free_size - next_free_size - aligned_size;
            alloc_info_.emplace(reinterpret_cast<void *>(next_aligned_addr), aligned_size);
            if (next_free_size == 0) {
                free_info_.erase(free_itr);
            } else {
                free_itr->second = next_free_size;
                register_candidate(reinterpret_cast<void *>(candidate_addr), next_free_size);
            }
            if (back_free_size > 0) {
                free_info_.emplace(reinterpret_cast<void *>(back_free_addr), back_free_size);
                register_candidate(reinterpret_cast<void *>(back_free_addr), back_free_size);
            }
            ptr = reinterpret_cast<void *>(next_aligned_addr);
            break;
        }
        candidate_itr++;
    }
    return ptr;
}

void *iddma_allocator::allocate_buffer(size_t size, size_t align) {
    std::lock_guard<std::mutex> lock(mutex_);
    void *ptr = nullptr;

    if (align < 64 || (align & (align-1)) != 0) {
        return nullptr;
    }
    size = (size + align - 1) & ~(align - 1);

    ptr = allocate_from_regions(size, align);
    if (ptr != nullptr) {
        return ptr;
    }

    size_t region_size = size;
    void *region_ptr = allocate_region(region_size, align);
    if (region_ptr == nullptr) {
        return nullptr;
    } else {
        region_info_[region_ptr] = region_size;
        free_info_[region_ptr] = region_size;
        register_candidate(region_ptr, region_size);
    }

    ptr = allocate_from_regions(size, align);

    if (ptr == nullptr) {
        printf("allocate_top: invalid allocation occurred\n");
        free_region(region_ptr, region_size);
        free_info_.erase(region_ptr);
        unregister_candidate(region_ptr, region_size);
    }

    return ptr;
}

void iddma_allocator::free_buffer(void *base_addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (base_addr == nullptr) {
        return;
    }
    auto base_buf_itr = alloc_info_.find(base_addr);
    if (base_buf_itr == alloc_info_.end()) {
        return;
    }

    size_t base_buf_size = base_buf_itr->second;
    alloc_info_.erase(base_buf_itr);

    auto upper_buf_itr = free_info_.upper_bound(base_addr);
    void *upper_addr = (upper_buf_itr != free_info_.end())?
                        upper_buf_itr->first : nullptr;
    size_t upper_buf_size = (upper_buf_itr != free_info_.end())?
                        upper_buf_itr->second : 0;

    auto lower_buf_itr = upper_buf_itr;
    if (lower_buf_itr != free_info_.begin()) {
        lower_buf_itr--;
    } else {
        lower_buf_itr = free_info_.end();
    }
    void *lower_addr = (lower_buf_itr != free_info_.end())?
                        lower_buf_itr->first : nullptr;
    size_t lower_buf_size = (lower_buf_itr != free_info_.end())?
                        lower_buf_itr->second : 0;

    bool is_base_boundary = (region_info_.find(base_addr) != region_info_.end());
    bool is_upper_boundary = (region_info_.find(upper_addr) != region_info_.end());

    bool do_lower_concat = (!is_base_boundary &&
            reinterpret_cast<uint64_t>(lower_addr) + lower_buf_size == reinterpret_cast<uint64_t>(base_addr));
    bool do_upper_concat = (!is_upper_boundary &&
            reinterpret_cast<uint64_t>(base_addr) + base_buf_size == reinterpret_cast<uint64_t>(upper_addr));

    if (do_lower_concat) {
        unregister_candidate(lower_addr, lower_buf_size);
        base_addr = lower_addr;
        base_buf_size += lower_buf_size;
    }
    if (do_upper_concat) {
        unregister_candidate(upper_addr, upper_buf_size);
        free_info_.erase(upper_buf_itr);
        base_buf_size += upper_buf_size;
    }

    auto region_itr = region_info_.find(base_addr);
    if (region_itr != region_info_.end() &&
        region_itr->second == base_buf_size) {
        if (do_lower_concat) {
            free_info_.erase(lower_buf_itr);
        }
        free_region(region_itr->first, region_itr->second);
        region_info_.erase(region_itr);
        return;
    }

    if (do_lower_concat) {
        lower_buf_itr->second = base_buf_size;
    } else {
        free_info_[base_addr] = base_buf_size;
    }
    register_candidate(base_addr, base_buf_size);
}

/* iddma_allocator with 4kb page */

iddma_cpu_4kb_mem_allocator::~iddma_cpu_4kb_mem_allocator() {
#ifdef _DEBUG
    printf("iddma_cpu_4kb_mem_allocator: destructor\n");
#endif
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto itr = region_info_.begin(); itr != region_info_.end(); itr++) {
        free_region(itr->first, itr->second);
    }
}

void *iddma_cpu_4kb_mem_allocator::allocate_region(size_t &aligned_size, size_t align) {
#ifdef _DEBUG
    printf("iddma_cpu_4kb_mem_allocator: alignment: 0x%lx, size: 0x%lx b aligned_alloc...\n", align, aligned_size);
#endif
    void *ptr = aligned_alloc(align, aligned_size);
    if (ptr == nullptr) {
        return nullptr;
    }
    return ptr;
}

void iddma_cpu_4kb_mem_allocator::free_region(void *addr, size_t size) {
#ifdef _DEBUG
    printf("iddma_cpu_4kb_mem_allocator: free_region... %p\n", addr);
#endif
    free(addr);
}

/* iddma_allocator shmem */
iddma_cpu_shmem_allocator::~iddma_cpu_shmem_allocator() {
#ifdef _DEBUG
    printf("iddma_cpu_shmem_allocator: destructor (type: %d)\n", type_);
#endif
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto itr = region_info_.begin(); itr != region_info_.end(); itr++) {
        free_region(itr->first, itr->second);
    }
}

void *iddma_cpu_shmem_allocator::allocate_region(size_t &aligned_size, size_t align) {
#ifdef _DEBUG
    printf("iddma_cpu_shmem_allocator: alignment: 0x%lx, size: 0x%lx b aligned_alloc...\n", align, aligned_size);
#endif
    void *ptr = nullptr;
    int id = -1;
    int flags = 0;
    switch (type_) {
    case KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_2MB:
        aligned_size = (aligned_size + KPAGE_SIZE_2MB - 1) & ~(KPAGE_SIZE_2MB - 1);
        flags = SHMEM_COMMON_FLAG | SHM_HUGETLB | SHM_HUGE_2MB;
        break;
    case KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_SHMEM_1GB:
    default:
        aligned_size = (aligned_size + KPAGE_SIZE_1GB - 1) & ~(KPAGE_SIZE_1GB - 1);
        flags = SHMEM_COMMON_FLAG | SHM_HUGETLB | SHM_HUGE_1GB;
        break;
    }

    id = shmget(IPC_PRIVATE, aligned_size, flags);
    if (id == -1) {
#ifdef _DEBUG
        printf("iddma_cpu_shmem_allocator: failed shmget\n");
#endif
        return nullptr;
    }
    ptr = shmat(id, 0, 0);
    if (ptr == reinterpret_cast<void *>(-1)) {
#ifdef _DEBUG
        printf("iddma_cpu_shmem_allocator: failed shmat\n");
#endif
        shmctl(id, IPC_RMID, 0);
        return nullptr;
    }

    ids_[ptr] = id;
    return ptr;
}

void iddma_cpu_shmem_allocator::free_region(void *addr, size_t size) {
#ifdef _DEBUG
    printf("iddma_cpu_shmem_allocator: free_region...\n");
#endif
    auto id_itr = ids_.find(addr);
    if (id_itr == ids_.end()) {
        return;
    }

    shmdt(addr);
    shmctl(id_itr->second, IPC_RMID, 0);

    ids_.erase(id_itr);
}

/* iddma_allocator mmap */
iddma_cpu_mmap_allocator::~iddma_cpu_mmap_allocator() {
#ifdef _DEBUG
    printf("iddma_cpu_mmap_allocator: destructor (type: %d)\n", type_);
#endif
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto itr = region_info_.begin(); itr != region_info_.end(); itr++) {
        free_region(itr->first, itr->second);
    }
}

void *iddma_cpu_mmap_allocator::allocate_region(size_t &aligned_size, size_t align) {
#ifdef _DEBUG
    printf("iddma_cpu_mmap_allocator: alignment: 0x%lx, size: 0x%lx b aligned_alloc...\n", align, aligned_size);
#endif
    void *ptr = nullptr;
    int flags = 0;
    switch (type_) {
    case KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_2MB:
        aligned_size = (aligned_size + KPAGE_SIZE_2MB - 1) & ~(KPAGE_SIZE_2MB - 1);
        flags = MMAP_COMMON_FLAG | MAP_HUGETLB | MAP_HUGE_2MB;
        break;
    case KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_1GB:
    default:
        aligned_size = (aligned_size + KPAGE_SIZE_1GB - 1) & ~(KPAGE_SIZE_1GB - 1);
        flags = MMAP_COMMON_FLAG | MAP_HUGETLB | MAP_HUGE_1GB;
        break;
    }

    ptr = mmap(nullptr, aligned_size, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (ptr == MAP_FAILED) {
#ifdef _DEBUG
        printf("iddma_cpu_mmap_allocator: mmap failed\n");
#endif
        return nullptr;
    }

    return ptr;
}

void iddma_cpu_mmap_allocator::free_region(void *addr, size_t size) {
#ifdef _DEBUG
    printf("iddma_cpu_mmap_allocator: free_region...: %p %lx\n", addr, size);
#endif
    munmap(addr, size);
}
