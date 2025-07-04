/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <gtest/gtest.h>
#include <iddma.h>
#include "test.h"
#include <sys/mman.h>
#ifndef MAP_HUGE_2MB
#define MAP_HUGE_2MB (21 << MAP_HUGE_SHIFT)
#endif
#ifndef MAP_HUGE_1GB
#define MAP_HUGE_1GB (30 << MAP_HUGE_SHIFT)
#endif
#if (defined __SUPPORT_GPU_NV__) || (defined __USE_GPU_NV__)
#include <cuda_runtime.h>
#endif
#include <random>
#include <fstream>

int iddma_test::id_ = 1;
const int iddma_test::s_ch_step_ = 1000;

iddma_test::iddma_test(void)
    : fail_(false) {}

iddma_test::~iddma_test(void) {
    std::set<void*> buffers;
    for (auto it : buffer_types_) {
        buffers.insert(it.first);
    }
    for (auto it : buffers) {
        free_buf(&it);
    }
}

void iddma_test::SetUp(void) {
    id_++;
    if (id_ % 256 == 0) {
        id_++;
    }
}

void iddma_test::TearDown(void) {
    for (int i=0; i<obj_.size(); i++) {
        iddma_destroy_object(obj_[i]);
    }
    obj_.clear();
}

bool iddma_test::create_iddma_object(iddma_device_type type, const std::vector<std::string>& options) {
    std::vector<const char*> option_ptrs;
    for (auto& it : options) {
        option_ptrs.push_back(&it[0]);
    }
    option_ptrs.push_back(nullptr);
    iddma_object obj = iddma_create_object(type, &option_ptrs[0]);
    if (obj) {
        obj_.push_back(obj);
    } else {
        fail_ = true;
    }
    return !fail_;
}

void iddma_test::create_random(uint64_t* buf, uint32_t size) {
    std::mt19937_64 rnd_engine(RANDOM_SEED);
    for (int i = 0; i < size; i++)
        buf[i] = rnd_engine();
}

void iddma_test::allocate_buf(iddma_test_buffer_type type, void** buf, uint32_t size) {
    bool done = false;
#if ((defined __SUPPORT_GPU_NV__) || (defined __USE_GPU_NV__))
    if (type == KIDDMA_TEST_BUFFER_GPU_DEV) {
        EXPECT_EQ(cudaMallocHost(buf, size), cudaSuccess);
        done = true;
    } else if (type == KIDDMA_TEST_BUFFER_GPU_HOST) {
        EXPECT_EQ(cudaMalloc(buf, size), cudaSuccess);
        done = true;
    } else
#endif
    if (type == KIDDMA_TEST_BUFFER_HUGE_ANY_OR_MALLOC ||
        type == KIDDMA_TEST_BUFFER_HUGE_ANY ||
        type == KIDDMA_TEST_BUFFER_HUGE_2M ||
        type == KIDDMA_TEST_BUFFER_HUGE_1G) {
        iddma_memory_type mem_type = (iddma_memory_type)(
            (type != KIDDMA_TEST_BUFFER_HUGE_1G ? KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_2MB : 0) |
            (type != KIDDMA_TEST_BUFFER_HUGE_2M ? KIDDMA_MEMORY_TYPE_CPU_HUGEPAGE_MMAP_1GB : 0));
        *buf = iddma_mem_allocate(size, mem_type, 4096);
        if (type == KIDDMA_TEST_BUFFER_HUGE_ANY_OR_MALLOC) {
            if (*buf) {
                type = KIDDMA_TEST_BUFFER_HUGE_ANY;
            } else {
                type = KIDDMA_TEST_BUFFER_MALLOC;
            }
        }
        done = true;
    }
    if (type == KIDDMA_TEST_BUFFER_MALLOC) {
        *buf = aligned_alloc(4096, size);
    } else if (!done) {
        FAIL() << "unimplemented allocation method";
    }
    if (*buf) {
        buffer_types_[*buf] = type;
        buffer_sizes_[*buf] = size;
    }
}

void iddma_test::copy_buf(void* dst, const void* src, uint32_t size) {
    auto src_type_it = buffer_types_.find(const_cast<void*>(src));
    auto dst_type_it = buffer_types_.find(dst);
    ASSERT_NE(src_type_it, buffer_types_.end());
    ASSERT_NE(dst_type_it, buffer_types_.end());
    auto src_size_it = buffer_sizes_.find(const_cast<void*>(src));
    auto dst_size_it = buffer_sizes_.find(dst);
    ASSERT_NE(src_size_it, buffer_sizes_.end());
    ASSERT_NE(dst_size_it, buffer_sizes_.end());
    ASSERT_LE(size, src_size_it->second);
    ASSERT_LE(size, dst_size_it->second);
    auto src_type = src_type_it->second;
    auto dst_type = dst_type_it->second;
#if ((defined __SUPPORT_GPU_NV__) || (defined __USE_GPU_NV__))
    if (src_type == KIDDMA_TEST_BUFFER_GPU_DEV ||
        dst_type == KIDDMA_TEST_BUFFER_GPU_DEV) {
        EXPECT_EQ(cudaMemcpy(dst, src, size, cudaMemcpyDefault), cudaSuccess);
    } else
#endif
    {
        memcpy(dst, src, size);
    }
}

void iddma_test::fill_buf(void* dst, int val, uint32_t size) {
    auto dst_type_it = buffer_types_.find(dst);
    ASSERT_NE(dst_type_it, buffer_types_.end());
    auto dst_size_it = buffer_sizes_.find(dst);
    ASSERT_NE(dst_size_it, buffer_sizes_.end());
    ASSERT_LE(size, dst_size_it->second);
    auto dst_type = dst_type_it->second;
#if ((defined __SUPPORT_GPU_NV__) || (defined __USE_GPU_NV__))
    if (dst_type == KIDDMA_TEST_BUFFER_GPU_DEV) {
        EXPECT_EQ(cudaMemset(dst, val, size), cudaSuccess);
    } else
#endif
    {
        memset(dst, val, size);
    }
}

void iddma_test::free_buf(void** buf) {
    auto buf_type_it = buffer_types_.find(*buf);
    ASSERT_NE(buf_type_it, buffer_types_.end());
    auto buf_size_it = buffer_sizes_.find(*buf);
    ASSERT_NE(buf_size_it, buffer_sizes_.end());
    auto buf_type = buf_type_it->second;
#if ((defined __SUPPORT_GPU_NV__) || (defined __USE_GPU_NV__))
    if (buf_type == KIDDMA_TEST_BUFFER_GPU_DEV) {
        EXPECT_EQ(cudaFree(*buf), cudaSuccess);
    } else if (buf_type == KIDDMA_TEST_BUFFER_GPU_HOST) {
        EXPECT_EQ(cudaFreeHost(*buf), cudaSuccess);
    } else
#endif
    if (buf_type == KIDDMA_TEST_BUFFER_HUGE_ANY ||
        buf_type == KIDDMA_TEST_BUFFER_HUGE_2M ||
        buf_type == KIDDMA_TEST_BUFFER_HUGE_1G) {
        iddma_mem_free(*buf);
    } else {
        free(*buf);
    }
    buffer_types_.erase(buf_type_it);
    buffer_sizes_.erase(buf_size_it);
    *buf = nullptr;
}

void iddma_test::create_data(void* data, void* expects, uint32_t size, const uint64_t* rand_buf, uint32_t rand_num) {
    FAIL() << "unimplemented";
}

iddma_status iddma_test::connect_iddmas(bool is_connector, int ch_id) {
    return connect_iddmas(is_connector, ch_id, ch_id);
}

iddma_status iddma_test::connect_iddmas(bool is_connector, int ch_id, int conn_ch_id) {
    int retry = 5;
    iddma_status status;
    if (is_connector) {
        do {
            status = iddma_connect(obj_[ch_id], id_ + conn_ch_id * s_ch_step_, NULL);
#ifdef _DEBUG
            printf("iddma_connect: %d\n", status);
#endif
            if (status == KIDDMA_SUCCESS) break;
            retry--;
        } while (retry);
    } else {
        do {
            status = iddma_listen(obj_[ch_id], id_ + conn_ch_id * s_ch_step_, NULL);
#ifdef _DEBUG
            printf("iddma_listen: %d\n", status);
#endif
            if (status) return status;
            status = iddma_accept(obj_[ch_id]);
#ifdef _DEBUG
            printf("iddma_accept: %d\n", status);
#endif
            if (status == KIDDMA_SUCCESS) break;
            retry--;
        } while (retry);
    }
    return status;
}
