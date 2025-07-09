/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef TEST_CPU_TCP_HPP_
#define TEST_CPU_TCP_HPP_

#include <test_cpu.hpp>

class iddma_cpu_tcp_test : public iddma_cpu_test {
protected:
    iddma_cpu_tcp_test(void);
    explicit iddma_cpu_tcp_test(bool mode);
    virtual void SetUp(void);
    virtual void test_set_up_opt(bool is_tx, std::vector<std::string>& options);
    void execute(void);
    void execute_cp(void);

    bool is_start_end_;
};

class iddma_cpu_tcp_nop_test : public iddma_cpu_tcp_test {
protected:
    iddma_cpu_tcp_nop_test(void);
    void create_data(void* data, void* expects, uint32_t size, const uint64_t* rand_buf, uint32_t rand_num);
};

class iddma_cpu_tcp_nop_test16 : public iddma_cpu_tcp_nop_test {
protected:
    iddma_cpu_tcp_nop_test16(void);
};

class iddma_cpu_tcp_nop_test1240 : public iddma_cpu_tcp_nop_test {
protected:
    iddma_cpu_tcp_nop_test1240(void);
};

class iddma_cpu_tcp_nop_cp_test : public iddma_cpu_tcp_test {
protected:
    iddma_cpu_tcp_nop_cp_test(void);
};

class iddma_cpu_tcp_nop_cp_test16 : public iddma_cpu_tcp_nop_cp_test {
protected:
    iddma_cpu_tcp_nop_cp_test16(void);
};

class iddma_cpu_tcp_nop_cp_test1240 : public iddma_cpu_tcp_nop_cp_test {
protected:
    iddma_cpu_tcp_nop_cp_test1240(void);
};

#endif // TEST_CPU_TCP_HPP_
