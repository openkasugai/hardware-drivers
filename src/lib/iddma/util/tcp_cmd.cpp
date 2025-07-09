/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <tcp_cmd.hpp>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <cstring>
#include <stdexcept>

//#define _DEBUG
#define _DEBUG_RC
static const int s_max_burst_size = 4096;
static const double s_transfer_split_time_in_sec = 0.01f; // 10msec in sec
static const int s_transfer_split_time_in_usec = 10000; // 10msec in usec

TcpCmd::TcpCmd(int port)
    : is_server_(true),
      port_(port),
      src_socket_(-1),
      dst_socket_(-1),
      epoll_fd_(-1),
      finalize_(false),
      limit_gbps_(0.f),
      cur_limit_bytes_in_split_(0),
      limit_bytes_in_split_(0),
      transfer_total_gbps_(0.),
      transfer_bytes_total_(0),
      transfer_duration_usec_(0),
      transfer_split_bytes_(0)
{
}

TcpCmd::TcpCmd(int port, const std::string& my_ip)
    : is_server_(true),
      port_(port),
      my_ip_(my_ip),
      src_socket_(-1),
      dst_socket_(-1),
      epoll_fd_(-1),
      finalize_(false),
      limit_gbps_(0.f),
      cur_limit_bytes_in_split_(0),
      limit_bytes_in_split_(0),
      transfer_total_gbps_(0.),
      transfer_bytes_total_(0),
      transfer_duration_usec_(0),
      transfer_split_bytes_(0)
{
}

TcpCmd::TcpCmd(const std::string& ip, int port)
    : is_server_(false),
      ip_(ip),
      port_(port),
      src_socket_(-1),
      dst_socket_(-1),
      epoll_fd_(-1),
      finalize_(false),
      limit_gbps_(0.f),
      cur_limit_bytes_in_split_(0),
      limit_bytes_in_split_(0),
      transfer_total_gbps_(0.),
      transfer_bytes_total_(0),
      transfer_duration_usec_(0),
      transfer_split_bytes_(0)
{
}

TcpCmd::TcpCmd(const std::string& ip, int port, const std::string& my_ip)
    : is_server_(false),
      ip_(ip),
      port_(port),
      my_ip_(my_ip),
      src_socket_(-1),
      dst_socket_(-1),
      epoll_fd_(-1),
      finalize_(false),
      limit_gbps_(0.f),
      cur_limit_bytes_in_split_(0),
      limit_bytes_in_split_(0),
      transfer_total_gbps_(0.),
      transfer_bytes_total_(0),
      transfer_duration_usec_(0),
      transfer_split_bytes_(0)
{
}

TcpCmd::~TcpCmd(void)
{
    finalize_ = true;
    close();
    for (auto& it : buffers_) {
        free(it.first);
    }
}

bool TcpCmd::listen(void)
{
    return is_server_;
}

bool TcpCmd::accept(void)
{
    if (!is_server_) return false;
    return initialize_server();
}

bool TcpCmd::connect(void)
{
    if (is_server_) return false;
    return initialize_client();
}

void TcpCmd::set_limit_gbps(float limit)
{
    limit_gbps_ = limit;
    limit_bytes_in_split_ = limit / 8.f * 1000000000.f * s_transfer_split_time_in_sec;
}

bool TcpCmd::initialize_server(void)
{
    struct sockaddr_in src_addr;
    memset(&src_addr, 0, sizeof(struct sockaddr_in));
    src_addr.sin_port = htons(port_);
    src_addr.sin_family = AF_INET;
    src_addr.sin_addr.s_addr = my_ip_.empty() ? INADDR_ANY : inet_addr(my_ip_.c_str());

    int retry = 10;
    int ret = -1;
    while (retry) {
        src_socket_ = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _DEBUG
        printf("src_socket_: %d %s %d\n", src_socket_, my_ip_.c_str(), port_);
#endif
        if (src_socket_ < 0) {
            retry--;
            usleep(1000000);
            continue;
        }

        ret = bind(src_socket_, (struct sockaddr*)&src_addr, sizeof(src_addr));
#ifdef _DEBUG
        printf("bind_: %d %d\n", src_socket_, ret);
#endif
        if (ret < 0) {
            retry--;
            if (!retry) break;
            ::close(src_socket_);
            usleep(1000000);
            continue;
        }
        break;
    }
    if (ret < 0) {
        if (src_socket_ >= 0) {
            ::close(src_socket_);
            return false;
        }
        return false;
    }

    retry = 100;
    socklen_t len(sizeof(dst_addr_));
    do {
        retry--;
#ifdef _DEBUG
            printf("listen:\n");
#endif
            if (::listen(src_socket_, 1) < 0) {
            int eno = errno;
#ifdef _DEBUG
            printf("listen: %d\n", eno);
#endif
            if (eno == EAGAIN) continue;
            ::close(src_socket_);
            return false;
        }
        dst_socket_ = ::accept(src_socket_, (struct sockaddr*)&dst_addr_, &len);
        if (dst_socket_ < 0) {
            int eno = errno;
#ifdef _DEBUG
            printf("accept: %d\n", eno);
#endif
            if (eno == EAGAIN) continue;
            ::close(src_socket_);
            return false;
        } else {
            int enable_val = 1;
            setsockopt(dst_socket_, IPPROTO_TCP, TCP_NODELAY, &enable_val, sizeof(enable_val));
            break;
        }
    } while (retry);
    return initialize_event();
}

bool TcpCmd::initialize_client(void)
{
    struct sockaddr_in dst_addr;
    memset(&dst_addr, 0, sizeof(struct sockaddr_in));
    dst_addr.sin_port = htons(port_);
    dst_addr.sin_family = AF_INET;
    dst_addr.sin_addr.s_addr = inet_addr(ip_.c_str());

    dst_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    int enable_val = 1;
    setsockopt(dst_socket_, IPPROTO_TCP, TCP_NODELAY, &enable_val, sizeof(enable_val));
#ifdef _DEBUG
    printf("dst_socket_: %d %s %d\n", dst_socket_, ip_.c_str(), port_);
#endif
    if (!my_ip_.empty()) {
        struct sockaddr_in src_addr;
        memset(&src_addr, 0, sizeof(struct sockaddr_in));
        src_addr.sin_family = AF_INET;
        src_addr.sin_addr.s_addr = inet_addr(my_ip_.c_str());
        int src_port = port_;
        do {
            src_addr.sin_port = htons(src_port);
            if (!::bind(dst_socket_, (struct sockaddr*)&src_addr, sizeof(src_addr))) break;
            src_port++;
        } while (src_port < 65536);
    }
    int retry = 100;
    for (; retry>0; retry--) {
        if (!::connect(dst_socket_, (struct sockaddr*)&dst_addr, sizeof(dst_addr))) {
            break;
        }
        usleep(1000000);
    }
    if (!retry) {
#ifdef _DEBUG
        printf("connect failed: %d\n", errno);
#endif
        ::close(dst_socket_);
        return false;
    }
    return initialize_event();
}

bool TcpCmd::initialize_event(void) {
    epoll_fd_ = epoll_create(1);
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = dst_socket_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, dst_socket_, &ev)) return false;

    transfer_split_start_ = std::chrono::system_clock::now();
    return true;
}

int TcpCmd::wait(int timeout_msec)
{
    struct epoll_event event;
    memset(&event, 0, sizeof(event));
    int nfd = epoll_wait(epoll_fd_, &event, 1, timeout_msec);
    if (nfd <= 0) return 0; // no event
    if (event.events & EPOLLERR) return -1;
    if (event.events & EPOLLIN) return 1;
    return 0;
}

int TcpCmd::send(const std::string& msg)
{
    int ret0 = wait(0);
    if (ret0 < 0) return ret0;
    int pos = 0;
    while (pos < msg.size()) {
        int ret = ::send(dst_socket_, &msg[pos], msg.size() - pos, 0);
        if (ret > 0) {
            pos += ret;
        } else if (ret == 0 || errno == EAGAIN) {
            usleep(100000);
        } else {
            return ret;
        }
    }
    return pos;
}

void TcpCmd::rate_control(int64_t duration)
{
    if (limit_gbps_ > 0.f) {
        bool need_rc = transfer_split_bytes_ || transfer_bytes_total_;
        bool need_mod_start = duration >= s_transfer_split_time_in_usec;
        while (duration >= s_transfer_split_time_in_usec) {
            if (need_rc) {
                transfer_bytes_total_ += transfer_split_bytes_;
                transfer_bytes_per_10msec_.push(transfer_split_bytes_);
                if (transfer_bytes_per_10msec_.size() > 100) {
                    transfer_bytes_total_ -= transfer_bytes_per_10msec_.front();
                    transfer_bytes_per_10msec_.pop();
                } else {
                    transfer_duration_usec_ += s_transfer_split_time_in_usec;
                }
                transfer_split_bytes_ = 0;
                duration -= s_transfer_split_time_in_usec;
            } else {
                duration = 0;
            }
        }
        if (need_mod_start) {
            auto now = std::chrono::system_clock::now();
            transfer_split_start_ = now - std::chrono::microseconds(duration);
        }
        if (transfer_duration_usec_) transfer_total_gbps_ = (double)transfer_bytes_total_ / transfer_duration_usec_ / 1000. * 8.;
    }
}

bool TcpCmd::available_with_rate_control(int size)
{
    if (!(limit_gbps_ > 0.f)) return true;
    if (transfer_split_bytes_ + size < limit_bytes_in_split_) return true;

    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - transfer_split_start_).count();
    rate_control(duration);
    if (transfer_total_gbps_ > limit_gbps_) return false;

    float gbps = (double)transfer_split_bytes_ / duration / 1000. * 8.;
    if (gbps > limit_gbps_) return false;
    if (transfer_split_bytes_ + size > limit_bytes_in_split_) return false;
    return true;
}

void TcpCmd::update_rate(int size)
{
    if (!(limit_gbps_ > 0.f)) return;

    transfer_split_bytes_ += size;
    if (transfer_split_bytes_ < limit_bytes_in_split_) {
#ifdef _DEBUG_RC
        //printf("gbps: %12.5f, lsize: %10x\n", transfer_total_gbps_, transfer_split_bytes_);
#endif
        return;
    }
    auto now = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - transfer_split_start_).count();
#ifdef _DEBUG_RC
    printf("gbps: %12.5f, lsize: %10x, lduration: %10.3lfms, gbps: %12.5lf\n", transfer_total_gbps_,
           transfer_split_bytes_, (double)duration/1000.,
           (double)transfer_split_bytes_ / duration / 1000. * 8.);
#endif
}

int TcpCmd::send(const char* msg, int len)
{
    int ret0 = wait(0);
    if (ret0 < 0) return ret0;
    int pos = 0;
    while (pos < len) {
        int send_size = len - pos;
        if (send_size > s_max_burst_size) send_size = s_max_burst_size;
        while (!available_with_rate_control(send_size) && !finalize_) {
            //usleep(1);
        }
        int ret = ::send(dst_socket_, &msg[pos], send_size, 0);
        if (ret > 0) {
            pos += ret;
            update_rate(ret);
        } else if (ret == 0 || errno == EAGAIN) {
            //usleep(1);
        } else {
            return ret;
        }
    }
    return pos;
}

int TcpCmd::recv(std::vector<char>& msg)
{
    msg.resize(msg.capacity());
    int size = ::recv(dst_socket_, &msg[0], msg.size(), 0);
    msg.resize(size);
    return size;
}

int TcpCmd::recv(char* data, int max_size)
{
    int size = ::recv(dst_socket_, data, max_size, 0);
    return size;
}

int TcpCmd::close(void)
{
    if (epoll_fd_ >= 0) {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
    }
    if (dst_socket_ >= 0) {
        ::close(dst_socket_);
        dst_socket_ = -1;
    }
    if (src_socket_ >= 0) {
        ::close(src_socket_);
        src_socket_ = -1;
    }
    return 0;
}

void* TcpCmd::create_buffer(uint32_t size)
{
    void* buf = malloc(size);
    buffers_[buf] = size;
    return buf;
}

void TcpCmd::destroy_buffer(void* ptr)
{
    auto it = buffers_.find(ptr);
    if (it != buffers_.end()) {
        free(it->first);
        buffers_.erase(it);
    }
}
