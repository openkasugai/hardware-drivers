/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _TCP_CMD_HPP__
#define _TCP_CMD_HPP__

#include <netinet/in.h>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <queue>

class TcpCmd
{
public:
    explicit TcpCmd(int port);
    TcpCmd(int port, const std::string& my_ip);
    TcpCmd(const std::string& ip, int port);
    TcpCmd(const std::string& ip, int port, const std::string& my_ip);
    ~TcpCmd(void);

    bool listen(void);
    bool accept(void);
    bool connect(void);
    void set_limit_gbps(float limit);

    int wait(int timeout_msec);
    int send(const std::string& msg);
    int send(const char* msg, int len);
    int recv(std::vector<char>& msg);
    int recv(char* data, int max_size);
    int close(void);

    void* create_buffer(uint32_t size);
    void destroy_buffer(void* ptr);

private:
    bool initialize_server(void);
    bool initialize_client(void);
    bool initialize_event(void);

    void rate_control(int64_t duration);
    bool available_with_rate_control(int size);
    void update_rate(int size);

    bool is_server_;
    std::string ip_;
    int port_;
    std::string my_ip_;

    int src_socket_;
    int dst_socket_;
    int epoll_fd_;
    bool finalize_;

    struct sockaddr_in dst_addr_;
    float limit_gbps_;
    uint32_t cur_limit_bytes_in_split_;
    uint32_t limit_bytes_in_split_;

    std::map<void*, uint32_t> buffers_;
    std::queue<uint32_t> transfer_bytes_per_10msec_;
    float transfer_total_gbps_;
    uint64_t transfer_bytes_total_;
    uint64_t transfer_duration_usec_;
    uint32_t transfer_split_bytes_;
    std::chrono::system_clock::time_point transfer_split_start_;
};

#endif // _TCP_CMD_HPP__
