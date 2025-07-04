/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef __TEST_XSE_TCP_HPP__
#define __TEST_XSE_TCP_HPP__

#include <gtest/gtest.h>
#include <test_xse.hpp>
#include <libroute_controller.h>
#include <iddma.h>
#include <stdint.h>
#include <vector>
#include <mutex> // NOLINT
#include <condition_variable> // NOLINT

class TcpCmd;

class iddma_xse_tcp_test : public iddma_xse_test {
protected:

    iddma_xse_tcp_test(void);
    iddma_xse_tcp_test(int num);
    iddma_xse_tcp_test(int num, int buf_size);
    iddma_xse_tcp_test(int tx_ch, int rx_ch, int func_id, int buf_size);

    virtual void SetUp();
    void execute_pre(void);
    void execute_wait(void);

    std::unique_ptr<TcpCmd> tcp_cmd_;
};

class iddma_xse_tcp_nop2_test : public iddma_xse_tcp_test {
public:
    iddma_xse_tcp_nop2_test(void);
};

class iddma_xse_tcp_nop16_test : public iddma_xse_tcp_test {
public:
    iddma_xse_tcp_nop16_test(void);
};

class iddma_xse_tcp_nop1240_test : public iddma_xse_tcp_test {
public:
    iddma_xse_tcp_nop1240_test(void);
};

class iddma_xse_tcp_nop2_2_test : public iddma_xse_tcp_test {
public:
    iddma_xse_tcp_nop2_2_test(void);
    iddma_xse_tcp_nop2_2_test(int buf_size);
};

class iddma_xse_tcp_nop2_16_test : public iddma_xse_tcp_nop2_test {
public:
    iddma_xse_tcp_nop2_16_test(void);
};

class iddma_xse_tcp_nop2_10240_test : public iddma_xse_tcp_nop2_test {
public:
    iddma_xse_tcp_nop2_10240_test(void);
};

#endif // __TEST_XSE_TCP_HPP__
