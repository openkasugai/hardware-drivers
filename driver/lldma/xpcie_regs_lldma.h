/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/
/**
 * @file xpcie_regs_lldma.h
 * @brief Header file for register map for LLDMA module
 */

#ifndef __LLDMA_XPCIE_REGS_LLDMA_H__
#define __LLDMA_XPCIE_REGS_LLDMA_H__

#include "../xpcie_device.h"


// LLDMA Register Map : 0x0001_0000-0x0001_FFFF
#define XPCIE_FPGA_LLDMA_OFFSET                            0x00010000 /**< Base address for LLDMA */
#define XPCIE_FPGA_LLDMA_SIZE                              0x10000    /**< Size of LLDMA */

#define XPCIE_FPGA_LLDMA_FPGA_INFO                         0x0040

#define XPCIE_FPGA_LLDMA_RXCH_AVAIL                        0x0200
#define XPCIE_FPGA_LLDMA_RXCH_ACTIVE                       0x0204
#define XPCIE_FPGA_LLDMA_RXCH_SEL                          0x020C
#define XPCIE_FPGA_LLDMA_RXCH_CTRL0                        0x0210
#define XPCIE_FPGA_LLDMA_RXCH_CTRL1                        0x0214
#define XPCIE_FPGA_LLDMA_ENQ_CTRL                          0x0220
#define XPCIE_FPGA_LLDMA_ENQ_ADDR_DN                       0x0228
#define XPCIE_FPGA_LLDMA_ENQ_ADDR_UP                       0x022C
#define XPCIE_FPGA_LLDMA_RBUF_WP                           0x0230
#define XPCIE_FPGA_LLDMA_RBUF_RP                           0x0234
#define XPCIE_FPGA_LLDMA_RBUF_ADDR_DN                      0x0238
#define XPCIE_FPGA_LLDMA_RBUF_ADDR_UP                      0x023C
#define XPCIE_FPGA_LLDMA_RBUF_SIZE                         0x0240

#define XPCIE_FPGA_LLDMA_TXCH_AVAIL                        0x0400
#define XPCIE_FPGA_LLDMA_TXCH_ACTIVE                       0x0404
#define XPCIE_FPGA_LLDMA_TXCH_SEL                          0x040C
#define XPCIE_FPGA_LLDMA_TXCH_CTRL0                        0x0410
#define XPCIE_FPGA_LLDMA_TXCH_CTRL1                        0x0414
#define XPCIE_FPGA_LLDMA_DEQ_CTRL                          0x0420
#define XPCIE_FPGA_LLDMA_DEQ_ADDR_DN                       0x0428
#define XPCIE_FPGA_LLDMA_DEQ_ADDR_UP                       0x042C
#define XPCIE_FPGA_LLDMA_SBUF_WP                           0x0430
#define XPCIE_FPGA_LLDMA_SBUF_RP                           0x0434
#define XPCIE_FPGA_LLDMA_SBUF_ADDR_DN                      0x0438
#define XPCIE_FPGA_LLDMA_SBUF_ADDR_UP                      0x043C
#define XPCIE_FPGA_LLDMA_SBUF_SIZE                         0x0440

// Chain InterFace down Register Definitions
#define XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_L(lane) (\
                                           ((lane) == 0) ? 0x0600 : \
                                           ((lane) == 1) ? 0x0608 : \
                                           ((lane) == 2) ? 0x0610 : \
                                                           0x0618)
#define XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_H(lane)            (XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_L(lane) + 0x4)
#define XPCIE_FPGA_LLDMA_CIF_DN_RX_DDR_SIZE                0x0680

// Chain InterFace up Register Definitions
#define XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_L(lane) (\
                                           ((lane) == 0) ? 0x0800 : \
                                           ((lane) == 1) ? 0x0808 : \
                                           ((lane) == 2) ? 0x0810 : \
                                                           0x0818)
#define XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_H(lane)            (XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_L(lane) + 0x4)
#define XPCIE_FPGA_LLDMA_CIF_UP_TX_DDR_SIZE                0x0880

// LLDMA debug register
#define XPCIE_FPGA_LLDMA_REQUEST_SIZE_OFFSET               0x1900

// Queue Control Register Definitions
#define __IS_RX_DMA(dir) ((dir) == DMA_HOST_TO_DEV || (dir) == DMA_D2D_RX || (dir) == DMA_D2D_D_RX || (dir) == DMA_NW_TO_DEV)
#define XPCIE_FPGA_LLDMA_CH_AVAIL(dir)    (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_RXCH_AVAIL \
                                                            : XPCIE_FPGA_LLDMA_TXCH_AVAIL)
#define XPCIE_FPGA_LLDMA_CH_ACTIVE(dir)   (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_RXCH_ACTIVE \
                                                            : XPCIE_FPGA_LLDMA_TXCH_ACTIVE)
#define XPCIE_FPGA_LLDMA_CH_SEL(dir)      (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_RXCH_SEL \
                                                            : XPCIE_FPGA_LLDMA_TXCH_SEL)
#define XPCIE_FPGA_LLDMA_CH_CTRL0(dir)    (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_RXCH_CTRL0 \
                                                            : XPCIE_FPGA_LLDMA_TXCH_CTRL0)
#define XPCIE_FPGA_LLDMA_CH_CTRL1(dir)    (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_RXCH_CTRL1 \
                                                            : XPCIE_FPGA_LLDMA_TXCH_CTRL1)
#define XPCIE_FPGA_LLDMA_Q_CTRL(dir)      (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_ENQ_CTRL \
                                                            : XPCIE_FPGA_LLDMA_DEQ_CTRL)
#define XPCIE_FPGA_LLDMA_Q_ADDR_DN(dir)   (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_ENQ_ADDR_DN \
                                                            : XPCIE_FPGA_LLDMA_DEQ_ADDR_DN)
#define XPCIE_FPGA_LLDMA_Q_ADDR_UP(dir)   (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_ENQ_ADDR_UP \
                                                            : XPCIE_FPGA_LLDMA_DEQ_ADDR_UP)
#define XPCIE_FPGA_LLDMA_BUF_WP(dir)      (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_RBUF_WP \
                                                            : XPCIE_FPGA_LLDMA_SBUF_WP)
#define XPCIE_FPGA_LLDMA_BUF_RP(dir)      (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_RBUF_RP \
                                                            : XPCIE_FPGA_LLDMA_SBUF_RP)
#define XPCIE_FPGA_LLDMA_BUF_ADDR_DN(dir) (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_RBUF_ADDR_DN \
                                                            : XPCIE_FPGA_LLDMA_SBUF_ADDR_DN)
#define XPCIE_FPGA_LLDMA_BUF_ADDR_UP(dir) (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_RBUF_ADDR_UP \
                                                            : XPCIE_FPGA_LLDMA_SBUF_ADDR_UP)
#define XPCIE_FPGA_LLDMA_BUF_SIZE(dir)    (__IS_RX_DMA(dir) ? XPCIE_FPGA_LLDMA_RBUF_SIZE \
                                                            : XPCIE_FPGA_LLDMA_SBUF_SIZE)

// Chain InterFace down Register Setting Data Definitions
#define XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_VAL_L(lane)  (\
                                            ((lane) == 0) ? 0x00000000 : \
                                            ((lane) == 1) ? 0x00800000 : \
                                            ((lane) == 2) ? 0x01000000 : \
                                                            0x01800000)
#define XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_VAL_H(lane)         0x00000000
#define XPCIE_FPGA_LLDMA_CIF_DN_RX_DDR_SIZE_VAL             0x00000000

// Chain InterFace up Register Setting Data Definitions
#define XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_VAL_L(lane)  (\
                                            ((lane) == 0) ? 0x02000000 : \
                                            ((lane) == 1) ? 0x02800000 : \
                                            ((lane) == 2) ? 0x03000000 : \
                                                            0x03800000)
#define XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_VAL_H(lane)         0x00000000
#define XPCIE_FPGA_LLDMA_CIF_UP_TX_DDR_SIZE_VAL             0x00000000

// Queue Control Commands Definitions
#define XPCIE_FPGA_LLDMA_ENABLE_IE                          0x00000001  // 0b0001
#define XPCIE_FPGA_LLDMA_ENABLE_OE                          0x00000002  // 0b0010
#define XPCIE_FPGA_LLDMA_ENABLE_CLEAR                       0x00000004  // 0b0100
#define XPCIE_FPGA_LLDMA_ENABLE_BUSY                        0x00000008  // 0b1000

#define XPCIE_FPGA_LLDMA_CH_CTRL1_INIT                      0x00000000  // mode:initial status
#define XPCIE_FPGA_LLDMA_CH_CTRL1_HOST                      0x00000000  // mode:enq/deq
#define XPCIE_FPGA_LLDMA_CH_CTRL1_D2D_H                     0x00000001  // mode:d2d-h
#define XPCIE_FPGA_LLDMA_CH_CTRL1_D2D_D                     0x00000002  // mode:d2d-d

#endif  /* __LLDMA_XPCIE_REGS_LLDMA_H__ */
