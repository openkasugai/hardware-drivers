/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <fcntl.h>
#include <libptu.h>
#include <ptu_reg.h>
#include <stdio.h>
#include <unistd.h>

#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <tuple>

#include <ptu_dev.hpp>
#ifdef __cplusplus
extern "C" {
#endif
#include <libfpgactl.h>
#include <liblogging.h>
#include <libfpga_internal/libfpgacommon_internal.h>
#ifdef __cplusplus
}
#endif

constexpr uint32_t PTU_BASE = 0x00020000;

// (dev_id, lane) -> ptu_dev
static std::map<std::tuple<uint32_t, uint32_t>, std::unique_ptr<ptu_dev>>
    ptu_devices;

int fpga_ptu_init(uint32_t dev_id, uint32_t lane, in_addr_t addr,
                  in_addr_t subnet, in_addr_t gateway, const uint8_t *mac) {
  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: device is not opend\n", dev_id,
                lane, __func__);
    return -1;
  }
  /* device */
  int dev_fd = dev->fd;

  bool inited =
      ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end();
  if (inited) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: already ptu initialized\n", dev_id,
                lane, __func__);
    return -1;
  }

  uint32_t base = PTU_BASE + 0x1000 * lane;
  ptu_devices[std::make_tuple(dev_id, lane)] =
      std::unique_ptr<ptu_dev>(new ptu_dev(dev_fd, lane, dev_id, base));

  int ret = ptu_devices[std::make_tuple(dev_id, lane)]->init(addr, subnet,
                                                               gateway, mac);
  if (ret != 0) {
    return -1;
  }

  // retransmission timer is 200ms
  ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
      PtuRegMap::TCP_RET_TIMER, (20 << 20) | (6000 << 4) | (1 << 3) | 5);

  ptu_devices[std::make_tuple(dev_id, lane)]->start_event();

  return 0;
}

int fpga_ptu_exit(uint32_t dev_id, uint32_t lane) {
  auto it = ptu_devices.find(std::make_tuple(dev_id, lane));
  if (it != ptu_devices.end()) {
    it->second->stop_event();
    ptu_devices.erase(it);
    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -1;
  }
}

int fpga_ptu_rtp(uint32_t dev_id, uint32_t lane, in_port_t rtp_sport,
                 in_port_t rtp_eport) {
  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    // enable RTP
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::RTPRX_ENA0, 0xffffffff);
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::RTPRX_ENA1, 0xffffffff);

    // set buffer address
    constexpr uint32_t RTP_BUF_OFFSET = 32UL * 2UL * 32UL * 1024UL * 1024UL;
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::RTPRX_BASE, RTP_BUF_OFFSET);

    // set RTP port
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::RTPRX_PORT, (rtp_sport << 16) | rtp_eport);

    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -1;
  }
}

int fpga_ptu_rtp_reset(uint32_t dev_id, uint32_t lane) {
  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    // reset RTP
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::RTPRX_RST0, 0xffffffff);
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::RTPRX_RST1, 0xffffffff);

    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -1;
  }
}

int fpga_ptu_listen(uint32_t dev_id, uint32_t lane, in_port_t port) {
  int ret = ptu_devices[std::make_tuple(dev_id, lane)]->listen(port);
  if (ret != 0) {
    return -1;
  }
  return 0;
}

int fpga_ptu_listen_close(uint32_t dev_id, uint32_t lane, in_port_t port) {
  return ptu_devices[std::make_tuple(dev_id, lane)]->listen_close(port);
}

int fpga_ptu_accept(uint32_t dev_id, uint32_t lane, in_port_t lport,
                    in_addr_t raddr, in_port_t rport,
                    const struct timeval *timeout, uint32_t *cid) {
  uint64_t timeout_us = 0;
  if (timeout != NULL) {
    timeout_us = timeout->tv_sec * 1000000 + timeout->tv_usec;
  }

  return ptu_devices[std::make_tuple(dev_id, lane)]->accept(
      lport, raddr, rport, timeout_us, cid);
}

int fpga_ptu_connect(uint32_t dev_id, uint32_t lane, in_port_t lport,
                     in_addr_t raddr, in_port_t rport,
                     const struct timeval *timeout, uint32_t *cid) {
  uint64_t timeout_us = 0;
  if (timeout != NULL) {
    timeout_us = timeout->tv_sec * 1000000 + timeout->tv_usec;
  }

  return ptu_devices[std::make_tuple(dev_id, lane)]->connect(
      lport, raddr, rport, timeout_us, cid);
}

int fpga_ptu_disconnect(uint32_t dev_id, uint32_t lane, uint32_t cid) {
  return ptu_devices[std::make_tuple(dev_id, lane)]->abort(cid);
}

int fpga_ptu_mod_setting(
  uint32_t dev_id,
  uint32_t lane,
  in_addr_t addr,
  in_addr_t gateway,
  uint8_t mac[6]
) {
  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u), addr(0x%x), gateway(0x%x), mac(%02x:%02x:%02x:%02x:%02x:%02x)\n",
                __func__, dev_id, lane, addr, gateway, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ptu_devices[std::make_tuple(dev_id, lane)]->modify(addr, gateway, mac);
    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }
}

int fpga_ptu_get_setting(
  uint32_t dev_id,
  uint32_t lane,
  in_addr_t *addr,
  in_addr_t *subnet,
  in_addr_t *gateway,
  uint8_t mac[6]
) {
  /* input check */
  if (!addr || !subnet || !gateway) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(addr %d, subnet %d, gateway %d) %s: Invalid argument\n", (uintptr_t)addr,
                (uintptr_t)subnet, (uintptr_t)gateway, __func__);
    return -INVALID_ARGUMENT;
  }

  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u)\n", __func__, dev_id, lane);
    *addr = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::MY_IPV4_ADDR);
    *subnet = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::MY_IPV4_SUBNET);
    *gateway = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::MY_IPV4_GATEWAY);

    uint32_t data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::MY_MAC_HI);
    mac[0] = static_cast<uint8_t>((data & 0x0000FF00) >> 8);
    mac[1] = static_cast<uint8_t>(data  & 0x000000FF);

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::MY_MAC_LO);
    mac[2] = static_cast<uint8_t>((data & 0xFF000000) >> 24);
    mac[3] = static_cast<uint8_t>((data & 0x00FF0000) >> 16);
    mac[4] = static_cast<uint8_t>((data & 0x0000FF00) >>  8);
    mac[5] = static_cast<uint8_t>(data  & 0x000000FF);

    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }
}

int fpga_ptu_set_arp_entry(
  uint32_t dev_id,
  uint32_t lane,
  in_addr_t addr,
  uint8_t mac[6]
) {
  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u), addr(0x%x), mac(%02x:%02x:%02x:%02x:%02x:%02x)\n",
                __func__, dev_id, lane, addr, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::ARP_ARG0, addr);

    uint32_t mac_hi =
        static_cast<uint32_t>(mac[0]) << 8 | static_cast<uint32_t>(mac[1]);
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::ARP_ARG1, mac_hi);

    uint32_t mac_lo = static_cast<uint32_t>(mac[2]) << 24 |
                      static_cast<uint32_t>(mac[3]) << 16 |
                      static_cast<uint32_t>(mac[4]) << 8 |
                      static_cast<uint32_t>(mac[5]);
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::ARP_ARG2, mac_lo);

    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::ARP_COMMAND, ARP_SET_ENTRY);
    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }
}

int fpga_ptu_del_arp_entry(
  uint32_t dev_id,
  uint32_t lane,
  in_addr_t addr
) {
  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u), addr(0x%x)\n", __func__, dev_id, lane, addr);
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::ARP_ARG0, addr);

    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::ARP_COMMAND, ARP_DEL_ENTRY);
    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }
}

int fpga_ptu_set_arp_retry(
  uint32_t dev_id,
  uint32_t lane,
  uint16_t timeout,
  uint16_t retry_num
) {
  /* input check */
  if ((timeout > 511) || (retry_num > 15)) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(timeout %d, retry_num %d)%s: Invalid argument\n", timeout,
                retry_num, __func__);
    return -INVALID_ARGUMENT;
  }

  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u), timeout(%u), retry_num(%u)\n", __func__, dev_id, lane, timeout, retry_num);
    uint32_t data = 0;
    data |= static_cast<uint32_t>(timeout   & 0x000001FF) << 16;
    data |= static_cast<uint32_t>(retry_num & 0x0000000F);
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::ARP_ARG0, data);

    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::ARP_COMMAND, ARP_SET_RETRY);
    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }
}

int fpga_ptu_set_arp_aging_en(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t enable_flag
) {
  /* input check */
  if (enable_flag > 1) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(enable_flag %d)%s: Invalid argument\n", enable_flag,
                __func__);
    return -INVALID_ARGUMENT;
  }

  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u), enable_flag(%u)\n", __func__, dev_id, lane, enable_flag);
    if (enable_flag == 1) {
      // enable
      ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
          PtuRegMap::ARP_COMMAND, ARP_ENA_AGE);
    } else {
      // disable
      ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
          PtuRegMap::ARP_COMMAND, ARP_DIS_AGE);
    }
    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }
}

int fpga_ptu_get_arp_status(
  uint32_t dev_id,
  uint32_t lane,
  fpga_arp_status_t *arp_status
) {
  /* input check */
  if (!arp_status) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(arp_status %lf) %s: Invalid argument\n", (uintptr_t)arp_status,
                __func__);
    return -INVALID_ARGUMENT;
  }

  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u)\n", __func__, dev_id, lane);
    uint32_t data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::ARP_STATUS);
    arp_status->sts_pmnt_rest       = static_cast<uint16_t>(data & 0x0000FFFF);
    arp_status->sts_entry_evicted   = static_cast<uint8_t>((data & 0x80000000) >> 31);
    arp_status->sts_aging_enabled   = static_cast<uint8_t>((data & 0x40000000) >> 30);
    arp_status->sts_ipv4_conflicted = static_cast<uint8_t>((data & 0x20000000) >> 29);

    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }
}

int fpga_ptu_clear_arp_status(
  uint32_t dev_id,
  uint32_t lane,
  fpga_arp_status_t arp_status
) {
  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u)\n", __func__, dev_id, lane);
    uint32_t data = 0;
    // Only sts_entry_evicted & sts_ipv4_conflicted
    data |= static_cast<uint32_t>(arp_status.sts_entry_evicted   & 0x01) << 31;
    data |= static_cast<uint32_t>(arp_status.sts_ipv4_conflicted & 0x01) << 29;
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::ARP_STATUS, data);
    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }
}

int fpga_ptu_get_arp_entry(
  uint32_t dev_id,
  uint32_t lane,
  uint8_t index,
  fpga_arp_entry_t *arp_entry
) {
  /* input check */
  if (!arp_entry) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(arp_entry %lf) %s: Invalid argument\n", (uintptr_t)arp_entry,
                __func__);
    return -INVALID_ARGUMENT;
  }
  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u), index(%u)\n", __func__, dev_id, lane, index);
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::ARP_ARG0, index);
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(
        PtuRegMap::ARP_COMMAND, ARP_DUMP_ENTRY);

    uint32_t datav = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::PTU_VERSION);
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] VERSION 0x%x\n", __func__, datav);
    uint32_t data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::ARP_ENTRY);
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] data 0x%x\n", __func__, data);
    arp_entry->dump_permanent = static_cast<uint8_t>((data  & 0x80000000) >> 31);
    arp_entry->dump_incmp     = static_cast<uint8_t>((data  & 0x40000000) >> 30);
    arp_entry->dump_used      = static_cast<uint8_t>((data  & 0x20000000) >> 29);
    arp_entry->dump_life      = static_cast<uint16_t>((data & 0x01FF0000) >> 16);
    arp_entry->dump_retry     = static_cast<uint8_t>((data  & 0x00000F00) >> 8);
    arp_entry->dump_arp_index = static_cast<uint8_t>(data   & 0x000000FF);

    uint32_t ipaddr = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::ARP_IPV4);
    arp_entry->dump_ipddr = ipaddr;

    uint32_t mac_hi = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::ARP_MAC_HI);
    arp_entry->dump_mac = static_cast<uint64_t>(mac_hi) << 32;

    uint32_t mac_lo = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::ARP_MAC_LO);
    arp_entry->dump_mac |= static_cast<uint64_t>(mac_lo);

    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }
}

int fpga_ptu_get_stat(
  uint32_t dev_id,
  uint32_t lane,
  fpga_ptu_stat_t *ptu_stat
) {
  /* input check */
  if (!ptu_stat) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(ptu_stat %lf) %s: Invalid argument\n", (uintptr_t)ptu_stat,
                __func__);
    return -INVALID_ARGUMENT;
  }
  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u)\n", __func__, dev_id, lane);
    uint32_t data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::RECV_FRAME_CNT);
    ptu_stat->recv_frame_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::RECV_RAWETH_CNT);
    ptu_stat->recv_raweth_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::DROP_RAWETH_CNT);
    ptu_stat->drop_raweth_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::RECV_RAWIP_CNT);
    ptu_stat->recv_rawip_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::DROP_RAWIP_CNT);
    ptu_stat->drop_rawip_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::RECV_UDP_CNT);
    ptu_stat->recv_tcp_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::DROP_UDP_CNT);
    ptu_stat->drop_tcp_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::RECV_TCP_CNT);
    ptu_stat->recv_tcp_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::DROP_TCP_CNT);
    ptu_stat->drop_tcp_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::DROP_TCP_CNT);
    ptu_stat->drop_tcp_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::SEND_FRAME_CNT);
    ptu_stat->send_frame_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::SEND_RAWETH_CNT);
    ptu_stat->send_raweth_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::SEND_RAWIP_CNT);
    ptu_stat->send_rawip_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::SEND_TCP_CNT);
    ptu_stat->send_tcp_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::TCP_CTL_STATUS);
    ptu_stat->tcp_ctl_status = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::RAWETH_RX_CMD_CNT);
    ptu_stat->raweth_rx_cmd_cnt = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::RAWETH_RX_LEN);
    ptu_stat->raweth_rx_len = data;

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::TCP_EVENT_MISS);
    ptu_stat->tcp_event_miss       = static_cast<uint16_t>((data & 0xFFFF0000) >> 16);
    ptu_stat->tcp_event_miss_queue = static_cast<uint16_t>(data  & 0x0000FFFF);

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::TCP_EVENT_CNT);
    ptu_stat->tcp_event_cnt   = static_cast<uint16_t>((data & 0xFFFF0000) >> 16);
    ptu_stat->tcp_event_merge = static_cast<uint16_t>(data  & 0x0000FFFF);

    data = 0;
    data = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(
        PtuRegMap::TCP_CMD_CNT);
    ptu_stat->tcp_cmd_cnt       = static_cast<uint16_t>((data & 0xFFFF0000) >> 16);
    ptu_stat->tcp_cmd_cnt_avail = static_cast<uint8_t>(data   & 0x000000FF);

    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }
}


int fpga_ptu_dump_tcb(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t cid,
  fpga_dump_tcb_t *dump_tcb
) {
  /* input check */
  if (!dump_tcb) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dump_tcb 0x%llx) %s: Invalid argument\n", (uintptr_t)dump_tcb,
                __func__);
    return -INVALID_ARGUMENT;
  }
  if (cid < 1 || cid > 511) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(cid %u) %s: Invalid argument\n", cid,
                __func__);
    return -INVALID_ARGUMENT;
  }
  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u), cid(%u)\n", __func__, dev_id, lane, cid);
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(PtuRegMap::TCP_CID, cid);
    ptu_devices[std::make_tuple(dev_id, lane)]->reg_write(PtuRegMap::TCP_COMMAND, TCP_HOST_CMD_DUMP_TCB);

    dump_tcb->tcb_usr_read = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(PtuRegMap::TCB_USR_READ);
    dump_tcb->tcb_usr_wrt  = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(PtuRegMap::TCB_USR_WRT);
    dump_tcb->tcb_snd_una  = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(PtuRegMap::TCB_SND_UNA);
    dump_tcb->tcb_snd_nxt  = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(PtuRegMap::TCB_SND_NXT);
    dump_tcb->tcb_rcv_nxt  = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(PtuRegMap::TCB_RCV_NXT);
    dump_tcb->tcb_rcv_up   = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(PtuRegMap::TCB_RCV_UP);
    dump_tcb->tcb_snd_wnd  = ptu_devices[std::make_tuple(dev_id, lane)]->reg_read(PtuRegMap::TCB_SND_WND);

    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }
}

struct fpga_ptu_wait_tcb_buffer_empty_struct {
  uint32_t dev_id;  /**< Argument for fpga_ptu_dump_tcb */
  uint32_t lane;    /**< Argument for fpga_ptu_dump_tcb */
  uint32_t cid;     /**< Argument for fpga_ptu_dump_tcb */
};

/**
 * @brief Function wrap fpga_ptu_dump_tcb() to enable __fpga_common_polling()
 * @retval 0 Success
 * @retval (>0) Error(continue polling)
 * @retval (<0) Error(stop polling)
 */
static int __fpga_ptu_wait_tcb_buffer_empty_clb(
  void *arg
) {
  struct fpga_ptu_wait_tcb_buffer_empty_struct *argument = (struct fpga_ptu_wait_tcb_buffer_empty_struct*)arg;
  fpga_dump_tcb_t dump_tcb;

  int ret = fpga_ptu_dump_tcb(argument->dev_id,
                              argument->lane,
                              argument->cid,
                              &dump_tcb);

  if (ret)
    return ret;
  else
    return (dump_tcb.tcb_usr_wrt == dump_tcb.tcb_snd_una) ? 0 : 1;
}

int fpga_ptu_wait_tcb_buffer_empty(
  uint32_t dev_id,
  uint32_t lane,
  uint32_t cid,
  const struct timeval *timeout,
  const struct timeval *interval,
  uint32_t *is_success
) {
  if (!is_success) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(is_success 0x%llx) %s: Invalid argument\n", (uintptr_t)is_success,
                __func__);
    return -INVALID_ARGUMENT;
  }
  if (cid < 1 || cid > 511) {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(cid %u) %s: Invalid argument\n", cid,
                __func__);
    return -INVALID_ARGUMENT;
  }
  if (ptu_devices.find(std::make_tuple(dev_id, lane)) != ptu_devices.end()) {
    log_libfpga(LIBFPGA_LOG_DEBUG, LIBPTU "[%s] dev_id(%u), lane(%u), cid(%u)\n", __func__, dev_id, lane, cid);

    struct fpga_ptu_wait_tcb_buffer_empty_struct clb_argument = {
      .dev_id = dev_id,
      .lane = lane,
      .cid = cid};

    int ret = __fpga_common_polling(
      timeout,
      interval,
      __fpga_ptu_wait_tcb_buffer_empty_clb,
      (void*)&clb_argument);  // NOLINT

    if (ret < 0)
      return ret;

    if (!ret)
      *is_success = true;
    else
      *is_success = false;

    return 0;
  } else {
    log_libfpga(LIBFPGA_LOG_ERROR,
                LIBPTU "(dev %u, ptu %u) %s: ptu not initialized\n", dev_id,
                lane, __func__);
    return -FAILURE_DEVICE_OPEN;
  }

  return 0;
}
