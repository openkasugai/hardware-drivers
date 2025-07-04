/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <stdint.h>
#include <gtest/gtest.h>
#include <mem_manage.h>
#include <sys/mman.h>

#ifndef MAP_HUGE_2MB
#define MAP_HUGE_2MB    (21 << MAP_HUGE_SHIFT)
#endif
#ifndef MAP_HUGE_1GB
#define MAP_HUGE_1GB    (30 << MAP_HUGE_SHIFT)
#endif

#ifdef ENABLE_CUDA
#include <cuda_runtime.h>

TEST(SingleProc, PinAndGetCudaPhysAddr)
{
    uint32_t size = 0x30000;

    int count;
    int ret = cudaGetDeviceCount(&count);
    printf("dev: %d, ret: %d\n", count, ret);
    for (int i = 0; i < count; i++) {
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, i);
        printf("Device Number: %d\n", i);
        printf("  Device name: %s\n", prop.name);
        printf("  Memory Clock Rate (MHz): %d\n",
               prop.memoryClockRate/1024);
        printf("  Memory Bus Width (bits): %d\n",
               prop.memoryBusWidth);
        printf("  Peak Memory Bandwidth (GB/s): %.1f\n",
               2.0*prop.memoryClockRate*(prop.memoryBusWidth/8)/1.0e6);
        printf("  Total global memory (Gbytes) %.1f\n",(float)(prop.totalGlobalMem)/1024.0/1024.0/1024.0);
        printf("  Shared memory per block (Kbytes) %.1f\n",(float)(prop.sharedMemPerBlock)/1024.0);
        printf("  minor-major: %d-%d\n", prop.minor, prop.major);
        printf("  Warp-size: %d\n", prop.warpSize);
        printf("  Concurrent kernels: %s\n", prop.concurrentKernels ? "yes" : "no");
        printf("  Concurrent computation/communication: %s\n\n",prop.deviceOverlap ? "yes" : "no");
    }

    void* dev_ptr = nullptr;
    ASSERT_EQ(cudaMalloc(&dev_ptr, size), cudaSuccess);
    ASSERT_NE(nullptr, dev_ptr);

    auto obj = memManageCreate();
    ASSERT_NE((MemManageObj)nullptr, obj);

    uint64_t token;
    EXPECT_EQ(memManagePinCudaDevBuffer(obj, dev_ptr, size, &token), kMemManage_Success);
    EXPECT_NE(token, 0UL);

    void* new_ptr;
    EXPECT_EQ(memManageGetPinnedDevBuffer(obj, token, size, &new_ptr), kMemManage_Success);
    EXPECT_NE((void*)nullptr, new_ptr);

    uint64_t phys_addrs[4];
    uint32_t sizes[4];
    uint32_t num;
    EXPECT_EQ(memManageGetPinnedDevPhysAddrs(obj, new_ptr, sizeof(phys_addrs)/sizeof(uint64_t), &num, phys_addrs, sizes), kMemManage_Success);
    printf("org virt: %p, new virt: %p, phys: %lx\n", dev_ptr, new_ptr, phys_addrs[0]);
    EXPECT_EQ(num, 1u);
    EXPECT_EQ(sizes[0], size);

    EXPECT_EQ(memManageReleasePinnedDevBuffer(obj, new_ptr), kMemManage_Success);
    EXPECT_EQ(memManageUnpinCudaDevBuffer(obj, dev_ptr, false), kMemManage_Success);
    EXPECT_EQ(memManageDestroy(obj), kMemManage_Success);
}

#endif

void test_hostmem(uint32_t size, uint32_t max_cont, int type) {
    void* dev_ptr = nullptr;

    if (type == 0) {
        dev_ptr = aligned_alloc(64, size);
    } else if (type == 1 || type == 2) {
        int prot = PROT_READ | PROT_WRITE;
        int flag_common = MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB;
        int flag_2mb = flag_common | MAP_HUGE_2MB;
        int flag_1gb = flag_common | MAP_HUGE_1GB;
        int flag_1st = type == 1 ? flag_2mb : flag_1gb;
        int flag_2nd = type == 1 ? flag_1gb : flag_2mb;
        dev_ptr = mmap(NULL, size, prot, flag_1st, -1, 0);
        if (dev_ptr == (void*)-1) {
            dev_ptr = mmap(NULL, size, prot, flag_2nd, -1, 0);
        }
    }
    ASSERT_NE(nullptr, dev_ptr);
    ASSERT_NE((void*)-1, dev_ptr);

    auto obj = memManageCreate();
    ASSERT_NE((MemManageObj)nullptr, obj);

    uint64_t token;
    EXPECT_EQ(memManagePinHostDevBuffer(obj, dev_ptr, size, &token), kMemManage_Success);
    EXPECT_NE(token, 0UL);

    void* new_ptr;
    EXPECT_EQ(memManageGetPinnedDevBuffer(obj, token, size, &new_ptr), kMemManage_Success);
    EXPECT_NE((void*)nullptr, new_ptr);

    std::vector<uint64_t> phys_addrs(max_cont);
    std::vector<uint32_t> sizes(max_cont);
    uint32_t num;
    EXPECT_EQ(memManageGetPinnedDevPhysAddrs(obj, new_ptr, max_cont, &num, &phys_addrs[0], &sizes[0]), kMemManage_Success);
    printf("org virt: %p, new virt: %p, num: %u\n", dev_ptr, new_ptr, num);
    EXPECT_LE(num, max_cont);
    uint32_t total = 0;
    for (int i=0; i<num; i++) {
        //printf("  phys[%3d]: %lx size: %x\n", i, phys_addrs[i], sizes[i]);
        total += sizes[i];
    }
    EXPECT_EQ(total, size);

    EXPECT_EQ(memManageReleasePinnedDevBuffer(obj, new_ptr), kMemManage_Success);
    EXPECT_EQ(memManageUnpinHostDevBuffer(obj, dev_ptr, false), kMemManage_Success);
    EXPECT_EQ(memManageDestroy(obj), kMemManage_Success);

    if (type == 0) {
        free(dev_ptr);
    } else if (type == 1) {
        munmap(dev_ptr, size);
    }
}

TEST(SingleProc, PinAndGet4kPage12kPhysAddr)
{
    test_hostmem(0x3000, 4, 0);
}

TEST(SingleProc, PinAndGet4kPage2MPhysAddr)
{
    test_hostmem(0x200000, 513, 0);
}

TEST(SingleProc, PinAndGet4kPage8MPhysAddr)
{
    test_hostmem(0x800000, 2049, 0);
}

TEST(SingleProc, PinAndGet2MPage8MPhysAddr)
{
    test_hostmem(0x800000, 2049, 1);
}

TEST(SingleProc, PinAndGet1GPage8MPhysAddr)
{
    test_hostmem(0x800000, 2049, 2);
}
