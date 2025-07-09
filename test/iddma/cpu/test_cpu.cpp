/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <gtest/gtest.h>

#include <test_cpu.hpp>

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

#include <chrono>
#include <thread>
#include <stack>
#ifdef __SUPPORT_GPU_NV__
#include <cuda_runtime.h>
#endif // __SUPPORT_GPU_NV__

#include <test_config.h>


iddma_cpu_test::iddma_cpu_test(void)
    : rand_buf_(nullptr),
      func_num_(1),
      buf_type_(KIDDMA_TEST_BUFFER_HUGE_ANY_OR_MALLOC),
      buf_size_(2048),
      buf_num_(2),
      transfer_num_(2),
      is_cp_(false),
      throughput_mode_(false),
      latency_mode_(false),
      start_delay_usec_(0),
      limit_gbps_(0.f) {}

void iddma_cpu_test::SetUp(void) {
    iddma_test::SetUp();
    allocate_buf(KIDDMA_TEST_BUFFER_MALLOC, &rand_buf_, sizeof(uint64_t) * RANDOM_IMM_NUM);
    create_random((uint64_t*)rand_buf_, RANDOM_IMM_NUM);

    std::vector<std::string> tx_options;
    std::vector<std::string> rx_options;
    tx_options.push_back("transfer_mode=send");
    rx_options.push_back("transfer_mode=recv");

    test_set_up_opt(true, tx_options);
    test_set_up_opt(false, rx_options);

    create_iddma_object(KIDDMA_DEVICE_TYPE_CPU, tx_options);
    create_iddma_object(KIDDMA_DEVICE_TYPE_CPU, rx_options);
}

void iddma_cpu_test::test_set_up_opt(bool is_tx, std::vector<std::string>& options) {}

void iddma_cpu_test::connector(bool is_connector, int obj_id, int ch_id, void** bufs, uint32_t buf_num, int* stat) {
#ifdef _DEBUG
    printf("connector[%d][%d]\n", obj_id, ch_id);
#endif
    iddma_status status = connect_iddmas(is_connector, obj_id, ch_id);
    if (status) {
#ifdef _DEBUG
        printf("connect status : %d\n", status);
#endif
        *stat = status;
        return;
    }
    for (int i=0; i<buf_num; i++) {
        status = iddma_mmap_populate(obj_[obj_id], bufs[i], buf_size_);
        if (status) {
#ifdef _DEBUG
            printf("populate status : %d\n", status);
#endif
            *stat = status;
            return;
        }
    }
    status = iddma_mmap_share(obj_[obj_id]);
    if (status) *stat = status;
}

void iddma_cpu_test::execute(void) {
    tx_bufs_.resize(buf_num_);
    rx_bufs_.resize(buf_num_);
    exp_bufs_.resize(buf_num_);
#ifdef _DEBUG
    printf("execute: %d %d\n", buf_num_, buf_size_);
#endif
    for (int i=0; i<buf_num_; i++) {
        allocate_buf(buf_type_, &tx_bufs_[i], buf_size_);
        allocate_buf(buf_type_, &rx_bufs_[i], buf_size_);
        allocate_buf(buf_type_, &exp_bufs_[i], buf_size_);
        create_data(tx_bufs_[i], exp_bufs_[i], buf_size_, (const uint64_t*)rand_buf_, RANDOM_IMM_NUM);
    }

#if 0
    {
        int tx_stat = 0;
        int rx_stat = 0;
        std::thread tx_connector(&iddma_cpu_test::connector, this, true, 0, 0,
                                 tx_bufs_.data(), tx_bufs_.size(), &tx_stat);
        std::thread rx_connector(&iddma_cpu_test::connector, this, false, 1, func_num_*2-1,
        rx_bufs_.data(), rx_bufs_.size(), &rx_stat);
        tx_connector.join();
        rx_connector.join();
        EXPECT_EQ(tx_stat, 0);
        EXPECT_EQ(rx_stat, 0);
    }
#endif
    iddma_status status;
    // basically, should connect from stream source to stream sink.
    // but fpga toe has a problem not to finish listening any port.
    // at first, listen from counterpart
    status = connect_iddmas(false, 1, func_num_);
    EXPECT_EQ(status, KIDDMA_SUCCESS);
    for (int i=0; i<rx_bufs_.size(); i++) {
        status = iddma_mmap_populate(obj_[1], rx_bufs_[i], buf_size_);
        EXPECT_EQ(status, KIDDMA_SUCCESS);
    }
#ifdef _DEBUG
    printf("listen: accepted\n");
#endif
    usleep(500000);
    status = iddma_mmap_share(obj_[1]);
    EXPECT_EQ(status, KIDDMA_SUCCESS);
#ifdef _DEBUG
    printf("listen: share done\n");
#endif
    // secondly, connect to counterpart
    // W.A. wait a moment for fpga toe is ready.
    usleep(500000 + func_num_ * 1000000);
    status = connect_iddmas(true, 0);
    EXPECT_EQ(status, KIDDMA_SUCCESS);
    for (int i=0; i<tx_bufs_.size(); i++) {
        status = iddma_mmap_populate(obj_[0], tx_bufs_[i], buf_size_);
        EXPECT_EQ(status, KIDDMA_SUCCESS);
    }
    status = iddma_mmap_share(obj_[0]);
    EXPECT_EQ(status, KIDDMA_SUCCESS);
    // call mmap_share() after connection established

    bool send_start = false;
    bool send_ok = true;
    bool recv_ok = true;
    bool recv_result_ok = true;
    uint64_t microsec = 0;
    bool send_ready = false;
    bool recv_ready = false;
    auto start_time = std::chrono::system_clock::now();
    auto end_time = std::chrono::system_clock::now();
    if (!latency_mode_) {
        std::thread tx_worker(&iddma_cpu_test::send_worker, this, 0, &send_start, &send_ok, &send_ready,
                              nullptr, nullptr, 0, limit_gbps_);
        std::thread rx_worker(&iddma_cpu_test::recv_worker, this, 1, &send_ok, &recv_ok, &recv_result_ok, &send_start, &recv_ready,
                              nullptr, nullptr, 0);

        {
            usleep(start_delay_usec_);
            std::unique_lock<std::mutex> lock(mutex_);
            while (!send_ready || !recv_ready) cond_.wait(lock);
            send_start = true;
            cond_.notify_all();
        }

        start_time = std::chrono::system_clock::now();
        printf("transfer start: %ld\n",
               std::chrono::duration_cast<std::chrono::milliseconds>(start_time.time_since_epoch()).count());
        tx_worker.join();
        rx_worker.join();
        end_time = std::chrono::system_clock::now();

        printf("CPU: timestamps transfer: %ld %ld\n",
               std::chrono::duration_cast<std::chrono::milliseconds>(start_time.time_since_epoch()).count(),
               std::chrono::duration_cast<std::chrono::milliseconds>(end_time.time_since_epoch()).count());
        microsec = std::chrono::duration_cast<std::chrono::microseconds>(end_time-start_time).count();
    } else {
        ASSERT_FALSE(latency_mode_);
    }
    uint64_t transferred_size = (uint64_t)buf_size_ * transfer_num_;
    if (throughput_mode_) {
        printf("CPU: transferred: %16.6lf GiB, time: %10.3lfmsec, throughput: %12.4lfGB/s\n",
               (double)transferred_size / 1024 / 1024 / 1024 ,
               (double)microsec / 1000, (double)transferred_size / microsec / 1000.);
    } else if (latency_mode_) {
        printf("CPU: size: %10dbyte, transferred: %lutimes, time: %10.3lfmsec, latency: %12.4lfmsec\n",
               buf_size_, transferred_size / buf_size_,
               (double)microsec / 1000, (double)microsec * buf_size_ / transferred_size / 1000.);
    }
    if (!recv_result_ok) {
        printf("incorrect transfer\n");
    }

    EXPECT_TRUE(send_ok);
    EXPECT_TRUE(recv_ok);

    for (int i=0; i<2; i++) {
        while ((status = iddma_close(obj_[i])) == KIDDMA_ERROR_CONNECTION_TIMEOUT);
        EXPECT_EQ(KIDDMA_SUCCESS, status);
    }
}

void iddma_cpu_test::send_worker(int id, bool* send_start, bool* send_ok, bool* ready,
                                 uint32_t* out_cpl, const uint32_t* in_cpl, uint32_t out_cpl_offset,
                                 float limit_gbps) {
    worker_core(obj_[id], send_ok, nullptr, nullptr, send_start, ready, iddma_send, nullptr, iddma_poll_send, check_function(),
                out_cpl, in_cpl, out_cpl_offset, 0.f /*limit_gbps*/);
}

void iddma_cpu_test::recv_worker(int id, bool* send_ok, bool* recv_ok, bool* recv_result_ok, bool *send_start, bool* ready,
                                 uint32_t* out_cpl, const uint32_t* in_cpl, uint32_t out_cpl_offset) {
    using std::placeholders::_1;
    using std::placeholders::_2;
    worker_core(obj_[id], recv_ok, send_ok, recv_result_ok, send_start, ready, nullptr, iddma_recv, iddma_poll_recv,
                throughput_mode_ ? check_function() : std::bind(&iddma_cpu_test::checker, this, _1, _2),
                out_cpl, in_cpl, out_cpl_offset, 0.f);
}

void iddma_cpu_test::worker_core(iddma_object obj, bool* op_ok, bool* continous, bool* result_ok, bool* send_start, bool* ready,
                                 send_function send_func, recv_function recv_func, poll_function poll_func, check_function check_func,
                                 uint32_t* out_cpl, const uint32_t* in_cpl, uint32_t out_cpl_offset,
                                 float limit_gbps) {
    uint32_t buf_id = 0;
    uint32_t issue_num = 0;
    uint32_t compl_num = 0;
    int timeout_count = 0;
    int timeout_max = 20;
    bool issue_error = false;
    bool compl_error = false;
    uint64_t total_bytes = 0;
    uint32_t poll_min = buf_num_ < MAX_QUEUE_SIZE - 1 ? buf_num_ : MAX_QUEUE_SIZE - 1;
    if (out_cpl_offset > 0) {
        if (poll_min <= out_cpl_offset) poll_min = 1;
        else poll_min -= out_cpl_offset;
    }
    {
        std::unique_lock<std::mutex> lock(mutex_);
        *ready = true;
        cond_.notify_all();
        while (!(*send_start)) {
            cond_.wait(lock);
        }
    }
    auto start_time = std::chrono::system_clock::now();
    auto end_time = start_time;

    while (compl_num < transfer_num_ && timeout_count < timeout_max) {
        iddma_status stat;
        uint32_t inflights = issue_num - compl_num;
        bool need_issue = inflights < buf_num_ && (!in_cpl || issue_num < *in_cpl);
        if (limit_gbps > 0.f) {
            auto cur_time = std::chrono::system_clock::now();
            auto cur_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(cur_time - start_time).count();
            double gbps = (double)total_bytes / cur_nsec * 8.;
            need_issue &= gbps < limit_gbps;
        }
        if (issue_num < transfer_num_ && !issue_error && need_issue) {
            if (send_func) {
                stat = send_func(obj, tx_bufs_[buf_id], buf_size_);
            } else {
                stat = recv_func(obj, rx_bufs_[buf_id], buf_size_);
            }
            if (stat == KIDDMA_SUCCESS) {
                buf_id = (buf_id + 1) % buf_num_;
                issue_num++;
                total_bytes += buf_size_;
#ifdef _DEBUG
                printf("%s %u\n", send_func ? "send" : "recv", issue_num);
#endif
            } else if (stat != KIDDMA_ERROR_QUEUE_IS_FULL) {
                issue_error = true;
                printf("%s error: %d\n", send_func ? "send" : "recv", stat);
            }
        }
        if ((issue_num - compl_num >= poll_min ||
             issue_num == transfer_num_ || issue_error) && !compl_error) {
            iddma_queue_element qe;
            stat = poll_func(obj, 1, &qe);
            if (stat == KIDDMA_SUCCESS) {
                if (check_func && result_ok) {
                    check_func(compl_num % buf_num_, result_ok);
                }
                if (!compl_num) {
                    start_time = std::chrono::system_clock::now();
                } else {
                    end_time = std::chrono::system_clock::now();
                }
                compl_num++;
                if (out_cpl) *out_cpl = compl_num + out_cpl_offset;
#ifdef _DEBUG
                printf("%s poll %d %u/%u, %lx %lx %lx\n", send_func ? "send" : "recv", stat, compl_num, issue_num, qe.addr, qe.size, qe.status);
#endif
                timeout_count = 0;
            } else if (stat != KIDDMA_ERROR_POLL_TIMEOUT) {
                compl_error = true;
                printf("poll_%s error: %d\n", send_func ? "send" : "recv", stat);
            } else {
                timeout_count++;
                if (timeout_count > timeout_max/2) printf("poll_%s timeout: %d/%d\n", send_func ? "send" : "recv", compl_num, issue_num);
                //usleep(1);
            }
        }
        if (issue_error && (compl_error || issue_num == compl_num)) {
            *op_ok = false;
            printf("[%s] issue error: %d, compl error: %d, issue_num: %d, compl_num: %d\n",
                   send_func ? "send" : "recv", issue_error, compl_error, issue_num, compl_num);
            break;
        }
    }
    auto micro_sec = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    if (timeout_count == timeout_max) {
        *op_ok = false;
    } else if (compl_num > 1) {
        printf("%s: %12.4lfmsec %uframe, %12.4lfusec/frame\n", send_func ? "send" : "recv",
               (double)micro_sec/1000, compl_num - 1, (double)micro_sec / (compl_num - 1));
        printf("CPU: timestamps %s: %ld %ld\n", send_func ? "send" : "recv",
               std::chrono::duration_cast<std::chrono::milliseconds>(start_time.time_since_epoch()).count(),
               std::chrono::duration_cast<std::chrono::milliseconds>(end_time.time_since_epoch()).count());
    }
}

void iddma_cpu_test::checker(int buf_id, bool* result_ok) {
    const uint32_t* rptr = (const uint32_t*)rx_bufs_[buf_id];
    const uint32_t* eptr = (const uint32_t*)exp_bufs_[buf_id];
    int i;
    int count = 0;
    static int frame = 0;
#ifdef _DEBUG
    printf("checker:\n");
#endif
    int check_size = buf_size_ > 65536 ? 65536 : buf_size_;
    for (i=0; i<(check_size & ~3); i+=4) {
        uint32_t rval = rptr[i/4];
        uint32_t eval = eptr[i/4];
        if (rval != eval) {
            *result_ok = false;
            printf("[%6d][%8x]: %08x %08x\n", frame, i, rval, eval);
            count++;
            if (count > 33) return;
        }
    }
    const uint8_t* rptr8 = (const uint8_t*)rptr;
    const uint8_t* eptr8 = (const uint8_t*)eptr;
    for (;i<check_size; i++) {
        uint8_t rval = rptr8[i];
        uint8_t eval = eptr8[i];
        if (rval != eval) {
            *result_ok = false;
            printf("[%8x]: %02x %02x\n", i, rval, eval);
            count++;
            if (count > 33) return;
        }
    }
    frame++;
}

iddma_cpu_nop_test::iddma_cpu_nop_test(void) {}

void iddma_cpu_nop_test::create_data(void* data, void* expects, uint32_t size, const uint64_t* rand_buf, uint32_t rand_num) {
    static uint32_t pos = 0;
    uint32_t i;
    for (i=0; i<(size & ~7); i+=8) {
        *((uint64_t*)((uint8_t*)data + i)) = rand_buf[pos];
        *((uint64_t*)((uint8_t*)expects + i)) = rand_buf[pos++];
        if (pos == rand_num) pos = 0;
        //printf("[%8x][%4x]: %016lx\n", pos, i, *((uint64_t*)((uint8_t*)data + i)));
    }
    if (i==size) return;

    uint64_t val = rand_buf[pos++];
    if (pos == rand_num) pos = 0;
    for (; i<size; i++) {
        *((uint8_t*)data + i) = val & 0xff;
        *((uint8_t*)expects + i) = val & 0xff;
        val >>= 8;
    }
}

iddma_cpu_vec_fp_inc_test::iddma_cpu_vec_fp_inc_test(void)
    : inc_val_(1.0f) {}

void iddma_cpu_vec_fp_inc_test::create_data(void* data, void* expects, uint32_t size,
                                            const uint64_t* rand_buf, uint32_t rand_num) {
    static uint32_t pos = 0;
    uint32_t i;
    for (i=0; i<(size & ~3); i+=4) {
        float* input = (float*)((uint8_t*)data + i);
        float* exp = (float*)((uint8_t*)expects + i);

        float val = (float)(rand_buf[pos++] & 0xfffff) / 16.f;
        *input = val;
        *exp = val + inc_val_;

        if (pos == rand_num) pos = 0;
        //printf("[%8x][%4x]: %016lx\n", pos, i, *((uint64_t*)((uint8_t*)data + i)));
    }
    ASSERT_TRUE(i==size);
}

#ifdef ENABLE_NOP
TEST_F(iddma_cpu_nop_test, TestNop2) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_NOP;
    execute();
}

TEST_F(iddma_cpu_nop_test, TestNop16) {
#ifdef USE_4KPAGES
    buf_type_ = KIDDMA_TEST_BUFFER_MALLOC;
#endif
    buf_size_ = 8192;
    buf_num_ = 5;
    transfer_num_ = 16;
    execute();
}

TEST_F(iddma_cpu_nop_test, TestNop10240) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_NOP_PERF;
    buf_size_ = 1024*1024;
    buf_num_ = 8;
    transfer_num_ = 10240;
    throughput_mode_ = true;
    execute();
}
#endif

#ifdef ENABLE_NOP_DUAL
TEST_F(iddma_cpu_nop_test, TestNopX2_2) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_NOP_DUAL;
    func_num_ = 2;
    execute();
}

TEST_F(iddma_cpu_nop_test, TestNopX2_16) {
    func_num_ = 2;
    buf_size_ = 8192;
    buf_num_ = 5;
    transfer_num_ = 16;
    execute();
}

TEST_F(iddma_cpu_nop_test, TestNopX2_10240) {
    func_num_ = 2;
    buf_size_ = 1024*1024;
    buf_num_ = 8;
    transfer_num_ = 10240;
    throughput_mode_ = true;
    execute();
}
#endif

#ifdef ENABLE_NOP_MMAPPED
TEST_F(iddma_cpu_nop_test, TestNopMM2) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_NOP_MMAPPED;
    execute();
}

TEST_F(iddma_cpu_nop_test, TestNopMM16) {
#ifdef USE_4KPAGES
    buf_type_ = KIDDMA_TEST_BUFFER_MALLOC;
#endif
    buf_size_ = 8192;
    buf_num_ = 5;
    transfer_num_ = 16;
    execute();
}

TEST_F(iddma_cpu_nop_test, TestNopMM10240) {
    buf_size_ = 1024*1024;
    buf_num_ = 8;
    transfer_num_ = 10240;
    throughput_mode_ = true;
    execute();
}

TEST_F(iddma_cpu_nop_test, TestNopMM_2DDR_10240) {
    buf_size_ = 1024*1024;
    buf_num_ = 8;
    transfer_num_ = 10240;
    throughput_mode_ = true;
    execute();
}
#endif

#ifdef ENABLE_VEC_FP_INC
TEST_F(iddma_cpu_vec_fp_inc_test, TestVecFPInc16) {
    id_ = KIDDMA_TEST_DEFAULTID_FPGA_VEC_FP_INC;
    buf_size_ = 8192;
    buf_num_ = 5;
    transfer_num_ = 16;
    execute();
}

TEST_F(iddma_cpu_vec_fp_inc_test, TestVecFPInc10240) {
    buf_size_ = 1024*1024;
    buf_num_ = 8;
    transfer_num_ = 10240;
    throughput_mode_ = true;
    inc_val_ = 3.0f;
    execute();
}
#endif
