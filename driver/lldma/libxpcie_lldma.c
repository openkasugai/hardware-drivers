/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include <linux/delay.h>

#include "libxpcie_lldma.h"
#include "xpcie_regs_lldma.h"


/**
 * Definition for alignment to the smaller side
 */
#define ALIGN_FLOOR(val, align) \
  (typeof(val))((val) & (~((typeof(val))((align) - 1))))

/**
 * Definition for alignment to the higher side
 */
#define ALIGN_CEIL(val, align)  \
  ALIGN_FLOOR(((val) + ((typeof(val))(align) - 1)), align)

/**
 * Definition for cache line size
 */
#define CACHE_LINE_SIZE 64


int
xpcie_fpga_common_get_lldma_module_info(
  fpga_dev_info_t *dev)
{
  fpga_module_info_t *info = &dev->mods.lldma;

  // Set Module base address
  info->base = XPCIE_FPGA_LLDMA_OFFSET;

  // Set Module length per 1 lane
  info->len = XPCIE_FPGA_LLDMA_SIZE;

  // Set Module num
  info->num = 1;

  // Get channel num
  xpcie_fpga_count_available_dma_channel(dev);

  return 0;
}


uint32_t
xpcie_fpga_get_version(
  fpga_dev_info_t *dev)
{
  return lldma_reg_read(dev, XPCIE_FPGA_LLDMA_FPGA_INFO);
}


int
queue_que_init(
  fpga_queue_enqdeq_t *qp,
  uint16_t size)
{
  uint64_t q_size, alloc_size;
  void *ptr;

  // Calculate queue area size with 64-bytes aligned
  q_size = sizeof(fpga_queue_t) + sizeof(fpga_desc_t) * size;
  q_size = ALIGN_CEIL(q_size, CACHE_LINE_SIZE);

  // Calculate allocating size with PAGE_SIZE aligned
  alloc_size = ALIGN_CEIL(q_size, PAGE_SIZE);

  // Allocate command queue on continuous memory
  ptr = kzalloc(alloc_size, GFP_DMA);
  if (ptr == NULL)
    return -ENOMEM;

  // Alignement command queue's offset by Page_size
  qp->que = (fpga_queue_t *)ALIGN_CEIL((uintptr_t)ptr, PAGE_SIZE);

  // Set information with command queue
  qp->que->size = size;           // Num of descriptor
  qp->status = FPGA_Q_STAT_FREE;
  qp->qp_mem_addr = ptr;
  qp->qp_mem_size = q_size;       // Command queue's size

  return 0;
}


void
queue_que_free(fpga_queue_enqdeq_t *qp)
{
  if (qp->qp_mem_addr != 0) {
    kfree(qp->qp_mem_addr);
  }
  memset(qp, 0, sizeof(fpga_queue_enqdeq_t));
}



int
xpcie_fpga_get_queue(
  fpga_dev_info_t *dev,
  fpga_ioctl_queue_t *ioctl_queue)
{
  fpga_queue_enqdeq_t *queue_info;
  fpga_queue_t *command_queue;

  xpcie_trace("%s: dir(%d), chid(%d), connector_id(%s)",
    __func__, ioctl_queue->dir, ioctl_queue->chid, ioctl_queue->connector_id);

  // Check direction of DMA
  switch (ioctl_queue->dir) {
  case DMA_HOST_TO_DEV:
  case DMA_NW_TO_DEV:
    queue_info = &dev->enqueues[ioctl_queue->chid];
    break;
  case DMA_DEV_TO_HOST:
  case DMA_DEV_TO_NW:
    queue_info = &dev->dequeues[ioctl_queue->chid];
    break;
  default:
    return -EINVAL;
  }

  // Check command queue's status and Get if free
  mutex_lock(&dev->queue_mutex);
  if (queue_info->status != FPGA_Q_STAT_FREE) {
    mutex_unlock(&dev->queue_mutex);
    return -EBUSY;
  }
  queue_info->status = FPGA_Q_STAT_USED;
  mutex_unlock(&dev->queue_mutex);

  // Initialize command queue
  command_queue = queue_info->que;
  memset(command_queue->ring, 0, sizeof(fpga_desc_t) * command_queue->size);

  // Start polling command queue
  xpcie_fpga_start_queue(dev, ioctl_queue->chid, ioctl_queue->dir);

  // Get command queue's size for user mmap(Not used now)
  ioctl_queue->map_size = queue_info->qp_mem_size;

  // Set connector_id at command queue
  strcpy(queue_info->connector_id, ioctl_queue->connector_id);

  return 0;
}


int
xpcie_fpga_put_queue(
  fpga_dev_info_t *dev,
  fpga_ioctl_queue_t *ioctl_queue)
{
  int ret;
  xpcie_trace("%s: dir(%d), chid(%d)", __func__, ioctl_queue->dir, ioctl_queue->chid);

  // Stop polling command queue and clean dma channel
  ret = xpcie_fpga_stop_queue(dev, ioctl_queue->chid, ioctl_queue->dir);
  if (ret < 0)
    return ret;

  // Put command queue(always 0)
  if (xpcie_fpga_put_queue_info(dev, ioctl_queue->chid, ioctl_queue->dir) < 0)
    return -EBUSY;

  return 0;
}


/**
 * @brief Function which get command queue matching connector_id at target direction
 * @param[in] dev
 *   Target FPGA's information
 * @param[in] connector_id
 *   Matching key for target dma channel
 * @param[in] dir
 *   Matching target direction of dma
 */
static int
xpcie_fpga_scan_queue(
  fpga_dev_info_t *dev,
  char * connector_id,
  uint16_t dir)
{
  int chid;
  fpga_queue_enqdeq_t *queue_info;

  xpcie_trace("%s: dir(%d), connector_id(%s)", __func__, dir, connector_id);

  for(chid = 0; chid < dev->available_dma_channel_num; chid++) {
    // Check direction of dma
    switch (dir) {
    case DMA_HOST_TO_DEV:
      queue_info = &dev->enqueues[chid];
      break;
    case DMA_DEV_TO_HOST:
      queue_info = &dev->dequeues[chid];
      break;
    default:
      return -EINVAL;
    }

    // Check if 'connector_id of user' equals to 'connector_id of command queue'
    if (strcmp(queue_info->connector_id, connector_id) == 0)
      return chid;
  }

  return -EBUSY;
}


int
xpcie_fpga_put_queue_info(
  fpga_dev_info_t *dev,
  uint16_t chid,
  uint16_t dir)
{
  fpga_queue_enqdeq_t *queue_info;

  xpcie_trace("%s: dir(%d), chid(%d)", __func__, dir, chid);

  // check direction of DMA
  switch (dir) {
  case DMA_HOST_TO_DEV:
  case DMA_NW_TO_DEV:
  case DMA_D2D_RX:
  case DMA_D2D_D_RX:
    queue_info = &dev->enqueues[chid];
    break;
  case DMA_DEV_TO_HOST:
  case DMA_DEV_TO_NW:
  case DMA_D2D_TX:
  case DMA_D2D_D_TX:
    queue_info = &dev->dequeues[chid];
    break;
  default:
    return -EINVAL;
  }

  mutex_lock(&dev->queue_mutex);

  // Clear connector_id of command queue
  memset(queue_info->connector_id, 0, sizeof(queue_info->connector_id));

  // Put command queue
  queue_info->status = FPGA_Q_STAT_FREE;

  mutex_unlock(&dev->queue_mutex);
  return 0;
}


int
xpcie_fpga_ref_queue(
  fpga_dev_info_t *dev,
  fpga_ioctl_queue_t *ioctl_queue)
{
  int chid;

  xpcie_trace("%s: dir(%d), connector_id(%s)", __func__, ioctl_queue->dir, ioctl_queue->connector_id);

  // Check DMA_RX's command queue's connector_id
  chid = xpcie_fpga_scan_queue(dev, ioctl_queue->connector_id, DMA_HOST_TO_DEV);
  if (chid < 0) {
    // Check DMA_TX's command queue's connector_id, when failed to get as DMA_RX
    chid = xpcie_fpga_scan_queue(dev, ioctl_queue->connector_id, DMA_DEV_TO_HOST);
    if (chid < 0) {
//      xpcie_err("%s : Not found connector_id = %s", __func__, ioctl_queue->connector_id);
      return chid;
    }
    // Get command queue's info as DMA_TX
    ioctl_queue->dir = DMA_DEV_TO_HOST;
    ioctl_queue->map_size = dev->dequeues[chid].qp_mem_size;
  } else {
    // Get command queue's info as DMA_RX
    ioctl_queue->dir = DMA_HOST_TO_DEV;
    ioctl_queue->map_size = dev->enqueues[chid].qp_mem_size;
  }
  ioctl_queue->chid = chid;
  return 0;
}


/**
 * @brief Function which set the haed address of command queue
 */
static void
queue_set_addr(
  fpga_dev_info_t *dev,
  uint16_t chid,
  uint16_t dir,
  uint64_t addr)  // 'addr' uses only when D2D
{
  uint64_t phys_addr;
  uint32_t phys_addr32_lo;
  uint32_t phys_addr32_hi;

  xpcie_trace("%s: dir(%d), chid(%d), addr(%#llx)", __func__, dir, chid, (uint64_t)addr);

  // Check input to avoid exceeding array
  if (chid >= dev->available_dma_channel_num)
    return;

  // Check direction of dma and Get phys_addr to set
  switch(dir){
  case DMA_HOST_TO_DEV:
    phys_addr = virt_to_phys(dev->enqueues[chid].que->ring);
    break;
  case DMA_DEV_TO_HOST:
    phys_addr = virt_to_phys(dev->dequeues[chid].que->ring);
    break;
  case DMA_D2D_RX:
  case DMA_D2D_D_RX:
  case DMA_D2D_TX:
  case DMA_D2D_D_TX:
    if(!addr)return;
    phys_addr = addr;
    break;
  case DMA_NW_TO_DEV:
  case DMA_DEV_TO_NW:
  default:
    // do nothing
    return;
  }

  // Select dma channel
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_SEL(dir), chid);

  // Divide input's 64bit value into upper 32bit and lower 32bit 
  phys_addr32_lo = (uint32_t)(phys_addr & 0xffffffff);
  phys_addr32_hi = (uint32_t)((phys_addr >> 32) & 0xffffffff);

  // Write upper 32bit and lower 32bit 
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_Q_ADDR_UP(dir), phys_addr32_hi);
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_Q_ADDR_DN(dir), phys_addr32_lo);
}


/**
 * @brief Function which control command queue's status
 */
static void
queue_set_ctrl(
  fpga_dev_info_t *dev,
  uint16_t chid,
  uint16_t dir,
  uint8_t cid)
{
  uint32_t q_ctrl   = 0;
  uint32_t ch_ctrl1 = XPCIE_FPGA_LLDMA_CH_CTRL1_HOST;

  xpcie_trace("%s: dir(%d), chid(%d), cid(%d)", __func__, dir, chid, cid);

  // Check input to avoid exceeding array
  if (chid >= dev->available_dma_channel_num)
    return;

  // Check direction of dma and Create values for writing to register
  switch(dir){
  case DMA_HOST_TO_DEV:
    q_ctrl |= dev->enqueues[chid].que->size << 8;
    break;
  case DMA_DEV_TO_HOST:
    q_ctrl |= dev->dequeues[chid].que->size << 8;
    break;
  case DMA_D2D_RX:
  case DMA_D2D_TX:
    if(cid == (uint8_t)-1)return;
    ch_ctrl1 = XPCIE_FPGA_LLDMA_CH_CTRL1_D2D_H;
    q_ctrl |= cid << 8;
    break;
  case DMA_D2D_D_RX:
  case DMA_D2D_D_TX:
    if(cid == (uint8_t)-1)return;
    ch_ctrl1 = XPCIE_FPGA_LLDMA_CH_CTRL1_D2D_D;
    q_ctrl |= cid << 8;
    break;
  case DMA_NW_TO_DEV:
  case DMA_DEV_TO_NW:
  default:
    // do nothing
    return;
  }

  // Select dma channel
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_SEL(dir), chid);

  // desc_num/peer_cid
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_Q_CTRL(dir), q_ctrl);

  // Set type of dma
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_CTRL1(dir), ch_ctrl1);
}


void
xpcie_fpga_start_queue(
  fpga_dev_info_t *dev,
  uint16_t chid,
  uint16_t dir)
{
  uint32_t data, enable;
  fpga_queue_enqdeq_t *queue_info;

  xpcie_trace("%s: dir(%d), chid(%d)", __func__, dir, chid);

  // Check input to avoid exceeding array
  if (chid >= dev->available_dma_channel_num)
    return;

  // Check direction of dma
  switch(dir){
  case DMA_HOST_TO_DEV:
  case DMA_NW_TO_DEV:
    queue_info = &dev->enqueues[chid];
    break;
  case DMA_DEV_TO_HOST:
  case DMA_DEV_TO_NW:
    queue_info = &dev->dequeues[chid];
    break;
  default:
    return;
  }

  // Set command queue's address which dma channel do polling
  queue_set_addr(dev, chid, dir, 0);

  // Set dma type as transfer with host and descriptor num
  queue_set_ctrl(dev, chid, dir, (uint8_t)-1);

  // Get read/write pointer
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_SEL(dir), chid);
  data = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_Q_CTRL(dir));
  queue_info->que->writehead = (data & 0xFF000000) >> 24;
  queue_info->que->readhead = (data & 0xFF000000) >> 24;

  // Start to polling
  switch(dir){
  case DMA_HOST_TO_DEV:
  case DMA_DEV_TO_HOST:
    enable = XPCIE_FPGA_LLDMA_ENABLE_IE | XPCIE_FPGA_LLDMA_ENABLE_OE; // 0b11
    lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_CTRL0(dir), enable);
    break;
  case DMA_DEV_TO_NW:
    enable = XPCIE_FPGA_LLDMA_ENABLE_IE; // 0b01
    lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_CTRL0(dir), enable);
    break;
  case DMA_NW_TO_DEV:
    enable = XPCIE_FPGA_LLDMA_ENABLE_OE; // 0b10
    lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_CTRL0(dir), enable);
    break;
  default:
    break;
  }

  return;
}


/**
 * @brief Function which do setting for buffer of D2D-H
 */
void
xpcie_fpga_set_buf(
  fpga_dev_info_t *dev,
  uint32_t dir,
  uint32_t buf_size,
  uint64_t buf_addr)
{
  uint32_t addr_lo = (uint32_t)(buf_addr & 0xFFFFFFFF);
  uint32_t addr_hi = (uint32_t)((buf_addr >> 32) & 0xFFFFFFFF);

  xpcie_trace("%s: dir(%d), buf_size(%#x), buf_addr(%#llx)",
    __func__, dir, buf_size, buf_addr);

  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_BUF_ADDR_DN(dir), addr_lo);
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_BUF_ADDR_UP(dir), addr_hi);
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_BUF_SIZE(dir), buf_size);
}


/**
 * @brief Function which do setting to deactivate DMA channel
 * @note
 *   @li XPCIE_FPGA_LLDMA_CH_CTRL0(*)[15:4]=n/a
 *   @li XPCIE_FPGA_LLDMA_CH_CTRL0(*)[3]=busy  : 0(free)/1(busy)  :R
 *   @li XPCIE_FPGA_LLDMA_CH_CTRL0(*)[2]=clear : 0(noop)/1(clear) :RW
 *   @li XPCIE_FPGA_LLDMA_CH_CTRL0(*)[1]=oe    : 0(close)/1(open) :RW
 *   @li XPCIE_FPGA_LLDMA_CH_CTRL0(*)[0]=ie    : 0(close)/1(open) :RW
 */
int
xpcie_fpga_stop_queue(
  fpga_dev_info_t *dev,
  uint16_t chid,
  uint16_t dir)
{
  uint32_t ctrl0;
  uint32_t monitor_val;
  int cnt;
  int wait_msec = 100;  // 100[ms]
  xpcie_trace("%s: dir(%d), chid(%d)", __func__, dir, chid);

  // Select channel
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_SEL(dir), chid);

  // ie/oe off
  ctrl0 = 0;
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_CTRL0(dir), ctrl0);  // 0b0000

  // Wait finishing pipe drain
  for(cnt = 0; cnt < (FPGA_DRAIN_POLLING_MS/wait_msec); cnt++){
    monitor_val = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_CH_CTRL0(dir));
    // Check ch_busy and ch_clear's bits are 0
    if (!monitor_val) {
      // clearing desc finished
      break;
    }
    msleep(wait_msec);
  }

  // clear on
  ctrl0 = XPCIE_FPGA_LLDMA_ENABLE_CLEAR;
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_CTRL0(dir), ctrl0);  // 0b0100

  // Check if Clearing pipe is success
  if(cnt == FPGA_DRAIN_POLLING_MS/wait_msec)
    return -EFAULT; // Timeout to check pipe drain

  return 0;
}


int
xpcie_fpga_dev_connect(
  fpga_dev_info_t *dev,
  uint32_t self_chid,
  uint32_t peer_chid,
  uint16_t dir,
  uint64_t peer_addr,
  uint32_t buf_size,
  uint64_t buf_addr,
  char *connector_id
){
  uint8_t cid;
  uint32_t enable;
  fpga_queue_enqdeq_t *queue_info;

  xpcie_trace("%s: self_chid(%d), peer_chid(%d), dir(%d), peer_addr(%#llx), buf_size(%#x), buf_addr(%#llx), connector_id(%s)",
    __func__, self_chid, peer_chid, dir, peer_addr, buf_size, buf_addr, connector_id);

  switch(dir){
  case DMA_D2D_RX:
  case DMA_D2D_D_RX:
    queue_info = &dev->enqueues[self_chid];
    break;
  case DMA_D2D_TX:
  case DMA_D2D_D_TX:
    queue_info = &dev->dequeues[self_chid];
    break;
  default:
    xpcie_err("%s error! Invalid direction = %d", __func__, dir);
    return -EINVAL;
  }
  /* (cid, RX_chid) = (0,0),(1,1),...,(15,15) */
  cid = peer_chid;

  /* D2D does NOT use command queue, but queue_status connected with DMA channel 1 to 1.
   * So, to prevent that it is attributed as free DMA channel by queue_status, need change queue_status */
  mutex_lock(&dev->queue_mutex);
  if (queue_info->status != FPGA_Q_STAT_FREE) {
    mutex_unlock(&dev->queue_mutex);
    return -EBUSY;
  }
  queue_info->status = FPGA_Q_STAT_USED;
  mutex_unlock(&dev->queue_mutex);

  /* set as graph mode and peer device's baseaddr_hw */
  queue_set_ctrl(dev, self_chid, dir, cid);
  queue_set_addr(dev, self_chid, dir, peer_addr);

  /* set buffer address and buffer length when D2D-H */
  switch(dir){
  case DMA_D2D_RX:
  case DMA_D2D_TX:
    xpcie_fpga_set_buf(dev, dir, buf_size, buf_addr);
    break;
  default:
    break;
  }

  /* activate LLDMA */
  enable = XPCIE_FPGA_LLDMA_ENABLE_IE | XPCIE_FPGA_LLDMA_ENABLE_OE;
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_SEL(dir), self_chid);
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_CTRL0(dir), enable);

  /* set connector_id */
  strcpy(queue_info->connector_id, connector_id);

  return 0;
}


int
xpcie_fpga_dev_disconnect(
  fpga_dev_info_t *dev,
  uint32_t self_chid,
  uint32_t dir
){
  xpcie_trace("%s: self_chid(%d), dir(%d)", __func__, self_chid, dir);
  {
    int ret;
    lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_SEL(dir), self_chid);

    /* *** stop lldma *** */
    ret = xpcie_fpga_stop_queue(dev, self_chid, dir);
    if(ret < 0)
      return ret;

    /* *** set initial value *** */
    lldma_reg_write(dev, XPCIE_FPGA_LLDMA_Q_CTRL(dir), (0x10) << 8);
    lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_CTRL1(dir), XPCIE_FPGA_LLDMA_CH_CTRL1_INIT);
    lldma_reg_write(dev, XPCIE_FPGA_LLDMA_Q_ADDR_UP(dir), 0x00000000);
    lldma_reg_write(dev, XPCIE_FPGA_LLDMA_Q_ADDR_DN(dir), 0x00000000);
    switch(dir){
    case DMA_D2D_RX:
    case DMA_D2D_TX:
    xpcie_fpga_set_buf(dev, dir, 0, 0);
      break;
    default:
      break;
    }

    /* D2D does NOT use command queue, but queue_status connected with DMA channel 1 to 1.
     * So, to prevent that it is attributed as used DMA channel by queue_status, need change queue_status */
    xpcie_fpga_put_queue_info(dev, self_chid, dir);

    return 0;
  }
}


void
xpcie_fpga_count_available_dma_channel(
  fpga_dev_info_t *dev)
{
  uint32_t regval;
  int i;
  regval = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_RXCH_AVAIL) | lldma_reg_read(dev, XPCIE_FPGA_LLDMA_TXCH_AVAIL);
  dev->available_dma_channel_num = 0;
  for (i = 0; i < sizeof(regval) * 8; i++)
    dev->available_dma_channel_num += ((regval >> i) & 1);
}


uint32_t
xpcie_fpga_get_channel_status(
  fpga_dev_info_t *dev,
  uint32_t chid,
  dma_dir_t dir)
{
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_SEL(dir), chid);

  // non-d2d(0x00)/d2d-h(0x01)/d2d-d(0x02)
  return lldma_reg_read(dev, XPCIE_FPGA_LLDMA_CH_CTRL1(dir));
}


static void
xpcie_fpga_set_cif_base_addr(
  fpga_dev_info_t *dev,
  uint32_t kernel_id,
  bool is_init
) {
  uint32_t dn_rx_val_l = 0;
  uint32_t dn_rx_val_h = 0;
  uint32_t up_tx_val_l = 0;
  uint32_t up_tx_val_h = 0;
  if (is_init) {
    dn_rx_val_l = XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_VAL_L(kernel_id);
    dn_rx_val_h = XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_VAL_H(kernel_id);
    up_tx_val_l = XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_VAL_L(kernel_id);
    up_tx_val_h = XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_VAL_H(kernel_id);
  }
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_L(kernel_id), dn_rx_val_l);
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_H(kernel_id), dn_rx_val_h);
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_L(kernel_id), up_tx_val_l);
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CIF_UP_TX_BASE_H(kernel_id), up_tx_val_h);
}


static void
xpcie_fpga_set_cif_size_ddr4(
  fpga_dev_info_t *dev,
  bool is_init
) {
  uint32_t dn_rx_ddr_size = 0;
  uint32_t up_tx_ddr_size = 0;
  if (is_init) {
    dn_rx_ddr_size = XPCIE_FPGA_LLDMA_CIF_DN_RX_DDR_SIZE_VAL;
    up_tx_ddr_size = XPCIE_FPGA_LLDMA_CIF_UP_TX_DDR_SIZE_VAL;
  }
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CIF_DN_RX_DDR_SIZE, dn_rx_ddr_size);
  lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CIF_UP_TX_DDR_SIZE, up_tx_ddr_size);
}


void
xpcie_fpga_read_cif_ddr4_regs(
  fpga_dev_info_t *dev,
  fpga_ioctl_lldma_buffer_regs_t *regs
) {
  int lane_id;
  for (lane_id = 0; lane_id < XPCIE_KERNEL_LANE_MAX; lane_id++) {
    regs->dn_rx_val_l[lane_id] = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_L(lane_id));
    regs->dn_rx_val_h[lane_id] = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_L(lane_id));
    regs->up_tx_val_l[lane_id] = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_L(lane_id));
    regs->up_tx_val_h[lane_id] = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_CIF_DN_RX_BASE_L(lane_id));
  }
  regs->dn_rx_ddr_size = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_CIF_DN_RX_DDR_SIZE);
  regs->up_tx_ddr_size = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_CIF_UP_TX_DDR_SIZE);
}


int
xpcie_fpga_get_avail_status(
  fpga_dev_info_t *dev,
  uint16_t dir)
{
  xpcie_trace("%s: dir(%d)", __func__, dir);
  {
    uint32_t data;

    data = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_CH_AVAIL(dir));

    return data;
  }
}


int
xpcie_fpga_get_active_status(
  fpga_dev_info_t *dev,
  uint16_t dir)
{
  xpcie_trace("%s: dir(%d)", __func__, dir);
  {
    uint32_t data;

    data = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_CH_ACTIVE(dir));

    return data;
  }
}


void
xpcie_fpga_get_cid_chain_queue(
  fpga_dev_info_t *dev,
  fpga_ioctl_cidchain_t *cidchain)
{
  xpcie_trace("%s: dir(%d), chid(%d)", __func__, cidchain->dir, cidchain->chid);
  {
    // get connection id and function chain controller number(0/1) for command queue
    uint32_t data;
    lldma_reg_write(dev, XPCIE_FPGA_LLDMA_CH_SEL(cidchain->dir), cidchain->chid);
    data = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_CH_CTRL1(cidchain->dir));
    cidchain->cid = (uint32_t)( (data & 0xFFFF0000) >> 16);
    cidchain->chain_no = (uint32_t)( (data & 0x0000FF00) >> 8);
  }
}


void
xpcie_fpga_get_request_size(
  fpga_dev_info_t *dev,
  uint16_t chid,
  uint32_t *req_size)
{
  *req_size = lldma_reg_read(dev, XPCIE_FPGA_LLDMA_REQUEST_SIZE_OFFSET + chid * 4);
}


void
xpcie_fpga_get_rxch_ctrl0(
  fpga_dev_info_t *dev,
  uint32_t *value)
{
  uint32_t value_rd = 0;
  uint32_t i;
  for (i=0; i<32 ; i++) { // txch_sel=0x00~0x1f
    lldma_reg_write(dev, XPCIE_FPGA_LLDMA_RXCH_SEL, i);
    value_rd |= lldma_reg_read(dev, XPCIE_FPGA_LLDMA_RXCH_CTRL0);
  }
  *value = value_rd;
#if 0
  *value = reg_read32(dev, XPCIE_FPGA_ENQ_ACTIVE);
  xpcie_info("%s: value %x", __func__, *value);
#endif
}


void
xpcie_fpga_set_lldma_buffer(
  fpga_dev_info_t *dev,
  bool is_init
) {
  int lane;
  xpcie_trace("%s: %s", __func__, is_init ? "set" : "clear");
  for (lane = 0; lane < XPCIE_KERNEL_LANE_MAX; lane++)
    xpcie_fpga_set_cif_base_addr(dev, lane, is_init);
  xpcie_fpga_set_cif_size_ddr4(dev, is_init);
}
