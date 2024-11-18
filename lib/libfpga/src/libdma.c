/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libdma.h>
#include <libshmem.h>
#include <liblogging.h>

#include <libfpga_internal/libfpgautil.h>
#include <libfpga_internal/libdpdkutil.h>

#include <stdio.h>
#include <getopt.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBDMA


/**
 * @enum ENQUEUE_ADDR_CHECK_TYPE
 * @brief Enumeration of flags to switch buffer checking
 */
enum ENQUEUE_ADDR_CHECK_TYPE {
  VIRT_ADDR_WITH_CHECK,     /**< normal : use libshmem buffer's virt address */
  VIRT_ADDR_WITHOUT_CHECK,  /**< debug : use dpdk but not libshmem buffer's virt address */
  PHYS_ADDR,                /**< debug : use buffer's phys address directly */
};


/**
 * static global variable: long options for fpga_dma_options_init()
 */
static const struct option
fpga_dma_long_options[] = {
  { "polling-timeout", required_argument, NULL, 'p' },
  { "polling-interval", required_argument, NULL, 'i' },
  { "refqueue-timeout", required_argument, NULL, 'r' },
  { "refqueue-interval", required_argument, NULL, 'q' },
  { NULL, 0, 0, 0 },
};

/**
 * static global variable: short options for fpga_dma_options_init()
 */
static const char
fpga_dma_short_options[] = {
  "p:i:r:q:"
};

/**
 * static global variable: Management table for fd to mmap command queue
 */
static int fd_ref_queue[LLDMA_DEV_MAX][LLDMA_DIR_MAX][LLDMA_CH_MAX];

/**
 * static global variable: Timeout time for fpga_dequeue()
 */
static int64_t libdma_dequeue_polling_timeout = DEQ_TIMEOUT_DEFAULT;

/**
 * static global variable: Interval time for fpga_dequeue()
 */
static int64_t libdma_dequeue_polling_interval = DEQ_INTERVAL_DEFAULT;

/**
 * static global variable: Timeout time for fpga_lldma_queue_setup()
 */
static int64_t libdma_refqueue_polling_timeout = REFQ_TIMEOUT_DEFAULT;

/**
 * static global variable: Interval time for fpga_lldma_queue_setup()
 */
static int64_t libdma_refqueue_polling_interval = REFQ_INTERVAL_DEFAULT;


int fpga_lldma_queue_setup(
  const char *connector_id,
  dma_info_t *dma_info
) {
  fpga_ioctl_queue_t ioctl_queue;
  void *mmap_addr;

  // Check input
  if ((!connector_id) || (!dma_info)) {
    llf_err(INVALID_ARGUMENT, "%s(connector_id(%s), dma_info(%#lx))\n",
      __func__, connector_id ? connector_id : "<null>", (uintptr_t)dma_info);
    return -INVALID_ARGUMENT;
  }
  if (strlen(connector_id) >= CONNECTOR_ID_NAME_MAX || strlen(connector_id) == 0) {
    llf_err(INVALID_ARGUMENT,
      "%s(connector_id(%s), dma_info(%#lx))\n", __func__, connector_id, (uintptr_t)dma_info);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(connector_id(%s), dma_info(%#lx))\n", __func__, connector_id, (uintptr_t)dma_info);

  // Initialize ioctl data
  memset(&ioctl_queue, 0, sizeof(ioctl_queue));
  strcpy(ioctl_queue.connector_id, connector_id);  // NOLINT

  // Repeat at the `interval` cycle until the `timeout` time
  for (
    int passed_sec = 0;
    passed_sec < libdma_refqueue_polling_timeout;
    passed_sec += libdma_refqueue_polling_interval
  ) {
    // Check if connector_id is exist in all opening devices
    for (int device_id = 0; device_id < FPGA_MAX_DEVICES; device_id++) {
      // Check whether device_id is valid
      fpga_device_t* dev = fpga_get_device(device_id);
      if (!dev)
        continue;

      // Create device_file name and open
      char filename[FILENAME_MAX];
      snprintf(filename, FILENAME_MAX, "%s%s", FPGA_DEVICE_PREFIX, dev->name);
      int tmpfd = fpgautil_open(filename, O_RDWR);
      if (tmpfd < 0) {
        int err = errno;
        llf_err(FAILURE_DEVICE_OPEN, "Failed to open device file %s(errno:%d)\n", filename, err);
        return -FAILURE_DEVICE_OPEN;
      }

      // Bind queue
      if (fpgautil_ioctl(tmpfd, XPCIE_DEV_LLDMA_BIND_QUEUE, &ioctl_queue) < 0) {
        // Failed to match connector_id, so check next FPGA
        fpgautil_close(tmpfd);
        continue;
      }

      // Succeed to bind command queue, so mmap command queue for enq/deq
      mmap_addr = mmap(0, ioctl_queue.map_size, PROT_READ | PROT_WRITE, MAP_SHARED, tmpfd, 0);
      if (mmap_addr == NULL) {
        int err = errno;
        fpgautil_close(tmpfd);
        llf_err(FAILURE_MMAP, "Failed to mmap queue area.(errno:%d)\n", err);
        return -FAILURE_MMAP;
      }

      // Set data for user variable
      dma_info->dev_id = device_id;
      dma_info->dir = (dma_dir_t)ioctl_queue.dir;
      dma_info->chid  = ioctl_queue.chid;
      dma_info->queue_addr = mmap_addr;
      dma_info->queue_size = (ioctl_queue.map_size - sizeof(fpga_queue_t)) / sizeof(fpga_desc_t);
      dma_info->connector_id = strdup(connector_id);
      if (!dma_info->connector_id) {
        llf_err(FAILURE_MEMORY_ALLOC, "Failed to allocate memory for connector_id(%s)\n", connector_id);
        fpgautil_close(tmpfd);
        return -FAILURE_MEMORY_ALLOC;
      }
      fd_ref_queue[dma_info->dev_id][dma_info->dir][dma_info->chid] = tmpfd;
      return 0;
    }

    // There were no mathced connector_id now, so sleep interval secounds
    sleep(libdma_refqueue_polling_interval);
    llf_dbg("  [%02d(sec)/%02d(sec)] Polling connector_id(%s)\n",
      passed_sec + libdma_refqueue_polling_interval, libdma_refqueue_polling_timeout, connector_id);
  }

  llf_err(CONNECTOR_ID_MISMATCH, "Failed to refqueue %s\n", connector_id);
  return -CONNECTOR_ID_MISMATCH;
}


// cppcheck-suppress unusedFunction
int fpga_lldma_queue_finish(
  dma_info_t *dma_info
) {
  if (!dma_info) {
    llf_err(INVALID_ARGUMENT, "%s(dma_info(%#lx))\n", __func__, (uintptr_t)dma_info);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dma_info(%#lx))\n", __func__, (uintptr_t)dma_info);

  munmap(dma_info->queue_addr, sizeof(fpga_queue_t) + (sizeof(fpga_desc_t) * dma_info->queue_size));

  fpgautil_close(fd_ref_queue[dma_info->dev_id][dma_info->dir][dma_info->chid]);

  free(dma_info->connector_id);

  return 0;
}


int set_dma_cmd(
  dmacmd_info_t *cmd_info,
  uint16_t task_id,
  void *data_addr,
  uint32_t data_len
) {
  if (!cmd_info) {
    llf_err(INVALID_ARGUMENT, "%s(cmd_info(%#lx), task_id(%hu), data_addr(%#lx), data_len(%#x))\n",
    __func__, (uintptr_t)cmd_info, task_id, (uintptr_t)data_addr, data_len);
    return -INVALID_ARGUMENT;
  }

  llf_dbg("%s(cmd_info(%#lx), task_id(%hu), data_addr(%#lx), data_len(%#x))\n",
    __func__, (uintptr_t)cmd_info, task_id, (uintptr_t)data_addr, data_len);

  cmd_info->task_id = task_id;
  cmd_info->data_len  = data_len;
  cmd_info->data_addr = data_addr;

  static bool reference_once = false;

  if (libdma_dequeue_polling_timeout < DEQ_TIMEOUT_MIN) {
    // invalid value change to default value
    libdma_dequeue_polling_timeout = DEQ_TIMEOUT_DEFAULT;
  }
  if ((libdma_dequeue_polling_interval <= 0)
    || (libdma_dequeue_polling_interval > DEQ_INTERVAL_MAX)
    || (libdma_dequeue_polling_timeout <= libdma_dequeue_polling_interval)
  ) {
    // invalid value change to default value
    libdma_dequeue_polling_interval = DEQ_INTERVAL_DEFAULT;
    reference_once = false;
  }

  if (!reference_once) {
    reference_once = true;
    // measure reference value of input interval,
    // however this value will NOT be used.
    struct timespec timer1, timer2, req, rem;
    req.tv_sec = 0; req.tv_nsec = libdma_dequeue_polling_interval * 1000;
    clock_gettime(CLOCK_REALTIME, &timer1);
    while (clock_nanosleep(CLOCK_REALTIME, 0, &req, &rem) == EINTR) {
      req.tv_sec  = rem.tv_sec;
      req.tv_nsec = rem.tv_nsec;
    }
    clock_gettime(CLOCK_REALTIME, &timer2);
    int64_t usec = (timer2.tv_sec  - timer1.tv_sec) * 1000000L + (timer2.tv_nsec - timer1.tv_nsec)/1000;
    llf_info(" polling_timeout = %ldus = %ldms\n",
      libdma_dequeue_polling_timeout, libdma_dequeue_polling_timeout/1000);
    llf_info(" polling_interval(input) = %ldus, polling_interval(sample) = %ldus\n",
      libdma_dequeue_polling_interval, usec);
  }

  return 0;
}



int get_dma_cmd(
  dmacmd_info_t cmd_info,
  uint16_t *task_id,
  void **data_addr,
  uint32_t *data_len,
  uint32_t *result_status
) {
  // Check input
  if (!task_id && !data_addr && !data_len && !result_status) {
    // Return error only when all pointer is NULL, return 0 when any pointer is not NULL.
    llf_err(INVALID_ARGUMENT, "%s(info(-), task_id(%#lx), data_addr(%#lx), data_len(%#lx), result_status(%#lx))\n",
      __func__, (uintptr_t)task_id, (uintptr_t)data_addr, (uintptr_t)data_len, (uintptr_t)result_status);
    return -INVALID_ARGUMENT;
  }

  if (result_status) {
    // result_status is not NULL, so get status of result
    if (task_id) *task_id = cmd_info.result_task_id;
    if (data_addr) *data_addr = cmd_info.result_data_addr;
    if (data_len) *data_len = cmd_info.result_data_len;
    *result_status = cmd_info.result_status;
  } else {
    // result_status is NULL, so get status of input data
    if (task_id) *task_id = cmd_info.task_id;
    if (data_addr) *data_addr = cmd_info.data_addr;
    if (data_len) *data_len = cmd_info.data_len;
  }

  return 0;
}


/**
 * @brief Execute enqueue into the command queue
 */
static int __fpga_enqueue(
  dma_info_t *dma_info,
  dmacmd_info_t *cmd_info,
  int addr_check_flag
) {
  fpga_queue_t *enq = (fpga_queue_t*)dma_info->queue_addr;  //NOLINT
  fpga_desc_t *desc;
  uint16_t next_head, current_head;
  uint64_t chklen;
  uint64_t dst_pa64 = 0;

  // Check if the size is less than 1KB(SHMEM_BOUNDARY_SIZE)
  if (cmd_info->data_len < SHMEM_BOUNDARY_SIZE) {
    llf_err(INVALID_ARGUMENT, "Invalid operation: data_len(%#x) shoud be larger than %#xB.\n",
      cmd_info->data_len, SHMEM_BOUNDARY_SIZE);
    return -INVALID_ARGUMENT;
  }

  // Check if the size is 64B(RTE_CACHE_LINE_SIZE) alignment
  if ((cmd_info->data_len % RTE_CACHE_LINE_SIZE) != 0) {
    llf_err(INVALID_ARGUMENT, "Invalid operation: data_len(%#x) should be %#xB aligned.\n",
      cmd_info->data_len, RTE_CACHE_LINE_SIZE);
    return -INVALID_ARGUMENT;
  }

  // Check if the data_addr is NULL
  if (!cmd_info->data_addr) {
    llf_err(INVALID_ADDRESS, "Invalid operation: data_addr is NULL.\n");
    return -INVALID_ADDRESS;
  }

  if (addr_check_flag == VIRT_ADDR_WITH_CHECK) {
    if (cmd_info->data_len && cmd_info->data_addr) {
      chklen = cmd_info->data_len;
      // Check if the data_addr is registered in libshmem and convert virt2phys
      dst_pa64 = dma_pa_from_va(cmd_info->data_addr, &chklen);
      // Check if the phys_addr is NULL, 1024B boundary, physically continuous
      if (!dst_pa64 || (dst_pa64 % SHMEM_BOUNDARY_SIZE != 0) || cmd_info->data_len != chklen) {
        llf_err(INVALID_ADDRESS, "Invalid operation: data is invalid(physaddr:%#lx, data_len:%#lx, chklen:%#lx)\n",
          dst_pa64, cmd_info->data_len, chklen);
        return -INVALID_ADDRESS;
      }
    }
  } else if (addr_check_flag == VIRT_ADDR_WITHOUT_CHECK) {
    // (debug)Use virt memory got by DPDK but not registerd into libshmem
    dst_pa64 = rte_mem_virt2phy(cmd_info->data_addr);
  } else if (addr_check_flag == PHYS_ADDR) {
    // (debug)Use phys memory directly
    dst_pa64 = (uint64_t)cmd_info->data_addr;
  } else {
    llf_err(INVALID_ARGUMENT, "Fatal error: Invalid TYPE[%d]\n", addr_check_flag);
    return -INVALID_ARGUMENT;
  }

  // Get free descriptor
  do {
    // Get current head's position
    current_head = enq->writehead;

    // Check if current head descriptor is now using
    if (enq->ring[current_head].task_id != 0) {
      llf_warn(ENQUEUE_QUEFULL, "Invalid operation: Command queue for %s channel(%d) is full.\n",
        IS_DMA_RX(dma_info->dir) ? "RX" : "TX", dma_info->chid);
      return -ENQUEUE_QUEFULL;
    }

    // Get next head's position to check later
    next_head = (current_head + 1);
    if (next_head == enq->size) next_head = 0;

    // Compare enq->writehead(in kernel(=used by other process)) and current_head(host_memory),
    // and if the data is the same, set next_head into enq->writehead and get current_head.
    // If the data is different, other process got current_head, so retry to get new current_head.
  } while (!rte_atomic16_cmpset(&enq->writehead, current_head, next_head));

  // Set current_head descriptor's address into cmd_info
  cmd_info->desc_addr = desc = &enq->ring[current_head];

  if (dst_pa64) {
    desc->addr = dst_pa64;
    desc->len  = cmd_info->data_len;
  } else {
    // dst_pa64 is always not 0, so no need this paragraph.
    desc->addr = 0;
    desc->len  = 0;
  }

  // Set cmd_info's task_id into descriptor
  desc->task_id = cmd_info->task_id;

  // To prevent setting CMD_READY before setting above information into descriptor(e.g. dst_pa64)
  rte_wmb();

  // By setting CMD_READY at desc->op, FPGA detect that valid data is stored.
  desc->op = CMD_READY;

  return 0;
}


// cppcheck-suppress unusedFunction
int fpga_enqueue(
  dma_info_t *dma_info,
  dmacmd_info_t *cmd_info
) {
  if (dma_info == NULL || cmd_info == NULL) {
    llf_err(INVALID_ARGUMENT, "%s(dma_info(%#lx), cmd_info(%#lx))\n",
      __func__, (uintptr_t)dma_info, (uintptr_t)cmd_info);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dma_info(%#lx), cmd_info(%#lx))\n",
    __func__, (uintptr_t)dma_info, (uintptr_t)cmd_info);

  return __fpga_enqueue(dma_info, cmd_info, VIRT_ADDR_WITH_CHECK);
}


// cppcheck-suppress unusedFunction
int fpga_enqueue_without_addrcheck(
  dma_info_t *dma_info,
  dmacmd_info_t *cmd_info
) {
  if (dma_info == NULL || cmd_info == NULL) {
    llf_err(INVALID_ARGUMENT, "%s(dma_info(%#lx), cmd_info(%#lx))\n",
      __func__, (uintptr_t)dma_info, (uintptr_t)cmd_info);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dma_info(%#lx), cmd_info(%#lx))\n",
    __func__, (uintptr_t)dma_info, (uintptr_t)cmd_info);

  return __fpga_enqueue(dma_info, cmd_info, VIRT_ADDR_WITHOUT_CHECK);
}


// cppcheck-suppress unusedFunction
int fpga_enqueue_with_physaddr(
  dma_info_t *dma_info,
  dmacmd_info_t *cmd_info
) {
  if (dma_info == NULL || cmd_info == NULL) {
    llf_err(INVALID_ARGUMENT, "%s(dma_info(%#lx), cmd_info(%#lx))\n",
      __func__, (uintptr_t)dma_info, (uintptr_t)cmd_info);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dma_info(%#lx), cmd_info(%#lx))\n",
    __func__, (uintptr_t)dma_info, (uintptr_t)cmd_info);

  return __fpga_enqueue(dma_info, cmd_info, PHYS_ADDR);
}


// cppcheck-suppress unusedFunction
int fpga_dequeue(
  dma_info_t *dma_info,
  dmacmd_info_t *cmd_info
) {
  if (dma_info == NULL || cmd_info == NULL) {
    llf_err(INVALID_ARGUMENT, "%s(dma_info(%#lx), cmd_info(%#lx))\n",
      __func__, (uintptr_t)dma_info, (uintptr_t)cmd_info);
    return -INVALID_ARGUMENT;
  }
  llf_dbg("%s(dma_info(%#lx), cmd_info(%#lx))\n",
    __func__, (uintptr_t)dma_info, (uintptr_t)cmd_info);

  fpga_queue_t *deq = (fpga_queue_t*)dma_info->queue_addr;  //NOLINT
  fpga_desc_t *desc;
  uint16_t next_head, current_head;
  int64_t usec;
  bool deq_flg = true;
  struct timespec req, rem;
  struct timespec timer1, timer2;

  // infnity loop
  while (true) {
    // Get free descriptor
    do {
      // Get current head' position and descriptor
      current_head = deq->readhead;
      desc = &deq->ring[current_head];

      // Check if current head descriptor's status is CMD_DONE
      if (desc->op != CMD_DONE) {
        // Wait for status becoming CMD_DONE by clock_nanosleep()
        goto deq_loop;
      }

      // Get next descriptor to check later
      next_head = (current_head + 1);
      if (next_head == deq->size) next_head = 0;

      // Compare deq->readhead(in kernel(=used by other process)) and current_head(host_memory),
      // and if the data is the same, set next_head into deq->readhead and get current_head.
      // If the data is different, other process already got current_head, so retry to get new current_head.
    } while (!rte_atomic16_cmpset(&deq->readhead, current_head, next_head));

    // Set result status into cmd_info
    cmd_info->result_task_id = desc->task_id;
    cmd_info->result_status = desc->status; /* always 0 */
    cmd_info->result_data_len = desc->len;
    cmd_info->result_data_addr = desc->addr ? local_phy2virt(desc->addr)
                                            : NULL;

    // Clear descriptor(i.e. set 0 into desc->op)
    memset(desc, 0, sizeof(fpga_desc_t));

    return 0;

  deq_loop:
    // Wait for descriptor's status becoming CMD_DONE
    req.tv_sec = 0;
    req.tv_nsec = 1000 * libdma_dequeue_polling_interval;  // polling time[nsec]
    if (deq_flg) {
      clock_gettime(CLOCK_REALTIME, &timer1);
      deq_flg = false;
    }
    clock_gettime(CLOCK_REALTIME, &timer2);
    usec = (timer2.tv_sec  - timer1.tv_sec) * 1000000L + (timer2.tv_nsec - timer1.tv_nsec)/1000;
    if (usec >= libdma_dequeue_polling_timeout) {
      llf_warn(DEQUEUE_TIMEOUT, "Error happened: Timeout of dequeue polling in %ldus = %ldms\n", usec, usec/1000);
      return -DEQUEUE_TIMEOUT;
    }
    while (clock_nanosleep(CLOCK_REALTIME, 0, &req, &rem) == EINTR) {
      req.tv_sec  = rem.tv_sec;
      req.tv_nsec = rem.tv_nsec;
    }
  }
}


int fpga_dma_options_init(
  int argc,
  char **argv
) {
  log_libfpga_cmdline_arg(LIBFPGA_LOG_DEBUG, argc, argv, LIBDMA "%s", __func__);

  int opt, ret;
  char **argvopt;
  int option_index;
  char *prgname = argv[0];
  const int old_optind = optind;
  const int old_optopt = optopt;
  char * const old_optarg = optarg;

  argvopt = argv;
  optind = 1;
  opterr = 0;

  while ((opt = getopt_long(argc, argvopt, fpga_dma_short_options,
                fpga_dma_long_options, &option_index)) != EOF) {
    switch (opt) {
    case 'p': {
      fpga_set_dequeue_polling_timeout(atol(optarg));
      break;
    }
    case 'i': {
      fpga_set_dequeue_polling_interval(atol(optarg));
      break;
    }
    case 'r': {
      fpga_set_refqueue_polling_timeout(atol(optarg));
      break;
    }
    case 'q': {
      fpga_set_refqueue_polling_interval(atol(optarg));
      break;
    }
    default:
      llf_err(INVALID_ARGUMENT, "Invalid opreration: unable to parse option[%s].\n", argvopt[optind - 1]);
      ret = -INVALID_ARGUMENT;
      goto out;
    }
  }

  if (optind >= 0)
    argv[optind - 1] = prgname;
  ret = optind - 1;

out:
  optind = old_optind;
  optopt = old_optopt;
  optarg = old_optarg;

  return ret;
}


void fpga_set_dequeue_polling_timeout(
  int64_t timeout
) {
  libdma_dequeue_polling_timeout = timeout;
}


void fpga_set_dequeue_polling_interval(
  int64_t interval
) {
  libdma_dequeue_polling_interval = interval;
}


void fpga_set_refqueue_polling_timeout(
  int64_t timeout
) {
  if (timeout > REFQ_TIMEOUT_MAX || timeout < 0)
    return;
  libdma_refqueue_polling_timeout = timeout;
}


void fpga_set_refqueue_polling_interval(
  int64_t interval
) {
  if (interval > REFQ_INTERVAL_MAX || interval < 0)
    return;
  libdma_refqueue_polling_interval = interval;
}


int64_t fpga_get_dequeue_polling_timeout(void) {
  return libdma_dequeue_polling_timeout;
}


int64_t fpga_get_dequeue_polling_interval(void) {
  return libdma_dequeue_polling_interval;
}


// cppcheck-suppress unusedFunction
int64_t fpga_get_refqueue_polling_timeout(void) {
  return libdma_refqueue_polling_timeout;
}


// cppcheck-suppress unusedFunction
int64_t fpga_get_refqueue_polling_interval(void) {
  return libdma_refqueue_polling_interval;
}
