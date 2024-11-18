/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libptu.h
 * @brief Header file for APIs for PTU control
 */

#pragma once

#include <netinet/in.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fpga_arp_status {
  uint16_t sts_pmnt_rest;
  uint8_t sts_entry_evicted;    // valid range 1-bit write clear
  uint8_t sts_aging_enabled;    // valid range 1bit
  uint8_t sts_ipv4_conflicted;  // valid range 1-bit write clear
} fpga_arp_status_t;

typedef struct fpga_arp_entry {
  uint32_t dump_ipddr;
  uint64_t dump_mac;         // valid range 48bit
  uint16_t dump_life;        // valid range 9 bits
  uint8_t dump_permanent;    // valid range 1bit
  uint8_t dump_incmp;        // valid range 1bit
  uint8_t dump_used;         // valid range 1bit
  uint8_t dump_retry;        // valid range 4 bits
  uint8_t dump_arp_index;
} fpga_arp_entry_t;

typedef struct fpga_ptu_stat {
  uint32_t recv_frame_cnt;
  uint32_t recv_raweth_cnt;
  uint32_t drop_raweth_cnt;
  uint32_t recv_rawip_cnt;
  uint32_t drop_rawip_cnt;
  uint32_t recv_tcp_cnt;
  uint32_t drop_tcp_cnt;
  uint32_t send_frame_cnt;
  uint32_t send_raweth_cnt;
  uint32_t send_rawip_cnt;
  uint32_t send_tcp_cnt;
  uint32_t tcp_ctl_status;
  uint32_t raweth_rx_cmd_cnt;
  uint32_t raweth_rx_len;
  uint16_t tcp_event_miss;
  uint16_t tcp_event_miss_queue;
  uint16_t tcp_event_cnt;
  uint16_t tcp_event_merge;
  uint16_t tcp_cmd_cnt;
  uint8_t tcp_cmd_cnt_avail;
} fpga_ptu_stat_t;

typedef struct fpga_dump_tcb {
  uint32_t tcb_usr_read;
  uint32_t tcb_usr_wrt;
  uint32_t tcb_snd_una;
  uint32_t tcb_snd_nxt;
  uint32_t tcb_rcv_nxt;
  uint32_t tcb_rcv_up;
  uint32_t tcb_snd_wnd;
} fpga_dump_tcb_t;

/**
 * @brief Initialize PTU
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] addr IP address
 * @param[in] subnet Subnet mask
 * @param[in] gateway Default gateway
 * @param[in] mac MAC address
 * @return 0->success, -1->fail
 */
int fpga_ptu_init(uint32_t dev_id, uint32_t lane, in_addr_t addr,
                  in_addr_t subnet, in_addr_t gateway, const uint8_t *mac);

/**
 * @brief Terminate PTU
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @return 0->success, -1->fail
 */
int fpga_ptu_exit(uint32_t dev_id, uint32_t lane);

/**
 * @brief Set RTP parameter
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] rtp_sport Start port of RTP
 * @param[in] rtp_eport End port of RTP
 * @return 0->success, -1->fail
 */
int fpga_ptu_rtp(uint32_t dev_id, uint32_t lane, in_port_t rtp_sport,
                 in_port_t rtp_eport);

/**
 * @brief Reset RTP
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @return 0->success, -1->fail
 */
int fpga_ptu_rtp_reset(uint32_t dev_id, uint32_t lane);

/**
 * @brief TCP listen (thread safe)
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] lport Listen port
 * @return 0->success, -1->already listened
 */
int fpga_ptu_listen(uint32_t dev_id, uint32_t lane, in_port_t port);

/**
 * @brief Close listen port (thread safe)
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] lport Listen port
 * @return 0->success, -1->not listened
 */
int fpga_ptu_listen_close(uint32_t dev_id, uint32_t lane, in_port_t port);

/**
 * @brief Wait for TCP connection establishment on listen port (thread safe)
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] lport Listen port
 * @param[in] raddr Expected remote IP address
 * @param[in] rport Expected remote port
 * @param[in] timeout Timeout to wait for connection. NULL means no timeout.
 * @param[out] cid TCP connection ID
 * @return 0->success, -1->timeout, -2->connected from unexpected IP or port,
 * -3->not listen
 */
int fpga_ptu_accept(uint32_t dev_id, uint32_t lane, in_port_t lport,
                    in_addr_t raddr, in_port_t rport,
                    const struct timeval *timeout, uint32_t *cid);

/**
 * @brief Connect to remote host (thread safe)
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] lport local port
 * @param[in] raddr remote IP address
 * @param[in] rport remote port
 * @param[in] timeout Timeout to wait for connection. NULL means no timeout.
 * @param[out] cid TCP connection ID
 * @return 0->success, -1->timeout, -2->PTU error, -3->already requested but not
 * established or already established
 */
int fpga_ptu_connect(uint32_t dev_id, uint32_t lane, in_port_t lport,
                     in_addr_t raddr, in_port_t rport,
                     const struct timeval *timeout, uint32_t *cid);

/**
 * @brief Close connection (thread safe)
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] cid TCP connection ID
 * @return 0->success, -1->connection does not exist
 */
int fpga_ptu_disconnect(uint32_t dev_id, uint32_t lane, uint32_t cid);






/**
 * @brief Modify Setting
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] addr IP address
 * @param[in] gateway Default gateway
 * @param[in] mac MAC address
 * @return 0->success, -1->fail
 */
int fpga_ptu_mod_setting(uint32_t dev_id, uint32_t lane,
                         in_addr_t addr, in_addr_t gateway,
                         uint8_t mac[6]);

/**
 * @brief Get Setting
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[out] addr IP address
 * @param[out] subnet Subnet mask
 * @param[out] gateway Default gateway
 * @param[out] mac MAC address
 * @return 0->success, -1->fail
 */
int fpga_ptu_get_setting(uint32_t dev_id, uint32_t lane,
                         in_addr_t *addr, in_addr_t *subnet, in_addr_t *gateway,
                         uint8_t mac[6]);

/**
 * @brief Set ARP entry
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] addr IP address
 * @param[in] mac MAC address
 * @return 0->success, -1->fail
 */
int fpga_ptu_set_arp_entry(uint32_t dev_id, uint32_t lane, in_addr_t addr, uint8_t mac[6]);

/**
 * @brief Delete ARP entry
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] addr IP address
 * @return 0->success, -1->fail
 */
int fpga_ptu_del_arp_entry(uint32_t dev_id, uint32_t lane, in_addr_t addr);

/**
 * @brief Set ARP retry
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] timeout Timeout to wait for connection
 * @param[in] retry_num retry number
 * @return 0->success, -1->fail
 */
int fpga_ptu_set_arp_retry(uint32_t dev_id, uint32_t lane, uint16_t timeout, uint16_t retry_num);

/**
 * @brief Set ARP enable
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] enable_flag Aging enable flag
 * @return 0->success, -1->fail
 */
int fpga_ptu_set_arp_aging_en(uint32_t dev_id, uint32_t lane, uint8_t enable_flag);

/**
 * @brief Get ARP status
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[out] arp_status ARP status
 * @return 0->success, -1->fail
 */
int fpga_ptu_get_arp_status(uint32_t dev_id, uint32_t lane, fpga_arp_status_t *arp_status);

/**
 * @brief Clear ARP status
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] arp_status ARP status
 * @return 0->success, -1->fail
 */
int fpga_ptu_clear_arp_status(uint32_t dev_id, uint32_t lane, fpga_arp_status_t arp_status);

/**
 * @brief Get ARP entry
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[out] arp_entry ARP entry
 * @return 0->success, -1->fail
 */
int fpga_ptu_get_arp_entry(uint32_t dev_id, uint32_t lane, uint8_t index, fpga_arp_entry_t *arp_entry);

/**
 * @brief Get ARP entry
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[out] ptu_stat PTU stat
 * @return 0->success, -1->fail
 */
int fpga_ptu_get_stat(uint32_t dev_id, uint32_t lane, fpga_ptu_stat_t *ptu_stat);

/**
 * @brief Dump TCB
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] cid TCP connection ID
 * @param[out] dump_tcb TCB dump
 * @return 0->success, -1->fail
 */
int fpga_ptu_dump_tcb(uint32_t dev_id, uint32_t lane, uint32_t cid, fpga_dump_tcb_t *dump_tcb);

/**
 * @brief Poll TCB until TCP buffer empty
 * @param[in] dev_id Device ID
 * @param[in] lane PTU ID
 * @param[in] cid TCP connection ID
 * @param[in] timeout Timeout to wait for connection. NULL means no timeout.
 * @param[in] interval Interval to wait for connection. NULL means busy wait.
 * @param[out] is_success Result of polling(if polling success set true, otherwise set false)
 * @return 0->success, -1->fail(`is_success` is not defined)
 */
int fpga_ptu_wait_tcb_buffer_empty(uint32_t dev_id, uint32_t lane, uint32_t cid,
                                   const struct timeval *timeout, const struct timeval *interval,
                                   uint32_t *is_success);

#ifdef __cplusplus
}
#endif
