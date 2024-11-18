/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <liblldma.h>
#include <libshmem.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBLLDMA


/**
 * static global variable : Management table for fd to allocate DMA channel
 *                          DMA channel will be freed when fd closed,
 *                          so library should keep opening and close only when finish to use.
 */
static int fd_get_queue[LLDMA_DEV_MAX][LLDMA_DIR_MAX][LLDMA_CH_MAX];


/**
 * @brief Function which initialize fd management list(`fd_get_queue`) at first once.
 */
static void __liblldma_init(void) {
  static bool init_once = true;
  if (init_once) {
    // Set all fd = -1
    memset(fd_get_queue, 0xff, sizeof(fd_get_queue));
    init_once = !init_once;
  }
}


/**
 * @brief Function which check if target DMA channel is available or not
 */
static int __fpga_lldma_check_queue(
  fpga_device_t *dev,
  dma_dir_t dir,
  uint32_t chid
) {
  // Check in case
  if (!dev) {
    llf_err(INVALID_ARGUMENT, "Fatal error: device pointer is NULL.\n");
    return -INVALID_ARGUMENT;
  }
  uint32_t dev_id;
  if (fpga_get_dev_id(dev->name, &dev_id)) {
    llf_err(INVALID_DATA, "Fatal error: Something error happend...\n");
    return -INVALID_DATA;
  }

  fpga_ioctl_chsts_t ioctl_chsts;
  memset(&ioctl_chsts, 0, sizeof(ioctl_chsts));
  ioctl_chsts.dir = dir;

  // Open FPGA again because 1 fd can allocate only 1 DMA channel
  char filename[FILENAME_MAX];
  snprintf(filename, FILENAME_MAX, "%s%s", FPGA_DEVICE_PREFIX, dev->name);
  fd_get_queue[dev_id][dir][chid] = fpgautil_open(filename, O_RDWR);
  if (fd_get_queue[dev_id][dir][chid] < 0) {
    llf_err(FAILURE_DEVICE_OPEN, "Failed to open device file %s\n", filename);
    return -FAILURE_DEVICE_OPEN;
  }

  // Get status of the target DMA channel
  if (fpgautil_ioctl(fd_get_queue[dev_id][dir][chid], XPCIE_DEV_LLDMA_GET_CH_STAT, &ioctl_chsts) < 0) {
    int err = errno;
    fpgautil_close(fd_get_queue[dev_id][dir][chid]);
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_LLDMA_GET_CH_STAT(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  if ((ioctl_chsts.avail_status & (1 << chid)) == 0) {
    fpgautil_close(fd_get_queue[dev_id][dir][chid]);
    llf_err(UNAVAILABLE_CHID, "Invalid operation: %s channel(%d) is not implemented.\n",
      IS_DMA_RX(dir) ? "RX" : "TX", chid);
    return -UNAVAILABLE_CHID;
  }

  if ((ioctl_chsts.active_status & (1 << chid)) != 0) {
    fpgautil_close(fd_get_queue[dev_id][dir][chid]);
    llf_err(ALREADY_ACTIVE_CHID, "Invalid operation: %s channel(%d) is now being used.\n",
      IS_DMA_RX(dir) ? "RX" : "TX", chid);
    return -ALREADY_ACTIVE_CHID;
  }

  return 0;
}


int fpga_lldma_init(
  uint32_t dev_id,
  dma_dir_t dir,
  uint32_t chid,
  const char *connector_id,
  dma_info_t *dma_info
) {
  __liblldma_init();
  int ret = 0;

  fpga_device_t *dev = fpga_get_device(dev_id);
  // Check input
  if (!dev || dir >= LLDMA_DIR_MAX || chid >= LLDMA_CH_MAX) {
    ret = -INVALID_ARGUMENT;
    goto invalid;
  }
  if (!connector_id || (strlen(connector_id)) >= CONNECTOR_ID_NAME_MAX) {
    ret = -INVALID_ARGUMENT;
    goto invalid;
  }
  if (!dma_info) {
    ret = -INVALID_ARGUMENT;
    goto invalid;
  }
  llf_dbg("%s(dev_id(%u), dir(%d), chid(%u), connector_id(%s), dma_info(%#lx))\n",
    __func__, dev_id, dir, chid, connector_id, (uintptr_t)dma_info);

  if ((ret = __fpga_lldma_check_queue(dev, dir, chid)) < 0) {
    llf_err(-ret, "Invalid operation: %s channel(%u) is not available.\n",
      IS_DMA_RX(dir) ? "RX" : "TX", chid);
    return ret;
  }

  // Set data for ioctl
  fpga_ioctl_queue_t ioctl_queue;
  memset(&ioctl_queue, 0, sizeof(ioctl_queue));
  ioctl_queue.dir = dir;
  ioctl_queue.chid = chid;
  strcpy(ioctl_queue.connector_id, connector_id); // NOLINT

  // Execute ioctl
  if (fpgautil_ioctl(fd_get_queue[dev_id][dir][chid], XPCIE_DEV_LLDMA_ALLOC_QUEUE, &ioctl_queue) < 0) {
    int err = errno;
    if (err == EBUSY) {
      fpgautil_close(fd_get_queue[dev_id][dir][chid]);
      llf_err(ALREADY_ASSIGNED, "Invalid operation: %s channel(%u) is Already used.\n",
        IS_DMA_RX(dir) ? "RX" : "TX", chid);
      ret = -ALREADY_ASSIGNED;
    } else {
      fpgautil_close(fd_get_queue[dev_id][dir][chid]);
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_LLDMA_ALLOC_QUEUE(errno:%d)\n", err);
      ret = -FAILURE_IOCTL;
    }
    return ret;
  }

  // Set information of DMA channel into argument(dma_info)
  dma_info->dev_id = dev_id;
  dma_info->dir = (dma_dir_t)dir;
  dma_info->chid  = chid;
  dma_info->queue_addr = NULL;
  dma_info->queue_size = 0;
  dma_info->connector_id = strdup(connector_id);
  if (!dma_info->connector_id) {
    llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for connector_id(%s)\n", connector_id);
    fpgautil_close(fd_get_queue[dev_id][dir][chid]);
    return -FAILURE_MEMORY_ALLOC;
  }

  return 0;

invalid:
  llf_err(-ret, "%s(dev_id(%u), dir(%d), chid(%u), connector_id(%s), dma_info(%#lx))\n",
    __func__, dev_id, dir, chid, connector_id ? connector_id : "<null>", (uintptr_t)dma_info);

  return ret;
}


int fpga_lldma_finish(
  dma_info_t *dma_info
) {
  __liblldma_init();
  if (!dma_info) {
    llf_err(INVALID_ARGUMENT, "%s(dma_info(%#lx))\n", __func__, (uintptr_t)dma_info);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dma_info(%#lx))\n", __func__, (uintptr_t)dma_info);
  int ret;

  fpga_ioctl_queue_t ioctl_queue;
  memset(&ioctl_queue, 0, sizeof(ioctl_queue));
  ioctl_queue.dir = dma_info->dir;
  ioctl_queue.chid = dma_info->chid;

  int fd = fd_get_queue[dma_info->dev_id][dma_info->dir][dma_info->chid];
  if (fd == -1) {
    llf_err(INVALID_ARGUMENT, "Invalid operation: This channel is not allocated(dir:%s, chid:%d)\n",
        IS_DMA_RX(dma_info->dir) ? "RX" : "TX", dma_info->chid);
    return -INVALID_ARGUMENT;
  }
  ret = fpgautil_ioctl(fd, XPCIE_DEV_LLDMA_FREE_QUEUE, &ioctl_queue);
  fpgautil_close(fd);
  free(dma_info->connector_id);
  if (ret < 0) {
    int err = errno;
    if (err == EBUSY) {
      llf_err(INVALID_ARGUMENT, "Fatal error: Failed to stop DMA channel(dir:%s, chid:%d)\n",
        IS_DMA_RX(ioctl_queue.dir) ? "RX" : "TX", ioctl_queue.chid);
      return -INVALID_ARGUMENT;
    } else {
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_LLDMA_FREE_QUEUE(errno:%d)\n", err);
      return -FAILURE_IOCTL;
    }
  }
  fd_get_queue[dma_info->dev_id][dma_info->dir][dma_info->chid] = -1;
  return 0;
}


static int __fpga_lldma_check_buf_valid(
  void* va,
  uint32_t size,
  uint64_t *pa
) {
  if (pa == NULL) {
    llf_err(INVALID_ARGUMENT, "%s(va(%#lx), size(%#x), pa(%#lx))\n",
      __func__, (uintptr_t)va, size, (uintptr_t)pa);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(va(%#lx), size(%#x), pa(%#lx))\n",
    __func__, (uintptr_t)va, size, (uintptr_t)pa);

  int ret = 0;
  uint64_t dst_pa64 = 0;

  // Check if buffer size is less than 4KB(SHMEM_MIN_SIZE_BUF)
  if (size < SHMEM_MIN_SIZE_BUF) {
    llf_err(INVALID_ARGUMENT, "Invalid operation: size(%#x) shoud be larger than %#xB.\n",
      size, SHMEM_MIN_SIZE_BUF);
    return -INVALID_ARGUMENT;
  }
  // TODO: Check if buffer size is less than `D2D-H interval timing`(LLDMA:0x1404[3:0])

  // Check if buffer size is power of 2
  for (typeof(size) temp_size = size; temp_size; temp_size >>= 1) {
    if (temp_size & 0b1) {
      if ((temp_size ^ 0b1) == 0b0) {
        break;
      } else {
        llf_err(INVALID_ARGUMENT, "Invalid operation: size(%#x) should be power of 2.\n", size);
        return -INVALID_ARGUMENT;
      }
    }
  }

  if (size && va) {
    uint64_t chklen = size;
    // Check if buffer address is registered in libshmem and convert virt2phys
    dst_pa64 = dma_pa_from_va(va, &chklen);
    // Check if buffer physical address is NULL, 1024B boundary, physically continuous
    if (!dst_pa64 || (dst_pa64 % SHMEM_BOUNDARY_SIZE != 0) || size != chklen) {
      llf_err(INVALID_ADDRESS, "Invalid operation: address is invalid(physaddr:%#lx, chklen:%#lx)\n",
        dst_pa64, chklen);
      return -INVALID_ADDRESS;
    }
  } else {
    llf_err(INVALID_ADDRESS, "Invalid operation: va(%#lx) is NULL.\n", (uintptr_t)va);
    return -INVALID_ADDRESS;
  }

  // Return physical address to user with pa
  *pa = dst_pa64;
  llf_dbg(" *pa(%#lx))\n", dst_pa64);

  return ret;
}


static int __fpga_lldma_buf_connect(
  fpga_lldma_connect_t *connect_info,
  fpga_device_t *tx_dev,
  fpga_device_t *rx_dev,
  uint64_t dst_pa64
) {
  int tx_fd, rx_fd, ret = 0;
  fpga_ioctl_connect_t tx_connect;
  fpga_ioctl_connect_t rx_connect;

  // Check availability of dma channel
  if ((ret = __fpga_lldma_check_queue(tx_dev, DMA_DEV_TO_HOST, connect_info->tx_chid)) < 0) {
    llf_err(-ret, "Invalid operation: TX channel(%u) is not available.\n", connect_info->tx_chid);
    return ret;
  }

  if ((ret = __fpga_lldma_check_queue(rx_dev, DMA_HOST_TO_DEV, connect_info->rx_chid)) < 0) {
    llf_err(-ret, "Invalid operation: RX channel(%u) is not available.\n", connect_info->rx_chid);
    return ret;
  }
  tx_fd = fd_get_queue[connect_info->tx_dev_id][DMA_DEV_TO_HOST][connect_info->tx_chid];
  rx_fd = fd_get_queue[connect_info->rx_dev_id][DMA_HOST_TO_DEV][connect_info->rx_chid];

  // TX LLDMA setting
  tx_connect = (fpga_ioctl_connect_t){
    .self_dir = DMA_DEV_TO_HOST,
    .self_chid = connect_info->tx_chid,
    .peer_chid = connect_info->rx_chid,
    .peer_minor = rx_dev->dev_id,
    .buf_size = connect_info->buf_size,
    .buf_addr = dst_pa64
  };
  strcpy(tx_connect.connector_id, connect_info->connector_id);  // NOLINT
  if (fpgautil_ioctl(tx_fd, XPCIE_DEV_LLDMA_ALLOC_CONNECTION, &tx_connect) < 0) {
    int err = errno;
    if (err == EINVAL) {
      llf_err(INVALID_ARGUMENT, "Invalid operation: XPCIE_DEV_LLDMA_ALLOC_CONNECTION(TX)\n");
      ret = -INVALID_ARGUMENT;
    } else {
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_LLDMA_ALLOC_CONNECTION(TX, errno:%d, fd:%d)\n", err, tx_fd);
      ret = -FAILURE_IOCTL;
    }
    fpgautil_close(tx_fd);
    fd_get_queue[connect_info->tx_dev_id][DMA_DEV_TO_HOST][connect_info->tx_chid] = -1;
    return ret;
  }

  // RX LLDMA setting
  rx_connect = (fpga_ioctl_connect_t){
    .self_dir = DMA_HOST_TO_DEV,
    .self_chid = connect_info->rx_chid,
    .peer_chid = connect_info->tx_chid,
    .peer_minor = tx_dev->dev_id,
    .buf_size = connect_info->buf_size,
    .buf_addr = dst_pa64
  };
  strcpy(rx_connect.connector_id, connect_info->connector_id);  // NOLINT
  if (fpgautil_ioctl(rx_fd, XPCIE_DEV_LLDMA_ALLOC_CONNECTION, &rx_connect) < 0) {
    // store errno of RX
    int err = errno;
    if (err == EINVAL) {
      llf_err(INVALID_ARGUMENT, "Invalid operation: XPCIE_DEV_LLDMA_ALLOC_CONNECTION(RX)\n");
      ret = -INVALID_ARGUMENT;
    } else {
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_LLDMA_ALLOC_CONNECTION(RX, errno:%d, fd:%d)\n", err, rx_fd);
      ret = -FAILURE_IOCTL;
    }
    fpgautil_close(rx_fd);
    fd_get_queue[connect_info->rx_dev_id][DMA_HOST_TO_DEV][connect_info->rx_chid] = -1;

    // TX LLDMA disconnect
    if (fpgautil_ioctl(tx_fd, XPCIE_DEV_LLDMA_FREE_CONNECTION, NULL) < 0) {
      err = errno;
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_LLDMA_FREE_CONNECTION(TX, errno:%d, fd:%d)\n", err, tx_fd);
    }
    fpgautil_close(tx_fd);
    fd_get_queue[connect_info->tx_dev_id][DMA_DEV_TO_HOST][connect_info->tx_chid] = -1;

    return ret;
  }

  return 0;
}


static int __fpga_lldma_buf_disconnect(
  fpga_lldma_connect_t *connect_info,
  fpga_device_t *tx_dev,
  fpga_device_t *rx_dev
) {
  int tx_fd, rx_fd;
  int ret = 0;

  tx_fd = fd_get_queue[connect_info->tx_dev_id][DMA_DEV_TO_HOST][connect_info->tx_chid];
  rx_fd = fd_get_queue[connect_info->rx_dev_id][DMA_HOST_TO_DEV][connect_info->rx_chid];

  // RX LLDMA disconnect
  if (rx_fd >= 0) {
    if (fpgautil_ioctl(rx_fd, XPCIE_DEV_LLDMA_FREE_CONNECTION, NULL) < 0) {
      int err = errno;
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_LLDMA_FREE_CONNECTION(RX, errno:%d, fd:%d)\n", err, rx_fd);
      ret = -FAILURE_IOCTL;
    }
    fpgautil_close(rx_fd);
    fd_get_queue[connect_info->rx_dev_id][DMA_HOST_TO_DEV][connect_info->rx_chid] = -1;
  } else {
    llf_err(INVALID_ARGUMENT, "Invalid operation: This channel is not allocated(RX)\n");
    ret = -INVALID_ARGUMENT;
  }

  // TX LLDMA disconnect
  if (tx_fd >= 0) {
    if (fpgautil_ioctl(tx_fd, XPCIE_DEV_LLDMA_FREE_CONNECTION, NULL) < 0) {
      int err = errno;
      llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_LLDMA_FREE_CONNECTION(TX, errno:%d, fd:%d)\n", err, tx_fd);
      ret = -FAILURE_IOCTL;
    }
    fpgautil_close(tx_fd);
    fd_get_queue[connect_info->tx_dev_id][DMA_DEV_TO_HOST][connect_info->tx_chid] = -1;
  } else {
    llf_err(INVALID_ARGUMENT, "Invalid operation: This channel is not allocated(TX)\n");
    ret = -INVALID_ARGUMENT;
  }

  return ret;
}


// cppcheck-suppress unusedFunction
int fpga_lldma_buf_connect(
  fpga_lldma_connect_t *connect_info
) {
  __liblldma_init();
  uint64_t dst_pa64;
  fpga_device_t *tx_dev, *rx_dev;

  // Check input
  if (connect_info == NULL) {
    goto invalid_arg;
  }
  if ((!connect_info->connector_id)
    || ((strlen(connect_info->connector_id) + 1) >= CONNECTOR_ID_NAME_MAX)) {
    llf_err(INVALID_ARGUMENT, "Invalid operation: connector_id is invalid.\n");
    goto invalid_arg;
  }
  if (!(tx_dev = fpga_get_device(connect_info->tx_dev_id))
    || !(rx_dev = fpga_get_device(connect_info->rx_dev_id))) {
    llf_err(INVALID_ARGUMENT, "Invalid operation: dev_id is invalid.\n");
    goto invalid_arg;
  }
  if (connect_info->tx_chid >= LLDMA_CH_MAX || connect_info->rx_chid >= LLDMA_CH_MAX) {
    llf_err(INVALID_ARGUMENT, "Invalid operation: LLDMA's chid is invalid.\n");
    goto invalid_arg;
  }
  if (connect_info->tx_dev_id == connect_info->rx_dev_id) {
    llf_err(INVALID_ARGUMENT, "Invalid operation: The same dev_id is not supported.\n");
    goto invalid_arg;
  }
  if (__fpga_lldma_check_buf_valid(connect_info->buf_addr, connect_info->buf_size, &dst_pa64)) {
    llf_err(INVALID_ARGUMENT, "Invalid operation: The buffer is invalid.\n");
    goto invalid_arg;
  }
  llf_dbg("%s(connect_info(%#lx))\n", __func__, (uintptr_t)connect_info);

  return __fpga_lldma_buf_connect(
    connect_info,
    tx_dev,
    rx_dev,
    dst_pa64);

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(connect_info(%#lx))\n", __func__, (uintptr_t)connect_info);
  return -INVALID_ARGUMENT;
}


// cppcheck-suppress unusedFunction
int fpga_lldma_buf_disconnect(
    fpga_lldma_connect_t *connect_info
) {
  __liblldma_init();
  fpga_device_t *tx_dev, *rx_dev;

  // Check input
  if (connect_info == NULL) {
    goto invalid_arg;
  }
  if (!(tx_dev = fpga_get_device(connect_info->tx_dev_id))
    || !(rx_dev = fpga_get_device(connect_info->rx_dev_id))) {
    goto invalid_arg;
  }
  if (connect_info->tx_chid >= LLDMA_CH_MAX || connect_info->rx_chid >= LLDMA_CH_MAX) {
    goto invalid_arg;
  }
  llf_dbg("%s(connect_info(%#lx))\n", __func__, (uintptr_t)connect_info);

  return __fpga_lldma_buf_disconnect(
    connect_info,
    tx_dev,
    rx_dev);

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(connect_info(%#lx))\n", __func__, (uintptr_t)connect_info);
  return -INVALID_ARGUMENT;
}


// cppcheck-suppress unusedFunction
int fpga_lldma_direct_connect(
  fpga_lldma_connect_t *connect_info
) {
  __liblldma_init();
  fpga_device_t *tx_dev, *rx_dev;

  // Check input
  if (connect_info == NULL) {
    goto invalid_arg;
  }
  if ((!connect_info->connector_id)
    || ((strlen(connect_info->connector_id) + 1) >= CONNECTOR_ID_NAME_MAX)) {
    goto invalid_arg;
  }
  if (!(tx_dev = fpga_get_device(connect_info->tx_dev_id))
    || !(rx_dev = fpga_get_device(connect_info->rx_dev_id))) {
    goto invalid_arg;
  }
  if (connect_info->tx_chid >= LLDMA_CH_MAX || connect_info->rx_chid >= LLDMA_CH_MAX) {
    goto invalid_arg;
  }
  if (connect_info->tx_dev_id == connect_info->rx_dev_id) {
    llf_err(INVALID_ARGUMENT, "Invalid operation: The same dev_id is not supported.\n",
      connect_info->tx_dev_id);
    goto invalid_arg;
  }
  llf_dbg("%s(connect_info(%#lx))\n", __func__, (uintptr_t)connect_info);

  return __fpga_lldma_buf_connect(
    connect_info,
    tx_dev,
    rx_dev,
    0);

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(connect_info(%#lx))\n", __func__, (uintptr_t)connect_info);
  return -INVALID_ARGUMENT;
}


// cppcheck-suppress unusedFunction
int fpga_lldma_direct_disconnect(
    fpga_lldma_connect_t *connect_info
) {
  __liblldma_init();
  fpga_device_t *tx_dev, *rx_dev;

  // Check input
  if (connect_info == NULL) {
    goto invalid_arg;
  }
  if (!(tx_dev = fpga_get_device(connect_info->tx_dev_id))
    || !(rx_dev = fpga_get_device(connect_info->rx_dev_id))) {
    goto invalid_arg;
  }
  if (connect_info->tx_chid >= LLDMA_CH_MAX || connect_info->rx_chid >= LLDMA_CH_MAX) {
    goto invalid_arg;
  }
  llf_dbg("%s(connect_info(%#lx))\n", __func__, (uintptr_t)connect_info);

  return __fpga_lldma_buf_disconnect(
    connect_info,
    tx_dev,
    rx_dev);

invalid_arg:
  llf_err(INVALID_ARGUMENT, "%s(connect_info(%#lx))\n", __func__, (uintptr_t)connect_info);
  return -INVALID_ARGUMENT;
}


int fpga_lldma_get_rxch_ctrl0(
  uint32_t dev_id,
  uint32_t *rxch_ctrl0
) {
  llf_dbg("%s()\n", __func__);

  /* input check */
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !rxch_ctrl0) {
    llf_err(INVALID_ARGUMENT,
      "%s(dev_id(%u), rxch_ctrl0(%#lx))\n", __func__, dev_id, (uintptr_t)rxch_ctrl0);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(dev_id(%u), rxch_ctrl0(%#lx))\n", __func__, dev_id, (uintptr_t)rxch_ctrl0);

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_LLDMA_GET_RXCH_CTRL0, rxch_ctrl0) < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_LLDMA_GET_RXCH_CTRL0(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_lldma_get_rsize(
  dma_info_t *dma_info,
  uint32_t* rsize
) {
  __liblldma_init();
  if (!dma_info || !rsize) {
    llf_err(INVALID_ARGUMENT, "%s(dma_info(%#lx), rsize(%#lx))\n",
      __func__, (uintptr_t)dma_info, (uintptr_t)rsize);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dma_info(%#lx), rsize(%#lx))\n",
    __func__, (uintptr_t)dma_info, (uintptr_t)rsize);


  int ret = -1;
  fpga_ioctl_up_info_t ioctl_info;

  memset(&ioctl_info, 0, sizeof(ioctl_info));
  ioctl_info.chid = dma_info->chid;

  ret = fpgautil_ioctl(fd_get_queue[dma_info->dev_id][dma_info->dir][dma_info->chid], XPCIE_DEV_LLDMA_GET_UP_SIZE, &ioctl_info);
  if (ret < 0) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_LLDMA_GET_UP_SIZE(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  *rsize = ioctl_info.size;

  return 0;
}


/**
 * @brief Function which control LLDMA's buffer
 */
static int __fpga_lldma_control_buffer(
  fpga_device_t *dev,
  fpga_ioctl_lldma_buffer_cmd_t cmd,
  fpga_lldma_buffer_t *buf
) {
  fpga_ioctl_lldma_buffer_t ioctl_info;
  memset(&ioctl_info, 0, sizeof(ioctl_info));
  ioctl_info.cmd = cmd;

  if (fpgautil_ioctl(dev->fd, XPCIE_DEV_LLDMA_CTRL_DDR_BUFFER, &ioctl_info)) {
    int err = errno;
    llf_err(FAILURE_IOCTL, "Failed to ioctl XPCIE_DEV_LLDMA_CTRL_DDR_BUFFER(errno:%d)\n", err);
    return -FAILURE_IOCTL;
  }

  if (buf) {
    for (int lane_id = 0; lane_id < XPCIE_KERNEL_LANE_MAX; lane_id++) {
      buf->dn_rx_val_l[lane_id] = ioctl_info.regs.dn_rx_val_l[lane_id];
      buf->dn_rx_val_h[lane_id] = ioctl_info.regs.dn_rx_val_h[lane_id];
      buf->up_tx_val_l[lane_id] = ioctl_info.regs.up_tx_val_l[lane_id];
      buf->up_tx_val_h[lane_id] = ioctl_info.regs.up_tx_val_h[lane_id];
    }
    buf->dn_rx_ddr_size = ioctl_info.regs.dn_rx_ddr_size;
    buf->up_tx_ddr_size = ioctl_info.regs.up_tx_ddr_size;
  }

  return 0;
}


int fpga_lldma_setup_buffer(
  uint32_t dev_id
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  return __fpga_lldma_control_buffer(dev, CMD_FPGA_IOCTL_LLDMA_BUF_SET, NULL);
}


// cppcheck-suppress unusedFunction
int fpga_lldma_cleanup_buffer(
  uint32_t dev_id
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u))\n", __func__, dev_id);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u))\n", __func__, dev_id);

  return __fpga_lldma_control_buffer(dev, CMD_FPGA_IOCTL_LLDMA_BUF_CLR, NULL);
}


// cppcheck-suppress unusedFunction
int fpga_lldma_get_buffer(
  uint32_t dev_id,
  fpga_lldma_buffer_t *buf
) {
  fpga_device_t *dev = fpga_get_device(dev_id);
  if (!dev || !buf) {
    llf_err(INVALID_ARGUMENT, "%s(dev_id(%u), buf(%#lx))\n", __func__, dev_id, (uintptr_t)buf);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dev_id(%u), buf(%#lx))\n", __func__, dev_id, (uintptr_t)buf);

  return __fpga_lldma_control_buffer(dev, CMD_FPGA_IOCTL_LLDMA_BUF_GET, buf);
}
