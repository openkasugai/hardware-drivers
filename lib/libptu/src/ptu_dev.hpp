/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file ptu_dev.hpp
 * @brief Header file for class definition for PTU control
 */

#pragma once

#include <stdint.h>
#include <sys/time.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

// key of socket table
struct sock_info {
  uint32_t laddr;
  uint16_t lport;
  uint32_t raddr;
  uint16_t rport;
};

struct sock_info_hash {
  std::size_t operator()(sock_info info) const {
    int val = info.laddr + info.lport + info.raddr + info.rport;
    return std::hash<int>()(val);
  }
};

inline bool operator==(const sock_info& a, const sock_info& b) {
  return a.laddr == b.laddr && a.lport == b.lport && a.raddr == b.raddr &&
         a.rport == b.rport;
}

class ptu_dev {
 public:
  ptu_dev(int fd, uint32_t id, uint32_t dev_id, uint32_t base);
  ~ptu_dev(void);

  int init(uint32_t ip_addr, uint32_t netmask, uint32_t gateway,
           const uint8_t* mac);
  int modify(uint32_t ip_addr, uint32_t gateway,
           const uint8_t *mac);
  int listen(uint16_t lport);
  int listen_close(uint16_t lport);
  int accept(uint16_t lport, uint32_t raddr, uint16_t rport,
             uint64_t timeout_us, uint32_t* cid);
  int connect(uint16_t lport, uint32_t raddr, uint16_t rport,
              uint64_t timeout_us, uint32_t* cid);
  int abort(uint32_t cid);
  void start_event();
  void stop_event();

  void reg_write(uint32_t reg_idx, uint32_t value);
  uint32_t reg_read(uint32_t reg_idx);

 private:
  enum class tcp_st { SYN_SENT, ESTABLISHED };

  struct ptu_tcp_evt {
    uint16_t cid;
    uint16_t factor;
    uint32_t laddr;
    uint16_t lport;
    uint32_t raddr;
    uint16_t rport;
  };

  struct ptu_tcp_conn {
    uint16_t cid;
    tcp_st state;
    sock_info info;
  };

  void tcp_listen(uint16_t lport);
  void tcp_listen_close(uint16_t lport);
  void tcp_connect(uint16_t lport, uint32_t raddr, uint16_t rport);
  void tcp_abort(uint16_t cid);
  void tcp_release(uint16_t cid);
  bool get_tcp_event(ptu_tcp_evt& evt);
  void reg_write_priv(uint32_t reg_idx, uint32_t value);
  uint32_t reg_read_priv(uint32_t reg_idx);

  int fd_;                                     // file descriptor of xpcieN
  uint32_t id_;                                // PTU ID
  uint32_t dev_id_;                            // Device ID
  uint32_t base_;                              // register base address
  uint32_t ip_addr_;                           // my IP address
  std::mutex mtx_dev_;                         // for PTU register access
  std::thread th_evt_;                         // thread for event monitoring
  std::atomic<bool> stop_evt_;                 // flag of stopping event thread
  std::unordered_set<uint16_t> listen_socks_;  // set of listened port
  std::mutex mtx_listen_socks_;                // for listen_socks_
  std::unordered_map<sock_info, ptu_tcp_conn, sock_info_hash>
      socks_;                         // sockets
  std::mutex mtx_socks_;              // for socks_
  std::condition_variable cv_socks_;  // for socks_
};
