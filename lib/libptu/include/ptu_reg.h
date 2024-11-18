/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file ptu_reg.h
 * @brief Header file for definition of PTU registers and enums
 */

#pragma once

#ifdef __cplusplus
struct PtuRegMap {
#endif
  enum {
    // control registers
    INIT_PTU,
    MY_IPV4_ADDR,
    MY_IPV4_SUBNET,
    MY_IPV4_GATEWAY,
    MY_MAC_HI,
    MY_MAC_LO,
    IP_PARAM,  // | recv_any_dest_ip[24] | TTL[23:16] | IP_ID[15:0] |
    TCPRXTX_BASE,
    RAWETH_RX_ID_W,
    RAWETH_RX_ADDR,
    RAWETH_RX_START,
    RAWETH_RX_RECV,
    RAWETH_TX_ADDR,  // must be 64 bytes aligned
    RAWETH_TX_LEN,
    RAWETH_TX_ID_W,
    RAWETH_TX_START,  // [0] start, [1] error
    RAWETH_TX_RECV,
    RAWIP_RX_ID_W,  // [31:0] id, write=start receiving
    RAWIP_RX_ADDR,  // must be 64 bytes aligned
    RAWIP_TX_DST_IP,
    RAWIP_TX_PORT,  // [31:16] src_port, [15:0] dst_port for UDP
    RAWIP_TX_ADDR,
    RAWIP_TX_PROT_LEN,  // [23:16] protocol of ip, [15:0] transfer bytes
    RAWIP_TX_ID_W,  // [31:0] id, write=start sending
    TCP_SET_ISS,  // set initial sending sequence number
    TCP_CWND,  // [31] disable congestion control, [3:0] initial congestion window size (default: 10)
    TCP_SEND_TIMER,  // [15:10] accumulate threshold for ack response, [9:0] delayed ack timer (msec)
    TCP_RET_TIMER,    // [29:20] retransmit timer (x10msec), [17:4] fin timer (x10msec), [3] timer is doubled by each retransmit, [2:0] number of retry including first send
    TCP_RET_SIZE,     // [11] fast retransmit all unacked data, [10:6] fast retransmit size = mss x 2^fast_ret_size, [5] retransmit all unacked data, [4:0] retransmit size = mss x 2^ret_size
    TCP_REMOTE_IP,
    TCP_REMOTE_PORT,
    TCP_LOCAL_PORT,  // | listen_port[31:16] | local_port[15:0] |
    TCP_CID,
    TCP_USR_REL_SEQ,  // usr_read when read data, or usr_wrt when write data
    TCP_COMMAND,    // user_send=1, cnn_open=2, cnn_close=3, lsn_open=4, lsn_close=5, read_data=6, write_data=8, write_urg=9, abort=10, release=11, dump_tcb=12, init_lsn_tbl=16
    ARP_COMMAND,  // set_entry=1, del_entry=2, set_retry_life=3, disable_age=4, enable_age=5, dump_entry=6
    ARP_ARG0,  // command 1:ipv4_addr, 3:[24:16] max_life,[3:0]retry_count, 6:arp_index
    ARP_ARG1,  // command 1:mac_hi
    ARP_ARG2,  // command 1:mac_lo
    // option feature ctrl registers
    RTPRX_BASE,
    RTPRX_PORT,  // port range received as RTP, [31:16]: start_port, [15:0]: end_port
    RTPRX_ENA0,
    RTPRX_ENA1,
    RTPRX_RST0,
    RTPRX_RST1,

    REG_MAX_CTL_IDX,

    // status registers
    PTU_VERSION,  // [31:24]: major version, [23:16]: minor version, [15:8]: patch version, [7:0]: reserved
    PTU_FEATURE,  // [20:16]: TCP buffer size bit-width, features enable [4]:RTP [3]:ARP [2]:ping [1]:RawIpTx [0]:TCP
    PTU_EVENT,    // interrupt event source [0]: TCP, [1]: RawIP recv, [2]: RawIP send, [3]: RawEth recv, [4]: RawEth send
    TCP_EVENT_QUE,  // [31:16] cid for event, [15:0] event factors
    TCP_EVENT_LOCAL_IP,  // corresponding local ip of notified TCP event
    TCP_EVENT_REMOTE_IP,  // corresponding remote ip of notified TCP event
    TCP_EVENT_PORT,  // [31:16] local_port, [15:0] remote_port of notified TCP event
    TCP_EVENT_REL_SND_UNA,  // relative send unacked sequence number at notified event
    TCP_EVENT_REL_RCV_NXT,  // relative receive next read sequence number at notified event
    TCP_EVENT_REL_RCV_UP,  // relative receive urgent pointer at notified event
    TCP_EVENT_MISS,  // [31:16] missed tcp event count, [15:0] current tcp event number
    TCP_EVENT_CNT,  // [31:16] total tcp event count, [15:0] merged tcp event number
    TCP_CMD_CNT,  // [31:16] missed tcp command count, [7:0] current issueable command number
    RECV_FRAME_CNT,
    RECV_RAWETH_CNT,
    DROP_RAWETH_CNT,
    RECV_RAWIP_CNT,
    DROP_RAWIP_CNT,
    RECV_UDP_CNT,
    DROP_UDP_CNT,
    RECV_TCP_CNT,
    DROP_TCP_CNT,
    SEND_FRAME_CNT,
    SEND_RAWETH_CNT,
    SEND_RAWIP_CNT,
    SEND_TCP_CNT,
    TCP_CTL_STATUS,
    RAWETH_RX_CMD_CNT,
    RAWETH_RX_LEN,
    RAWETH_RX_ID_R,
    RAWETH_TX_CMD_CNT,
    RAWETH_TX_ID_R,
    RAWIP_RX_CMD_CNT,  // current issueable command number
    RAWIP_RX_PROT_LEN,  // [23:16] upper protocol, [15:0] bytes
    RAWIP_RX_SRC_IP,  // [31:0] source IP address for UDP
    RAWIP_RX_PORT,  // [31:16] src_port, [15:0] dst_port for UDP
    RAWIP_RX_ID_R,  // write=acknoledge
    RAWIP_TX_CMD_CNT,  // current issueable command number
    RAWIP_TX_ID_R,  // write=acknoledge
    RAWIP_TX_STATUS,  // write=retry
    ARP_STATUS,  // [0]arp entry evicted
    ARP_ENTRY,  // [31:23]life, [22:19]retry, [18]permanent, [17]incmp, [16]used, [7:0]arp_index
    ARP_IPV4,  // [31:0] arp ipv4 address
    ARP_MAC_HI,  // [15:0] high bits of MAC address
    ARP_MAC_LO,  // [31:0] low bits of MAC address
    // Tcp control block status
    TCB_USR_READ,
    TCB_USR_WRT,
    TCB_SND_UNA,
    TCB_SND_NXT,
    TCB_RCV_NXT,
    TCB_RCV_UP,
    TCB_SND_WND,
    TCB_CNG_CNN,  // [31:16] congestion winodw, [7:4] congestion state, [3:0] connection state
    TCB_OOO_SEQ0,
    TCB_OOO_SEQ1,
    TCB_OOO_SIZE0,
    TCB_OOO_SIZE1,
    // option feature status registers
    RECV_RTP_CNT,
    DROP_RTP_CNT,
    ILL_ORD_RTP_CNT,
    RTP_ORUN0,
    RTP_ORUN1,
    DELIM_CNT0,
    DELIM_CNT1,
    DELIM_CNT2,
    DELIM_CNT3,
    DELIM_CNT4,
    DELIM_CNT5,
    // status for debug
    RXBUF_STATUS,  // [31:16]: once use, [15:0]: in use
    WDMA_CNT,
    WDMA_ERR_CNT,
    WDMA_CYCLES,  // [31:16] maximum wdmac cycles, [15:0] last wdmac cycles
    RDMA_CNT,
    RDMA_ERR_CNT,
    RDMA_CYCLES,  // [31:16] maximum rdmac cycles, [15:0] last rdmac cycles
    TCP_MODE0,  // [31:28] tcbtxwt, [27:24] tcpsend, [23:20] cnnstttx, [19:16] tcbtxrd, [15:12] tcbrxwt, [11:8] tcprecv, [7:4] cnnsttrx, [3:0] tcbrxrd
    TCP_SEND_CYCLES,
    TCP_GEN_CYCLES,
    RET_TIMER_CYCLES,
    BRIDGE_DROP_CNT,
    REG_MAX_IDX
  };
#ifdef __cplusplus
};
#endif

  enum {
    PTU_EVE_TCP         = 0b1,
    PTU_EVE_RAWIP_RECV  = 0b10,
    PTU_EVE_RAWIP_SEND  = 0b100,
    PTU_EVE_RAWETH_RECV = 0b1000,
    PTU_EVE_RAWETH_SEND = 0b10000
  };

  enum {
    TCP_HOST_CMD_USR_SEND     = 1,
    TCP_HOST_CMD_CNN_OPEN     = 2,
    TCP_HOST_CMD_CNN_CLOSE    = 3,
    TCP_HOST_CMD_LSN_OPEN     = 4,
    TCP_HOST_CMD_LSN_CLOSE    = 5,
    TCP_HOST_CMD_READ         = 6,
    TCP_HOST_CMD_WRITE        = 8,
    TCP_HOST_CMD_WRITE_URG    = 9,
    TCP_HOST_CMD_ABORT        = 10,
    TCP_HOST_CMD_RELEASE      = 11,
    TCP_HOST_CMD_DUMP_TCB     = 12,
    TCP_HOST_CMD_INIT_LSN_TBL = 16
  };

  enum {
    TCP_EVE_ESTABLISHED      = 0b1,           // connection established
    TCP_EVE_CLOSE_WAIT       = 0b10,          // received fin flag and waiting close command
    TCP_EVE_DISCONNECT       = 0b100,         // disconnected
    TCP_EVE_SYN_TIMEOUT      = 0b1000,        // timeout during sending SYN
    TCP_EVE_SYN_ACK_TIMEOUT  = 0b10000,       // timeout during sending SYN_ACK
    TCP_EVE_TIMEOUT          = 0b100000,      // timeout after connection established
    TCP_EVE_RECV_DATA        = 0b1000000,     // received data
    TCP_EVE_SEND_DATA        = 0b10000000,    // received ack for sent data
    TCP_EVE_RECV_URGENT_DATA = 0b100000000,   // received urgent data (urg flag effective)
    TCP_EVE_RECV_RST         = 0b1000000000,  // received RST segment
    TCP_EVE_INVLD_CONNECTION = 0b10000000000  // access to disconnected connection
  };

  enum {
    ARP_SET_ENTRY = 0x1,
    ARP_DEL_ENTRY = 0x2,
    ARP_SET_RETRY = 0x3,
    ARP_DIS_AGE = 0x4,
    ARP_ENA_AGE = 0x5,
    ARP_DUMP_ENTRY = 0x6
  };

