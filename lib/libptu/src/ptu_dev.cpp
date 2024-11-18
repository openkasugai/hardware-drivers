/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <ptu_dev.hpp>

#include <ptu_reg.h>

#include <algorithm>
#include <chrono>

#include <ptu_reg_func.hpp>

#ifdef __cplusplus
extern "C" {
#endif
#include <liblogging.h>
#ifdef __cplusplus
}
#endif

constexpr int POLL_EVT_MS = 100;  // 100ms

ptu_dev::ptu_dev(int fd, uint32_t id, uint32_t dev_id, uint32_t base)
    : fd_(fd),
      id_(id),
      dev_id_(dev_id),
      base_(base),
      ip_addr_(0),
      stop_evt_(false) {}

ptu_dev::~ptu_dev(void) {
  stop_event();
  std::unordered_set<uint16_t> socks;
  {
      std::lock_guard<std::mutex> lk(mtx_listen_socks_);
      socks = listen_socks_;
  }
  for (auto& it : socks) {
      tcp_listen_close(it);
  }
}

int ptu_dev::init(uint32_t ip_addr, uint32_t netmask, uint32_t gateway,
                  const uint8_t *mac) {
  reg_write(PtuRegMap::MY_IPV4_ADDR, ip_addr);
  reg_write(PtuRegMap::MY_IPV4_GATEWAY, gateway);

  uint32_t mac_hi =
      static_cast<uint32_t>(mac[0]) << 8 | static_cast<uint32_t>(mac[1]);
  reg_write(PtuRegMap::MY_MAC_HI, mac_hi);

  uint32_t mac_lo = static_cast<uint32_t>(mac[2]) << 24 |
                    static_cast<uint32_t>(mac[3]) << 16 |
                    static_cast<uint32_t>(mac[4]) << 8 |
                    static_cast<uint32_t>(mac[5]);
  reg_write(PtuRegMap::MY_MAC_LO, mac_lo);

  const uint32_t tcprxtx_base_val = 0x200000;
  reg_write(PtuRegMap::TCPRXTX_BASE, tcprxtx_base_val);

  ip_addr_ = ip_addr;

  return 0;
}

int ptu_dev::modify(uint32_t ip_addr, uint32_t gateway, const uint8_t *mac) {
  reg_write(PtuRegMap::MY_IPV4_ADDR, ip_addr);
  reg_write(PtuRegMap::MY_IPV4_GATEWAY, gateway);

  uint32_t mac_hi =
      static_cast<uint32_t>(mac[0]) << 8 | static_cast<uint32_t>(mac[1]);
  reg_write(PtuRegMap::MY_MAC_HI, mac_hi);

  uint32_t mac_lo = static_cast<uint32_t>(mac[2]) << 24 |
                    static_cast<uint32_t>(mac[3]) << 16 |
                    static_cast<uint32_t>(mac[4]) << 8 |
                    static_cast<uint32_t>(mac[5]);
  reg_write(PtuRegMap::MY_MAC_LO, mac_lo);

  ip_addr_ = ip_addr;

  return 0;
}

int ptu_dev::listen(uint16_t lport) {
  std::lock_guard<std::mutex> lk(mtx_listen_socks_);
  if (listen_socks_.find(lport) != listen_socks_.end()) {
    // already listen
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: already listened port %u\n",
                dev_id_, id_, __func__, lport);
    return -1;
  }
  listen_socks_.insert(lport);

  tcp_listen(lport);

  return 0;
}

int ptu_dev::listen_close(uint16_t lport) {
  std::lock_guard<std::mutex> lk(mtx_listen_socks_);
  if (listen_socks_.find(lport) == listen_socks_.end()) {
    // not listen
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: not listen port %u\n", dev_id_,
                id_, __func__, lport);
    return -1;
  }
  listen_socks_.erase(lport);

  tcp_listen_close(lport);

  return 0;
}

int ptu_dev::accept(uint16_t lport, uint32_t raddr, uint16_t rport,
                    uint64_t timeout_us, uint32_t *cid) {
  sock_info info;
  info.laddr = ip_addr_;
  info.lport = lport;
  info.raddr = raddr;
  info.rport = rport;

  // already established
  {
    std::lock_guard<std::mutex> lk(mtx_socks_);
    if (socks_.find(info) != socks_.end()) {
      if (socks_[info].state == tcp_st::ESTABLISHED) {
        *cid = socks_[info].cid;
        return 0;
      }
    }
  }

  // not listen
  {
    std::lock_guard<std::mutex> lk_listen(mtx_listen_socks_);
    if (listen_socks_.find(lport) == listen_socks_.end()) {
      log_libfpga(LIBFPGA_LOG_ERROR,
                  LIBPTU "(dev %u, ptu %u) %s: not listen port %u\n", dev_id_,
                  id_, __func__, lport);
      return -3;
    }
  }

  // wait establish
  {
    std::unique_lock<std::mutex> lk(mtx_socks_);
    bool result;
    if (timeout_us == 0) {
      cv_socks_.wait(lk, [&] {
        return std::any_of(socks_.begin(), socks_.end(), [&](const auto &sock) {
          return sock.first.lport == lport;
        });
      });
      result = true;
    } else {
      result =
          cv_socks_.wait_for(lk, std::chrono::microseconds(timeout_us), [&] {
            return std::any_of(
                socks_.begin(), socks_.end(),
                [&](const auto &sock) { return sock.first.lport == lport; });
          });
    }

    if (!result) {
      // timeout
      log_libfpga(LIBFPGA_LOG_ERROR,
                  LIBPTU "(dev %u, ptu %u) %s: timeout %llu us\n", dev_id_, id_,
                  __func__, timeout_us);
      return -1;
    }

    if (socks_.find(info) == socks_.end()) {
      // connected from unexpected IP or port
      log_libfpga(LIBFPGA_LOG_ERROR,
                  LIBPTU
                  "(dev %u, ptu %u) %s: socket not found laddr=0x%08x lport=%u "
                  "raddr=0x%08x rport=%u\n",
                  dev_id_, id_, __func__, info.laddr, info.lport, info.raddr,
                  info.rport);
      return -2;
    } else {
      auto sock = socks_[info];
      if (sock.state == tcp_st::ESTABLISHED) {
        *cid = socks_[info].cid;
        return 0;
      } else {
        // unexpected state
        log_libfpga(LIBFPGA_LOG_ERROR,
                    LIBPTU
                    "(dev %u, ptu %u) %s: unexpected connection state cid=%u "
                    "state=%d\n",
                    dev_id_, id_, __func__, socks_[info].cid,
                    socks_[info].state);
        return -2;
      }
    }
  }
}

int ptu_dev::connect(uint16_t lport, uint32_t raddr, uint16_t rport,
                     uint64_t timeout_us, uint32_t *cid) {
  std::unique_lock<std::mutex> lk(mtx_socks_);

  sock_info info;
  info.laddr = ip_addr_;
  info.lport = lport;
  info.raddr = raddr;
  info.rport = rport;

  if (socks_.find(info) != socks_.end()) {
    // already connect requested
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU
                "(dev %u, ptu %u) %s: already connect requested laddr=0x%08x "
                "lport=%u raddr=0x%08x rport=%u\n",
                dev_id_, id_, __func__, info.laddr, info.lport, info.raddr,
                info.rport);
    return -3;
  }

  ptu_tcp_conn conn = {};
  conn.cid = 0;
  conn.state = tcp_st::SYN_SENT;
  conn.info = info;
  socks_[info] = conn;

  tcp_connect(lport, raddr, rport);

  bool result;
  if (timeout_us == 0) {
    cv_socks_.wait(lk, [&] {
      return socks_.find(info) == socks_.end() ||
             socks_[info].state != tcp_st::SYN_SENT;
    });
    result = true;
  } else {
    result = cv_socks_.wait_for(lk, std::chrono::microseconds(timeout_us), [&] {
      return socks_.find(info) == socks_.end() ||
             socks_[info].state != tcp_st::SYN_SENT;
    });
  }

  if (!result) {
    // timeout
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: timeout %llu us\n", dev_id_, id_,
                __func__, timeout_us);
    return -1;
  }

  if (socks_.find(info) != socks_.end() &&
      socks_[info].state == tcp_st::ESTABLISHED) {
    *cid = socks_[info].cid;
    return 0;
  } else {
    if (socks_.find(info) == socks_.end()) {
      // socket is already removed
      log_libfpga(LIBFPGA_LOG_ERROR,
                  LIBPTU
                  "(dev %u, ptu %u) %s: socket not found laddr=0x%08x lport=%u "
                  "raddr=0x%08x rport=%u\n",
                  dev_id_, id_, __func__, info.laddr, info.lport, info.raddr,
                  info.rport);
    } else {
      // unexpected state
      log_libfpga(
          LIBFPGA_LOG_ERROR,
          LIBPTU
          "(dev %u, ptu %u) %s: unexpected connection state cid=%u state=%d\n",
          dev_id_, id_, __func__, socks_[info].cid, socks_[info].state);
    }
    return -2;
  }
}

int ptu_dev::abort(uint32_t cid) {
  // unestablished cid
  {
    std::lock_guard<std::mutex> lk(mtx_socks_);
    bool found_cid =
        std::any_of(socks_.begin(), socks_.end(),
                    [&](const auto &sock) { return sock.second.cid == cid; });
    if (!found_cid) {
      return -1;
    }
  }

  tcp_abort(cid);

  return 0;
}

void ptu_dev::start_event() {
  th_evt_ = std::thread([&] {
    while (!stop_evt_) {
      ptu_tcp_evt evt;
      if (get_tcp_event(evt)) {
        ptu_tcp_conn conn;
        conn.cid = evt.cid;
        conn.info.laddr = evt.laddr;
        conn.info.lport = evt.lport;
        conn.info.raddr = evt.raddr;
        conn.info.rport = evt.rport;

        if (evt.factor & TCP_EVE_ESTABLISHED) {
          std::lock_guard<std::mutex> lk(mtx_socks_);
          conn.state = tcp_st::ESTABLISHED;
          socks_[conn.info] = conn;
          cv_socks_.notify_all();
        }
        if (evt.factor & TCP_EVE_CLOSE_WAIT) {
          log_libfpga(LIBFPGA_LOG_DEBUG,
                      LIBPTU
                      "(dev %u, ptu %u) %s: tcp_event close wait cid=%u\n",
                      dev_id_, id_, __func__, evt.cid);
        }
        if (evt.factor & TCP_EVE_DISCONNECT) {
          log_libfpga(LIBFPGA_LOG_DEBUG,
                      LIBPTU
                      "(dev %u, ptu %u) %s: tcp_event disconnect cid=%u\n",
                      dev_id_, id_, __func__, evt.cid);
          std::lock_guard<std::mutex> lk(mtx_socks_);
          if (socks_.find(conn.info) != socks_.end()) {
            socks_.erase(conn.info);
            cv_socks_.notify_all();
          }
          tcp_release(evt.cid);
        }
        if (evt.factor & TCP_EVE_SYN_TIMEOUT) {
          log_libfpga(LIBFPGA_LOG_DEBUG,
                      LIBPTU
                      "(dev %u, ptu %u) %s: tcp_event syn timeout cid=%u\n",
                      dev_id_, id_, __func__, evt.cid);
          std::lock_guard<std::mutex> lk(mtx_socks_);
          if (socks_.find(conn.info) != socks_.end()) {
            socks_.erase(conn.info);
            cv_socks_.notify_all();
          }
          tcp_release(evt.cid);
        }
        if (evt.factor & TCP_EVE_SYN_ACK_TIMEOUT) {
          log_libfpga(LIBFPGA_LOG_DEBUG,
                      LIBPTU
                      "(dev %u, ptu %u) %s: tcp_event syn ack timeout cid=%u\n",
                      dev_id_, id_, __func__, evt.cid);
          std::lock_guard<std::mutex> lk(mtx_socks_);
          if (socks_.find(conn.info) != socks_.end()) {
            socks_.erase(conn.info);
            cv_socks_.notify_all();
          }
          tcp_release(evt.cid);
        }
        if (evt.factor & TCP_EVE_TIMEOUT) {
          log_libfpga(LIBFPGA_LOG_DEBUG,
                      LIBPTU "(dev %u, ptu %u) %s: tcp_event timeout cid=%u\n",
                      dev_id_, id_, __func__, evt.cid);
          std::lock_guard<std::mutex> lk(mtx_socks_);
          if (socks_.find(conn.info) != socks_.end()) {
            socks_.erase(conn.info);
            cv_socks_.notify_all();
          }
          tcp_release(evt.cid);
        }
        if (evt.factor & TCP_EVE_RECV_DATA) {
          log_libfpga(LIBFPGA_LOG_DEBUG,
                      LIBPTU
                      "(dev %u, ptu %u) %s: tcp_event recv data cid=%u\n",
                      dev_id_, id_, __func__, evt.cid);
        }
        if (evt.factor & TCP_EVE_SEND_DATA) {
          log_libfpga(LIBFPGA_LOG_DEBUG,
                      LIBPTU
                      "(dev %u, ptu %u) %s: tcp_event send data cid=%u\n",
                      dev_id_, id_, __func__, evt.cid);
        }
        if (evt.factor & TCP_EVE_RECV_URGENT_DATA) {
          log_libfpga(
              LIBFPGA_LOG_DEBUG,
              LIBPTU "(dev %u, ptu %u) %s: tcp_event recv urgent data cid=%u\n",
              dev_id_, id_, __func__, evt.cid);
        }
        if (evt.factor & TCP_EVE_RECV_RST) {
          log_libfpga(LIBFPGA_LOG_DEBUG,
                      LIBPTU "(dev %u, ptu %u) %s: tcp_event recv rst cid=%u\n",
                      dev_id_, id_, __func__, evt.cid);
          std::lock_guard<std::mutex> lk(mtx_socks_);
          if (socks_.find(conn.info) != socks_.end()) {
            socks_.erase(conn.info);
            cv_socks_.notify_all();
          }
          tcp_release(evt.cid);
        }
        if (evt.factor & TCP_EVE_INVLD_CONNECTION) {
          log_libfpga(
              LIBFPGA_LOG_DEBUG,
              LIBPTU
              "(dev %u, ptu %u) %s: tcp_event invalid connection cid=%u\n",
              dev_id_, id_, __func__, evt.cid);
        }
      } else {
        // poll with POLL_EVT_MS only when there are no TCP events
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_EVT_MS));
      }
    }
  });
}

void ptu_dev::stop_event() {
  stop_evt_ = true;
  if (th_evt_.joinable()) {
    th_evt_.join();
  }
}

void ptu_dev::reg_write(uint32_t reg_idx, uint32_t value) {
  std::lock_guard<std::mutex> lk(mtx_dev_);
  reg_write_priv(reg_idx, value);
}

uint32_t ptu_dev::reg_read(uint32_t reg_idx) {
  std::lock_guard<std::mutex> lk(mtx_dev_);
  return reg_read_priv(reg_idx);
}

void ptu_dev::tcp_listen(uint16_t lport) {
  std::lock_guard<std::mutex> lk(mtx_dev_);

  reg_write_priv(PtuRegMap::TCP_LOCAL_PORT, lport << 16);
  reg_write_priv(PtuRegMap::TCP_COMMAND, TCP_HOST_CMD_LSN_OPEN);
}

void ptu_dev::tcp_listen_close(uint16_t lport) {
  std::lock_guard<std::mutex> lk(mtx_dev_);

  reg_write_priv(PtuRegMap::TCP_LOCAL_PORT, lport << 16);
  reg_write_priv(PtuRegMap::TCP_COMMAND, TCP_HOST_CMD_LSN_CLOSE);
}

void ptu_dev::tcp_connect(uint16_t lport, uint32_t raddr, uint16_t rport) {
  std::lock_guard<std::mutex> lk(mtx_dev_);

  reg_write_priv(PtuRegMap::TCP_REMOTE_IP, raddr);
  reg_write_priv(PtuRegMap::TCP_REMOTE_PORT, rport);
  reg_write_priv(PtuRegMap::TCP_LOCAL_PORT, lport);
  reg_write_priv(PtuRegMap::TCP_COMMAND, TCP_HOST_CMD_CNN_OPEN);
}

void ptu_dev::tcp_abort(uint16_t cid) {
  std::lock_guard<std::mutex> lk(mtx_dev_);

  reg_write_priv(PtuRegMap::TCP_CID, cid);
  reg_write_priv(PtuRegMap::TCP_COMMAND, TCP_HOST_CMD_ABORT);
}

void ptu_dev::tcp_release(uint16_t cid) {
  std::lock_guard<std::mutex> lk(mtx_dev_);

  reg_write_priv(PtuRegMap::TCP_CID, cid);
  reg_write_priv(PtuRegMap::TCP_COMMAND, TCP_HOST_CMD_RELEASE);
}

bool ptu_dev::get_tcp_event(ptu_tcp_evt &evt) {
  std::lock_guard<std::mutex> lk(mtx_dev_);

  uint32_t cid_factor = reg_read_priv(PtuRegMap::TCP_EVENT_QUE);
  uint16_t factor = cid_factor & 0xffff;
  uint16_t cid = cid_factor >> 16;

  if (cid_factor == 0xffffffff) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: cannot read TCP_EVENT_QUE\n",
                dev_id_, id_, __func__);
    return false;
  }

  if (factor != 0) {
    uint32_t evt_raddr = reg_read_priv(PtuRegMap::TCP_EVENT_REMOTE_IP);
    uint32_t evt_laddr = reg_read_priv(PtuRegMap::TCP_EVENT_LOCAL_IP);
    uint32_t evt_port = reg_read_priv(PtuRegMap::TCP_EVENT_PORT);
    uint16_t evt_rport = evt_port & 0xffff;
    uint16_t evt_lport = evt_port >> 16;

    reg_write_priv(PtuRegMap::TCP_EVENT_QUE, 0);

    evt.cid = cid;
    evt.factor = factor;
    evt.laddr = evt_laddr;
    evt.lport = evt_lport;
    evt.raddr = evt_raddr;
    evt.rport = evt_rport;

    if (cid == 0) {
      log_libfpga(LIBFPGA_LOG_ERROR,
                  LIBPTU
                  "(dev %u, ptu %u) %s: factor!=0 but cid==0 factor=0x%04x\n",
                  dev_id_, id_, __func__, factor);
    }
    return true;
  } else {
    // no event
    return false;
  }
}

void ptu_dev::reg_write_priv(uint32_t reg_idx, uint32_t value) {
  ptu_reg_write(fd_, base_, reg_idx, value);
}

uint32_t ptu_dev::reg_read_priv(uint32_t reg_idx) {
  return ptu_reg_read(fd_, base_, reg_idx);
}
