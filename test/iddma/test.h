/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef TEST_CONNECTION_H_
#define TEST_CONNECTION_H_

#include <gtest/gtest.h>
#include <utility>
#include <random>
#include <string>
#include <vector>
#include <iddma.h>

#define RANDOM_SEED 1204
#define RANDOM_IMM_NUM 16777217

#define FPGA_FILTER_RESIZE_BUFFER_NUM 8
#define TCP_BASE_PORT 30000
#define TCP_CTRL_BASE_PORT 20000

#define ALIGN_CEIL64(x) (((x) + 63) & ~63)

typedef enum {
    KIDDMA_TEST_DEFAULTID_FPGA_NOP = 11,
    KIDDMA_TEST_DEFAULTID_FPGA_NOP_PERF = 21,
    KIDDMA_TEST_DEFAULTID_FPGA_NOP_DUAL = 31,
    KIDDMA_TEST_DEFAULTID_FPGA_NOP_MMAPPED = 41,
    KIDDMA_TEST_DEFAULTID_FPGA_VEC_FP_INC = 51,
    KIDDMA_TEST_DEFAULTID_FPGA_TCP_NOP = 61,
    KIDDMA_TEST_DEFAULTID_FPGA_TCP_NOP_PERF = 71,
} iddma_test_defaultid;

typedef enum {
    KIDDMA_TEST_BUFFER_MALLOC = 0,
    KIDDMA_TEST_BUFFER_HUGE_2M,
    KIDDMA_TEST_BUFFER_HUGE_1G,
    KIDDMA_TEST_BUFFER_HUGE_ANY,
    KIDDMA_TEST_BUFFER_HUGE_ANY_OR_MALLOC,
    KIDDMA_TEST_BUFFER_GPU_DEV,
    KIDDMA_TEST_BUFFER_GPU_HOST,
} iddma_test_buffer_type;

class iddma_test : public testing::Test {
protected:
    iddma_test(void);

    static const int s_ch_step_;

    std::vector<iddma_object> obj_;
    static int id_;
    bool fail_;

    virtual ~iddma_test(void);

    virtual void SetUp(void);
    virtual void TearDown(void);

    virtual void execute(void) = 0;

    bool create_iddma_object(iddma_device_type type, const std::vector<std::string>& options);
    void create_random(uint64_t* buf, uint32_t size);

    void allocate_buf(iddma_test_buffer_type type, void** buf, uint32_t size);
    void copy_buf(void* dst, const void* src, uint32_t size);
    void fill_buf(void* dst, int val, uint32_t size);
    void free_buf(void** buf);

    virtual void create_data(void* data, void* expects, uint32_t size, const uint64_t* rand_buf, uint32_t rand_num);

    iddma_status connect_iddmas(bool is_connector, int ch_id);
    iddma_status connect_iddmas(bool is_connector, int ch_id, int conn_ch_id);

private:
    std::map<void*, iddma_test_buffer_type> buffer_types_;
    std::map<void*, uint32_t> buffer_sizes_;
};

#endif // TEST_CONNECTION_H_
